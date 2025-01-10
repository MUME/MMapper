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

NODISCARD static AnsiCombo::AnsiItem initAnsiItem(const AnsiColor16 ansiCode,
                                                  const AnsiColor16LocationEnum mode)
{
    auto defColor = mode == AnsiColor16LocationEnum::Foreground ? AnsiColor16Enum::black
                                                                : AnsiColor16Enum::white;

    const AnsiColor16Enum color = ansiCode.color.value_or(defColor);

    auto make_pix = [color]() {
        QPixmap pix(20, 20);
        pix.fill(mmqt::toQColor(color));
        // Draw border
        {
            QPainter paint(&pix);
            paint.setPen(Qt::black);
            paint.drawRect(0, 0, 19, 19);
        }
        return pix;
    };

    AnsiCombo::AnsiItem retVal;
    retVal.color = ansiCode;
    retVal.loc = mode;
    retVal.picture = make_pix();
    retVal.description = mmqt::toQStringLatin1(ansiCode.isDefault() ? "none"
                                                                    : ansiCode.to_string_view());
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

    add(initAnsiItem(AnsiColor16{}, m_mode));

#define INIT(N, lower, UPPER) \
    add(initAnsiItem(AnsiColor16{AnsiColor16Enum::lower}, m_mode)); \
    add(initAnsiItem(AnsiColor16{AnsiColor16Enum::UPPER}, m_mode));

    XFOREACH_ANSI_COLOR_0_7(INIT)

#undef INIT

    this->setAnsiCode(AnsiColor16{});
}

AnsiCombo::AnsiColor AnsiCombo::colorFromString(const QString &colString)
{
    auto stdString = colString.toUtf8().toStdString();

    // No need to proceed if the color is empty
    if (colString.isEmpty()) {
        return AnsiColor{};
    }

    // TODO: use existing test (prepend an ESC if necessary)
    static const QRegularExpression re(R"(^\[((?:\d+[;:])*\d+)m$)");
    if (!re.match(colString).hasMatch()) {
        qWarning() << "String did not contain valid ANSI: " << colString;
        return AnsiColor{};
    }

    // TODO: use existing interface to split the string
    const AnsiColorState state = [&colString]() -> AnsiColorState {
        AnsiColorState result_state;
        QString tmpStr = colString;
        tmpStr.chop(1);
        tmpStr.remove(0, 1);
        for (const auto &s : tmpStr.split(";", Qt::SkipEmptyParts)) {
            const auto n = s.toInt();
            result_state.receive(n);
        }
        return result_state;
    }();

    if (!state.hasCompleteState()) {
        qWarning() << "String did not contain valid ANSI: " << colString;
    }

    auto toAnsiColor = [](const RawAnsi &raw) {
        AnsiColor color;
        color.fg = toAnsiColor16(raw.fg);
        color.bg = toAnsiColor16(raw.bg);
        color.bold = raw.hasBold();
        color.italic = raw.hasItalic();
        color.underline = raw.hasUnderline();
        return color;
    };

    const RawAnsi raw = state.getRawAnsi();
    return toAnsiColor(raw);
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
