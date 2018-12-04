/************************************************************************
**
** Authors:   Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
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

#include "ansicombo.h"

#include <cassert>
#include <type_traits>
#include <QRegularExpression>
#include <QString>
#include <QtGui>
#include <QtWidgets>

#include "../global/Color.h"

AnsiCombo::AnsiCombo(AnsiMode mode, QWidget *parent)
    : AnsiCombo(parent)
{
    initColours(mode);
}

AnsiCombo::AnsiCombo(QWidget *parent)
    : super(parent)
{
    setEditable(true);

    connect(this, &QComboBox::editTextChanged, this, &AnsiCombo::afterEdit);
}

void AnsiCombo::fillAnsiList()
{
    clear();

    for (const AnsiItem &item : colours) {
        addItem(item.picture, item.description, item.ansiCode);
    }
}

QString AnsiCombo::getAnsiString() const
{
    return currentData(Qt::UserRole).toString();
}

void AnsiCombo::setAnsiString(const QString &inputString)
{
    const int index = findData(inputString, Qt::UserRole, Qt::MatchCaseSensitive);

    if (index >= 0) {
        setCurrentIndex(index);
    } else {
        setCurrentIndex(findText("none"));
    }
}

void AnsiCombo::afterEdit(const QString &t)
{
    setAnsiString(t);
}

void AnsiCombo::initColours(AnsiMode mode)
{
    switch (mode) {
    case AnsiMode::ANSI_FG:
        colours.push_back(initAnsiItem(DEFAULT_FG));
        for (int i = 30; i < 38; ++i) {
            colours.push_back(initAnsiItem(i));
        }
        for (int i = 90; i < 98; ++i) {
            colours.push_back(initAnsiItem(i));
        }
        break;
    case AnsiMode::ANSI_BG:
        colours.push_back(initAnsiItem(DEFAULT_BG));
        for (int i = 40; i < 48; ++i) {
            colours.push_back(initAnsiItem(i));
        }
        for (int i = 100; i < 108; ++i) {
            colours.push_back(initAnsiItem(i));
        }
        break;
    }
    fillAnsiList();
}

AnsiCombo::AnsiItem AnsiCombo::initAnsiItem(int index)
{
    QColor col;
    AnsiItem retVal;

    if (colorFromNumber(index, col, retVal.description)) {
        QPixmap pix(20, 20);

        pix.fill(col);

        retVal.picture = pix;
        if (index != DEFAULT_FG && index != DEFAULT_BG) {
            retVal.ansiCode = QString("%1").arg(index);
        } else {
            retVal.ansiCode = QString("none");
        }
    }
    return retVal;
}

AnsiCombo::AnsiColor AnsiCombo::colorFromString(const QString &colString)
{
    AnsiColor color;

    // No need to proceed if the color is empty
    if (colString.isEmpty())
        return color;

    QRegularExpression re(R"(^\[((?:\d+;)*\d+)m$)");
    if (!re.match(colString).hasMatch())
        return color;

    const auto update_fg = [&color](const int n) {
        assert((30 <= n && n <= 37) || (90 <= n && n <= 97) || n == DEFAULT_FG);
        color.ansiCodeFg = n;
        colorFromNumber(color.ansiCodeFg, color.colFg, color.intelligibleNameFg);
    };
    const auto update_bg = [&color](const int n) {
        assert((40 <= n && n <= 47) || (100 <= n && n <= 107) || n == DEFAULT_BG);
        color.ansiCodeBg = n;
        colorFromNumber(color.ansiCodeBg, color.colBg, color.intelligibleNameBg);
    };

    // matches
    QString tmpStr = colString;

    tmpStr.chop(1);
    tmpStr.remove(0, 1);

    for (const auto &s : tmpStr.split(";", QString::SkipEmptyParts)) {
        switch (const auto n = s.toInt()) {
        case 0:
            /* Ansi reset will never happen, but it doesn't hurt to have it. */
            color.bold = false;
            color.underline = false;
            update_fg(DEFAULT_FG);
            update_bg(DEFAULT_BG);
            break;

        case 1:
            color.bold = true;
            break;

        case 3:
            color.italic = true;
            break;

        case 4:
            color.underline = true;
            break;

        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 97:
            update_fg(n);
            break;

        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
        case 100:
        case 101:
        case 102:
        case 103:
        case 104:
        case 105:
        case 106:
        case 107:
            update_bg(n);
            break;
        }
    }

    return color;
}

bool AnsiCombo::colorFromNumber(int numColor, QColor &col, QString &intelligibleName)
{
    intelligibleName = tr("undefined!");
    col = ansiColor(AnsiColorTable::white);

    const bool foreground = (30 <= numColor && numColor <= 37) || (90 <= numColor && numColor <= 97)
                            || numColor == DEFAULT_FG;
    const bool background = (40 <= numColor && numColor <= 47)
                            || (100 <= numColor && numColor <= 107) || numColor == DEFAULT_BG;
    const bool retVal = (foreground || background);

    /* TODO: Simplify this. E.g. se ansi_color_table[n-30], etc. */
    switch (numColor) {
    case DEFAULT_FG:
        col = ansiColor(AnsiColorTable::white);
        intelligibleName = tr("none");
        break;
    case DEFAULT_BG:
        col = ansiColor(AnsiColorTable::black);
        intelligibleName = tr("none");
        break;
    case 30:
    case 40:
        col = ansiColor(AnsiColorTable::black);
        intelligibleName = tr("black");
        break;
    case 31:
    case 41:
        col = ansiColor(AnsiColorTable::red);
        intelligibleName = tr("red");
        break;
    case 32:
    case 42:
        col = ansiColor(AnsiColorTable::green);
        intelligibleName = tr("green");
        break;
    case 33:
    case 43:
        col = ansiColor(AnsiColorTable::yellow);
        intelligibleName = tr("yellow");
        break;
    case 34:
    case 44:
        col = ansiColor(AnsiColorTable::blue);
        intelligibleName = tr("blue");
        break;
    case 35:
    case 45:
        col = ansiColor(AnsiColorTable::magenta);
        intelligibleName = tr("magenta");
        break;
    case 36:
    case 46:
        col = ansiColor(AnsiColorTable::cyan);
        intelligibleName = tr("cyan");
        break;
    case 37:
    case 47:
        col = ansiColor(AnsiColorTable::white);
        intelligibleName = tr("white");
        break;
    case 90:
    case 100:
        col = ansiColor(AnsiColorTable::BLACK);
        intelligibleName = tr("BLACK");
        break;
    case 91:
    case 101:
        col = ansiColor(AnsiColorTable::RED);
        intelligibleName = tr("RED");
        break;
    case 92:
    case 102:
        col = ansiColor(AnsiColorTable::GREEN);
        intelligibleName = tr("GREEN");
        break;
    case 93:
    case 103:
        col = ansiColor(AnsiColorTable::YELLOW);
        intelligibleName = tr("YELLOW");
        break;
    case 94:
    case 104:
        col = ansiColor(AnsiColorTable::BLUE);
        intelligibleName = tr("BLUE");
        break;
    case 95:
    case 105:
        col = ansiColor(AnsiColorTable::MAGENTA);
        intelligibleName = tr("MAGENTA");
        break;
    case 96:
    case 106:
        col = ansiColor(AnsiColorTable::CYAN);
        intelligibleName = tr("CYAN");
        break;
    case 97:
    case 107:
        col = ansiColor(AnsiColorTable::WHITE);
        intelligibleName = tr("WHITE");
        break;
    }
    return retVal;
}

template<typename Derived, typename Base>
static inline Derived *qdynamic_downcast(Base *ptr)
{
    static_assert(std::is_base_of<Base, Derived>::value, "");
    return dynamic_cast<Derived *>(qobject_cast<Derived *>(ptr));
}

void AnsiCombo::makeWidgetColoured(QWidget *pWidget, const QString &ansiColor)
{
    if (pWidget != nullptr) {
        AnsiColor color = colorFromString(ansiColor);

        QPalette palette = pWidget->palette();

        // crucial call to have background filled
        pWidget->setAutoFillBackground(true);

        palette.setColor(QPalette::WindowText, color.colFg);
        palette.setColor(QPalette::Window, color.colBg);

        pWidget->setPalette(palette);
        pWidget->setBackgroundRole(QPalette::Window);

        if (auto *pLabel = qdynamic_downcast<QLabel>(pWidget)) {
            QString displayString = color.intelligibleNameFg;
            if (color.bold)
                displayString = QString("<b>%1</b>").arg(displayString);
            if (color.italic)
                displayString = QString("<i>%1</i>").arg(displayString);
            if (color.underline)
                displayString = QString("<u>%1</u>").arg(displayString);
            pLabel->setText(displayString);
        }
    }
}
