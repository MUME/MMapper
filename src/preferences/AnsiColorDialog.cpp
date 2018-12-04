/************************************************************************
**
** Authors:   Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
**            Marek Krejza <krejza@gmail.com> (Caligor)
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "AnsiColorDialog.h"
#include "ui_AnsiColorDialog.h"

#include "ansicombo.h"

AnsiColorDialog::AnsiColorDialog(const QString &ansiString, QWidget *parent)
    : QDialog(parent)
    , ansiString{ansiString}
    , ui(new Ui::AnsiColorDialog)
{
    ui->setupUi(this);

    setWindowTitle("Ansi Color Dialog");

    ui->foregroundAnsiCombo->initColours(AnsiMode::ANSI_FG);
    ui->backgroundAnsiCombo->initColours(AnsiMode::ANSI_BG);

    connect(ui->foregroundAnsiCombo,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this,
            &AnsiColorDialog::ansiComboChange);

    connect(ui->backgroundAnsiCombo,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this,
            &AnsiColorDialog::ansiComboChange);

    connect(ui->boldCheckBox, &QAbstractButton::toggled, this, &AnsiColorDialog::ansiComboChange);
    connect(ui->underlineCheckBox,
            &QAbstractButton::toggled,
            this,
            &AnsiColorDialog::ansiComboChange);

    updateColors();
}

AnsiColorDialog::AnsiColorDialog(QWidget *parent)
    : AnsiColorDialog("[0m", parent)
{}

QString AnsiColorDialog::getColor(const QString &ansiColor, QWidget *parent)
{
    AnsiColorDialog dlg(ansiColor, parent);
    if (dlg.exec() == QDialog::Accepted)
        return dlg.getAnsiString();
    return ansiColor;
}

AnsiColorDialog::~AnsiColorDialog()
{
    delete ui;
}

void AnsiColorDialog::ansiComboChange()
{
    generateNewAnsiColor();
    updateColors();
}

void AnsiColorDialog::updateColors()
{
    // TODO: turn this into a struct!
    QColor colFg;
    QColor colBg;
    int ansiCodeFg;
    int ansiCodeBg;
    QString intelligibleNameFg;
    QString intelligibleNameBg;
    bool bold;
    bool underline;

    AnsiCombo::makeWidgetColoured(ui->exampleLabel, ansiString);

    AnsiCombo::colorFromString(ansiString,
                               colFg,
                               ansiCodeFg,
                               intelligibleNameFg,
                               colBg,
                               ansiCodeBg,
                               intelligibleNameBg,
                               bold,
                               underline);

    ui->boldCheckBox->setChecked(bold);
    ui->underlineCheckBox->setChecked(underline);

    QString displayString = ansiString.isEmpty() ? "[0m" : ansiString;
    if (bold)
        displayString = QString("<b>%1</b>").arg(displayString);
    if (underline)
        displayString = QString("<u>%1</u>").arg(displayString);
    ui->exampleLabel->setText(displayString);

    ui->foregroundAnsiCombo->setEditable(false);
    ui->backgroundAnsiCombo->setEditable(false);
    ui->foregroundAnsiCombo->setText(QString("%1").arg(ansiCodeFg));
    ui->backgroundAnsiCombo->setText(QString("%1").arg(ansiCodeBg));
}

void AnsiColorDialog::generateNewAnsiColor()
{
    const auto getColor = [](auto color, auto bg, auto bold, auto underline) {
        QString result = "";
        auto add = [&result](auto &s) {
            if (result.isEmpty())
                result += "[";
            else
                result += ";";
            result += s;
        };

        const auto &colorText = color->text();
        if (colorText != "none")
            add(colorText);

        const auto &bgText = bg->text();
        if (bgText != "none")
            add(bgText);

        if (bold->isChecked())
            add("1");

        if (underline->isChecked())
            add("4");

        if (result.isEmpty())
            return result;

        result.append("m");
        return result;
    };

    ansiString = getColor(ui->foregroundAnsiCombo,
                          ui->backgroundAnsiCombo,
                          ui->boldCheckBox,
                          ui->underlineCheckBox);

    emit newAnsiString(ansiString);
}
