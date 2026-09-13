// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "ansicombo.h"

#include "../global/AnsiTextUtils.h"
#include "../global/TextUtils.h"

#include <cassert>
#include <memory>
#include <type_traits>

#include <QRegularExpression>
#include <QString>
#include <QtGui>
#include <QtWidgets>

AnsiCombo::AnsiCombo(const AnsiColor16LocationEnum mode, QWidget *const parent)
    : AnsiCombo(parent)
{
    initColours(mode);
}

AnsiCombo::AnsiCombo(QWidget *const parent)
    : super(parent)
{}

AnsiColor16 AnsiCombo::getAnsiCode() const
{
    const int idx = currentIndex();
    const int data = currentData().toInt();
    assert(idx == data);

    const auto &item = m_map.getItemAtUiIndex(static_cast<size_t>(idx));
    return item.color;
}

void AnsiCombo::setAnsiCode(const AnsiColor16 ansiCode)
{
    const auto &item = m_map.getItem(ansiCode);
    const int index = static_cast<int>(item.ui_index);
    setCurrentIndex(index);
    assert(getAnsiCode() == ansiCode);
}

NODISCARD static AnsiCombo::AnsiItem initAnsiItem(const AnsiColorTables::Entry &tableEntry,
                                                  const AnsiColor16LocationEnum mode)
{
    // The "no color" swatch's fill has no genuine ANSI color to draw, so it
    // falls back to black for foreground and white for background, just to
    // visually distinguish the two combos' "none" rows.
    const auto defColor = mode == AnsiColor16LocationEnum::Foreground ? AnsiColor16Enum::black
                                                                      : AnsiColor16Enum::white;
    auto make_pix = [&tableEntry, defColor]() {
        QPixmap pix(20, 20);
        pix.fill(mmqt::toQColor(tableEntry.color.color.value_or(defColor)));
        // Draw border
        {
            QPainter paint(&pix);
            paint.setPen(Qt::black);
            paint.drawRect(0, 0, 19, 19);
        }
        return pix;
    };

    AnsiCombo::AnsiItem retVal;
    retVal.color = tableEntry.color;
    retVal.loc = mode;
    retVal.picture = make_pix();
    retVal.description = tableEntry.description;
    return retVal;
}

void AnsiCombo::initColours(const AnsiColor16LocationEnum change)
{
    m_mode = change;
    m_map.clear();
    clear();

    auto add = [this](AnsiCombo::AnsiItem item) {
        m_map.insert(item);
        const auto &found = m_map.getItem(item.color);
        QVariant userData = static_cast<int>(found.ui_index);
        addItem(item.picture, item.description, userData);
    };

    for (const AnsiColorTables::Entry &entry : AnsiColorTables::colorTable()) {
        add(initAnsiItem(entry, m_mode));
    }

    this->setAnsiCode(AnsiColor16{});
}

AnsiCombo::AnsiColor AnsiCombo::colorFromString(const QString &colString)
{
    return AnsiColorTables::colorFromString(colString);
}

// TODO: move this some place more appropriate.
template<typename Derived, typename Base>
NODISCARD static inline Derived *qdynamic_downcast(Base *ptr)
{
    static_assert(std::is_base_of_v<Base, Derived>);
    return qobject_cast<Derived *>(ptr);
}

void AnsiCombo::makeWidgetColoured(QWidget *const pWidget,
                                   const QString &ansiColor,
                                   const bool changeText)
{
    if (pWidget == nullptr) {
        assert(false);
        return;
    }

    AnsiColor color = colorFromString(ansiColor);
    QPalette palette = pWidget->palette();

    // crucial call to have background filled
    pWidget->setAutoFillBackground(true);

    palette.setColor(QPalette::Window,
                     mmqt::toQColor(color.bg.color.value_or(AnsiColor16Enum::white)));
    palette.setColor(QPalette::WindowText,
                     mmqt::toQColor(color.fg.color.value_or(AnsiColor16Enum::black)));

    pWidget->setPalette(palette);
    pWidget->setBackgroundRole(QPalette::Window);

    auto *pLabel = qdynamic_downcast<QLabel>(pWidget);
    if (pLabel == nullptr) {
        return;
    }

    const auto display_string = [&color, &changeText](auto labelText) -> QString {
        if (!changeText) {
            // Strip previous HTML entities
            QRegularExpression re(R"(</?[b|i|u]>)");
            labelText.replace(re, "");
            return labelText;
        }

        const bool hasBg = color.bg.hasColor();
        const bool hasFg = color.fg.hasColor();

        if (!hasFg && !hasBg) {
            return QString("none");
        } else if (hasFg && !hasBg) {
            return color.getFgName();
        } else if (!hasFg && hasBg) {
            return QString("on %2").arg(color.getBgName());
        } else {
            return QString("%1 on %2").arg(color.getFgName()).arg(color.getBgName());
        }
    };

    QString displayString = display_string(pLabel->text());

    if (color.bold) {
        displayString = QString("<b>%1</b>").arg(displayString);
    }
    if (color.italic) {
        displayString = QString("<i>%1</i>").arg(displayString);
    }
    if (color.underline) {
        displayString = QString("<u>%1</u>").arg(displayString);
    }

    pLabel->setText(displayString);
}
