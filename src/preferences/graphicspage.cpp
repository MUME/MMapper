// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "graphicspage.h"

#include <QString>
#include <QtGui>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "ui_graphicspage.h"

GraphicsPage::GraphicsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GraphicsPage)
{
    const auto &settings = getConfig().canvas;

    ui->setupUi(this);

    connect(ui->bgChangeColor, &QAbstractButton::clicked, this, [this]() {
        changeColorClicked(setConfig().canvas.backgroundColor, ui->bgChangeColor);
    });
    connect(ui->darkPushButton, &QAbstractButton::clicked, this, [this]() {
        changeColorClicked(setConfig().canvas.roomDarkColor, ui->darkPushButton);
    });
    connect(ui->darkLitPushButton, &QAbstractButton::clicked, this, [this]() {
        changeColorClicked(setConfig().canvas.roomDarkLitColor, ui->darkLitPushButton);
    });
    connect(ui->antialiasingSamplesComboBox,
            &QComboBox::currentTextChanged,
            this,
            &GraphicsPage::antialiasingSamplesTextChanged);
    connect(ui->trilinearFilteringCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::trilinearFilteringStateChanged);
    connect(ui->softwareOpenGLCheckBox,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::softwareOpenGLStateChanged);

    connect(ui->updated, &QCheckBox::stateChanged, this, &GraphicsPage::updatedStateChanged);
    connect(ui->drawNotMappedExits,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::drawNotMappedExitsStateChanged);
    connect(ui->drawNoMatchExits,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::drawNoMatchExitsStateChanged);
    connect(ui->drawDoorNames,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::drawDoorNamesStateChanged);
    connect(ui->drawUpperLayersTextured,
            &QCheckBox::stateChanged,
            this,
            &GraphicsPage::drawUpperLayersTexturedStateChanged);

    QPixmap bgPix(16, 16);
    bgPix.fill(settings.backgroundColor);
    ui->bgChangeColor->setIcon(QIcon(bgPix));
    QPixmap darkRoomPix(16, 16);
    darkRoomPix.fill(settings.roomDarkColor);
    ui->darkPushButton->setIcon(QIcon(darkRoomPix));
    QPixmap darkRoomLitPix(16, 16);
    darkRoomLitPix.fill(settings.roomDarkLitColor);
    ui->darkLitPushButton->setIcon(QIcon(darkRoomLitPix));

    const QString antiAliasingSamples = QString::number(settings.antialiasingSamples);
    const int index = std::max(0, ui->antialiasingSamplesComboBox->findText(antiAliasingSamples));
    ui->antialiasingSamplesComboBox->setCurrentIndex(index);
    ui->trilinearFilteringCheckBox->setChecked(settings.trilinearFiltering);
    if (getCurrentPlatform() == Platform::Mac) {
        ui->softwareOpenGLCheckBox->setEnabled(false);
        ui->softwareOpenGLCheckBox->setChecked(false);
    } else
        ui->softwareOpenGLCheckBox->setChecked(settings.softwareOpenGL);

    ui->updated->setChecked(settings.showUpdated);
    ui->drawNotMappedExits->setChecked(settings.drawNotMappedExits);
    ui->drawNoMatchExits->setChecked(settings.drawNoMatchExits);
    ui->drawUpperLayersTextured->setChecked(settings.drawUpperLayersTextured);
    ui->drawDoorNames->setChecked(settings.drawDoorNames);
}

void GraphicsPage::changeColorClicked(QColor &oldColor, QPushButton *pushButton)
{
    const QColor newColor = QColorDialog::getColor(oldColor, this);
    if (newColor.isValid() && newColor != oldColor) {
        QPixmap pix(16, 16);
        pix.fill(newColor);
        pushButton->setIcon(QIcon(pix));
        oldColor = newColor;
    }
}

void GraphicsPage::antialiasingSamplesTextChanged(const QString & /*unused*/)
{
    setConfig().canvas.antialiasingSamples = ui->antialiasingSamplesComboBox->currentText().toInt();
}

void GraphicsPage::trilinearFilteringStateChanged(int /*unused*/)
{
    setConfig().canvas.trilinearFiltering = ui->trilinearFilteringCheckBox->isChecked();
}

void GraphicsPage::softwareOpenGLStateChanged(int /*unused*/)
{
    setConfig().canvas.softwareOpenGL = ui->softwareOpenGLCheckBox->isChecked();
}

void GraphicsPage::updatedStateChanged(int /*unused*/)
{
    setConfig().canvas.showUpdated = ui->updated->isChecked();
}

void GraphicsPage::drawNotMappedExitsStateChanged(int /*unused*/)
{
    setConfig().canvas.drawNotMappedExits = ui->drawNotMappedExits->isChecked();
}

void GraphicsPage::drawNoMatchExitsStateChanged(int /*unused*/)
{
    setConfig().canvas.drawNoMatchExits = ui->drawNoMatchExits->isChecked();
}

void GraphicsPage::drawDoorNamesStateChanged(int /*unused*/)
{
    setConfig().canvas.drawDoorNames = ui->drawDoorNames->isChecked();
}

void GraphicsPage::drawUpperLayersTexturedStateChanged(int /*unused*/)
{
    setConfig().canvas.drawUpperLayersTextured = ui->drawUpperLayersTextured->isChecked();
}
