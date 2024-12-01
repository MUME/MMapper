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

struct NODISCARD CastErrorException : public std::runtime_error
{
    CastErrorEnum err = CastErrorEnum::Success;

    // This is just to avoid a padding warning.
    static_assert(alignof(std::runtime_error) > sizeof(CastErrorEnum));
    MAYBE_UNUSED char unused_pad[alignof(std::runtime_error) - sizeof(CastErrorEnum)]{};

    explicit CastErrorException(const CastErrorEnum e)
        : std::runtime_error{"CastErrorException"}
        , err{e}
    {
        assert(e != CastErrorEnum::Success);
    }
    ~CastErrorException() override;
};
