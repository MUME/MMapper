// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "graphicspage.h"

#include <memory>
#include <QString>
#include <QtGui>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "../global/utils.h"
#include "AdvancedGraphics.h"
#include "ui_graphicspage.h"

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
            &GraphicsPage::antialiasingSamplesTextChanged);
    connect(ui->trilinearFilteringCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::trilinearFilteringStateChanged);
    connect(ui->softwareOpenGLCheckBox, &QCheckBox::clicked, this, [this]() {
        QMessageBox::information(this,
                                 "Restart Required",
                                 "Please restart MMapper for this change to take effect.");
        setConfig().canvas.softwareOpenGL = ui->softwareOpenGLCheckBox->isChecked();
    });

    connect(ui->updated, &QCheckBox::stateChanged, this, &GraphicsPage::updatedStateChanged);
    connect(ui->drawNotMappedExits,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::drawNotMappedExitsStateChanged);
    connect(ui->drawDoorNames,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::drawDoorNamesStateChanged);
    connect(ui->drawUpperLayersTextured,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::drawUpperLayersTexturedStateChanged);

    connect(m_advanced.get(),
            &AdvancedGraphicsGroupBox::sig_graphicsSettingsChanged,
            this,
            &GraphicsPage::graphicsSettingsChanged);
}

GraphicsPage::~GraphicsPage()
{
    delete ui;
}

void GraphicsPage::loadConfig()
{
    const auto &settings = getConfig().canvas;
    setIconColor(ui->bgChangeColor, settings.backgroundColor);
    setIconColor(ui->darkPushButton, settings.roomDarkColor);
    setIconColor(ui->darkLitPushButton, settings.roomDarkLitColor);
    setIconColor(ui->connectionNormalPushButton, settings.connectionNormalColor);

    const QString antiAliasingSamples = QString::number(settings.antialiasingSamples);
    const int index = std::max(0, ui->antialiasingSamplesComboBox->findText(antiAliasingSamples));
    ui->antialiasingSamplesComboBox->setCurrentIndex(index);
    ui->trilinearFilteringCheckBox->setChecked(settings.trilinearFiltering);
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac) {
        ui->softwareOpenGLCheckBox->setEnabled(false);
        ui->softwareOpenGLCheckBox->setChecked(false);
    } else {
        ui->softwareOpenGLCheckBox->setChecked(settings.softwareOpenGL);
    }

    ui->updated->setChecked(settings.showUpdated);
    ui->drawNotMappedExits->setChecked(settings.drawNotMappedExits);
    ui->drawUpperLayersTextured->setChecked(settings.drawUpperLayersTextured);
    ui->drawDoorNames->setChecked(settings.drawDoorNames);
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

void GraphicsPage::antialiasingSamplesTextChanged(const QString & /*unused*/)
{
    setConfig().canvas.antialiasingSamples = ui->antialiasingSamplesComboBox->currentText().toInt();
    graphicsSettingsChanged();
}

void GraphicsPage::trilinearFilteringStateChanged(int /*unused*/)
{
    setConfig().canvas.trilinearFiltering = ui->trilinearFilteringCheckBox->isChecked();
    graphicsSettingsChanged();
}

void GraphicsPage::updatedStateChanged(int /*unused*/)
{
    setConfig().canvas.showUpdated = ui->updated->isChecked();
}

void GraphicsPage::drawNotMappedExitsStateChanged(int /*unused*/)
{
    setConfig().canvas.drawNotMappedExits = ui->drawNotMappedExits->isChecked();
}

void GraphicsPage::drawDoorNamesStateChanged(int /*unused*/)
{
    setConfig().canvas.drawDoorNames = ui->drawDoorNames->isChecked();
    graphicsSettingsChanged();
}

void GraphicsPage::drawUpperLayersTexturedStateChanged(int /*unused*/)
{
    setConfig().canvas.drawUpperLayersTextured = ui->drawUpperLayersTextured->isChecked();
    graphicsSettingsChanged();
}
