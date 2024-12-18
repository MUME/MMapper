// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "configdialog.h"

#include "autologpage.h"
#include "clientpage.h"
#include "generalpage.h"
#include "graphicspage.h"
#include "mumeprotocolpage.h"
#include "parserpage.h"
#include "pathmachinepage.h"
#include "ui_configdialog.h"

#include <QIcon>
#include <QListWidget>
#include <QtWidgets>

ConfigDialog::ConfigDialog(QWidget *const parent)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Config Dialog"));

    createIcons();

    auto generalPage = new GeneralPage(this);
    auto graphicsPage = new GraphicsPage(this);
    auto parserPage = new ParserPage(this);
    auto clientPage = new ClientPage(this);
    auto autoLogPage = new AutoLogPage(this);
    auto mumeProtocolPage = new MumeProtocolPage(this);
    auto pathmachinePage = new PathmachinePage(this);

    m_pagesWidget = new QStackedWidget(this);

    auto *const pagesWidget = m_pagesWidget;
    pagesWidget->addWidget(generalPage);
    pagesWidget->addWidget(graphicsPage);
    pagesWidget->addWidget(parserPage);
    pagesWidget->addWidget(clientPage);
    pagesWidget->addWidget(autoLogPage);
    pagesWidget->addWidget(mumeProtocolPage);
    pagesWidget->addWidget(pathmachinePage);
    pagesWidget->setCurrentIndex(0);

    ui->pagesScrollArea->setWidget(pagesWidget);

    ui->contentsWidget->setCurrentItem(ui->contentsWidget->item(0));
    connect(ui->contentsWidget,
            &QListWidget::currentItemChanged,
            this,
            &ConfigDialog::slot_changePage);
    connect(ui->closeButton, &QAbstractButton::clicked, this, &QWidget::close);

    connect(generalPage, &GeneralPage::sig_factoryReset, this, [this]() {
        qDebug() << "Reloading config due to factory reset";
        emit sig_loadConfig();
    });
    connect(this, &ConfigDialog::sig_loadConfig, generalPage, &GeneralPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, graphicsPage, &GraphicsPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, parserPage, &ParserPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, clientPage, &ClientPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, autoLogPage, &AutoLogPage::slot_loadConfig);
    connect(this,
            &ConfigDialog::sig_loadConfig,
            mumeProtocolPage,
            &MumeProtocolPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, pathmachinePage, &PathmachinePage::slot_loadConfig);

    connect(graphicsPage,
            &GraphicsPage::sig_graphicsSettingsChanged,
            this,
            &ConfigDialog::sig_graphicsSettingsChanged);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::showEvent(QShowEvent *const event)
{
    // Populate the preference pages from config each time the widget is shown
    emit sig_loadConfig();

    // Move widget to center of parent's location
    auto pos = parentWidget()->pos();
    pos.setX(pos.x() + (parentWidget()->width() / 2) - (width() / 2));
    move(pos);

    event->accept();
}

void ConfigDialog::createIcons()
{
    auto *configButton = new QListWidgetItem(ui->contentsWidget);
    configButton->setIcon(QIcon(":/icons/generalcfg.png"));
    configButton->setText(tr("General"));
    configButton->setTextAlignment(Qt::AlignHCenter);
    configButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    auto *graphicsButton = new QListWidgetItem(ui->contentsWidget);
    graphicsButton->setIcon(QIcon(":/icons/graphicscfg.png"));
    graphicsButton->setText(tr("Graphics"));
    graphicsButton->setTextAlignment(Qt::AlignHCenter);
    graphicsButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    auto *updateButton = new QListWidgetItem(ui->contentsWidget);
    updateButton->setIcon(QIcon(":/icons/parsercfg.png"));
    updateButton->setText(tr("Parser"));
    updateButton->setTextAlignment(Qt::AlignHCenter);
    updateButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    auto *clientButton = new QListWidgetItem(ui->contentsWidget);
    clientButton->setIcon(QIcon(":/icons/terminal.png"));
    clientButton->setText(tr("Integrated\nMud Client"));
    clientButton->setTextAlignment(Qt::AlignHCenter);
    clientButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    auto *autoLogButton = new QListWidgetItem(ui->contentsWidget);
    autoLogButton->setIcon(QIcon(":/icons/autologgercfg.png"));
    autoLogButton->setText(tr("Auto\nLogger"));
    autoLogButton->setTextAlignment(Qt::AlignHCenter);
    autoLogButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    auto *mpiButton = new QListWidgetItem(ui->contentsWidget);
    mpiButton->setIcon(QIcon(":/icons/mumeprotocolcfg.png"));
    mpiButton->setText(tr("Mume\nProtocol"));
    mpiButton->setTextAlignment(Qt::AlignHCenter);
    mpiButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    auto *pathButton = new QListWidgetItem(ui->contentsWidget);
    pathButton->setIcon(QIcon(":/icons/pathmachinecfg.png"));
    pathButton->setText(tr("Path\nMachine"));
    pathButton->setTextAlignment(Qt::AlignHCenter);
    pathButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

void ConfigDialog::slot_changePage(QListWidgetItem *current, QListWidgetItem *const previous)
{
    if (current == nullptr) {
        current = previous;
    }
    ui->pagesScrollArea->verticalScrollBar()->setSliderPosition(0);
    m_pagesWidget->setCurrentIndex(ui->contentsWidget->row(current));
}
