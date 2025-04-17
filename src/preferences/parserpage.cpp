// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "parserpage.h"

#include "../configuration/configuration.h"
#include "../global/Charset.h"
#include "../parser/AbstractParser-Utils.h"
#include "AnsiColorDialog.h"
#include "ansicombo.h"

#include <QColor>
#include <QComboBox>
#include <QString>
#include <QValidator>
#include <QtWidgets>

class NODISCARD CommandPrefixValidator final : public QValidator
{
public:
    explicit CommandPrefixValidator(QObject *parent);
    ~CommandPrefixValidator() final;

    void fixup(QString &input) const override
    {
        mmqt::toLatin1InPlace(input); // transliterates non-latin1 codepoints
    }

    QValidator::State validate(QString &input, int & /* pos */) const override
    {
        if (input.isEmpty()) {
            return QValidator::State::Intermediate;
        }

        const bool valid = input.length() == 1 && isValidPrefix(input[0].toLatin1());
        return valid ? QValidator::State::Acceptable : QValidator::State::Invalid;
    }
};

CommandPrefixValidator::CommandPrefixValidator(QObject *const parent)
    : QValidator(parent)
{}

CommandPrefixValidator::~CommandPrefixValidator() = default;

ParserPage::ParserPage(QWidget *const parent)
    : QWidget(parent)
{
    setupUi(this);

    connect(roomNameColorPushButton,
            &QAbstractButton::clicked,
            this,
            &ParserPage::slot_roomNameColorClicked);
    connect(roomDescColorPushButton,
            &QAbstractButton::clicked,
            this,
            &ParserPage::slot_roomDescColorClicked);

    connect(charPrefixLineEdit, &QLineEdit::editingFinished, this, [this]() {
        setConfig().parser.prefixChar = charPrefixLineEdit->text().at(0).toLatin1();
    });

    connect(encodeEmoji, &QCheckBox::clicked, this, [](bool checked) {
        setConfig().parser.encodeEmoji = checked;
    });
    connect(decodeEmoji, &QCheckBox::clicked, this, [](bool checked) {
        setConfig().parser.decodeEmoji = checked;
    });
}

void ParserPage::slot_loadConfig()
{
    const auto &settings = getConfig().parser;

    AnsiCombo::makeWidgetColoured(roomNameColorLabel, settings.roomNameColor);
    AnsiCombo::makeWidgetColoured(roomDescColorLabel, settings.roomDescColor);

    charPrefixLineEdit->setText(QString(settings.prefixChar));
    charPrefixLineEdit->setValidator(new CommandPrefixValidator(this));

    encodeEmoji->setChecked(settings.encodeEmoji);
    decodeEmoji->setChecked(settings.decodeEmoji);
}

void ParserPage::slot_roomNameColorClicked()
{
    QString ansiString = AnsiColorDialog::getColor(getConfig().parser.roomNameColor, this);
    AnsiCombo::makeWidgetColoured(roomNameColorLabel, ansiString);
    setConfig().parser.roomNameColor = ansiString;
}

void ParserPage::slot_roomDescColorClicked()
{
    QString ansiString = AnsiColorDialog::getColor(getConfig().parser.roomDescColor, this);
    AnsiCombo::makeWidgetColoured(roomDescColorLabel, ansiString);
    setConfig().parser.roomDescColor = ansiString;
}
