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
#include <QString>
#include <QtGui>
#include <QtWidgets>

static constexpr const int DEFAULT_FG = 254;
static constexpr const int DEFAULT_BG = 255;

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
        addItem(item.picture, item.ansiCode);
    }
}

QString AnsiCombo::text() const
{
    return super::currentText();
}

void AnsiCombo::setText(const QString &inputString)
{
    QString copiedString = inputString;

    switch (copiedString.toInt()) {
    case DEFAULT_FG:
    case DEFAULT_BG:
        copiedString = "none";
        break;
    }

    const int index = findText(copiedString);

    if (index >= 0) {
        setCurrentIndex(index);
    } else {
        super::setEditText(copiedString);
    }
}

void AnsiCombo::afterEdit(const QString &t)
{
    setText(t);
}

void AnsiCombo::initColours(AnsiMode mode)
{
    switch (mode) {
    case AnsiMode::ANSI_FG:
        colours.push_back(initAnsiItem(DEFAULT_FG));
        for (int i = 30; i < 38; ++i) {
            colours.push_back(initAnsiItem(i));
        }
        break;
    case AnsiMode::ANSI_BG:
        colours.push_back(initAnsiItem(DEFAULT_BG));
        for (int i = 40; i < 48; ++i) {
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

// FIXME: This should return a struct that reports the ansi found in colString.
// You can use std::pair<Result, bool> or std::tuple<Result, bool>
// until c++17 gives std::optional<Result>.
bool AnsiCombo::colorFromString(const QString &colString,
                                QColor &colFg,
                                int &ansiCodeFg,
                                QString &intelligibleNameFg,
                                QColor &colBg,
                                int &ansiCodeBg,
                                QString &intelligibleNameBg,
                                bool &bold,
                                bool &underline)
{
    ansiCodeFg = DEFAULT_FG;
    intelligibleNameFg = "none";
    colFg = QColor(Qt::white);
    ansiCodeBg = DEFAULT_BG;
    intelligibleNameBg = "none";
    colBg = QColor(Qt::black);
    bold = false;
    underline = false;

    // No need to proceed if the color is empty
    if (colString.isEmpty())
        return true;

    // REVISIT: shouldn't this be compiled and stuffed somewhere?
    // Is it safe to make this static?
    QRegExp re(R"(^\[((?:\d+;)*\d+)m$)");
    if (re.indexIn(colString) < 0)
        return false;

    const auto update_fg = [&ansiCodeFg, &colFg, &intelligibleNameFg](const int n) {
        // REVISIT: what about high colors?
        assert((30 <= n && n <= 37) || n == DEFAULT_FG);
        ansiCodeFg = n;
        colorFromNumber(ansiCodeFg, colFg, intelligibleNameFg);
    };
    const auto update_bg = [&ansiCodeBg, &colBg, &intelligibleNameBg](const int n) {
        assert((40 <= n && n <= 47) || n == DEFAULT_BG);
        ansiCodeBg = n;
        colorFromNumber(ansiCodeBg, colBg, intelligibleNameBg);
    };

    // matches
    QString tmpStr = colString;

    tmpStr.chop(1);
    tmpStr.remove(0, 1);

    for (const auto &s : tmpStr.split(";", QString::SkipEmptyParts)) {
        switch (const auto n = s.toInt()) {
        case 0:
            /* Ansi reset will never happen, but it doesn't hurt to have it. */
            bold = false;
            underline = false;
            update_fg(DEFAULT_FG);
            update_bg(DEFAULT_BG);
            break;

        case 1:
            bold = true;
            break;

        case 4:
            underline = true;
            break;

        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
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
            update_bg(n);
            break;
        }
    }

    return true;
}

bool AnsiCombo::colorFromNumber(int numColor, QColor &col, QString &intelligibleName)
{
    intelligibleName = tr("undefined!");
    col = Qt::white;

    const bool retVal = ((((numColor >= 30) && (numColor <= 37))
                          || ((numColor >= 40) && (numColor <= 47)))
                         || numColor == DEFAULT_FG || numColor == DEFAULT_BG);

    /* TODO: Simplify this. E.g. se ansi_color_table[n-30], etc. */
    switch (numColor) {
    case DEFAULT_FG:
        col = Qt::white;
        intelligibleName = tr("none");
        break;
    case DEFAULT_BG:
        col = Qt::black;
        intelligibleName = tr("none");
        break;
    case 30:
    case 40:
        col = Qt::black;
        intelligibleName = tr("black");
        break;
    case 31:
    case 41:
        col = Qt::red;
        intelligibleName = tr("red");
        break;
    case 32:
    case 42:
        col = Qt::green;
        intelligibleName = tr("green");
        break;
    case 33:
    case 43:
        col = Qt::yellow;
        intelligibleName = tr("yellow");
        break;
    case 34:
    case 44:
        col = Qt::blue;
        intelligibleName = tr("blue");
        break;
    case 35:
    case 45:
        col = Qt::magenta;
        intelligibleName = tr("magenta");
        break;
    case 36:
    case 46:
        col = Qt::cyan;
        intelligibleName = tr("cyan");
        break;
    case 37:
    case 47:
        col = Qt::white;
        intelligibleName = tr("white");
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
        QColor colFg;
        QColor colBg;
        int ansiCodeFg;
        int ansiCodeBg;
        QString intelligibleNameFg;
        QString intelligibleNameBg;
        bool bold;
        bool underline;

        colorFromString(ansiColor,
                        colFg,
                        ansiCodeFg,
                        intelligibleNameFg,
                        colBg,
                        ansiCodeBg,
                        intelligibleNameBg,
                        bold,
                        underline);

        QPalette palette = pWidget->palette();

        // crucial call to have background filled
        pWidget->setAutoFillBackground(true);

        palette.setColor(QPalette::WindowText, colFg);
        palette.setColor(QPalette::Window, colBg);

        pWidget->setPalette(palette);
        pWidget->setBackgroundRole(QPalette::Window);

        if (auto *pLabel = qdynamic_downcast<QLabel>(pWidget)) {
            QString displayString = intelligibleNameFg;
            if (bold)
                displayString = QString("<b>%1</b>").arg(displayString);
            if (underline)
                displayString = QString("<u>%1</u>").arg(displayString);
            pLabel->setText(displayString);
        }
    }
}
