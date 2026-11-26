// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "grouppage.h"

#include "../configuration/configuration.h"
#include "ui_grouppage.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
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

    connect(ui->showTokensCheckBox, &QCheckBox::stateChanged, this, [this](int checked) {
        setConfig().groupManager.showTokens = checked;
        emit sig_groupSettingsChanged();
    });

    connect(ui->showMapTokensCheckBox, &QCheckBox::stateChanged, this, [this](int checked) {
        setConfig().groupManager.showMapTokens = checked;
        emit sig_groupSettingsChanged();
    });

    const QString tokenSizeText = QString::number(getConfig().groupManager.tokenIconSize) + " px";
    const int tokenSizeIndex = ui->tokenSizeComboBox->findText(tokenSizeText);
    if (tokenSizeIndex >= 0) {
        ui->tokenSizeComboBox->setCurrentIndex(tokenSizeIndex);
    }

    connect(ui->tokenSizeComboBox, &QComboBox::currentTextChanged, this, [this](const QString &txt) {
        const int value = txt.section(' ', 0, 0).toInt();
        setConfig().groupManager.tokenIconSize = value;
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

    ui->showTokensCheckBox->setChecked(settings.showTokens);
    ui->showMapTokensCheckBox->setChecked(settings.showMapTokens);
    const QString tokenSizeText = QString::number(settings.tokenIconSize) + " px";
    const int tokenSizeIndex = ui->tokenSizeComboBox->findText(tokenSizeText);
    if (tokenSizeIndex >= 0) {
        ui->tokenSizeComboBox->setCurrentIndex(tokenSizeIndex);
    }
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
