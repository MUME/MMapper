// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AnsiColorDialog.h"

#include "ansicombo.h"
#include "ui_AnsiColorDialog.h"

AnsiColorDialog::AnsiColorDialog(const QString &ansiString, QWidget *parent)
    : QDialog(parent)
    , ansiString{ansiString}
    , ui(new Ui::AnsiColorDialog)
{
    ui->setupUi(this);

    ui->foregroundAnsiCombo->initColours(AnsiModeEnum::ANSI_FG);
    ui->backgroundAnsiCombo->initColours(AnsiModeEnum::ANSI_BG);

    slot_updateColors();

    connect(ui->foregroundAnsiCombo,
            QOverload<const QString &>::of(&QComboBox::textActivated),
            this,
            &AnsiColorDialog::slot_ansiComboChange);

    connect(ui->backgroundAnsiCombo,
            QOverload<const QString &>::of(&QComboBox::textActivated),
            this,
            &AnsiColorDialog::slot_ansiComboChange);

    connect(ui->boldCheckBox,
            &QAbstractButton::toggled,
            this,
            &AnsiColorDialog::slot_ansiComboChange);
    connect(ui->italicCheckBox,
            &QAbstractButton::toggled,
            this,
            &AnsiColorDialog::slot_ansiComboChange);
    connect(ui->underlineCheckBox,
            &QAbstractButton::toggled,
            this,
            &AnsiColorDialog::slot_ansiComboChange);
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

void AnsiColorDialog::slot_ansiComboChange()
{
    slot_generateNewAnsiColor();
    slot_updateColors();
}

void AnsiColorDialog::slot_updateColors()
{
    AnsiCombo::makeWidgetColoured(ui->exampleLabel, ansiString, false);

    AnsiCombo::AnsiColor color = AnsiCombo::colorFromString(ansiString);

    ui->boldCheckBox->setChecked(color.bold);
    ui->italicCheckBox->setChecked(color.italic);
    ui->underlineCheckBox->setChecked(color.underline);

    QString toolTipString = ansiString.isEmpty() ? "[0m" : ansiString;
    ui->exampleLabel->setToolTip(toolTipString);

    ui->foregroundAnsiCombo->setAnsiCode(color.ansiCodeFg);
    ui->backgroundAnsiCombo->setAnsiCode(color.ansiCodeBg);
}

void AnsiColorDialog::slot_generateNewAnsiColor()
{
    const auto getColor = [](auto fg, auto bg, auto bold, auto italic, auto underline) {
        QString result = "";
        auto add = [&result](auto &s) {
            if (result.isEmpty())
                result += "[";
            else
                result += ";";
            result += s;
        };

        const auto colorText = QString("%1").arg(fg->getAnsiCode());
        if (colorText != QString("%1").arg(AnsiCombo::DEFAULT_FG))
            add(colorText);

        const auto bgText = QString("%1").arg(bg->getAnsiCode());
        if (bgText != QString("%1").arg(AnsiCombo::DEFAULT_BG))
            add(bgText);

        if (bold->isChecked())
            add("1");

        if (italic->isChecked())
            add("3");

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
                          ui->italicCheckBox,
                          ui->underlineCheckBox);

    emit sig_newAnsiString(ansiString);
}
