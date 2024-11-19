#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "roomid.h"

enum class NODISCARD InOutEnum { In, Out };
NODISCARD static inline constexpr InOutEnum opposite(const InOutEnum mode)
{
    return (mode == InOutEnum::In) ? InOutEnum::Out : InOutEnum::In;
}
