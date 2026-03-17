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
 *
 * Note: This class is not thread-safe.
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
        if (m_rebuildFunctions[block]) {
            MMLOG_WARNING() << "UboManager::registerRebuildFunction: overwriting existing "
                               "rebuild function for UBO block "
                            << static_cast<int>(block);
        }
        m_rebuildFunctions[block] = std::move(func);
    }

    /**
     * @brief Checks if a UBO block is currently dirty/invalid.
     */
    bool isInvalid(Legacy::SharedVboEnum block) const { return !m_boundBuffers[block].has_value(); }

    /**
     * @brief Rebuilds the UBO if it's invalid using the registered rebuild function.
     * @return The bound buffer ID, or 0 if failed.
     */
    GLuint updateIfInvalid(Legacy::Functions &gl, Legacy::SharedVboEnum block)
    {
        if (const auto bound = m_boundBuffers[block]) {
            return *bound;
        }

        const auto &func = m_rebuildFunctions[block];
        assert(func && "UBO block is invalid and no rebuild function is registered");
        if (func) {
            func(gl);

            if (const auto bound = m_boundBuffers[block]) {
                return *bound;
            }

            MMLOG_ERROR() << "UboManager::updateIfInvalid: rebuild function failed to call "
                             "update() for block "
                          << static_cast<int>(block);
            assert(false && "Rebuild function must call update()");
        }
        return 0;
    }

    /**
     * @brief Uploads data to the UBO and marks it as valid.
     * Also binds it to its assigned point.
     * @return The bound buffer ID.
     *
     * Overload for bulk vector data.
     */
    template<typename T, typename A>
    GLuint update(Legacy::Functions &gl, Legacy::SharedVboEnum block, const std::vector<T, A> &data)
    {
        Legacy::VBO &vbo = getOrCreateVbo(gl, block);
        static_cast<void>(gl.setVbo(GL_UNIFORM_BUFFER, vbo.get(), data, BufferUsageEnum::DYNAMIC_DRAW));
        return bind_internal(gl, block, vbo.get());
    }

    /**
     * @brief Uploads data to the UBO and marks it as valid.
     * Also binds it to its assigned point.
     * @return The bound buffer ID.
     *
     * Overload for single trivially-copyable objects.
     */
    template<typename T>
    GLuint update(Legacy::Functions &gl, Legacy::SharedVboEnum block, const T &data)
    {
        Legacy::VBO &vbo = getOrCreateVbo(gl, block);
        gl.setVbo(GL_UNIFORM_BUFFER, vbo.get(), data, BufferUsageEnum::DYNAMIC_DRAW);
        return bind_internal(gl, block, vbo.get());
    }

    /**
     * @brief Binds the UBO to its assigned point.
     * If invalid and a rebuild function is registered, it will be updated first.
     */
    void bind(Legacy::Functions &gl, Legacy::SharedVboEnum block)
    {
        const GLuint buffer = updateIfInvalid(gl, block);

        if (buffer == 0) {
            MMLOG_ERROR() << "UboManager::bind: attempted to bind invalid UBO block "
                          << static_cast<int>(block);
            assert(false
                   && "UboManager::bind: attempted to bind invalid UBO block after "
                      "updateIfInvalid (missing or failing rebuild function?)");
            return;
        }

        // updateIfInvalid already ensured it is bound.
        assert(m_boundBuffers[block] == buffer);
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
     * @return The bound buffer ID.
     *
     * Note: This implementation explicitly assumes that Legacy::SharedVboEnum values
     * are 0-based, contiguous, and directly correspond to UBO binding indices in
     * shader blocks.
     */
    GLuint bind_internal(Legacy::Functions &gl, Legacy::SharedVboEnum block, GLuint buffer)
    {
        const auto bindingIndex = Legacy::getUboBindingIndex(block);
        assert(static_cast<std::size_t>(bindingIndex) < m_boundBuffers.size());

        auto &bound = m_boundBuffers[block];
        if (!bound.has_value() || bound.value() != buffer) {
            gl.glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex, buffer);
            bound = buffer;
        }
        return buffer;
    }

private:
    EnumIndexedArray<RebuildFunction, Legacy::SharedVboEnum> m_rebuildFunctions;
    EnumIndexedArray<std::optional<GLuint>, Legacy::SharedVboEnum> m_boundBuffers;
};

} // namespace Legacy
