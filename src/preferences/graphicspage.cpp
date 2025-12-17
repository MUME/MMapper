// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "graphicspage.h"

#include "../configuration/configuration.h"
#include "../global/utils.h"
#include "AdvancedGraphics.h"
#include "ui_graphicspage.h"

#include <memory>

#include <QString>
#include <QtGui>
#include <QtWidgets>

static void setIconColor(QPushButton *const button, const XNamedColor &namedColor)
{
    QPixmap bgPix(16, 16);
    bgPix.fill(namedColor.getColor().getQColor());
    button->setIcon(QIcon(bgPix));
}

GraphicsPage::GraphicsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GraphicsPage)
{
    ui->setupUi(this);

    m_advanced = std::make_unique<AdvancedGraphicsGroupBox>(deref(ui->groupBox_Advanced));

    connect(ui->bgChangeColor, &QAbstractButton::clicked, this, [this]() {
        changeColorClicked(setConfig().canvas.backgroundColor, ui->bgChangeColor);
        graphicsSettingsChanged();
    });
    connect(ui->darkPushButton, &QAbstractButton::clicked, this, [this]() {
        changeColorClicked(setConfig().canvas.roomDarkColor, ui->darkPushButton);
        graphicsSettingsChanged();
    });
    connect(ui->darkLitPushButton, &QAbstractButton::clicked, this, [this]() {
        changeColorClicked(setConfig().canvas.roomDarkLitColor, ui->darkLitPushButton);
        graphicsSettingsChanged();
    });
    connect(ui->connectionNormalPushButton, &QAbstractButton::clicked, this, [this]() {
        changeColorClicked(setConfig().canvas.connectionNormalColor, ui->connectionNormalPushButton);
        graphicsSettingsChanged();
    });
    connect(ui->antialiasingSamplesComboBox,
            &QComboBox::currentTextChanged,
            this,
            &GraphicsPage::slot_antialiasingSamplesTextChanged);
    connect(ui->trilinearFilteringCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::slot_trilinearFilteringStateChanged);

    connect(ui->drawUnsavedChanges, &QCheckBox::stateChanged, this, [this](int /*unused*/) {
        setConfig().canvas.showUnsavedChanges.set(ui->drawUnsavedChanges->isChecked());
        graphicsSettingsChanged();
    });
    connect(ui->drawNeedsUpdate,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::slot_drawNeedsUpdateStateChanged);
    connect(ui->drawNotMappedExits,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::slot_drawNotMappedExitsStateChanged);
    connect(ui->drawDoorNames,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::slot_drawDoorNamesStateChanged);
    connect(ui->drawUpperLayersTextured,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::slot_drawUpperLayersTexturedStateChanged);

    connect(ui->resourceLineEdit, &QLineEdit::textChanged, this, [](const QString &text) {
        setConfig().canvas.resourcesDirectory = text;
    });
    connect(ui->resourcePushButton, &QAbstractButton::clicked, this, [this](bool /*unused*/) {
        const auto &resourceDir = getConfig().canvas.resourcesDirectory;
        QString directory = QFileDialog::getExistingDirectory(ui->resourcePushButton,
                                                              "Choose resource location ...",
                                                              resourceDir);
        if (!directory.isEmpty()) {
            ui->resourceLineEdit->setText(directory);
            setConfig().canvas.resourcesDirectory = directory;
        }
    });

    connect(ui->tileSetComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &GraphicsPage::slot_textureSetChanged);
    connect(ui->enableSeasonalTilesCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::slot_enableSeasonalTexturesStateChanged);

    connect(m_advanced.get(),
            &AdvancedGraphicsGroupBox::sig_graphicsSettingsChanged,
            this,
            &GraphicsPage::slot_graphicsSettingsChanged);
}

GraphicsPage::~GraphicsPage()
{
    delete ui;
}

void GraphicsPage::slot_loadConfig()
{
    const auto &settings = getConfig().canvas;
    setIconColor(ui->bgChangeColor, settings.backgroundColor);
    setIconColor(ui->darkPushButton, settings.roomDarkColor);
    setIconColor(ui->darkLitPushButton, settings.roomDarkLitColor);
    setIconColor(ui->connectionNormalPushButton, settings.connectionNormalColor);

    const QString antiAliasingSamples = QString::number(settings.antialiasingSamples);
    const int index = utils::clampNonNegative(
        ui->antialiasingSamplesComboBox->findText(antiAliasingSamples));
    ui->antialiasingSamplesComboBox->setCurrentIndex(index);
    ui->trilinearFilteringCheckBox->setChecked(settings.trilinearFiltering);

    ui->drawUnsavedChanges->setChecked(settings.showUnsavedChanges.get());
    ui->drawNeedsUpdate->setChecked(settings.showMissingMapId.get());
    ui->drawNotMappedExits->setChecked(settings.showUnmappedExits.get());
    ui->drawDoorNames->setChecked(settings.drawDoorNames);
    ui->drawUpperLayersTextured->setChecked(settings.drawUpperLayersTextured);

    ui->resourceLineEdit->setText(settings.resourcesDirectory);

    // Block signals to prevent texture reload when just loading config
    ui->tileSetComboBox->blockSignals(true);
    ui->enableSeasonalTilesCheckBox->blockSignals(true);

    ui->tileSetComboBox->setCurrentIndex(static_cast<int>(settings.textureSet));
    ui->enableSeasonalTilesCheckBox->setChecked(settings.enableSeasonalTextures);

    ui->tileSetComboBox->blockSignals(false);
    ui->enableSeasonalTilesCheckBox->blockSignals(false);
}

void GraphicsPage::changeColorClicked(XNamedColor &namedColor, QPushButton *const pushButton)
{
    const QColor origColor = namedColor.getColor().getQColor();
    const QColor newColor = QColorDialog::getColor(origColor, this);
    if (newColor.isValid() && newColor != origColor) {
        namedColor = Color(newColor);
        setIconColor(pushButton, namedColor);
    }
}

void GraphicsPage::slot_antialiasingSamplesTextChanged(const QString & /*unused*/)
{
    setConfig().canvas.antialiasingSamples = ui->antialiasingSamplesComboBox->currentText().toInt();
    graphicsSettingsChanged();
}

void GraphicsPage::slot_trilinearFilteringStateChanged(int /*unused*/)
{
    setConfig().canvas.trilinearFiltering = ui->trilinearFilteringCheckBox->isChecked();
    graphicsSettingsChanged();
}

void GraphicsPage::slot_drawNeedsUpdateStateChanged(int /*unused*/)
{
    setConfig().canvas.showMissingMapId.set(ui->drawNeedsUpdate->isChecked());
    graphicsSettingsChanged();
}

void GraphicsPage::slot_drawNotMappedExitsStateChanged(int /*unused*/)
{
    setConfig().canvas.showUnmappedExits.set(ui->drawNotMappedExits->isChecked());
    graphicsSettingsChanged();
}

void GraphicsPage::slot_drawDoorNamesStateChanged(int /*unused*/)
{
    setConfig().canvas.drawDoorNames = ui->drawDoorNames->isChecked();
    graphicsSettingsChanged();
}

void GraphicsPage::slot_drawUpperLayersTexturedStateChanged(int /*unused*/)
{
    setConfig().canvas.drawUpperLayersTextured = ui->drawUpperLayersTextured->isChecked();
    graphicsSettingsChanged();
}

void GraphicsPage::slot_textureSetChanged(int index)
{
    auto &config = setConfig().canvas;
    switch (index) {
    case 0:
        config.textureSet = TextureSetEnum::CLASSIC;
        break;
    case 1:
        config.textureSet = TextureSetEnum::MODERN;
        break;
    case 2:
        config.textureSet = TextureSetEnum::CUSTOM;
        break;
    default:
        config.textureSet = TextureSetEnum::MODERN;
        break;
    }
    graphicsSettingsChanged();
    emit sig_textureSettingsChanged();
}

void GraphicsPage::slot_enableSeasonalTexturesStateChanged(int /*unused*/)
{
    setConfig().canvas.enableSeasonalTextures = ui->enableSeasonalTilesCheckBox->isChecked();
    graphicsSettingsChanged();
    emit sig_textureSettingsChanged();
}
