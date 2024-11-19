#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "Change.h"
#include "ChangeTypes.h"

struct NODISCARD AbstractChangeVisitor
{
public:
    virtual ~AbstractChangeVisitor();

private:
#define X_NOP()
#define X_DECL_PURE_VIRT_ACCEPT(_Type) virtual void virt_accept(const _Type &) = 0;
    XFOREACH_CHANGE_TYPE(X_DECL_PURE_VIRT_ACCEPT, X_NOP)
#undef X_DECL_PURE_VIRT_ACCEPT
#undef X_NOP

public:
    void accept(const Change &change)
    {
        change.acceptVisitor([this](const auto &x) { this->virt_accept(x); });
    }
};
