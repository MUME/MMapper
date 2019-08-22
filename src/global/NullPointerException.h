#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <stdexcept>

#include "RuleOf5.h"

struct NullPointerException final : public std::runtime_error
{
    explicit NullPointerException();
    virtual ~NullPointerException() override;
    DEFAULT_CTORS_AND_ASSIGN_OPS(NullPointerException);
};
