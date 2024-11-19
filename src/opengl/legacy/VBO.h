#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/utils.h"
#include "Legacy.h"

#include <memory>
#include <vector>

namespace Legacy {

extern bool LOG_VBO_ALLOCATIONS;
extern bool LOG_VBO_STATIC_UPLOADS;

class NODISCARD VBO final
{
private:
    WeakFunctions m_weakFunctions;
    GLuint m_vbo = 0;

public:
    VBO() = default;

    DELETE_CTORS_AND_ASSIGN_OPS(VBO);
    ~VBO() { reset(); }

public:
    void emplace(const SharedFunctions &sharedFunctions);
    void reset();
    NODISCARD GLuint get() const;

public:
    NODISCARD explicit operator bool() const { return m_vbo != 0; }

public:
    void unsafe_swapVboId(VBO &other) { std::swap(m_vbo, other.m_vbo); }
};

using SharedVbo = std::shared_ptr<VBO>;
using WeakVbo = std::weak_ptr<VBO>;

class NODISCARD StaticVbos final : private std::vector<SharedVbo>
{
private:
    using base = std::vector<SharedVbo>;

public:
    StaticVbos() = default;

public:
    NODISCARD SharedVbo alloc()
    {
        base::emplace_back(std::make_shared<VBO>());
        return base::back();
    }

    void resetAll() { base::clear(); }
};

} // namespace Legacy
