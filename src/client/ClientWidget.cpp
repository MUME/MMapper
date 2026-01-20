// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "ClientWidget.h"

#include "../configuration/configuration.h"
#include "../global/AnsiOstream.h"
#include "../proxy/connectionlistener.h"
#include "ClientTelnet.h"
#include "HotkeyManager.h"
#include "PreviewWidget.h"
#include "displaywidget.h"
#include "stackedinputwidget.h"
#include "ui_ClientWidget.h"

#include <memory>

#include <QDateTime>
#include <QFileDialog>
#include <QScrollBar>
#include <QString>
#include <QTimer>

ClientWidget::ClientWidget(ConnectionListener &listener,
                           HotkeyManager &hotkeyManager,
                           QWidget *const parent)
    : QWidget(parent)
    , m_listener{listener}
    , m_hotkeyManager{hotkeyManager}
{
    setWindowTitle("MMapper Client");

    initPipeline();

    auto &ui = getUi();

    // Port
    ui.port->setText(QString("%1").arg(getConfig().connection.localPort));

    ui.playButton->setFocus();
    QObject::connect(ui.playButton, &QAbstractButton::clicked, this, [this]() {
        getUi().parent->setCurrentIndex(1);
        getTelnet().connectToHost(m_listener);
    });

    ui.input->installEventFilter(this);
    ui.display->setFocusPolicy(Qt::TabFocus);

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        ui.playButton->click();
    }
}

ClientWidget::~ClientWidget() = default;

ClientWidget::Pipeline::~Pipeline()
{
    objs.clientTelnet.reset();
    objs.ui.reset();
}

QSize ClientWidget::minimumSizeHint() const
{
    return m_pipeline.objs.ui->display->sizeHint();
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
        NODISCARD PreviewWidget &getPreview() { return getSelf().getPreview(); }

    private:
        void virt_sendUserInput(const QString &msg) final
        {
            if (!getTelnet().isConnected()) {
                getTelnet().connectToHost(getSelf().m_listener);
            } else {
                getTelnet().sendToMud(msg);
            }
        }

        void virt_displayMessage(const QString &msg) final
        {
            getDisplay().slot_displayText(msg);
            getPreview().displayText(msg);
        }

        void virt_showMessage(const QString &msg, MAYBE_UNUSED int timeout) final
        {
            // REVISIT: Why is timeout ignored?
            getSelf().slot_onShowMessage(msg);
        }
        void virt_requestPassword() final { getSelf().getInput().requestPassword(); }
        void virt_scrollDisplay(bool pageUp) final
        {
            if (auto *scrollBar = getDisplay().verticalScrollBar()) {
                int pageStep = scrollBar->pageStep();
                int delta = pageUp ? -pageStep : pageStep;
                scrollBar->setValue(scrollBar->value() + delta);
            }
        }

        std::optional<QString> virt_getHotkey(const Hotkey &hk) final
        {
            auto &hotkeys = getSelf().getHotkeys();
            if (auto cmd = hotkeys.getCommand(hk)) {
                return mmqt::toQStringUtf8(*cmd);
            }
            return std::nullopt;
        }
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
        void virt_showPreview(bool visible) final { getSelf().getPreview().setVisible(visible); }
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
        PreviewWidget &getPreview() { return getClient().getPreview(); }
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
            getClient().displayReconnectHint();
            getClient().relayMessage("Disconnected using the integrated client");
        }
        void virt_socketError(const QString &errorStr) final
        {
            getDisplay().slot_displayText(QString("\nInternal error! %1\n").arg(errorStr));
        }
        void virt_echoModeChanged(const bool echo) final
        {
            getInput().setEchoMode(echo ? EchoModeEnum::Visible : EchoModeEnum::Hidden);
        }

        void virt_sendToUser(const QString &str) final
        {
            getDisplay().slot_displayText(str);
            getPreview().displayText(str);

            // Re-open the password dialog if we get a message in hidden echo mode
            if (getClient().getInput().getEchoMode() == EchoModeEnum::Hidden) {
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

PreviewWidget &ClientWidget::getPreview()
{
    return deref(getUi().preview);
}

StackedInputWidget &ClientWidget::getInput()
{
    return deref(getUi().input);
}

ClientTelnet &ClientWidget::getTelnet() // NOLINT (no, this shouldn't be const)
{
    return deref(m_pipeline.objs.clientTelnet);
}

HotkeyManager &ClientWidget::getHotkeys()
{
    return m_hotkeyManager;
}

void ClientWidget::slot_onVisibilityChanged(const bool /*visible*/)
{
    if (!isUsingClient()) {
        return;
    }

    // Delay connecting to verify that visibility is not just the dock popping back in
    QTimer::singleShot(500, [this]() {
        if (getTelnet().isConnected() && !isVisible()) {
            // Disconnect if the widget is closed or minimized
            getTelnet().disconnectFromHost();
        } else if (!getTelnet().isConnected() && isVisible()) {
            getInput().setFocus();
        }
    });
}

bool ClientWidget::isUsingClient() const
{
    return getUi().parent->currentIndex() != 0;
}

void ClientWidget::displayReconnectHint()
{
    constexpr const auto whiteOnCyan = getRawAnsi(AnsiColor16Enum::white, AnsiColor16Enum::cyan);
    std::stringstream oss;
    AnsiOstream aos{oss};
    aos.writeWithColor(whiteOnCyan, "\n\n\nPress return to reconnect.\n");
    getDisplay().slot_displayText(mmqt::toQStringUtf8(oss.str()));
}

void ClientWidget::slot_onShowMessage(const QString &message)
{
    relayMessage(message);
}

void ClientWidget::slot_saveLog()
{
    const auto getDocStringUtf8 = [](const QTextDocument *const pDoc) -> QByteArray {
        auto &doc = deref(pDoc);
        const QString string = doc.toPlainText();
        return string.toUtf8();
    };
    const QByteArray logContent = getDocStringUtf8(getDisplay().document());
    QString newFileName = "log-" + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")
                          + ".txt";
    QFileDialog::saveFileContent(logContent, newFileName);
}

void ClientWidget::slot_saveLogAsHtml()
{
    const auto getDocStringUtf8 = [](const QTextDocument *const pDoc) -> QByteArray {
        auto &doc = deref(pDoc);
        const QString string = doc.toHtml();
        return string.toUtf8();
    };
    const QByteArray logContent = getDocStringUtf8(getDisplay().document());
    QString newFileNameHtml = "log-" + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")
                              + ".html";
    QFileDialog::saveFileContent(logContent, newFileNameHtml);
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
