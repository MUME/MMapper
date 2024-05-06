// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "clientpage.h"

#include "../configuration/configuration.h"
#include "ui_clientpage.h"

#include <QFont>
#include <QFontInfo>
#include <QPalette>
#include <QSpinBox>
#include <QString>
#include <QtGui>
#include <QtWidgets>

ClientPage::ClientPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ClientPage)
{
    ui->setupUi(this);

    connect(ui->fontPushButton, &QAbstractButton::pressed, this, &ClientPage::slot_onChangeFont);
    connect(ui->bgColorPushButton,
            &QAbstractButton::pressed,
            this,
            &ClientPage::slot_onChangeBackgroundColor);
    connect(ui->fgColorPushButton,
            &QAbstractButton::pressed,
            this,
            &ClientPage::slot_onChangeForegroundColor);

    connect(ui->columnsSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &ClientPage::slot_onChangeColumns);
    connect(ui->rowsSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &ClientPage::slot_onChangeRows);
    connect(ui->scrollbackSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &ClientPage::slot_onChangeLinesOfScrollback);

    connect(ui->inputHistorySpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &ClientPage::slot_onChangeLinesOfInputHistory);
    connect(ui->tabDictionarySpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &ClientPage::slot_onChangeTabCompletionDictionarySize);

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

void ClientPage::slot_loadConfig()
{
    updateFontAndColors();

    const auto &settings = getConfig().integratedClient;
    ui->columnsSpinBox->setValue(settings.columns);
    ui->rowsSpinBox->setValue(settings.rows);
    ui->scrollbackSpinBox->setValue(settings.linesOfScrollback);
    ui->inputHistorySpinBox->setValue(settings.linesOfInputHistory);
    ui->tabDictionarySpinBox->setValue(settings.tabCompletionDictionarySize);
    ui->clearInputCheckBox->setChecked(settings.clearInputOnEnter);
    ui->autoResizeTerminalCheckBox->setChecked(settings.autoResizeTerminal);
}

void ClientPage::updateFontAndColors()
{
    const auto &settings = getConfig().integratedClient;
    QFont font;
    font.fromString(settings.font);
    ui->exampleLabel->setFont(font);

    QFontInfo fi(font);
    ui->fontPushButton->setText(
        QString("%1 %2, %3").arg(fi.family()).arg(fi.styleName()).arg(fi.pointSize()));

    QPixmap fgPix(16, 16);
    fgPix.fill(settings.foregroundColor);
    ui->fgColorPushButton->setIcon(QIcon(fgPix));

    QPixmap bgPix(16, 16);
    bgPix.fill(settings.backgroundColor);
    ui->bgColorPushButton->setIcon(QIcon(bgPix));

    QPalette palette = ui->exampleLabel->palette();
    ui->exampleLabel->setAutoFillBackground(true);
    palette.setColor(QPalette::WindowText, settings.foregroundColor);
    palette.setColor(QPalette::Window, settings.backgroundColor);
    ui->exampleLabel->setPalette(palette);
    ui->exampleLabel->setBackgroundRole(QPalette::Window);
}

void ClientPage::slot_onChangeFont()
{
    auto &fontDescription = setConfig().integratedClient.font;
    QFont oldFont;
    oldFont.fromString(fontDescription);

    bool ok = false;
    const QFont newFont = QFontDialog::getFont(&ok,
                                               oldFont,
                                               this,
                                               "Select Font",
                                               QFontDialog::MonospacedFonts);
    if (ok) {
        fontDescription = newFont.toString();
        updateFontAndColors();
    }
}

void ClientPage::slot_onChangeBackgroundColor()
{
    auto &backgroundColor = setConfig().integratedClient.backgroundColor;
    const QColor newColor = QColorDialog::getColor(backgroundColor, this);
    if (newColor.isValid() && newColor != backgroundColor) {
        backgroundColor = newColor;
        updateFontAndColors();
    }
}

void ClientPage::slot_onChangeForegroundColor()
{
    auto &foregroundColor = setConfig().integratedClient.foregroundColor;
    const QColor newColor = QColorDialog::getColor(foregroundColor, this);
    if (newColor.isValid() && newColor != foregroundColor) {
        foregroundColor = newColor;
        updateFontAndColors();
    }
}

void ClientPage::slot_onChangeColumns(const int value)
{
    setConfig().integratedClient.columns = value;
}

void ClientPage::slot_onChangeRows(const int value)
{
    setConfig().integratedClient.rows = value;
}

void ClientPage::slot_onChangeLinesOfScrollback(const int value)
{
    setConfig().integratedClient.linesOfScrollback = value;
}

void ClientPage::slot_onChangeLinesOfInputHistory(const int value)
{
    setConfig().integratedClient.linesOfInputHistory = value;
}

void ClientPage::slot_onChangeTabCompletionDictionarySize(const int value)
{
    setConfig().integratedClient.tabCompletionDictionarySize = value;
}
