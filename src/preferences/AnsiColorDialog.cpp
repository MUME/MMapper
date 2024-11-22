// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AnsiColorDialog.h"

#include "../global/logging.h"
#include "ansicombo.h"
#include "ui_AnsiColorDialog.h"

AnsiColorDialog::AnsiColorDialog(const QString &ansiString, QWidget *parent)
    : QDialog(parent)
    , ansiString{ansiString}
    , ui(new Ui::AnsiColorDialog)
{
    ui->setupUi(this);

    AnsiCombo *const bg = ui->backgroundAnsiCombo;
    AnsiCombo *const fg = ui->foregroundAnsiCombo;

    bg->initColours(AnsiColor16LocationEnum::Background);
    fg->initColours(AnsiColor16LocationEnum::Foreground);

    slot_updateColors();

    auto connectCombo = [this](AnsiCombo *const combo) {
        connect(combo,
                QOverload<const QString &>::of(&QComboBox::textActivated),
                this,
                &AnsiColorDialog::ansiComboChange);
        if (false)
            connect(combo,
                    QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this,
                    &AnsiColorDialog::ansiComboChange);
    };

    auto connectCheckBox = [this](QCheckBox *const checkBox) {
        connect(checkBox, &QAbstractButton::toggled, this, &AnsiColorDialog::ansiComboChange);
    };

    connectCombo(bg);
    connectCombo(fg);
    connectCheckBox(ui->boldCheckBox);
    connectCheckBox(ui->italicCheckBox);
    connectCheckBox(ui->underlineCheckBox);
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

void AnsiColorDialog::ansiComboChange()
{
    blockSignals(true);
    slot_ansiComboChange();
    blockSignals(false);
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

    ui->backgroundAnsiCombo->setAnsiCode(color.bg);
    ui->foregroundAnsiCombo->setAnsiCode(color.fg);
}

void AnsiColorDialog::slot_generateNewAnsiColor()
{
    const auto getColor = [](const AnsiCombo *const fg,
                             const AnsiCombo *const bg,
                             const QCheckBox *const bold,
                             const QCheckBox *const italic,
                             const QCheckBox *const underline) -> QString {
        RawAnsi raw;

        raw.fg = AnsiColorVariant{fg->getAnsiCode()};
        raw.bg = AnsiColorVariant{bg->getAnsiCode()};

        if (bold->isChecked())
            raw.setBold();
        if (italic->isChecked())
            raw.setItalic();
        if (underline->isChecked())
            raw.setUnderline();

        if (raw == RawAnsi{})
            return "";

        const AnsiString s = ansi_string(ANSI_COLOR_SUPPORT_HI, raw);
        if (s.isEmpty())
            return "";

        auto sv = s.getStdStringView();
        assert(sv.front() == char_consts::C_ESC);
        assert(sv.back() == 'm');
        sv.remove_prefix(1);
        return mmqt::toQStringLatin1(sv);
    };

    ansiString = getColor(ui->foregroundAnsiCombo,
                          ui->backgroundAnsiCombo,
                          ui->boldCheckBox,
                          ui->italicCheckBox,
                          ui->underlineCheckBox);

    if (false)
        MMLOG() << "new ansi string " << mmqt::toStdStringLatin1(ansiString);
}
