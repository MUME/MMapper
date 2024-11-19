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
#include <QRegularExpression>
#include <QString>
#include <QValidator>
#include <QtWidgets>

class NODISCARD CommandPrefixValidator final : public QValidator
{
public:
    explicit CommandPrefixValidator(QObject *parent);
    ~CommandPrefixValidator() final;

    void fixup(QString &input) const override { mmqt::toLatin1InPlace(input); }

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

    connect(removeEndDescPattern,
            &QAbstractButton::clicked,
            this,
            &ParserPage::slot_removeEndDescPatternClicked);

    connect(addEndDescPattern,
            &QAbstractButton::clicked,
            this,
            &ParserPage::slot_addEndDescPatternClicked);

    connect(testPattern, &QAbstractButton::clicked, this, &ParserPage::slot_testPatternClicked);
    connect(validPattern, &QAbstractButton::clicked, this, &ParserPage::slot_validPatternClicked);

    connect(endDescPatternsList,
            QOverload<const QString &>::of(&QComboBox::textActivated),
            this,
            &ParserPage::slot_endDescPatternsListActivated);

    connect(suppressXmlTagsCheckBox,
            &QCheckBox::stateChanged,
            this,
            &ParserPage::slot_suppressXmlTagsCheckBoxStateChanged);
}

void ParserPage::slot_loadConfig()
{
    const auto &settings = getConfig().parser;

    AnsiCombo::makeWidgetColoured(roomNameColorLabel, settings.roomNameColor);
    AnsiCombo::makeWidgetColoured(roomDescColorLabel, settings.roomDescColor);

    charPrefixLineEdit->setText(QString(settings.prefixChar));
    charPrefixLineEdit->setValidator(new CommandPrefixValidator(this));

    suppressXmlTagsCheckBox->setChecked(settings.removeXmlTags);
    suppressXmlTagsCheckBox->setEnabled(true);

    endDescPatternsList->clear();
    endDescPatternsList->addItems(settings.noDescriptionPatternsList);
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

void ParserPage::slot_suppressXmlTagsCheckBoxStateChanged(int /*unused*/)
{
    setConfig().parser.removeXmlTags = suppressXmlTagsCheckBox->isChecked();
}

void ParserPage::savePatterns()
{
    const auto save = [](const auto &input) -> QStringList {
        const auto count = input->count();
        QStringList result;
        result.reserve(count);
        for (int i = 0; i < count; i++) {
            result.append(input->itemText(i));
        }
        return result;
    };

    auto &settings = setConfig().parser;
    settings.noDescriptionPatternsList = save(endDescPatternsList);
}

void ParserPage::slot_removeEndDescPatternClicked()
{
    endDescPatternsList->removeItem(endDescPatternsList->currentIndex());
    savePatterns();
}

void ParserPage::slot_testPatternClicked()
{
    using namespace char_consts;
    QString pattern = newPattern->text();
    QString str = testString->text();
    bool matches = false;

    if ((pattern)[0] != C_POUND_SIGN) {
    } else {
        switch (static_cast<int>((pattern[1]).toLatin1())) {
        case C_EXCLAMATION: // !
            if (QRegularExpression(pattern.remove(0, 2)).match(str).hasMatch()) {
                matches = true;
            }
            break;
        case C_LESS_THAN: // <
            if (str.startsWith((pattern).remove(0, 2))) {
                matches = true;
            }
            break;
        case C_EQUALS: // =
            if (str == ((pattern).remove(0, 2))) {
                matches = true;
            }
            break;
        case C_GREATER_THAN: // >
            if (str.endsWith((pattern).remove(0, 2))) {
                matches = true;
            }
            break;
        case C_QUESTION_MARK: // ?
            if (str.contains((pattern).remove(0, 2))) {
                matches = true;
            }
            break;
        default:;
        }
    }

    str = (matches) ? tr("Pattern matches!!!") : tr("Pattern doesn't match!!!");

    QMessageBox::information(this, tr("Pattern match test"), str);
}

void ParserPage::slot_validPatternClicked()
{
    using namespace char_consts;
    QString pattern = newPattern->text();
    QString str = "Pattern '" + pattern + "' is valid!!!";

    if (((pattern)[0] != C_POUND_SIGN)
        || (((pattern)[1] != C_EXCLAMATION) && ((pattern)[1] != C_QUESTION_MARK)
            && ((pattern)[1] != C_LESS_THAN) && ((pattern)[1] != C_GREATER_THAN)
            && ((pattern)[1] != C_EQUALS))) {
        str = "Pattern must begin with '#t', where t means type of pattern (!?<>=)";
    } else if ((pattern)[1] == C_EXCLAMATION) {
        QRegularExpression rx(pattern.remove(0, 2));
        if (!rx.isValid()) {
            str = "Pattern '" + pattern + "' is not valid!!!";
        }
    }

    QMessageBox::information(this,
                             "Pattern match test",
                             str,
                             "&Discard",
                             nullptr, // Enter == button 0
                             nullptr);
}

void ParserPage::slot_endDescPatternsListActivated(const QString &str)
{
    newPattern->setText(str);
}

void ParserPage::slot_addEndDescPatternClicked()
{
    if (QString str = newPattern->text(); str != "") {
        endDescPatternsList->addItem(str);
        endDescPatternsList->setCurrentIndex(endDescPatternsList->count() - 1);
    }
    savePatterns();
}
