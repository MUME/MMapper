// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#pragma once

#include "../global/macros.h"

#include <optional>

#include <QEvent>
#include <QPalette>

class QColor;
class QWidget;

// Hypothetically, Qt is supposed to provide the option to automatically change the window palette
// based on focus, but we can't get that to work, so this clunky class exists only to track two
// palettes and swap them when handling a focus change message.
class NODISCARD PaletteManager final
{
public:
    enum class NODISCARD FocusState { Focused, Unfocused };

private:
    QPalette m_focused;
    QPalette m_unfocused;
    MAYBE_UNUSED bool m_initialized = false;

public:
    void init(QWidget &widget, const std::optional<QColor> &activeBg, const QColor &inactiveBg);
    void setFocusState(QWidget &widget, FocusState focusState);
    void setFocused(QWidget &widget) { setFocusState(widget, FocusState::Focused); }
    void setUnfocused(QWidget &widget) { setFocusState(widget, FocusState::Unfocused); }
    void tryUpdateFromFocusEvent(QWidget &widget, QEvent::Type type);
};
