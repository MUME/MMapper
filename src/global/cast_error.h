#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "macros.h"

#include <cassert>
#include <cstdint>
#include <stdexcept>

enum class NODISCARD CastErrorEnum : uint8_t {
    Success,
    NotFinite,
    TooSmall,
    TooBig,
    RoundTripFailure,
};

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
struct NODISCARD CastErrorException : public std::runtime_error
{
    CastErrorEnum err = CastErrorEnum::Success;

    explicit CastErrorException(const CastErrorEnum e)
        : std::runtime_error{"CastErrorException"}
        , err{e}
    {
        assert(e != CastErrorEnum::Success);
    }
    ~CastErrorException() override;
};
#ifdef __clang__
#pragma clang diagnostic pop
#endif
