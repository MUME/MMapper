/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "clientwidget.h"
#include "ctelnet.h"
#include "displaywidget.h"
#include "stackedinputwidget.h"

#include "configuration/configuration.h"
#include "mainwindow/mainwindow.h"

#include <QCloseEvent>
#include <QDebug>
#include <QMenuBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>

ClientWidget::ClientWidget(QWidget *parent)
    : QDialog(parent)
    , m_connected(false)
{
    setWindowTitle("MMapper Client");
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_splitter = new QSplitter(this);
    m_splitter->setOrientation(Qt::Vertical);
    layout->addWidget(m_splitter);

    // Add the primary widgets to the smart splitter
    m_display = new DisplayWidget(this);
    m_splitter->addWidget(m_display);
    m_splitter->setCollapsible(m_splitter->indexOf(m_display), false);

    m_input = new StackedInputWidget(this);
    m_splitter->addWidget(m_input);
    m_splitter->setCollapsible(m_splitter->indexOf(m_input), false);

    m_statusBar = new QStatusBar(this);
    m_statusBar->showMessage(tr("Ready"), 5000);
    layout->addWidget(m_statusBar);

    // Keyboard input on the display widget should be redirected to the input widget
    m_display->setFocusProxy(m_input);
    m_input->installEventFilter(this);

    // Focus should be on the input
    m_input->setFocus();

    // Connect the signals/slots
    m_telnet = new cTelnet(this);
    connect(m_telnet, &cTelnet::disconnected, this, &ClientWidget::onDisconnected);
    connect(m_telnet, &cTelnet::connected, this, &ClientWidget::onConnected);
    connect(m_telnet, &cTelnet::socketError, this, &ClientWidget::onSocketError);

    // Input
    connect(m_input, &StackedInputWidget::sendUserInput, this, &ClientWidget::sendToMud);
    connect(m_telnet, &cTelnet::echoModeChanged, m_input, &StackedInputWidget::toggleEchoMode);
    connect(m_input, &StackedInputWidget::showMessage, m_statusBar, &QStatusBar::showMessage);

    // Display
    connect(m_input, &StackedInputWidget::displayMessage, m_display, &DisplayWidget::displayText);
    connect(this, &ClientWidget::sendToUser, m_display, &DisplayWidget::displayText);
    connect(m_telnet, &cTelnet::sendToUser, m_display, &DisplayWidget::displayText);
    connect(m_display, &DisplayWidget::showMessage, m_statusBar, &QStatusBar::showMessage);
    readSettings();

    auto *menuBar = new QMenuBar(this);
    layout->setMenuBar(menuBar);

    QMenu *fileMenu = menuBar->addMenu("&File");

    QAction *connectAct = new QAction(QIcon(":/icons/online.png"), tr("Co&nnect"), this);
    connectAct->setShortcut(QKeySequence(tr("Ctrl+N")));
    connect(connectAct, &QAction::triggered, this, &ClientWidget::connectToHost);
    connect(connectAct, &QAction::hovered, [this]() {
        m_statusBar->showMessage(tr("Connect to the remote host"), 1000);
    });

    QAction *disconnectAct = new QAction(QIcon(":/icons/offline.png"), tr("&Disconnect"), this);
    disconnectAct->setShortcut(QKeySequence(tr("Ctrl+D")));
    connect(disconnectAct, &QAction::triggered, this, &ClientWidget::disconnectFromHost);
    connect(disconnectAct, &QAction::hovered, [this]() {
        m_statusBar->showMessage(tr("Disconnect from the remote host"), 1000);
    });

    QAction *saveLog = new QAction(QIcon::fromTheme("document-save", QIcon(":/icons/save.png")),
                                   tr("&Save log as..."),
                                   this);
    saveLog->setShortcut(QKeySequence(tr("Ctrl+S")));
    connect(saveLog, &QAction::triggered, this, &ClientWidget::saveLog);
    connect(saveLog, &QAction::hovered, [this]() {
        m_statusBar->showMessage(tr("Save log as file"), 1000);
    });

    QAction *closeAct = new QAction(QIcon::fromTheme("window-close", QIcon(":/icons/exit.png")),
                                    tr("E&xit"),
                                    this);
    closeAct->setShortcut(QKeySequence(tr("Ctrl+Q")));
    connect(closeAct, &QAction::triggered, this, &ClientWidget::close);
    connect(closeAct, &QAction::hovered, [this]() {
        m_statusBar->showMessage(tr("Close the mud client"), 1000);
    });

    fileMenu->addAction(connectAct);
    fileMenu->addAction(disconnectAct);
    fileMenu->addAction(saveLog);
    fileMenu->addAction(closeAct);

    QMenu *editMenu = menuBar->addMenu("&Edit");
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
    delete m_statusBar;
}

void ClientWidget::readSettings()
{
    QSettings settings("Caligor soft", "MMapper2");
    settings.beginGroup("Integrated Mud Client");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    settings.endGroup();
    move(pos);
    resize(size);
}

void ClientWidget::writeSettings()
{
    QSettings settings("Caligor soft", "MMapper2");
    settings.beginGroup("Integrated Mud Client");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.endGroup();
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
    QPointer<QFileDialog> save = new QFileDialog(this, "Choose log file name ...");
    save->setFileMode(QFileDialog::AnyFile);
    save->setDirectory(QDir::current());
    save->setNameFilters(QStringList() << "Text log (*.log *.txt)"
                                       << "HTML log (*.htm *.html)");
    save->setDefaultSuffix("txt");
    save->setAcceptMode(QFileDialog::AcceptSave);

    QStringList fileNames;
    if (save->exec() != 0) {
        fileNames = save->selectedFiles();
    }

    if (fileNames.isEmpty()) {
        m_statusBar->showMessage(tr("No filename provided"), 2000);
        return;
    }

    QFile document(fileNames[0]);
    if (!document.open(QFile::WriteOnly | QFile::Text)) {
        m_statusBar->showMessage(QString("Error occur while opening %1").arg(document.fileName()),
                                 2000);
        return;
    }

    QString nameFilter = save->selectedNameFilter();
    if (nameFilter.contains("HTML")) {
        document.write(m_display->document()->toHtml().toLocal8Bit());

    } else {
        document.write(m_display->document()->toPlainText().toLocal8Bit());
    }
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
