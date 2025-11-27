#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "Legacy.h"

namespace Legacy {

extern bool LOG_VAO_ALLOCATIONS;

class NODISCARD VAO final
{
private:
    static inline constexpr GLuint INVALID_VAOID = 0;
    WeakFunctions m_weakFunctions;
    GLuint m_vao = INVALID_VAOID;

public:
    VAO() = default;
    ~VAO() { reset(); }

    DELETE_CTORS_AND_ASSIGN_OPS(VAO);

public:
    void emplace(const SharedFunctions &sharedFunctions);
    void reset();
    NODISCARD GLuint get() const;

public:
    NODISCARD explicit operator bool() const { return m_vao != INVALID_VAOID; }
};

} // namespace Legacy
