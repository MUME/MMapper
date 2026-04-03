#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "Legacy.h"

#include <memory>

namespace Legacy {

// TFO = Transform Feedback Object
class NODISCARD TFO final
{
private:
    static inline constexpr GLuint INVALID_TFO = 0;
    WeakFunctions m_weakFunctions;
    GLuint m_tfo = INVALID_TFO;

public:
    TFO() = default;
    ~TFO() { reset(); }

    DELETE_CTORS_AND_ASSIGN_OPS(TFO);

public:
    void emplace(const SharedFunctions &sharedFunctions);
    void reset();
    NODISCARD GLuint get() const;

public:
    NODISCARD explicit operator bool() const { return m_tfo != INVALID_TFO; }
};

using SharedTfo = std::shared_ptr<TFO>;
using WeakTfo = std::weak_ptr<TFO>;

} // namespace Legacy
