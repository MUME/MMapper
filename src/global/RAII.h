#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RuleOf5.h"
#include "utils.h"

#include <algorithm>
#include <functional>

class NODISCARD RAIICallback final
{
private:
    using Callback = std::function<void()>;
    Callback m_callback;
    bool m_was_moved = false;

public:
    /* move ctor */
    RAIICallback(RAIICallback &&rhs) noexcept;
    DELETE_COPY_CTOR(RAIICallback);
    DELETE_ASSIGN_OPS(RAIICallback);

public:
    explicit RAIICallback(Callback &&callback) noexcept;
    ~RAIICallback();
};
