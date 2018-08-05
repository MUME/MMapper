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
    const auto &settings = Config().canvas;

    ui->setupUi(this);

    connect(ui->changeColor, &QAbstractButton::clicked, this, &GraphicsPage::changeColorClicked);
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
    ui->changeColor->setIcon(QIcon(bgPix));

    const QString antiAliasingSamples = QString::number(settings.antialiasingSamples);
    const int index = std::max(0, ui->antialiasingSamplesComboBox->findText(antiAliasingSamples));
    ui->antialiasingSamplesComboBox->setCurrentIndex(index);
    ui->trilinearFilteringCheckBox->setChecked(settings.trilinearFiltering);
    ui->softwareOpenGLCheckBox->setChecked(settings.softwareOpenGL);

    ui->updated->setChecked(settings.showUpdated);
    ui->drawNotMappedExits->setChecked(settings.drawNotMappedExits);
    ui->drawNoMatchExits->setChecked(settings.drawNoMatchExits);
    ui->drawUpperLayersTextured->setChecked(settings.drawUpperLayersTextured);
    ui->drawDoorNames->setChecked(settings.drawDoorNames);
}

void GraphicsPage::changeColorClicked()
{
    auto &backgroundColor = Config().canvas.backgroundColor;
    const QColor newColor = QColorDialog::getColor(backgroundColor, this);
    if (newColor.isValid() && newColor != backgroundColor) {
        QPixmap bgPix(16, 16);
        bgPix.fill(newColor);
        ui->changeColor->setIcon(QIcon(bgPix));
        backgroundColor = newColor;
    }
}

void GraphicsPage::antialiasingSamplesTextChanged(const QString & /*unused*/)
{
    Config().canvas.antialiasingSamples = ui->antialiasingSamplesComboBox->currentText().toInt();
}

void GraphicsPage::trilinearFilteringStateChanged(int /*unused*/)
{
    Config().canvas.trilinearFiltering = ui->trilinearFilteringCheckBox->isChecked();
}

void GraphicsPage::softwareOpenGLStateChanged(int /*unused*/)
{
    Config().canvas.softwareOpenGL = ui->softwareOpenGLCheckBox->isChecked();
}

void GraphicsPage::updatedStateChanged(int /*unused*/)
{
    Config().canvas.showUpdated = ui->updated->isChecked();
}

void GraphicsPage::drawNotMappedExitsStateChanged(int /*unused*/)
{
    Config().canvas.drawNotMappedExits = ui->drawNotMappedExits->isChecked();
}

void GraphicsPage::drawNoMatchExitsStateChanged(int /*unused*/)
{
    Config().canvas.drawNoMatchExits = ui->drawNoMatchExits->isChecked();
}

void GraphicsPage::drawDoorNamesStateChanged(int /*unused*/)
{
    Config().canvas.drawDoorNames = ui->drawDoorNames->isChecked();
}

void GraphicsPage::drawUpperLayersTexturedStateChanged(int /*unused*/)
{
    Config().canvas.drawUpperLayersTextured = ui->drawUpperLayersTextured->isChecked();
}
