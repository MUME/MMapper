#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/EnumIndexedArray.h"
#include "Legacy.h"

#include <memory>
#include <vector>

namespace Legacy {

extern bool LOG_VBO_ALLOCATIONS;
extern bool LOG_VBO_STATIC_UPLOADS;

class NODISCARD VBO final
{
private:
    static inline constexpr GLuint INVALID_VBOID = 0;
    WeakFunctions m_weakFunctions;
    GLuint m_vbo = INVALID_VBOID;

public:
    VBO() = default;
    ~VBO() { reset(); }

    DELETE_CTORS_AND_ASSIGN_OPS(VBO);

public:
    void emplace(const SharedFunctions &sharedFunctions);
    void reset();
    NODISCARD GLuint get() const;

public:
    NODISCARD explicit operator bool() const { return m_vbo != INVALID_VBOID; }

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

class NODISCARD SharedVbos final
    : private EnumIndexedArray<SharedVbo, SharedVboEnum, NUM_SHARED_VBOS>
{
private:
    using base = EnumIndexedArray<SharedVbo, SharedVboEnum, NUM_SHARED_VBOS>;

public:
    SharedVbos() = default;

public:
    NODISCARD SharedVbo get(const SharedVboEnum buffer)
    {
        SharedVbo &shared = base::operator[](buffer);
        if (shared == nullptr) {
            shared = std::make_shared<VBO>();
        }
        return shared;
    }

    void reset(const SharedVboEnum buffer) { base::operator[](buffer).reset(); }

    void resetAll()
    {
        base::for_each([](auto &shared) { shared.reset(); });
    }
};

class NODISCARD Program final
{
private:
    static inline constexpr GLuint INVALID_PROGRAM = 0;
    WeakFunctions m_weakFunctions;
    GLuint m_program = INVALID_PROGRAM;

private:
    void swapWith(Program &other) noexcept;

public:
    Program() noexcept = default;
    ~Program() { reset(); }
    Program(Program &&) noexcept;
    Program &operator=(Program &&) noexcept;
    DELETE_COPIES(Program);

public:
    void emplace(const SharedFunctions &sharedFunctions);
    void reset();
    NODISCARD GLuint get() const;

public:
    NODISCARD explicit operator bool() const { return m_program != INVALID_PROGRAM; }
};

} // namespace Legacy
