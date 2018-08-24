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

#include "clientpage.h"

#include <QFont>
#include <QFontInfo>
#include <QSpinBox>
#include <QString>
#include <QtGui>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "ui_clientpage.h"

ClientPage::ClientPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ClientPage)
{
    ui->setupUi(this);

    updateFontAndColors();

    const auto &settings = getConfig().integratedClient;
    ui->columnsSpinBox->setValue(settings.columns);
    ui->rowsSpinBox->setValue(settings.rows);
    ui->scrollbackSpinBox->setValue(settings.linesOfScrollback);
    ui->clearInputCheckBox->setChecked(settings.clearInputOnEnter);
    ui->autoResizeTerminalCheckBox->setChecked(settings.autoResizeTerminal);

    connect(ui->fontPushButton, &QAbstractButton::pressed, this, &ClientPage::onChangeFont);
    connect(ui->bgColorPushButton,
            &QAbstractButton::pressed,
            this,
            &ClientPage::onChangeBackgroundColor);
    connect(ui->fgColorPushButton,
            &QAbstractButton::pressed,
            this,
            &ClientPage::onChangeForegroundColor);

    connect(ui->columnsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onChangeColumns(int)));
    connect(ui->rowsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onChangeRows(int)));
    connect(ui->scrollbackSpinBox,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(onChangeLinesOfScrollback(int)));

    connect(ui->inputHistorySpinBox,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(onChangeLinesOfInputHistory(int)));
    connect(ui->tabDictionarySpinBox,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(onChangeTabCompletionDictionarySize(int)));

    connect(ui->clearInputCheckBox, &QCheckBox::toggled, [](bool isChecked) {
        /* NOTE: This directly modifies the global setting. */
        setConfig().integratedClient.clearInputOnEnter = isChecked;
    });

    connect(ui->autoResizeTerminalCheckBox, &QCheckBox::toggled, [](bool isChecked) {
        /* NOTE: This directly modifies the global setting. */
        setConfig().integratedClient.autoResizeTerminal = isChecked;
    });
}

ClientPage::~ClientPage()
{
    delete ui;
}

void ClientPage::updateFontAndColors()
{
    const auto &settings = getConfig().integratedClient;
    ui->exampleLineEdit->setFont(settings.font);

    QFontInfo fi(settings.font);
    ui->fontPushButton->setText(
        QString("%1 %2, %3").arg(fi.family()).arg(fi.styleName()).arg(fi.pointSize()));

    QPixmap fgPix(16, 16);
    fgPix.fill(settings.foregroundColor);
    ui->fgColorPushButton->setIcon(QIcon(fgPix));

    QPixmap bgPix(16, 16);
    bgPix.fill(settings.backgroundColor);
    ui->bgColorPushButton->setIcon(QIcon(bgPix));
    ui->exampleLineEdit->setStyleSheet(QString("background: %1; color: %2")
                                           .arg(settings.backgroundColor.name())
                                           .arg(settings.foregroundColor.name()));
}

void ClientPage::onChangeFont()
{
    auto &font = setConfig().integratedClient.font;

    bool ok = false;
    const QFont newFont = QFontDialog::getFont(&ok,
                                               font,
                                               this,
                                               "Select Font",
                                               QFontDialog::MonospacedFonts);
    if (ok) {
        font = newFont;
        updateFontAndColors();
    }
}

void ClientPage::onChangeBackgroundColor()
{
    auto &backgroundColor = setConfig().integratedClient.backgroundColor;
    const QColor newColor = QColorDialog::getColor(backgroundColor, this);
    if (newColor.isValid() && newColor != backgroundColor) {
        backgroundColor = newColor;
        updateFontAndColors();
    }
}

void ClientPage::onChangeForegroundColor()
{
    auto &foregroundColor = setConfig().integratedClient.foregroundColor;
    const QColor newColor = QColorDialog::getColor(foregroundColor, this);
    if (newColor.isValid() && newColor != foregroundColor) {
        foregroundColor = newColor;
        updateFontAndColors();
    }
}

void ClientPage::onChangeColumns(const int value)
{
    setConfig().integratedClient.columns = value;
}

void ClientPage::onChangeRows(const int value)
{
    setConfig().integratedClient.rows = value;
}

void ClientPage::onChangeLinesOfScrollback(const int value)
{
    setConfig().integratedClient.linesOfScrollback = value;
}

void ClientPage::onChangeLinesOfInputHistory(const int value)
{
    setConfig().integratedClient.linesOfInputHistory = value;
}

void ClientPage::onChangeTabCompletionDictionarySize(const int value)
{
    setConfig().integratedClient.tabCompletionDictionarySize = value;
}
