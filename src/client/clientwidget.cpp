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

ClientWidget::ClientWidget(QWidget *parent) : QDialog(parent),
    m_connected(false)
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
    m_display->installEventFilter(m_input);

    // Focus should be on the input
    m_input->setFocus();

    // Connect the signals/slots
    m_telnet = new cTelnet(this);
    connect(m_telnet, SIGNAL(disconnected()), this, SLOT(onDisconnected()) );
    connect(m_telnet, SIGNAL(connected()), this, SLOT(onConnected()) );
    connect(m_telnet, SIGNAL(socketError(const QString &)), this,
            SLOT(onSocketError(const QString &)) );

    // Input
    connect(m_input, SIGNAL(sendUserInput(const QByteArray &)),
            this, SLOT(sendToMud(const QByteArray &)));
    connect(m_telnet, SIGNAL(echoModeChanged(bool)),
            m_input, SLOT(toggleEchoMode(bool)));
    connect(m_input, SIGNAL(showMessage(const QString &, int)), m_statusBar,
            SLOT(showMessage(const QString &, int)));

    // Display
    connect(m_input, SIGNAL(displayMessage(const QString &)), m_display,
            SLOT(displayText(const QString &)));
    connect(this, SIGNAL(sendToUser(const QString &)), m_display,
            SLOT(displayText(const QString &)));
    connect(m_telnet, SIGNAL(sendToUser(const QString &)), m_display,
            SLOT(displayText(const QString &)));
    connect(m_display, SIGNAL(showMessage(const QString &, int)), m_statusBar,
            SLOT(showMessage(const QString &, int)));
    readSettings();


    auto *menuBar = new QMenuBar(this);
    layout->setMenuBar(menuBar);

    QMenu *fileMenu = menuBar->addMenu("&File");

    QAction *connectAct = new QAction(QIcon(":/icons/online.png"), tr("Co&nnect"), this);
    connectAct->setShortcut(tr("Ctrl+N"));
    connect(connectAct, SIGNAL(triggered()), this, SLOT(connectToHost()) );
    connect(connectAct, &QAction::hovered, [this]() {
        m_statusBar->showMessage(tr("Connect to the remote host"), 1000);
    });

    QAction *disconnectAct = new QAction(QIcon(":/icons/offline.png"), tr("&Disconnect"), this);
    disconnectAct->setShortcut(tr("Ctrl+D"));
    connect(disconnectAct, SIGNAL(triggered()), this, SLOT(disconnectFromHost()) );
    connect(disconnectAct, &QAction::hovered, [this]() {
        m_statusBar->showMessage(tr("Disconnect from the remote host"), 1000);
    });

    QAction *saveLog = new QAction(QIcon::fromTheme("document-save", QIcon(":/icons/save.png")),
                                   tr("&Save log as..."), this);
    saveLog->setShortcut(tr("Ctrl+S"));
    connect(saveLog, SIGNAL(triggered()), this, SLOT(saveLog()));
    connect(saveLog, &QAction::hovered, [this]() {
        m_statusBar->showMessage(tr("Save log as file"), 1000);
    });

    QAction *closeAct = new QAction(QIcon::fromTheme("window-close", QIcon(":/icons/exit.png")),
                                    tr("E&xit"), this);
    closeAct->setShortcut(tr("Ctrl+Q"));
    connect(closeAct, SIGNAL(triggered()), this, SLOT(close()));
    connect(closeAct, &QAction::hovered, [this]() {
        m_statusBar->showMessage(tr("Close the mud client"), 1000);
    });

    fileMenu->addAction(connectAct);
    fileMenu->addAction(disconnectAct);
    fileMenu->addAction(saveLog);
    fileMenu->addAction(closeAct);

    QMenu *editMenu = menuBar->addMenu("&Edit");
    QAction *cutAct = new QAction(QIcon::fromTheme("edit-cut", QIcon(":/icons/cut.png")), tr("Cu&t"),
                                  this);
    cutAct->setShortcut(tr("Ctrl+X"));
    cutAct->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    editMenu->addAction(cutAct);
    connect(cutAct, SIGNAL(triggered()), m_input, SLOT(cut()));

    QAction *copyAct = new QAction(QIcon::fromTheme("edit-copy", QIcon(":/icons/copy.png")),
                                   tr("&Copy"), this);
    copyAct->setShortcut(tr("Ctrl+C"));
    copyAct->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    editMenu->addAction(copyAct);
    connect(copyAct, SIGNAL(triggered()), m_input, SLOT(copy()));

    QAction *pasteAct = new QAction(QIcon::fromTheme("edit-paste", QIcon(":/icons/paste.png")),
                                    tr("&Paste"), this);
    pasteAct->setShortcut(tr("Ctrl+V"));
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    editMenu->addAction(pasteAct);
    connect(pasteAct, SIGNAL(triggered()), m_input, SLOT(paste()));
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

void ClientWidget::sendToMud(const QByteArray &ba)
{
    if (m_connected) {
        m_telnet->sendToMud(ba);
    }
}

void ClientWidget::saveLog()
{
    QPointer<QFileDialog> save = new QFileDialog(this, "Choose log file name ...");
    save->setFileMode(QFileDialog::AnyFile);
    save->setDirectory(QDir::current());
    save->setNameFilters(QStringList() << "Text log (*.log *.txt)" << "HTML log (*.htm *.html)");
    save->setDefaultSuffix("txt");
    save->setAcceptMode(QFileDialog::AcceptSave);

    QStringList fileNames;
    if (save->exec() != 0) {
        fileNames = save->selectedFiles();
    }

    if (fileNames.isEmpty()) {
        m_statusBar->showMessage(tr("No filename provided"), 2000);
        return ;
    }

    QFile document(fileNames[0]);
    if (!document.open(QFile::WriteOnly | QFile::Text)) {
        m_statusBar->showMessage(QString("Error occur while opening %1").arg(document.fileName()), 2000);
        return ;
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
