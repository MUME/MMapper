// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "clientpage.h"

#include <QFont>
#include <QFontInfo>
#include <QPalette>
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

void ClientPage::loadConfig()
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

void ClientPage::onChangeFont()
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
