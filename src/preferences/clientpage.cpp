// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "clientpage.h"

#include "../configuration/configuration.h"
#include "../global/ConfigConsts-Computed.h"
#include "../global/macros.h"
#include "../global/window_utils.h"
#include "ui_clientpage.h"

#include <QFont>
#include <QFontInfo>
#include <QPalette>
#include <QRadioButton>
#include <QSpinBox>
#include <QString>
#include <QValidator>
#include <QtGui>
#include <QtWidgets>

class NODISCARD CustomSeparatorValidator final : public QValidator
{
public:
    explicit CustomSeparatorValidator(QObject *parent);
    ~CustomSeparatorValidator() final;

    void fixup(QString &input) const override
    {
        mmqt::toLatin1InPlace(input); // transliterates non-latin1 codepoints
    }

    QValidator::State validate(QString &input, int & /* pos */) const override
    {
        if (input.length() != 1) {
            return QValidator::State::Intermediate;
        }

        const auto c = input.front();
        const bool valid = c != char_consts::C_BACKSLASH && c.isPrint() && !c.isSpace();
        return valid ? QValidator::State::Acceptable : QValidator::State::Invalid;
    }
};

CustomSeparatorValidator::CustomSeparatorValidator(QObject *const parent)
    : QValidator(parent)
{}

CustomSeparatorValidator::~CustomSeparatorValidator() = default;

ClientPage::ClientPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ClientPage)
{
    ui->setupUi(this);

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        ui->howToPlayGroupBox->hide(); // only the built-in client exists
    }
    const auto setGameClient = [](const GameClientEnum client) {
        return [client](const bool checked) {
            if (checked) {
                setConfig().general.gameClient = client;
            }
        };
    };
    connect(ui->askRadioButton, &QRadioButton::toggled, this, setGameClient(GameClientEnum::ASK));
    connect(ui->builtInRadioButton,
            &QRadioButton::toggled,
            this,
            setGameClient(GameClientEnum::BUILT_IN));
    connect(ui->externalRadioButton,
            &QRadioButton::toggled,
            this,
            setGameClient(GameClientEnum::EXTERNAL));

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
    connect(ui->previewSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            [](const int value) { setConfig().integratedClient.linesOfPeekPreview = value; });

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

    connect(ui->audibleBellCheckBox, &QCheckBox::toggled, [](bool isChecked) {
        setConfig().integratedClient.audibleBell = isChecked;
    });

    connect(ui->visualBellCheckBox, &QCheckBox::toggled, [](bool isChecked) {
        setConfig().integratedClient.visualBell = isChecked;
    });

    connect(ui->commandSeparatorCheckBox, &QCheckBox::toggled, this, [this](bool isChecked) {
        setConfig().integratedClient.useCommandSeparator = isChecked;
        ui->commandSeparatorLineEdit->setEnabled(isChecked);
    });

    connect(ui->commandSeparatorLineEdit, &QLineEdit::textChanged, this, [](const QString &text) {
        if (text.length() == 1) {
            setConfig().integratedClient.commandSeparator = text;
        }
    });

    ui->commandSeparatorLineEdit->setValidator(new CustomSeparatorValidator(this));
}

ClientPage::~ClientPage()
{
    delete ui;
}

void ClientPage::slot_loadConfig()
{
    updateFontAndColors();

    switch (getConfig().general.gameClient) {
    case GameClientEnum::ASK:
        ui->askRadioButton->setChecked(true);
        break;
    case GameClientEnum::BUILT_IN:
        ui->builtInRadioButton->setChecked(true);
        break;
    case GameClientEnum::EXTERNAL:
        ui->externalRadioButton->setChecked(true);
        break;
    }
    ui->externalRadioButton->setText(tr("Use my own MUD client (connect it to localhost, port %1)")
                                         .arg(getConfig().connection.localPort));

    const auto &settings = getConfig().integratedClient;
    ui->columnsSpinBox->setValue(settings.columns);
    ui->rowsSpinBox->setValue(settings.rows);
    ui->scrollbackSpinBox->setValue(settings.linesOfScrollback);
    ui->previewSpinBox->setValue(settings.linesOfPeekPreview);
    ui->inputHistorySpinBox->setValue(settings.linesOfInputHistory);
    ui->tabDictionarySpinBox->setValue(settings.tabCompletionDictionarySize);
    ui->clearInputCheckBox->setChecked(settings.clearInputOnEnter);
    ui->autoResizeTerminalCheckBox->setChecked(settings.autoResizeTerminal);
    ui->audibleBellCheckBox->setChecked(settings.audibleBell);
    ui->visualBellCheckBox->setChecked(settings.visualBell);
    ui->commandSeparatorCheckBox->setChecked(settings.useCommandSeparator);
    ui->commandSeparatorLineEdit->setText(settings.commandSeparator);
    ui->commandSeparatorLineEdit->setEnabled(settings.useCommandSeparator);
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
    QFont oldFont;
    oldFont.fromString(getConfig().integratedClient.font);

    mmqt::showFontDialog(this,
                         oldFont,
                         "Select Font",
                         QFontDialog::MonospacedFonts,
                         [this](const QFont &newFont) {
                             setConfig().integratedClient.font = newFont.toString();
                             updateFontAndColors();
                         });
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
