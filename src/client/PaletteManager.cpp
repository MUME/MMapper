// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "PaletteManager.h"

#include "../global/Color.h"

#include <QWidget>

namespace { // anonymous
NODISCARD auto initPaletteBackground(QWidget &w,
                                     const std::optional<QColor> &opt,
                                     const Qt::BrushStyle style)
{
    auto p = w.palette();
    if (opt.has_value()) {
        auto &bg = opt.value();
        p.setBrush(QPalette::Base, QBrush(bg, style));
        p.setColor(QPalette::Text, mmqt::textColor(bg));
    } else {
        auto b = p.brush(QPalette::Base);
        b.setStyle(style);
        p.setBrush(QPalette::Base, b);
        p.setColor(QPalette::Text, mmqt::textColor(b.color()));
    }
    return p;
}
} // namespace

void PaletteManager::init(QWidget &widget,
                          const std::optional<QColor> &activeBg,
                          const QColor &inactiveBg)
{
    m_focused = initPaletteBackground(widget, activeBg, Qt::BrushStyle::SolidPattern);
    m_unfocused = initPaletteBackground(widget, inactiveBg, Qt::BrushStyle::BDiagPattern);
    m_initialized = true;
}

void PaletteManager::setFocusState(QWidget &widget, const FocusStateEnum focusState)
{
    assert(m_initialized);
    switch (focusState) {
    case FocusStateEnum::Focused:
        widget.setPalette(m_focused);
        break;
    case FocusStateEnum::Unfocused:
        widget.setPalette(m_unfocused);
        break;
    }
}

void PaletteManager::tryUpdateFromFocusEvent(QWidget &widget, const QEvent::Type type)
{
    if (type == QEvent::FocusIn) {
        setFocused(widget);
    } else if (type == QEvent::FocusOut) {
        setUnfocused(widget);
    }
}
