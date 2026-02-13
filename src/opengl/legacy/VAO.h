#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../../global/EnumIndexedArray.h"
#include "Legacy.h"

#include <memory>

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

using SharedVao = std::shared_ptr<VAO>;
using WeakVao = std::weak_ptr<VAO>;

class NODISCARD SharedVaos final
    : private EnumIndexedArray<SharedVao, SharedVaoEnum, NUM_SHARED_VAOS>
{
private:
    using base = EnumIndexedArray<SharedVao, SharedVaoEnum, NUM_SHARED_VAOS>;

public:
    SharedVaos() = default;

public:
    NODISCARD SharedVao get(const SharedVaoEnum vao)
    {
        SharedVao &shared = base::operator[](vao);
        if (shared == nullptr) {
            shared = std::make_shared<VAO>();
        }
        return shared;
    }

    void reset(const SharedVaoEnum vao) { base::operator[](vao).reset(); }

    void resetAll()
    {
        base::for_each([](auto &shared) { shared.reset(); });
    }
};

} // namespace Legacy
