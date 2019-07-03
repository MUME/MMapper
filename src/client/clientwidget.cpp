// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "clientwidget.h"

#include <memory>
#include <QMessageLogContext>
#include <QSize>
#include <QString>
#include <QtGui>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "../global/utils.h"
#include "ClientTelnet.h"
#include "displaywidget.h"
#include "stackedinputwidget.h"

ClientWidget::ClientWidget(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("MMapper Client");
    setWindowFlags(windowFlags() | Qt::WindowType::Widget);
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_splitter = new QSplitter(this);
    m_splitter->setOrientation(Qt::Vertical);
    setCentralWidget(m_splitter);

    // Add the primary widgets to the smart splitter
    m_display = new DisplayWidget(this);
    m_splitter->addWidget(m_display);
    m_splitter->setCollapsible(m_splitter->indexOf(m_display), false);

    m_input = new StackedInputWidget(this);
    m_splitter->addWidget(m_input);
    m_splitter->setCollapsible(m_splitter->indexOf(m_input), false);

    statusBar()->showMessage(tr("Ready"), 5000);

    // Keyboard input on the display widget should be redirected to the input widget
    m_display->setFocusProxy(m_input);
    m_input->installEventFilter(this);

    // Focus should be on the input
    m_input->setFocus();

    // Connect the signals/slots
    m_telnet = std::make_unique<ClientTelnet>(this);
    ClientTelnet *const telnet = m_telnet.get();
    connect(telnet, &ClientTelnet::disconnected, this, &ClientWidget::onDisconnected);
    connect(telnet, &ClientTelnet::connected, this, &ClientWidget::onConnected);
    connect(telnet, &ClientTelnet::socketError, this, &ClientWidget::onSocketError);

    // Input
    connect(m_input, &StackedInputWidget::sendUserInput, this, &ClientWidget::sendToMud);
    connect(telnet, &ClientTelnet::echoModeChanged, m_input, &StackedInputWidget::toggleEchoMode);
    connect(m_input, &StackedInputWidget::showMessage, statusBar(), &QStatusBar::showMessage);

    // Display
    connect(m_input, &StackedInputWidget::displayMessage, m_display, &DisplayWidget::displayText);
    connect(this, &ClientWidget::sendToUser, m_display, &DisplayWidget::displayText);
    connect(telnet, &ClientTelnet::sendToUser, m_display, &DisplayWidget::displayText);
    connect(m_display, &DisplayWidget::showMessage, statusBar(), &QStatusBar::showMessage);
    connect(m_display,
            &DisplayWidget::windowSizeChanged,
            telnet,
            &ClientTelnet::onWindowSizeChanged);
    readSettings();

    QMenu *fileMenu = menuBar()->addMenu("&File");

    QAction *connectAct = new QAction(QIcon(":/icons/online.png"), tr("Co&nnect"), this);
    connectAct->setShortcut(QKeySequence(tr("Ctrl+N")));
    connect(connectAct, &QAction::triggered, this, &ClientWidget::connectToHost);
    connectAct->setStatusTip(tr("Connect to the remote host"));

    QAction *disconnectAct = new QAction(QIcon(":/icons/offline.png"), tr("&Disconnect"), this);
    disconnectAct->setShortcut(QKeySequence(tr("Ctrl+D")));
    connect(disconnectAct, &QAction::triggered, this, &ClientWidget::disconnectFromHost);
    disconnectAct->setStatusTip(tr("Disconnect from the remote host"));
    QAction *saveLog = new QAction(QIcon::fromTheme("document-save", QIcon(":/icons/save.png")),
                                   tr("&Save log as..."),
                                   this);
    saveLog->setShortcut(QKeySequence(tr("Ctrl+S")));
    connect(saveLog, &QAction::triggered, this, &ClientWidget::saveLog);
    saveLog->setStatusTip(tr("Save log as file"));

    QAction *closeAct = new QAction(QIcon::fromTheme("window-close", QIcon(":/icons/exit.png")),
                                    tr("E&xit"),
                                    this);
    closeAct->setShortcut(QKeySequence(tr("Ctrl+Q")));
    connect(closeAct, &QAction::triggered, this, &ClientWidget::close);
    closeAct->setStatusTip(tr("Close the mud client"));

    fileMenu->addAction(connectAct);
    fileMenu->addAction(disconnectAct);
    fileMenu->addAction(saveLog);
    fileMenu->addAction(closeAct);

    QMenu *editMenu = menuBar()->addMenu("&Edit");
    QAction *cutAct = new QAction(QIcon::fromTheme("edit-cut", QIcon(":/icons/cut.png")),
                                  tr("Cu&t"),
                                  this);
    cutAct->setShortcut(QKeySequence(tr("Ctrl+X")));
    cutAct->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    editMenu->addAction(cutAct);
    connect(cutAct, &QAction::triggered, m_input, &StackedInputWidget::cut);

    QAction *copyAct = new QAction(QIcon::fromTheme("edit-copy", QIcon(":/icons/copy.png")),
                                   tr("&Copy"),
                                   this);
    copyAct->setShortcut(QKeySequence(tr("Ctrl+C")));
    copyAct->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    editMenu->addAction(copyAct);
    connect(copyAct, &QAction::triggered, this, &ClientWidget::copy);
    connect(m_display, &DisplayWidget::copyAvailable, [this](bool copyAvailable) {
        m_displayCopyAvailable = copyAvailable;
    });

    QAction *pasteAct = new QAction(QIcon::fromTheme("edit-paste", QIcon(":/icons/paste.png")),
                                    tr("&Paste"),
                                    this);
    pasteAct->setShortcut(QKeySequence(tr("Ctrl+V")));
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    editMenu->addAction(pasteAct);
    connect(pasteAct, &QAction::triggered, m_input, &StackedInputWidget::paste);
}

ClientWidget::~ClientWidget()
{
    writeSettings();
    delete m_display;
    delete m_input;
    delete m_splitter;
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

void ClientWidget::closeEvent(QCloseEvent *event)
{
    disconnectFromHost();
    emit sendToUser("\n\n\n");
    event->accept();
}

void ClientWidget::connectToHost()
{
    if (m_connected) {
        emit sendToUser("Disconnect first.\r\n");
    } else {
        emit sendToUser("\r\nTrying... ");
        m_telnet->connectToHost();
    }
}

void ClientWidget::disconnectFromHost()
{
    if (m_connected) {
        emit sendToUser("\r\nTrying... ");
    }
    m_telnet->disconnectFromHost();
}

void ClientWidget::onDisconnected()
{
    emit sendToUser("disconnected!\r\n");
    m_connected = false;
}

void ClientWidget::onConnected()
{
    emit sendToUser("connected!\r\n");
    m_connected = true;
}

void ClientWidget::onSocketError(const QString &errorStr)
{
    emit sendToUser(QString("error! %1\r\n").arg(errorStr));
    m_connected = false;
}

void ClientWidget::sendToMud(const QString &str)
{
    if (m_connected) {
        m_telnet->sendToMud(str);
    }
}

void ClientWidget::copy()
{
    if (m_displayCopyAvailable) {
        m_display->copy();
    } else {
        m_input->copy();
    }
}

bool ClientWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride && obj != this) {
        // Shortcuts sent from child widgets should be handled by this widget
        event->ignore();
        return true;
    }
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->matches(QKeySequence::Copy)) {
            copy();
            keyEvent->accept();
            return true;
        } else if (keyEvent->matches(QKeySequence::Cut)) {
            m_input->cut();
            keyEvent->accept();
            return true;
        } else if (keyEvent->matches(QKeySequence::Paste)) {
            m_input->paste();
            keyEvent->accept();
            return true;
        }
    }
    // Standard event processing
    return QObject::eventFilter(obj, event);
}

void ClientWidget::saveLog()
{
    struct Result
    {
        QStringList filenames{};
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

        if (save->exec() == QDialog::Accepted)
            return Result{save->selectedFiles(), save->selectedNameFilter().contains("HTML")};

        return Result{};
    };

    const auto result = getFileNames();
    const auto &fileNames = result.filenames;

    if (fileNames.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return;
    }

    QFile document(fileNames[0]);
    if (!document.open(QFile::WriteOnly | QFile::Text)) {
        statusBar()->showMessage(QString("Error occurred while opening %1").arg(document.fileName()),
                                 2000);
        return;
    }

    const auto getDocString8bit = [](const QTextDocument *const pDoc,
                                     const bool isHtml) -> QByteArray {
        auto &doc = deref(pDoc);
        const QString string = isHtml ? doc.toHtml() : doc.toPlainText();
        return string.toLocal8Bit();
    };
    document.write(getDocString8bit(m_display->document(), result.isHtml));
    document.close();
}

QSize ClientWidget::minimumSizeHint() const
{
    return {500, 480};
}

QSize ClientWidget::sizeHint() const
{
    return {500, 480};
}
