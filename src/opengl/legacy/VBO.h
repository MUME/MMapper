#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../../global/EnumIndexedArray.h"
#include "../../global/logging.h"
#include "Legacy.h"
#include "ScopedBinder.h"

#include <memory>
#include <vector>

namespace Legacy {

class NODISCARD VBO final
{
private:
    static inline constexpr GLuint INVALID_VBOID = 0;
    WeakFunctions m_weakFunctions;
    GLuint m_vbo = INVALID_VBOID;
    GLenum m_boundTarget = GL_ARRAY_BUFFER;
    bool m_bound = false;
    bool m_bound_buffer_base = false;

public:
    VBO() = default;
    ~VBO() { reset(); }

    DELETE_CTORS_AND_ASSIGN_OPS(VBO);

public:
    void emplace(const SharedFunctions &sharedFunctions);
    void reset();
    NODISCARD GLuint get() const;

public:
    NODISCARD bool isValid() const { return m_vbo != INVALID_VBOID; }
    DEPRECATED_MSG("use isValid()")
    NODISCARD explicit operator bool() const { return isValid(); }

public:
    void unsafe_swapVboId(VBO &other)
    {
        assert(!m_bound);
        assert(!other.m_bound);
        std::swap(m_vbo, other.m_vbo);
        std::swap(m_weakFunctions, other.m_weakFunctions);
    }

    using Unbinder = ScopedBinder<VBO>;

    void unbind_impl(Badge<Unbinder>)
    {
        assert(m_bound);
        m_bound = false;
        if (const auto shared = m_weakFunctions.lock(); shared == nullptr) {
            MMLOG_WARNING() << "Failed to unbind VBO " << m_vbo;
        } else {
            shared->glBindBuffer(m_boundTarget, 0);
        }
    }

    NODISCARD Unbinder bind(const GLenum target)
    {
        assert(isValid());
        assert(!m_bound);
        assert(!m_bound_buffer_base); // remove this line if needed.

        if (const auto shared = m_weakFunctions.lock(); shared == nullptr) {
            MMLOG_WARNING() << "Failed to bind VBO " << m_vbo;
            return Unbinder{};
        } else {
            deref(shared).glBindBuffer(target, m_vbo);
            m_boundTarget = target;
            m_bound = true;
            return Unbinder{*this};
        }
    }
    NODISCARD auto bindBufferBase(const GLenum target, const GLuint index)
    {
        assert(isValid());
        assert(!m_bound_buffer_base);
        assert(!m_bound); // remove this line if needed.

        class NODISCARD Unbinder final
        {
        private:
            VBO *m_self = nullptr;
            GLenum m_target = GL_ARRAY_BUFFER;
            GLuint m_index = 0;

        public:
            explicit Unbinder() = default;
            explicit Unbinder(VBO &self, const GLenum target_, const GLuint index_)
                : m_self{&self}
                , m_target{target_}
                , m_index{index_}
            {}
            DELETE_CTORS_AND_ASSIGN_OPS(Unbinder);
            ~Unbinder()
            {
                if (m_self == nullptr) {
                    return;
                }
                auto &self = *m_self;
                assert(self.m_bound_buffer_base);
                self.m_bound_buffer_base = false;
                if (const auto shared = self.m_weakFunctions.lock(); shared == nullptr) {
                    MMLOG_WARNING()
                        << "Failed to unbind buffer base " << m_index << " VBO " << self.m_vbo;
                } else {
                    deref(shared).glBindBufferBase(m_target, m_index, 0);
                }
            }
        };

        if (const auto shared = m_weakFunctions.lock(); shared == nullptr) {
            MMLOG_WARNING() << "Failed to bind buffer base " << index << " VBO " << m_vbo;
            return Unbinder{};
        } else {
            deref(shared).glBindBufferBase(target, index, m_vbo);
            m_bound_buffer_base = true;
            return Unbinder{*this, target, index};
        }
    }

    template<typename T>
    NODISCARD GLsizei setVbo(const GLenum target,
                             const View<T> batch,
                             const BufferUsageEnum usage = BufferUsageEnum::DYNAMIC_DRAW)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        const auto numElements = static_cast<GLsizei>(batch.size());
        const auto elementSize = static_cast<GLsizei>(sizeof(T));
        const auto numBytes = numElements * elementSize;
        MAYBE_UNUSED auto vbo_binder = bind(target);
        if (const auto shared = m_weakFunctions.lock(); shared == nullptr) {
            MMLOG_WARNING() << "Failed to set VBO " << m_vbo;
            return 0;
        } else {
            shared->glBufferData(target, numBytes, batch.data(), Legacy::toGLenum(usage));
            return numElements;
        }
    }

    template<typename T>
    void setVbo(const GLenum target,
                const T &data,
                const BufferUsageEnum usage = BufferUsageEnum::DYNAMIC_DRAW)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        MAYBE_UNUSED auto vbo_binder = bind(target);
        if (const auto shared = m_weakFunctions.lock(); shared == nullptr) {
            MMLOG_WARNING() << "Failed to set VBO " << m_vbo;
        } else {
            shared->glBufferData(target, sizeof(T), &data, Legacy::toGLenum(usage));
        }
    }

    template<typename T>
    NODISCARD auto setVbo(const DrawModeEnum mode,
                          const View<T> batch,
                          const BufferUsageEnum usage = BufferUsageEnum::DYNAMIC_DRAW)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        using Pair = std::pair<DrawModeEnum, GLsizei>;
        if (const auto shared = m_weakFunctions.lock(); shared == nullptr) {
            MMLOG_WARNING() << "Failed to set VBO " << m_vbo;
            return Pair{mode, 0};
        } else {
            if (mode == DrawModeEnum::QUADS) {
                const auto tris = convertQuadsToTris(batch);
                return Pair{DrawModeEnum::TRIANGLES, setVbo(GL_ARRAY_BUFFER, View<T>{tris}, usage)};
            }
            return Pair{mode, setVbo(GL_ARRAY_BUFFER, batch, usage)};
        }
    }

    void clearVbo(const BufferUsageEnum usage = BufferUsageEnum::DYNAMIC_DRAW)
    {
        MAYBE_UNUSED auto vbo_binder = bind(GL_ARRAY_BUFFER);
        if (const auto shared = m_weakFunctions.lock(); shared == nullptr) {
            MMLOG_WARNING() << "Failed to clear VBO " << m_vbo;
        } else {
            shared->glBufferData(GL_ARRAY_BUFFER, 0, nullptr, Legacy::toGLenum(usage));
        }
    }
};

using SharedVbo = std::shared_ptr<VBO>;
using WeakVbo = std::weak_ptr<VBO>;

class NODISCARD StaticVbos final : private std::vector<SharedVbo>
{
private:
    using base = std::vector<SharedVbo>;

public:
    explicit StaticVbos() = default;

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
    explicit SharedVbos() = default;

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
    explicit Program() noexcept = default;
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
