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
#include "ui_clientpage.h"
#include "configuration/configuration.h"

#include <QFont>
#include <QFontInfo>
#include <QFontDialog>

#include <QColorDialog>

#include <QSpinBox>

ClientPage::ClientPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ClientPage)
{
    ui->setupUi(this);

    updateFontAndColors();

    ui->columnsSpinBox->setValue(Config().m_clientColumns);
    ui->rowsSpinBox->setValue(Config().m_clientRows);
    ui->scrollbackSpinBox->setValue(Config().m_clientLinesOfScrollback);

    ui->clearInputCheckBox->setChecked(Config().m_clientClearInputOnEnter);

    connect(ui->fontPushButton, SIGNAL(pressed()), this, SLOT(onChangeFont()));
    connect(ui->bgColorPushBotton, SIGNAL(pressed()), this, SLOT(onChangeBackgroundColor()));
    connect(ui->fgColorPushButton, SIGNAL(pressed()), this, SLOT(onChangeForegroundColor()));

    connect(ui->columnsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onChangeColumns(int)));
    connect(ui->rowsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onChangeRows(int)));
    connect(ui->scrollbackSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(onChangeLinesOfScrollback(int)));

    connect(ui->clearInputCheckBox, &QCheckBox::toggled, [ = ] (bool isChecked) {
        Config().m_clientClearInputOnEnter = isChecked;
    });
}

ClientPage::~ClientPage()
{
    delete ui;
}

void ClientPage::updateFontAndColors()
{
    QFont font = Config().m_clientFont;
    ui->exampleLineEdit->setFont(font);

    QFontInfo fi(font);
    ui->fontPushButton->setText(QString("%1, %2").arg(fi.styleName()).arg(fi.pointSize()));

    QPixmap fgPix(16, 16);
    fgPix.fill(Config().m_clientForegroundColor);
    ui->fgColorPushButton->setIcon(QIcon(fgPix));

    QPixmap bgPix(16, 16);
    bgPix.fill(Config().m_clientBackgroundColor);
    ui->bgColorPushBotton->setIcon(QIcon(bgPix));
    ui->exampleLineEdit->setStyleSheet(QString("background: %1; color: %2")
                                       .arg(Config().m_clientBackgroundColor.name())
                                       .arg(Config().m_clientForegroundColor.name()));
}

void ClientPage::onChangeFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, Config().m_clientFont, this, "Select Font",
                                      QFontDialog::MonospacedFonts);
    if (ok) {
        Config().m_clientFont = font;
        updateFontAndColors();
    }
}

void ClientPage::onChangeBackgroundColor()
{
    const QColor newColor = QColorDialog::getColor(Config().m_clientBackgroundColor, this);
    if (newColor.isValid() && newColor != Config().m_clientBackgroundColor) {
        Config().m_clientBackgroundColor = newColor;
        updateFontAndColors();
    }
}

void ClientPage::onChangeForegroundColor()
{
    const QColor newColor = QColorDialog::getColor(Config().m_clientForegroundColor, this);
    if (newColor.isValid() && newColor != Config().m_clientForegroundColor) {
        Config().m_clientForegroundColor = newColor;
        updateFontAndColors();
    }
}

void ClientPage::onChangeColumns(int value)
{
    Config().m_clientColumns = value;
}

void ClientPage::onChangeRows(int value)
{
    Config().m_clientRows = value;
}

void ClientPage::onChangeLinesOfScrollback(int value)
{
    Config().m_clientLinesOfScrollback = value;
}
