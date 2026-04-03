#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "macros.h"

#include <cassert>
#include <memory>
#include <stdexcept>

#include <QPointer>

namespace mmqt {
template<typename T, typename... Args>
    requires(std::is_base_of_v<QObject, T>)
NODISCARD QPointer<T> makeQPointer(Args &&...args)
{
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    if (ptr->QObject::parent() == nullptr) {
        throw std::runtime_error("QObject constructed without a parent");
    }
    // transfer ownership, now that we've confirmed that the parent object owns it
    return QPointer<T>{ptr.release()};
}
} // namespace mmqt
