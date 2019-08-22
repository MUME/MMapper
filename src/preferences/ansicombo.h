#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

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
