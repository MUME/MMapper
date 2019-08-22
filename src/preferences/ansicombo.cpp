// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "ansicombo.h"

#include <cassert>
#include <memory>
#include <type_traits>
#include <QRegularExpression>
#include <QString>
#include <QtGui>
#include <QtWidgets>

#include "../global/Color.h"

struct AnsiItem
{
    int ansiCode{};
    QString description{};
    QIcon picture{};
};
using AnsiItemVector = QVector<AnsiItem>;

AnsiCombo::AnsiCombo(AnsiMode mode, QWidget *parent)
    : AnsiCombo(parent)
{
    initColours(mode);
}

AnsiCombo::AnsiCombo(QWidget *parent)
    : super(parent)
{}

int AnsiCombo::getAnsiCode() const
{
    return currentData(Qt::UserRole).toInt();
}

void AnsiCombo::setAnsiCode(const int ansiCode)
{
    const int index = findData(ansiCode, Qt::UserRole, Qt::MatchCaseSensitive);

    if (index >= 0) {
        setCurrentIndex(index);
    } else {
        qWarning() << "Could not find ansiCode" << ansiCode;
        setCurrentIndex(findText(NONE));
    }
}

static AnsiItem initAnsiItem(int ansiCode)
{
    QColor col;
    AnsiItem retVal;

    if (AnsiCombo::colorFromNumber(ansiCode, col, retVal.description)) {
        QPixmap pix(20, 20);
        pix.fill(col);

        // Draw border
        std::unique_ptr<QPainter> paint(new QPainter(&pix));
        paint->setPen(Qt::black);
        paint->drawRect(0, 0, 19, 19);

        retVal.picture = pix;
        retVal.ansiCode = ansiCode;
    }
    return retVal;
}

void AnsiCombo::initColours(AnsiMode change)
{
    mode = change;
    AnsiItemVector colours{};

    const auto high_color = [](int i) { return i + 60; };

    switch (mode) {
    case AnsiMode::ANSI_FG:
        colours.push_back(initAnsiItem(DEFAULT_FG));
        for (int i = 30; i < 38; ++i) {
            colours.push_back(initAnsiItem(i));
            colours.push_back(initAnsiItem(high_color(i)));
        }
        break;
    case AnsiMode::ANSI_BG:
        colours.push_back(initAnsiItem(DEFAULT_BG));
        for (int i = 40; i < 48; ++i) {
            colours.push_back(initAnsiItem(i));
            colours.push_back(initAnsiItem(high_color(i)));
        }
        break;
    }

    clear();

    for (const AnsiItem &item : colours) {
        addItem(item.picture, item.description, item.ansiCode);
    }
}

AnsiCombo::AnsiColor AnsiCombo::colorFromString(const QString &colString)
{
    AnsiColor color;

    // No need to proceed if the color is empty
    if (colString.isEmpty())
        return color;

    static const QRegularExpression re(R"(^\[((?:\d+;)*\d+)m$)");
    if (!re.match(colString).hasMatch()) {
        qWarning() << "String did not contain valid ANSI: " << colString;
        return color;
    }

    const auto update_fg = [&color](const int n) {
        assert((30 <= n && n <= 37) || (90 <= n && n <= 97) || n == DEFAULT_FG);
        color.ansiCodeFg = n;
        if (!colorFromNumber(color.ansiCodeFg, color.colFg, color.intelligibleNameFg))
            qWarning() << "Unable to extract foreground color and name from ansiCode" << n;
    };
    const auto update_bg = [&color](const int n) {
        assert((40 <= n && n <= 47) || (100 <= n && n <= 107) || n == DEFAULT_BG);
        color.ansiCodeBg = n;
        if (!colorFromNumber(color.ansiCodeBg, color.colBg, color.intelligibleNameBg))
            qWarning() << "Unable to extract background color and name from ansiCode" << n;
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
            color.italic = false;
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
        intelligibleName = tr(NONE);
        break;
    case DEFAULT_BG:
        col = ansiColor(AnsiColorTable::black);
        intelligibleName = tr(NONE);
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
    static_assert(std::is_base_of_v<Base, Derived>);
    return qobject_cast<Derived *>(ptr);
}

void AnsiCombo::makeWidgetColoured(QWidget *const pWidget,
                                   const QString &ansiColor,
                                   const bool changeText)
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
            const auto display_string = [&color, &changeText](auto labelText) {
                if (!changeText) {
                    // Strip previous HTML entities
                    QRegularExpression re(R"(</?[b|i|u]>)");
                    labelText.replace(re, "");
                    return labelText;
                }

                const bool hasFg = color.intelligibleNameFg != NONE;
                const bool hasBg = color.intelligibleNameBg != NONE;
                if (!hasFg && !hasBg)
                    return QString(NONE);
                else if (hasFg && !hasBg)
                    return color.intelligibleNameFg;
                else if (!hasFg && hasBg)
                    return QString("on %2").arg(color.intelligibleNameBg);
                else
                    return QString("%1 on %2")
                        .arg(color.intelligibleNameFg)
                        .arg(color.intelligibleNameBg);
            };
            QString displayString = display_string(pLabel->text());
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
