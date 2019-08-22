#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <algorithm>
#include <functional>

#include "RuleOf5.h"
#include "utils.h"

class NODISCARD RAIIBool final
{
private:
    bool &ref;
    bool moved = false;

public:
    explicit RAIIBool(bool &b);
    ~RAIIBool();

public:
    /* move ctor */
    RAIIBool(RAIIBool &&rhs);
    DELETE_COPY_CTOR(RAIIBool);
    DELETE_ASSIGN_OPS(RAIIBool);
};

class NODISCARD RAIICallback final
{
private:
    using Callback = std::function<void()>;
    Callback callback;
    bool moved = false;

public:
    /* move ctor */
    RAIICallback(RAIICallback &&rhs);
    DELETE_COPY_CTOR(RAIICallback);
    DELETE_ASSIGN_OPS(RAIICallback);

public:
    explicit RAIICallback(Callback &&callback);
    ~RAIICallback() noexcept(false);
};
