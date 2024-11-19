#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "RuleOf5.h"
#include "macros.h"
#include "mm_source_location.h"

#include <chrono>
#include <string_view>

#include <sys/types.h>

struct NODISCARD Timer final
{
private:
    using clock = std::chrono::steady_clock;
    static inline thread_local size_t tl_depth = 0;
    clock::time_point m_beg = clock::now();
    mm::source_location m_loc;
    std::string_view m_name = "";
    size_t m_depth = tl_depth++;

public:
    explicit Timer(mm::source_location loc, std::string_view name);
    DELETE_CTORS_AND_ASSIGN_OPS(Timer);
    ~Timer();
};

#define DECL_TIMER(var, name) auto var = (Timer{MM_SOURCE_LOCATION(), name})
