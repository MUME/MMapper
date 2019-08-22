// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "configdialog.h"

#include <QIcon>
#include <QListWidget>
#include <QtWidgets>

#include "clientpage.h"
#include "generalpage.h"
#include "graphicspage.h"
#include "groupmanagerpage.h"
#include "mumeprotocolpage.h"
#include "parserpage.h"
#include "pathmachinepage.h"
#include "ui_configdialog.h"

ConfigDialog::ConfigDialog(Mmapper2Group *gm, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
    , m_groupManager(gm)
{
    ui->setupUi(this);

    setWindowTitle(tr("Config Dialog"));

    ui->contentsWidget->setViewMode(QListView::IconMode);
    ui->contentsWidget->setIconSize(QSize(70, 70));
    ui->contentsWidget->setMovement(QListView::Static);
    ui->contentsWidget->setMaximumWidth(110);
    ui->contentsWidget->setMinimumWidth(110);
    ui->contentsWidget->setSpacing(9);

    createIcons();

    pagesWidget = new QStackedWidget(this);
    pagesWidget->addWidget(new GeneralPage(this));
    pagesWidget->addWidget(new GraphicsPage(this));
    pagesWidget->addWidget(new ParserPage(this));
    pagesWidget->addWidget(new ClientPage(this));
    pagesWidget->addWidget(new GroupManagerPage(m_groupManager, this));
    pagesWidget->addWidget(new MumeProtocolPage(this));
    pagesWidget->addWidget(new PathmachinePage(this));
    pagesWidget->setCurrentIndex(0);

    ui->pagesScrollArea->setMinimumWidth(520);
    ui->pagesScrollArea->setWidget(pagesWidget);

    ui->contentsWidget->setCurrentItem(ui->contentsWidget->item(0));
    connect(ui->contentsWidget, &QListWidget::currentItemChanged, this, &ConfigDialog::changePage);
    connect(ui->closeButton, &QAbstractButton::clicked, this, &QWidget::close);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
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

    auto *groupButton = new QListWidgetItem(ui->contentsWidget);
    groupButton->setIcon(QIcon(":/icons/groupcfg.png"));
    groupButton->setText(tr("Group\nManager"));
    groupButton->setTextAlignment(Qt::AlignHCenter);
    groupButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

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

void ConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (current == nullptr) {
        current = previous;
    }
    ui->pagesScrollArea->verticalScrollBar()->setSliderPosition(0);
    pagesWidget->setCurrentIndex(ui->contentsWidget->row(current));
}
