#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/MmQtHandle.h"
#include "../global/macros.h"
#include "Change.h"
#include "Map.h"

#include <vector>

class NODISCARD ChangeList final
{
private:
    std::vector<Change> m_changes;

public:
    void add(const Change &change) { m_changes.emplace_back(change); }
#define X_NOP()
#define X_DEFINE_ADD(_Type) \
    void add(const _Type &change) { add(Change{change}); }
    XFOREACH_CHANGE_TYPE(X_DEFINE_ADD, X_NOP)
#undef X_DEFINE_ADD
#undef X_NOP

public:
    const std::vector<Change> &getChanges() const { return m_changes; }
    NODISCARD bool empty() const { return m_changes.empty(); }
};

using SigMapChangeList = MmQtHandle<ChangeList>;
Q_DECLARE_METATYPE(SigMapChangeList)
