// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "configdialog.h"

#include "autologpage.h"
#include "clientpage.h"
#include "commspage.h"
#include "generalpage.h"
#include "graphicspage.h"
#include "grouppage.h"
#include "hotkeyspage.h"
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
    auto groupPage = new GroupPage(this);
    auto autoLogPage = new AutoLogPage(this);
    auto mumeProtocolPage = new MumeProtocolPage(this);
    auto pathmachinePage = new PathmachinePage(this);
    auto hotkeysPage = new HotkeysPage(this);
    auto commsPage = new CommsPage(this);

    m_pagesWidget = new QStackedWidget(this);

    auto *const pagesWidget = m_pagesWidget;
    pagesWidget->addWidget(generalPage);
    pagesWidget->addWidget(graphicsPage);
    pagesWidget->addWidget(hotkeysPage);
    pagesWidget->addWidget(commsPage);
    pagesWidget->addWidget(parserPage);
    pagesWidget->addWidget(clientPage);
    pagesWidget->addWidget(groupPage);
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
    connect(this, &ConfigDialog::sig_loadConfig, groupPage, &GroupPage::slot_loadConfig);
    connect(groupPage,
            &GroupPage::sig_groupSettingsChanged,
            this,
            &ConfigDialog::sig_groupSettingsChanged);
    connect(this,
            &ConfigDialog::sig_loadConfig,
            mumeProtocolPage,
            &MumeProtocolPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, pathmachinePage, &PathmachinePage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, hotkeysPage, &HotkeysPage::slot_loadConfig);
    connect(hotkeysPage, &HotkeysPage::sig_hotkeysChanged, this, &ConfigDialog::sig_hotkeysChanged);
    connect(this, &ConfigDialog::sig_loadConfig, commsPage, &CommsPage::slot_loadConfig);
    connect(commsPage,
            &CommsPage::sig_commsSettingsChanged,
            this,
            &ConfigDialog::sig_commsSettingsChanged);
    connect(graphicsPage,
            &GraphicsPage::sig_graphicsSettingsChanged,
            this,
            &ConfigDialog::sig_graphicsSettingsChanged);
    connect(graphicsPage,
            &GraphicsPage::sig_textureSettingsChanged,
            this,
            &ConfigDialog::sig_textureSettingsChanged);
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
    const QSize iconTargetSize = ui->contentsWidget->iconSize();

    auto addItem = [&](const QString &iconPath, const QString &label) {
        QPixmap pixmap(iconPath);
        QPixmap scaled = pixmap.scaled(iconTargetSize,
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);

        auto *item = new QListWidgetItem(QIcon(scaled), label, ui->contentsWidget);
        item->setTextAlignment(Qt::AlignHCenter);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return item;
    };

    addItem(":/icons/generalcfg.png", tr("General"));
    addItem(":/icons/graphicscfg.png", tr("Graphics"));
    addItem(":/icons/hotkeys.png", tr("Hotkeys"));
    addItem(":/icons/comms.png", tr("Comms"));
    addItem(":/icons/parsercfg.png", tr("Parser"));
    addItem(":/icons/terminal.png", tr("Integrated\nMud Client"));
    addItem(":/icons/group-recolor.png", tr("Group Panel"));
    addItem(":/icons/autologgercfg.png", tr("Auto\nLogger"));
    addItem(":/icons/mumeprotocolcfg.png", tr("Mume\nProtocol"));
    addItem(":/icons/pathmachinecfg.png", tr("Path\nMachine"));
}

void ConfigDialog::slot_changePage(QListWidgetItem *current, QListWidgetItem *const previous)
{
    if (current == nullptr) {
        current = previous;
    }
    ui->pagesScrollArea->verticalScrollBar()->setSliderPosition(0);
    m_pagesWidget->setCurrentIndex(ui->contentsWidget->row(current));
}
