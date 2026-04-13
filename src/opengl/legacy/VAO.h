#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../../global/EnumIndexedArray.h"
#include "../../global/logging.h"
#include "Legacy.h"
#include "ScopedBinder.h"

#include <memory>

namespace Legacy {

class NODISCARD VAO final
{
private:
    static inline constexpr GLuint INVALID_VAOID = 0;
    WeakFunctions m_weakFunctions;
    GLuint m_vao = INVALID_VAOID;
    bool m_bound = false;

public:
    VAO() = default;
    ~VAO() { reset(); }

    DELETE_CTORS_AND_ASSIGN_OPS(VAO);

public:
    void emplace(const SharedFunctions &sharedFunctions);
    void reset();
    NODISCARD GLuint get() const;

public:
    NODISCARD bool isValid() const { return m_vao != INVALID_VAOID; }
    DEPRECATED_MSG("use IsValid")
    NODISCARD explicit operator bool() const { return isValid(); }
    void unsafe_swapVaoId(VAO &other)
    {
        assert(!m_bound);
        assert(!other.m_bound);
        std::swap(m_vao, other.m_vao);
        std::swap(m_weakFunctions, other.m_weakFunctions);
    }

public:
    using Unbinder = ScopedBinder<VAO>;

    void unbind_impl(Badge<Unbinder>)
    {
        assert(isValid());
        assert(m_bound);
        m_bound = false;
        if (const auto shared = m_weakFunctions.lock(); shared == nullptr) {
            MMLOG_WARNING() << "Failed to unbind VAO " << m_vao;
        } else {
            shared->glBindVertexArray(Badge<VAO>(), 0);
        }
    }

    NODISCARD Unbinder bind()
    {
        assert(isValid());
        assert(!m_bound);

        if (const auto shared = m_weakFunctions.lock(); shared == nullptr) {
            MMLOG_WARNING() << "Failed to bind VAO " << m_vao;
            return Unbinder{};
        } else {
            shared->glBindVertexArray(Badge<VAO>(), m_vao);
            m_bound = true;
            return Unbinder{*this};
        }
    }
};

using SharedVao = std::shared_ptr<VAO>;
using WeakVao = std::weak_ptr<VAO>;

class NODISCARD SharedVaos final
    : private EnumIndexedArray<SharedVao, SharedVaoEnum, NUM_SHARED_VAOS>
{
private:
    using base = EnumIndexedArray<SharedVao, SharedVaoEnum, NUM_SHARED_VAOS>;

public:
    explicit SharedVaos() = default;

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

class NODISCARD StaticVaos final : private std::vector<SharedVao>
{
private:
    using base = std::vector<SharedVao>;

public:
    explicit StaticVaos() = default;

public:
    NODISCARD SharedVao alloc()
    {
        base::emplace_back(std::make_shared<VAO>());
        return base::back();
    }

    void resetAll() { base::clear(); }
};

} // namespace Legacy
