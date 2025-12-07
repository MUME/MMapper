#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "Crtp.h"

struct NODISCARD ExitFields final : public ExitFieldsGetters<ExitFields>,
                                    public ExitFieldsSetters<ExitFields>
{
public:
#define X_DECL_FIELD(_Type, _Prop, _OptInit) _Type _Prop _OptInit;
    XFOREACH_EXIT_PROPERTY(X_DECL_FIELD)
#undef X_DECL_FIELD

public:
    NODISCARD bool operator==(const ExitFields &rhs) const
    {
#define X_CHECK(_Type, _Prop, _OptInit) \
    if ((_Prop) != (rhs._Prop)) { \
        return false; \
    }
        XFOREACH_EXIT_PROPERTY(X_CHECK)
#undef X_CHECK
        return true;
    }
    NODISCARD bool operator!=(const ExitFields &rhs) const { return !(rhs == *this); }

private:
    friend ExitFieldsGetters<ExitFields>;
    friend ExitFieldsSetters<ExitFields>;
    NODISCARD ExitFields &getExitFields() { return *this; }
    NODISCARD const ExitFields &getExitFields() const { return *this; }
};
