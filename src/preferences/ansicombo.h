#pragma once
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

#include <QColor>
#include <QComboBox>
#include <QIcon>
#include <QString>
#include <QVector>
#include <QtCore>

#include "../global/Color.h"

class QObject;
class QWidget;

enum class AnsiMode { ANSI_FG, ANSI_BG };

class AnsiCombo : public QComboBox
{
    using super = QComboBox;
    Q_OBJECT

public:
    static void makeWidgetColoured(QWidget *,
                                   const QString &ansiColor,
                                   const bool changeText = true);

    explicit AnsiCombo(AnsiMode mode, QWidget *parent = nullptr);
    explicit AnsiCombo(QWidget *parent = nullptr);

    void initColours(AnsiMode mode);

    AnsiMode getMode() const { return mode; }

    /// get currently selected ANSI code like 32 for green colour
    int getAnsiCode() const;

    void setAnsiCode(const int);

    static constexpr const char *NONE = "none";
    static constexpr const int DEFAULT_FG = 254;
    static constexpr const int DEFAULT_BG = 255;

    struct AnsiColor
    {
        QColor colFg{ansiColor(AnsiColorTable::white)};
        QColor colBg{ansiColor(AnsiColorTable::black)};
        int ansiCodeFg{DEFAULT_FG};
        int ansiCodeBg{DEFAULT_BG};
        QString intelligibleNameFg{NONE};
        QString intelligibleNameBg{NONE};
        bool bold{false};
        bool italic{false};
        bool underline{false};
    };

    ///\return true if string is valid ANSI color code
    static AnsiColor colorFromString(const QString &ansiString);

    ///\return true, if index is valid color code
    static bool colorFromNumber(int numColor, QColor &col, QString &intelligibleName);

private:
    AnsiMode mode;
};
