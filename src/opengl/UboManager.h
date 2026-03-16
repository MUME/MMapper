#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/EnumIndexedArray.h"
#include "../global/RuleOf5.h"
#include "../global/logging.h"
#include "../global/utils.h"
#include "legacy/Legacy.h"
#include "legacy/VBO.h"

#include <cassert>
#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <type_traits>
#include <vector>

namespace Legacy {

/**
 * @brief Central manager for Uniform Buffer Objects (UBOs).
 *
 * Tracks which UBOs are currently valid on the GPU and coordinates their updates.
 * Follows a lazy rebuild pattern: UBOs are only updated when a bind() is requested
 * and the block is marked as dirty (represented by std::nullopt in the bound buffer tracker).
 */
class UboManager final
{
public:
    using RebuildFunction = std::function<void(Legacy::Functions &gl)>;

    UboManager() { invalidateAll(); }
    DELETE_CTORS_AND_ASSIGN_OPS(UboManager);

public:
    /**
     * @brief Marks a UBO block as dirty by resetting its bound state.
     */
    void invalidate(Legacy::SharedVboEnum block) { m_boundBuffers[block] = std::nullopt; }

    /**
     * @brief Marks all UBO blocks as dirty.
     */
    void invalidateAll()
    {
        m_boundBuffers.for_each([](std::optional<GLuint> &v) { v = std::nullopt; });
    }

    /**
     * @brief Registers a function that can rebuild the UBO data.
     */
    void registerRebuildFunction(Legacy::SharedVboEnum block, RebuildFunction func)
    {
        assert(!m_rebuildFunctions[block] && "Rebuild function already registered for this UBO block");
        m_rebuildFunctions[block] = std::move(func);
    }

    /**
     * @brief Checks if a UBO block is currently dirty/invalid.
     */
    bool isInvalid(Legacy::SharedVboEnum block) const { return !m_boundBuffers[block].has_value(); }

    /**
     * @brief Rebuilds the UBO if it's invalid using the registered rebuild function.
     */
    void updateIfInvalid(Legacy::Functions &gl, Legacy::SharedVboEnum block)
    {
        if (isInvalid(block)) {
            const auto &func = m_rebuildFunctions[block];
            assert(func && "UBO block is invalid and no rebuild function is registered");
            if (func) {
                func(gl);
                // The rebuild function is expected to call update() which marks it valid.
                assert(!isInvalid(block));
            }
        }
    }

    /**
     * @brief Uploads data to the UBO and marks it as valid.
     * Also binds it to its assigned point.
     *
     * Overload for bulk vector data.
     */
    template<typename T, typename A>
    void update(Legacy::Functions &gl, Legacy::SharedVboEnum block, const std::vector<T, A> &data)
    {
        Legacy::VBO &vbo = getOrCreateVbo(gl, block);
        static_cast<void>(gl.setUbo(vbo.get(), data, BufferUsageEnum::DYNAMIC_DRAW));
        bind_internal(gl, block, vbo.get());
    }

    /**
     * @brief Uploads data to the UBO and marks it as valid.
     * Also binds it to its assigned point.
     *
     * Overload for single trivially-copyable objects.
     */
    template<typename T>
    void update(Legacy::Functions &gl, Legacy::SharedVboEnum block, const T &data)
    {
        static_assert(std::is_trivially_copyable_v<T>,
                      "T must be trivially copyable for UBO upload");
        Legacy::VBO &vbo = getOrCreateVbo(gl, block);
        gl.setUboSingle(vbo.get(), data, BufferUsageEnum::DYNAMIC_DRAW);
        bind_internal(gl, block, vbo.get());
    }

    /**
     * @brief Binds the UBO to its assigned point.
     * If invalid and a rebuild function is registered, it will be updated first.
     */
    void bind(Legacy::Functions &gl, Legacy::SharedVboEnum block)
    {
        updateIfInvalid(gl, block);

        if (isInvalid(block)) {
            MMLOG_ERROR() << "UboManager::bind: attempted to bind invalid UBO block "
                          << static_cast<int>(block);
            assert(false
                   && "UboManager::bind: attempted to bind invalid UBO block after "
                      "updateIfInvalid (missing or failing rebuild function?)");
            return;
        }

        Legacy::VBO &vbo = getOrCreateVbo(gl, block);
        bind_internal(gl, block, vbo.get());
    }

private:
    Legacy::VBO &getOrCreateVbo(Legacy::Functions &gl, Legacy::SharedVboEnum block)
    {
        const auto sharedVbo = gl.getSharedVbos().get(block);
        Legacy::VBO &vbo = deref(sharedVbo);

        if (!vbo) {
            vbo.emplace(gl.shared_from_this());
        }
        return vbo;
    }

private:
    /**
     * @brief Binds the UBO to its assigned point.
     *
     * Note: This implementation explicitly assumes that Legacy::SharedVboEnum values
     * are 0-based, contiguous, and directly correspond to UBO binding indices in
     * shader blocks.
     */
    void bind_internal(Legacy::Functions &gl, Legacy::SharedVboEnum block, GLuint buffer)
    {
        static_assert(static_cast<int>(Legacy::SharedVboEnum::NamedColorsBlock) == 0,
                      "Legacy::SharedVboEnum must be 0-based for UBO binding indexing");

        const auto bindingIndex = static_cast<std::size_t>(block);
        assert(bindingIndex < m_boundBuffers.size());

#ifndef NDEBUG
        static std::once_flag limitsOnce;
        std::call_once(limitsOnce, [&gl, this]() {
            GLint maxBindings = 0;
            gl.glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxBindings);
            assert(m_boundBuffers.size() <= static_cast<std::size_t>(maxBindings));
        });
#endif

        auto &bound = m_boundBuffers[block];
        if (!bound.has_value() || bound.value() != buffer) {
            gl.glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLuint>(bindingIndex), buffer);
            bound = buffer;
        }
    }

private:
    EnumIndexedArray<RebuildFunction, Legacy::SharedVboEnum> m_rebuildFunctions;
    EnumIndexedArray<std::optional<GLuint>, Legacy::SharedVboEnum> m_boundBuffers;
};

} // namespace Legacy
