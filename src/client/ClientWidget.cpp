// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "ClientWidget.h"

#include <QString>
#include <QTimer>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "ClientTelnet.h"
#include "displaywidget.h"
#include "stackedinputwidget.h"
#include "ui_ClientWidget.h"

ClientWidget::ClientWidget(QWidget *const parent)
    : QWidget(parent)
    , ui(new Ui::ClientWidget)
    , m_telnet(new ClientTelnet(this))
{
    setWindowTitle("MMapper Client");
    ui->setupUi(this);

    // Port
    ui->port->setText(QString("%1").arg(getConfig().connection.localPort));

    ui->playButton->setFocus();
    connect(ui->playButton, &QAbstractButton::clicked, this, [this]() {
        ui->parent->setCurrentIndex(1);
        m_telnet->connectToHost();
    });

    // Keyboard input on the display widget should be redirected to the input widget
    ui->display->setFocusProxy(ui->input);
    ui->input->installEventFilter(this);

    // Connect the signals/slots
    connect(m_telnet, &ClientTelnet::disconnected, this, [this]() {
        ui->display->displayText("\n\n\n");
        emit relayMessage("Disconnected using the integrated client");
    });
    connect(m_telnet, &ClientTelnet::connected, this, [this]() {
        emit relayMessage("Connected using the integrated client");

        // Focus should be on the input
        ui->input->setFocus();
    });
    connect(m_telnet, &ClientTelnet::socketError, this, [this](const QString &errorStr) {
        ui->display->displayText(QString("\nInternal error! %1\n").arg(errorStr));
    });

    // Input
    connect(ui->input, &StackedInputWidget::sendUserInput, m_telnet, &ClientTelnet::sendToMud);
    connect(m_telnet,
            &ClientTelnet::echoModeChanged,
            ui->input,
            &StackedInputWidget::toggleEchoMode);
    connect(ui->input, &StackedInputWidget::showMessage, this, &ClientWidget::onShowMessage);

    // Display
    connect(ui->input,
            &StackedInputWidget::displayMessage,
            ui->display,
            &DisplayWidget::displayText);
    connect(m_telnet, &ClientTelnet::sendToUser, ui->display, &DisplayWidget::displayText);
    connect(ui->display, &DisplayWidget::showMessage, this, &ClientWidget::onShowMessage);
    connect(ui->display,
            &DisplayWidget::windowSizeChanged,
            m_telnet,
            &ClientTelnet::onWindowSizeChanged);

    readSettings();
}

ClientWidget::~ClientWidget()
{
    delete ui;
}

void ClientWidget::readSettings()
{
    if (!restoreGeometry(getConfig().integratedClient.geometry)) {
        setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                        Qt::AlignCenter,
                                        size(),
                                        qApp->primaryScreen()->availableGeometry()));
    }
}

void ClientWidget::writeSettings() const
{
    setConfig().integratedClient.geometry = saveGeometry();
}

void ClientWidget::onVisibilityChanged(const bool visible)
{
    if (!visible) {
        // Disconnect if the widget is closed or minimized
        m_telnet->disconnectFromHost();

    } else if (visible && isUsingClient()) {
        // Connect if the client was previously activated and the widget is re-opened
        // Delay connecting to allow the proxy a chance to purge the previous connection
        QTimer::singleShot(500, [this]() { m_telnet->connectToHost(); });
    }
}

bool ClientWidget::isUsingClient() const
{
    return ui->parent->currentIndex() != 0;
}

void ClientWidget::onShowMessage(const QString &message)
{
    emit relayMessage(message);
}

void ClientWidget::saveLog()
{
    struct Result
    {
        QStringList filenames;
        bool isHtml = false;
    };
    const auto getFileNames = [this]() {
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
        emit relayMessage(tr("No filename provided"));
        return;
    }

    QFile document(fileNames[0]);
    if (!document.open(QFile::WriteOnly | QFile::Text)) {
        emit relayMessage(QString("Error occurred while opening %1").arg(document.fileName()));
        return;
    }

    const auto getDocString8bit = [](const QTextDocument *const pDoc,
                                     const bool isHtml) -> QByteArray {
        auto &doc = deref(pDoc);
        const QString string = isHtml ? doc.toHtml() : doc.toPlainText();
        return string.toLocal8Bit();
    };
    document.write(getDocString8bit(ui->display->document(), result.isHtml));
    document.close();
}

bool ClientWidget::eventFilter(QObject *const obj, QEvent *const event)
{
    if (event->type() == QEvent::ShortcutOverride && obj != this) {
        // Shortcuts sent from child widgets should be handled by this widget
        event->ignore();
        return true;
    }
    if (event->type() == QEvent::KeyPress) {
        if (auto *const keyEvent = dynamic_cast<QKeyEvent *>(event)) {
            if (keyEvent->matches(QKeySequence::Copy)) {
                ui->input->copy();
                keyEvent->accept();
                return true;
            } else if (keyEvent->matches(QKeySequence::Cut)) {
                ui->input->cut();
                keyEvent->accept();
                return true;
            } else if (keyEvent->matches(QKeySequence::Paste)) {
                ui->input->paste();
                keyEvent->accept();
                return true;
            }
        }
    }
    // Standard event processing
    return QObject::eventFilter(obj, event);
}
