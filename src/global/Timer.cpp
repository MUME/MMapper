// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Timer.h"

#include "logging.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <utility>

Timer::Timer(mm::source_location loc, std::string_view name)
    : m_loc{loc}
    , m_name{name}
{
    if (!loc.file_name()) {
        throw std::invalid_argument("file");
    }
    if (!loc.function_name()) {
        throw std::invalid_argument("function");
    }
}

Timer::~Timer()
{
    if (tl_depth-- == 0) {
        std::abort();
    }

    auto end = clock::now();
    const auto ns = static_cast<std::chrono::nanoseconds>(end - m_beg).count();
    const auto ONE_SECOND = int64_t(1'000'000'000);

    if (ns < ONE_SECOND / 50) {
        return;
    }

    std::ostringstream os;
    os << "[timer] ";

    for (size_t i = 0; i < m_depth; ++i) {
        os << char_consts::C_SPACE;
    }
    os << m_name << ": ";

    //const auto old_precision = os.precision();
    //os.precision(3);

    os << static_cast<double>(ns) * 1e-6 << " ms";

    // os.precision(old_precision);

    const auto s = std::move(os).str();
    mm::InfoOstream{m_loc} << s;
}
