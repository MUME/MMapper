// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "ClientWidget.h"

#include "../configuration/configuration.h"
#include "ClientTelnet.h"
#include "displaywidget.h"
#include "stackedinputwidget.h"
#include "ui_ClientWidget.h"

#include <memory>

#include <QFileDialog>
#include <QString>
#include <QTimer>

ClientWidget::ClientWidget(QWidget *const parent)
    : QWidget(parent)
{
    setWindowTitle("MMapper Client");

    initPipeline();

    auto &ui = getUi();

    // Port
    ui.port->setText(QString("%1").arg(getConfig().connection.localPort));

    ui.playButton->setFocus();
    QObject::connect(ui.playButton, &QAbstractButton::clicked, this, [this]() {
        getUi().parent->setCurrentIndex(1);
        getTelnet().connectToHost();
    });

    ui.input->installEventFilter(this);
    ui.input->setFocus();
    ui.display->setFocusPolicy(Qt::TabFocus);
}

ClientWidget::~ClientWidget() = default;

ClientWidget::Pipeline::~Pipeline()
{
    objs.clientTelnet.reset();
    objs.ui.reset();
}

void ClientWidget::initPipeline()
{
    m_pipeline.objs.ui = std::make_unique<Ui::ClientWidget>();
    getUi().setupUi(this); // creates stacked input and display

    initStackedInputWidget();
    initDisplayWidget();

    initClientTelnet();
}

void ClientWidget::initStackedInputWidget()
{
    class NODISCARD LocalStackedInputWidgetOutputs final : public StackedInputWidgetOutputs
    {
    private:
        ClientWidget &m_self;

    public:
        explicit LocalStackedInputWidgetOutputs(ClientWidget &self)
            : m_self{self}
        {}

    private:
        NODISCARD ClientWidget &getSelf() { return m_self; }
        NODISCARD ClientTelnet &getTelnet() { return getSelf().getTelnet(); }
        NODISCARD DisplayWidget &getDisplay() { return getSelf().getDisplay(); }

    private:
        void virt_sendUserInput(const QString &msg) final { getTelnet().sendToMud(msg); }

        void virt_displayMessage(const QString &msg) final { getDisplay().slot_displayText(msg); }

        void virt_showMessage(const QString &msg, MAYBE_UNUSED int timeout) final
        {
            // REVISIT: Why is timeout ignored?
            getSelf().slot_onShowMessage(msg);
        }
        void virt_requestPassword() final { getSelf().getInput().requestPassword(); }
    };
    auto &out = m_pipeline.outputs.stackedInputWidgetOutputs;
    out = std::make_unique<LocalStackedInputWidgetOutputs>(*this);
    getInput().init(deref(out));
}
void ClientWidget::initDisplayWidget()
{
    class NODISCARD LocalDisplayWidgetOutputs final : public DisplayWidgetOutputs
    {
    private:
        ClientWidget &m_self;

    public:
        explicit LocalDisplayWidgetOutputs(ClientWidget &self)
            : m_self{self}
        {}

    private:
        NODISCARD ClientWidget &getSelf() { return m_self; }
        NODISCARD ClientTelnet &getTelnet() { return getSelf().getTelnet(); }

    private:
        void virt_showMessage(const QString &msg, int /*timeout*/) final
        {
            getSelf().slot_onShowMessage(msg);
        }
        void virt_windowSizeChanged(const int width, const int height) final
        {
            getTelnet().onWindowSizeChanged(width, height);
        }

        void virt_returnFocusToInput() final { getSelf().getInput().setFocus(); }
    };
    auto &out = m_pipeline.outputs.displayWidgetOutputs;
    out = std::make_unique<LocalDisplayWidgetOutputs>(*this);
    getDisplay().init(deref(out));
}
void ClientWidget::initClientTelnet()
{
    class NODISCARD LocalClientTelnetOutputs final : public ClientTelnetOutputs
    {
    private:
        ClientWidget &m_self;

    public:
        explicit LocalClientTelnetOutputs(ClientWidget &self)
            : m_self{self}
        {}

    private:
        ClientWidget &getClient() { return m_self; }
        DisplayWidget &getDisplay() { return getClient().getDisplay(); }
        StackedInputWidget &getInput() { return getClient().getInput(); }

    private:
        void virt_connected() final
        {
            getClient().relayMessage("Connected using the integrated client");
            // Focus should be on the input
            getInput().setFocus();
        }
        void virt_disconnected() final
        {
            getDisplay().slot_displayText("\n\n\n");
            getClient().relayMessage("Disconnected using the integrated client");
        }
        void virt_socketError(const QString &errorStr) final
        {
            getDisplay().slot_displayText(QString("\nInternal error! %1\n").arg(errorStr));
        }
        void virt_echoModeChanged(const bool echo) final
        {
            getInput().setEchoMode(echo ? EchoMode::Visible : EchoMode::Hidden);
        }
        void virt_sendToUser(const QString &str) final
        {
            getDisplay().slot_displayText(str);

            // Re-open the password dialog if we get a message in hidden echo mode
            if (getClient().getInput().getEchoMode() == EchoMode::Hidden) {
                getClient().getInput().requestPassword();
            }
        }
    };
    auto &out = m_pipeline.outputs.clientTelnetOutputs;
    out = std::make_unique<LocalClientTelnetOutputs>(*this);
    m_pipeline.objs.clientTelnet = std::make_unique<ClientTelnet>(deref(out));
}

DisplayWidget &ClientWidget::getDisplay()
{
    return deref(getUi().display);
}

StackedInputWidget &ClientWidget::getInput()
{
    return deref(getUi().input);
}

ClientTelnet &ClientWidget::getTelnet() // NOLINT (no, this shouldn't be const)
{
    return deref(m_pipeline.objs.clientTelnet);
}

void ClientWidget::slot_onVisibilityChanged(const bool /*visible*/)
{
    if (!isUsingClient()) {
        return;
    }

    // Delay connecting to verify that visibility is not just the dock popping back in
    QTimer::singleShot(500, [this]() {
        if (!isVisible()) {
            // Disconnect if the widget is closed or minimized
            getTelnet().disconnectFromHost();

        } else {
            // Connect if the client was previously activated and the widget is re-opened
            getTelnet().connectToHost();
        }
    });
}

bool ClientWidget::isUsingClient() const
{
    return getUi().parent->currentIndex() != 0;
}

void ClientWidget::slot_onShowMessage(const QString &message)
{
    relayMessage(message);
}

void ClientWidget::slot_saveLog()
{
    struct NODISCARD Result
    {
        QStringList filenames;
        bool isHtml = false;
    };
    const auto getFileNames = [this]() -> Result {
        auto save = std::make_unique<QFileDialog>(this, "Choose log file name ...");
        save->setFileMode(QFileDialog::AnyFile);
        save->setDirectory(QDir::current());
        save->setNameFilters(QStringList() << "Text log (*.log *.txt)"
                                           << "HTML log (*.htm *.html)");
        save->setDefaultSuffix("txt");
        save->setAcceptMode(QFileDialog::AcceptSave);

        if (save->exec() == QDialog::Accepted) {
            const QString nameFilter = save->selectedNameFilter().toLower();
            const bool isHtml = nameFilter.endsWith(".htm") || nameFilter.endsWith(".html");
            return Result{save->selectedFiles(), isHtml};
        }

        return Result{};
    };

    const auto result = getFileNames();
    const auto &fileNames = result.filenames;

    if (fileNames.isEmpty()) {
        relayMessage(tr("No filename provided"));
        return;
    }

    QFile document(fileNames[0]);
    if (!document.open(QFile::WriteOnly | QFile::Text)) {
        relayMessage(QString("Error occurred while opening %1").arg(document.fileName()));
        return;
    }

    const auto getDocStringUtf8 = [](const QTextDocument *const pDoc,
                                     const bool isHtml) -> QByteArray {
        auto &doc = deref(pDoc);
        const QString string = isHtml ? doc.toHtml() : doc.toPlainText();
        return string.toUtf8();
    };
    document.write(getDocStringUtf8(getDisplay().document(), result.isHtml));
    document.close();
}

bool ClientWidget::focusNextPrevChild(MAYBE_UNUSED bool next)
{
    if (getInput().hasFocus()) {
        getDisplay().setFocus();
    } else {
        getInput().setFocus();
    }
    return true;
}
