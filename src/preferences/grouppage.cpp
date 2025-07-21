// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "grouppage.h"

#include "../configuration/configuration.h"
#include "ui_grouppage.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QPixmap>
#include <QPushButton>

GroupPage::GroupPage(QWidget *const parent)
    : QWidget(parent)
    , ui(new Ui::GroupPage)
{
    ui->setupUi(this);

    connect(ui->yourColorPushButton, &QPushButton::clicked, this, &GroupPage::slot_chooseColor);

    connect(ui->npcOverrideColorCheckBox, &QCheckBox::stateChanged, this, [this](int checked) {
        setConfig().groupManager.npcColorOverride = checked;
        emit sig_groupSettingsChanged();
    });
    connect(ui->npcOverrideColorPushButton,
            &QPushButton::clicked,
            this,
            &GroupPage::slot_chooseNpcOverrideColor);

    connect(ui->npcSortBottomCheckbox, &QCheckBox::stateChanged, this, [this](int checked) {
        setConfig().groupManager.npcSortBottom = checked;
        emit sig_groupSettingsChanged();
    });
    connect(ui->npcHideCheckbox, &QCheckBox::stateChanged, this, [this](int checked) {
        setConfig().groupManager.npcHide = checked;
        emit sig_groupSettingsChanged();
    });
    ui->showTokensCheckbox->setChecked(getConfig().groupManager.showTokens);
    connect(ui->showTokensCheckbox, &QCheckBox::stateChanged, this, [this](int checked) {
        setConfig().groupManager.showTokens = checked;
        emit sig_groupSettingsChanged();
    });
    ui->showMapTokensCheckbox->setChecked(getConfig().groupManager.showMapTokens);
    connect(ui->showMapTokensCheckbox, &QCheckBox::stateChanged, this, [this](int checked) {
        setConfig().groupManager.showMapTokens = checked;
        emit sig_groupSettingsChanged(); // refresh map instantly
    });
    ui->tokenSizeComboBox->setCurrentText(QString::number(getConfig().groupManager.tokenIconSize)
                                          + " px");

    connect(ui->tokenSizeComboBox, &QComboBox::currentTextChanged, this, [this](const QString &txt) {
        // strip " px" and convert to int
        int value = txt.section(' ', 0, 0).toInt();
        setConfig().groupManager.tokenIconSize = value;
        emit sig_groupSettingsChanged(); // live update
    });
    ui->chkShowNpcGhosts->setChecked(getConfig().groupManager.showNpcGhosts);
    connect(ui->chkShowNpcGhosts, &QCheckBox::stateChanged, this, [this](int checked) {
        setConfig().groupManager.showNpcGhosts = checked;
        emit sig_groupSettingsChanged();
    });
    slot_loadConfig();
}

GroupPage::~GroupPage()
{
    delete ui;
}

void GroupPage::slot_loadConfig()
{
    const auto &settings = getConfig().groupManager;

    QPixmap yourPix(16, 16);
    yourPix.fill(settings.color);
    ui->yourColorPushButton->setIcon(QIcon(yourPix));

    ui->npcOverrideColorCheckBox->setChecked(settings.npcColorOverride);
    QPixmap npcOverridePix(16, 16);
    npcOverridePix.fill(settings.npcColor);
    ui->npcOverrideColorPushButton->setIcon(QIcon(npcOverridePix));

    ui->npcSortBottomCheckbox->setChecked(settings.npcSortBottom);
    ui->npcHideCheckbox->setChecked(settings.npcHide);
    ui->showTokensCheckbox->setChecked(settings.showTokens);
    ui->showMapTokensCheckbox->setChecked(settings.showMapTokens);
    ui->chkShowNpcGhosts->setChecked(settings.showNpcGhosts);
    ui->tokenSizeComboBox->setCurrentText(QString::number(settings.tokenIconSize) + " px");
}

void GroupPage::slot_chooseColor()
{
    const QColor color = QColorDialog::getColor(getConfig().groupManager.color,
                                                this,
                                                tr("Select Your Color"));

    if (color.isValid()) {
        setConfig().groupManager.color = color;
        slot_loadConfig();
        emit sig_groupSettingsChanged();
    }
}

void GroupPage::slot_chooseNpcOverrideColor()
{
    const QColor color = QColorDialog::getColor(getConfig().groupManager.npcColor,
                                                this,
                                                tr("Select NPC Override Color"));

    if (color.isValid()) {
        setConfig().groupManager.npcColor = color;
        slot_loadConfig();
        emit sig_groupSettingsChanged();
    }
}
