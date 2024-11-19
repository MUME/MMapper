// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "ClientWidget.h"

#include "../configuration/configuration.h"
#include "../global/MakeQPointer.h"
#include "ClientTelnet.h"
#include "displaywidget.h"
#include "stackedinputwidget.h"
#include "ui_ClientWidget.h"

#include <memory>

#include <QString>
#include <QTimer>
#include <QtWidgets>

ClientWidget::ClientWidget(QWidget *const parent)
    : QWidget(parent)
    , m_ui(std::make_unique<Ui::ClientWidget>())
    , m_telnet(mmqt::makeQPointer<ClientTelnet>(this))
{
    setWindowTitle("MMapper Client");

    auto &ui = deref(m_ui);
    ui.setupUi(this);
    std::ignore = deref(ui.display);
    std::ignore = deref(ui.input);

    // Port
    ui.port->setText(QString("%1").arg(getConfig().connection.localPort));

    ui.playButton->setFocus();
    connect(ui.playButton, &QAbstractButton::clicked, this, [this]() {
        deref(m_ui).parent->setCurrentIndex(1);
        m_telnet->connectToHost();
    });

    // Keyboard input on the display widget should be redirected to the input widget
    ui.display->setFocusProxy(ui.input);

    ui.input->installEventFilter(this);

    // Connect the signals/slots
    connect(m_telnet, &ClientTelnet::sig_disconnected, this, [this]() {
        deref(deref(m_ui).display).slot_displayText("\n\n\n");
        emit sig_relayMessage("Disconnected using the integrated client");
    });
    connect(m_telnet, &ClientTelnet::sig_connected, this, [this]() {
        emit sig_relayMessage("Connected using the integrated client");

        // Focus should be on the input
        deref(m_ui).input->setFocus();
    });
    connect(m_telnet, &ClientTelnet::sig_socketError, this, [this](const QString &errorStr) {
        deref(deref(m_ui).display).slot_displayText(QString("\nInternal error! %1\n").arg(errorStr));
    });

    // Input
    connect(ui.input,
            &StackedInputWidget::sig_sendUserInput,
            m_telnet,
            &ClientTelnet::slot_sendToMud);
    connect(m_telnet,
            &ClientTelnet::sig_echoModeChanged,
            ui.input,
            &StackedInputWidget::slot_toggleEchoMode);
    connect(ui.input, &StackedInputWidget::sig_showMessage, this, &ClientWidget::slot_onShowMessage);

    // Display
    connect(ui.input,
            &StackedInputWidget::sig_displayMessage,
            ui.display,
            &DisplayWidget::slot_displayText);
    connect(m_telnet, &ClientTelnet::sig_sendToUser, ui.display, &DisplayWidget::slot_displayText);
    connect(ui.display, &DisplayWidget::sig_showMessage, this, &ClientWidget::slot_onShowMessage);
    connect(ui.display,
            &DisplayWidget::sig_windowSizeChanged,
            m_telnet,
            &ClientTelnet::slot_onWindowSizeChanged);
}

ClientWidget::~ClientWidget() = default;

void ClientWidget::slot_onVisibilityChanged(const bool /*visible*/)
{
    if (!isUsingClient())
        return;

    // Delay connecting to verify that visibility is not just the dock popping back in
    QTimer::singleShot(500, [this]() {
        if (!isVisible()) {
            // Disconnect if the widget is closed or minimized
            m_telnet->disconnectFromHost();

        } else {
            // Connect if the client was previously activated and the widget is re-opened
            m_telnet->connectToHost();
        }
    });
}

bool ClientWidget::isUsingClient() const
{
    return deref(m_ui).parent->currentIndex() != 0;
}

void ClientWidget::slot_onShowMessage(const QString &message)
{
    emit sig_relayMessage(message);
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
        emit sig_relayMessage(tr("No filename provided"));
        return;
    }

    QFile document(fileNames[0]);
    if (!document.open(QFile::WriteOnly | QFile::Text)) {
        emit sig_relayMessage(QString("Error occurred while opening %1").arg(document.fileName()));
        return;
    }

    const auto getDocString8bit = [](const QTextDocument *const pDoc,
                                     const bool isHtml) -> QByteArray {
        auto &doc = deref(pDoc);
        const QString string = isHtml ? doc.toHtml() : doc.toPlainText();
        return string.toLocal8Bit();
    };
    document.write(getDocString8bit(deref(deref(m_ui).display).document(), result.isHtml));
    document.close();
}

bool ClientWidget::eventFilter(QObject *const obj, QEvent *const event)
{
    if (event->type() == QEvent::KeyPress) {
        if (auto *const keyEvent = dynamic_cast<QKeyEvent *>(event)) {
            Ui::ClientWidget &ui = deref(m_ui);
            DisplayWidget &display = deref(ui.display);
            StackedInputWidget &input = deref(ui.input);
            if (keyEvent->matches(QKeySequence::Copy)) {
                if (display.canCopy())
                    display.copy();
                else
                    input.slot_copy();
                keyEvent->accept();
                return true;
            } else if (keyEvent->matches(QKeySequence::Cut)) {
                input.slot_cut();
                keyEvent->accept();
                return true;
            } else if (keyEvent->matches(QKeySequence::Paste)) {
                input.slot_paste();
                keyEvent->accept();
                return true;
            }
        }
    }
    // Standard event processing
    return QObject::eventFilter(obj, event);
}
