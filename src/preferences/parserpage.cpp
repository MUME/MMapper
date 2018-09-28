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
#include "ansicombo.h"

ParserPage::ParserPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    roomNameAnsiColor->initColours(AnsiMode::ANSI_FG);
    roomNameAnsiColorBG->initColours(AnsiMode::ANSI_BG);
    roomDescAnsiColor->initColours(AnsiMode::ANSI_FG);
    roomDescAnsiColorBG->initColours(AnsiMode::ANSI_BG);

    updateColors();

    const auto &settings = getConfig().parser;

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

    connect(roomNameAnsiColor,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this,
            &ParserPage::roomNameColorChanged);
    connect(roomDescAnsiColor,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this,
            &ParserPage::roomDescColorChanged);

    connect(roomNameAnsiColorBG,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this,
            &ParserPage::roomNameColorBGChanged);
    connect(roomDescAnsiColorBG,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this,
            &ParserPage::roomDescColorBGChanged);

    connect(roomNameAnsiBold,
            &QAbstractButton::toggled,
            this,
            &ParserPage::anyColorToggleButtonToggled);
    connect(roomNameAnsiUnderline,
            &QAbstractButton::toggled,
            this,
            &ParserPage::anyColorToggleButtonToggled);
    connect(roomDescAnsiBold,
            &QAbstractButton::toggled,
            this,
            &ParserPage::anyColorToggleButtonToggled);
    connect(roomDescAnsiUnderline,
            &QAbstractButton::toggled,
            this,
            &ParserPage::anyColorToggleButtonToggled);

    connect(suppressXmlTagsCheckBox,
            &QCheckBox::stateChanged,
            this,
            &ParserPage::suppressXmlTagsCheckBoxStateChanged);
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

void ParserPage::updateColors()
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

    const auto ansiString = [](const QString &s) -> QString {
        return s.isEmpty() ? QString{"[0m"} : s;
    };

    // room name color
    const auto &settings = getConfig().parser;
    roomNameColorLabel->setText(ansiString(settings.roomNameColor));

    AnsiCombo::makeWidgetColoured(labelRoomColor, settings.roomNameColor);

    AnsiCombo::colorFromString(settings.roomNameColor,
                               colFg,
                               ansiCodeFg,
                               intelligibleNameFg,
                               colBg,
                               ansiCodeBg,
                               intelligibleNameBg,
                               bold,
                               underline);

    roomNameAnsiBold->setChecked(bold);
    roomNameAnsiUnderline->setChecked(underline);

    roomNameAnsiColor->setEditable(false);
    roomNameAnsiColorBG->setEditable(false);
    roomNameAnsiColor->setText(QString("%1").arg(ansiCodeFg));
    roomNameAnsiColorBG->setText(QString("%1").arg(ansiCodeBg));

    // room desc color
    roomDescColorLabel->setText(ansiString(settings.roomDescColor));

    AnsiCombo::makeWidgetColoured(labelRoomDesc, settings.roomDescColor);

    AnsiCombo::colorFromString(settings.roomDescColor,
                               colFg,
                               ansiCodeFg,
                               intelligibleNameFg,
                               colBg,
                               ansiCodeBg,
                               intelligibleNameBg,
                               bold,
                               underline);

    roomDescAnsiBold->setChecked(bold);
    roomDescAnsiUnderline->setChecked(underline);

    roomDescAnsiColor->setEditable(false);
    roomDescAnsiColorBG->setEditable(false);
    roomDescAnsiColor->setText(QString("%1").arg(ansiCodeFg));
    roomDescAnsiColorBG->setText(QString("%1").arg(ansiCodeBg));
}

void ParserPage::generateNewAnsiColor()
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

    auto &settings = setConfig().parser;
    settings.roomNameColor = getColor(roomNameAnsiColor,
                                      roomNameAnsiColorBG,
                                      roomNameAnsiBold,
                                      roomNameAnsiUnderline);
    settings.roomDescColor = getColor(roomDescAnsiColor,
                                      roomDescAnsiColorBG,
                                      roomDescAnsiBold,
                                      roomDescAnsiUnderline);
}

void ParserPage::roomDescColorChanged(const QString & /*unused*/)
{
    generateNewAnsiColor();
    updateColors();
}

void ParserPage::roomNameColorChanged(const QString & /*unused*/)
{
    generateNewAnsiColor();
    updateColors();
}

void ParserPage::roomDescColorBGChanged(const QString & /*unused*/)
{
    generateNewAnsiColor();
    updateColors();
}

void ParserPage::roomNameColorBGChanged(const QString & /*unused*/)
{
    generateNewAnsiColor();
    updateColors();
}

void ParserPage::anyColorToggleButtonToggled(bool /*unused*/)
{
    generateNewAnsiColor();
    updateColors();
}
