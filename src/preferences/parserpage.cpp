/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#include "parserpage.h"

#include <QColor>
#include <QComboBox>
#include <QString>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "AnsiColorDialog.h"
#include "ansicombo.h"

ParserPage::ParserPage(QWidget *parent)
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

    suppressXmlTagsCheckBox->setChecked(settings.removeXmlTags);
    suppressXmlTagsCheckBox->setEnabled(true);

    forcePatternsList->clear();
    forcePatternsList->addItems(settings.moveForcePatternsList);
    endDescPatternsList->clear();
    endDescPatternsList->addItems(settings.noDescriptionPatternsList);

    connect(removeForcePattern,
            &QAbstractButton::clicked,
            this,
            &ParserPage::removeForcePatternClicked);
    connect(removeEndDescPattern,
            &QAbstractButton::clicked,
            this,
            &ParserPage::removeEndDescPatternClicked);

    connect(addForcePattern, &QAbstractButton::clicked, this, &ParserPage::addForcePatternClicked);
    connect(addEndDescPattern,
            &QAbstractButton::clicked,
            this,
            &ParserPage::addEndDescPatternClicked);

    connect(testPattern, &QAbstractButton::clicked, this, &ParserPage::testPatternClicked);
    connect(validPattern, &QAbstractButton::clicked, this, &ParserPage::validPatternClicked);

    connect(forcePatternsList,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this,
            &ParserPage::forcePatternsListActivated);
    connect(endDescPatternsList,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
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
    settings.moveForcePatternsList = save(forcePatternsList);
    settings.noDescriptionPatternsList = save(endDescPatternsList);
}

void ParserPage::removeForcePatternClicked()
{
    forcePatternsList->removeItem(forcePatternsList->currentIndex());
    savePatterns();
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
    QRegExp rx;

    if ((pattern)[0] != '#') {
    } else {
        switch (static_cast<int>((pattern[1]).toLatin1())) {
        case 33: // !
            rx.setPattern((pattern).remove(0, 2));
            if (rx.exactMatch(str)) {
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
        QRegExp rx(pattern.remove(0, 2));
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

void ParserPage::forcePatternsListActivated(const QString &str)
{
    newPattern->setText(str);
}

void ParserPage::endDescPatternsListActivated(const QString &str)
{
    newPattern->setText(str);
}

void ParserPage::addForcePatternClicked()
{
    QString str;
    if ((str = newPattern->text()) != "") {
        forcePatternsList->addItem(str);
        forcePatternsList->setCurrentIndex(forcePatternsList->count() - 1);
    }
    savePatterns();
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
