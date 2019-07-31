// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "parserpage.h"

#include <QColor>
#include <QComboBox>
#include <QRegularExpression>
#include <QString>
#include <QValidator>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "../parser/AbstractParser-Utils.h"
#include "AnsiColorDialog.h"
#include "ansicombo.h"

class CommandPrefixValidator final : public QValidator
{
public:
    explicit CommandPrefixValidator(QObject *const parent);
    virtual ~CommandPrefixValidator() override;

    void fixup(QString &input) const override { input = input.toLatin1(); }

    QValidator::State validate(QString &input, int & /* pos */) const override
    {
        if (input.isEmpty())
            return QValidator::State::Intermediate;

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

    const auto &settings = getConfig().parser;

    AnsiCombo::makeWidgetColoured(roomNameColorLabel, settings.roomNameColor);
    AnsiCombo::makeWidgetColoured(roomDescColorLabel, settings.roomDescColor);

    connect(roomNameColorPushButton,
            &QAbstractButton::clicked,
            this,
            &ParserPage::roomNameColorClicked);
    connect(roomDescColorPushButton,
            &QAbstractButton::clicked,
            this,
            &ParserPage::roomDescColorClicked);

    connect(charPrefixLineEdit, &QLineEdit::editingFinished, this, [this]() {
        setConfig().parser.prefixChar = charPrefixLineEdit->text().at(0).toLatin1();
    });
    charPrefixLineEdit->setText(QString(getConfig().parser.prefixChar));
    charPrefixLineEdit->setValidator(new CommandPrefixValidator(this));

    suppressXmlTagsCheckBox->setChecked(settings.removeXmlTags);
    suppressXmlTagsCheckBox->setEnabled(true);

    endDescPatternsList->clear();
    endDescPatternsList->addItems(settings.noDescriptionPatternsList);

    connect(removeEndDescPattern,
            &QAbstractButton::clicked,
            this,
            &ParserPage::removeEndDescPatternClicked);

    connect(addEndDescPattern,
            &QAbstractButton::clicked,
            this,
            &ParserPage::addEndDescPatternClicked);

    connect(testPattern, &QAbstractButton::clicked, this, &ParserPage::testPatternClicked);
    connect(validPattern, &QAbstractButton::clicked, this, &ParserPage::validPatternClicked);

    connect(endDescPatternsList,
            QOverload<const QString &>::of(&QComboBox::activated),
            this,
            &ParserPage::endDescPatternsListActivated);

    connect(suppressXmlTagsCheckBox,
            &QCheckBox::stateChanged,
            this,
            &ParserPage::suppressXmlTagsCheckBoxStateChanged);
}

void ParserPage::roomNameColorClicked()
{
    QString ansiString = AnsiColorDialog::getColor(getConfig().parser.roomNameColor, this);
    AnsiCombo::makeWidgetColoured(roomNameColorLabel, ansiString);
    setConfig().parser.roomNameColor = ansiString;
}

void ParserPage::roomDescColorClicked()
{
    QString ansiString = AnsiColorDialog::getColor(getConfig().parser.roomDescColor, this);
    AnsiCombo::makeWidgetColoured(roomDescColorLabel, ansiString);
    setConfig().parser.roomDescColor = ansiString;
}

void ParserPage::suppressXmlTagsCheckBoxStateChanged(int /*unused*/)
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

void ParserPage::removeEndDescPatternClicked()
{
    endDescPatternsList->removeItem(endDescPatternsList->currentIndex());
    savePatterns();
}

void ParserPage::testPatternClicked()
{
    QString pattern = newPattern->text();
    QString str = testString->text();
    bool matches = false;

    if ((pattern)[0] != '#') {
    } else {
        switch (static_cast<int>((pattern[1]).toLatin1())) {
        case 33: // !
            if (QRegularExpression(pattern.remove(0, 2)).match(str).hasMatch()) {
                matches = true;
            }
            break;
        case 60: // <
            if (str.startsWith((pattern).remove(0, 2))) {
                matches = true;
            }
            break;
        case 61: // =
            if (str == ((pattern).remove(0, 2))) {
                matches = true;
            }
            break;
        case 62: // >
            if (str.endsWith((pattern).remove(0, 2))) {
                matches = true;
            }
            break;
        case 63: // ?
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

void ParserPage::validPatternClicked()
{
    QString pattern = newPattern->text();
    QString str = "Pattern '" + pattern + "' is valid!!!";

    if (((pattern)[0] != '#')
        || (((pattern)[1] != '!') && ((pattern)[1] != '?') && ((pattern)[1] != '<')
            && ((pattern)[1] != '>') && ((pattern)[1] != '='))) {
        str = "Pattern must begin with '#t', where t means type of pattern (!?<>=)";
    } else if ((pattern)[1] == '!') {
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

void ParserPage::endDescPatternsListActivated(const QString &str)
{
    newPattern->setText(str);
}

void ParserPage::addEndDescPatternClicked()
{
    QString str;
    if ((str = newPattern->text()) != "") {
        endDescPatternsList->addItem(str);
        endDescPatternsList->setCurrentIndex(endDescPatternsList->count() - 1);
    }
    savePatterns();
}
