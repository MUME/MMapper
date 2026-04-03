#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/EnumIndexedArray.h"
#include "../global/RuleOf5.h"
#include "../global/logging.h"
#include "../global/utils.h"
#include "UboBlocks.h"
#include "legacy/Legacy.h"
#include "legacy/VBO.h"

#include <cassert>
#include <cstddef>
#include <functional>
#include <optional>
#include <tuple>
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
class NODISCARD UboManager final
{
public:
    using RebuildFunction = std::function<void(Functions &gl)>;

private:
    EnumIndexedArray<RebuildFunction, SharedVboEnum> m_rebuildFunctions;
    EnumIndexedArray<std::optional<GLuint>, SharedVboEnum> m_boundBuffers;

    // Tuple of all block types for shadow storage.
    SharedVboBlocks m_shadowBlocks;

public:
    UboManager() { invalidateAll(); }
    ~UboManager() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(UboManager);

public:
    /**
     * @brief Accesses the CPU-side shadow copy of a UBO block by its enum.
     */
    template<SharedVboEnum Block>
    NODISCARD BlockType_t<Block> &get()
    {
        return std::get<BlockType_t<Block>>(m_shadowBlocks);
    }

    /**
     * @brief Accesses the CPU-side shadow copy of a UBO block by its enum (const).
     */
    template<SharedVboEnum Block>
    NODISCARD const BlockType_t<Block> &get() const
    {
        return std::get<BlockType_t<Block>>(m_shadowBlocks);
    }

    /**
     * @brief Marks a UBO block as dirty by resetting its bound state.
     */
    void invalidate(const SharedVboEnum block) { m_boundBuffers[block] = std::nullopt; }

    /**
     * @brief Marks all UBO blocks as dirty.
     */
    void invalidateAll()
    {
        m_boundBuffers.for_each([](std::optional<GLuint> &v) { v = std::nullopt; });
    }

    /**
     * @brief Registers a function that can rebuild the UBO data.
     *
     * @param block           UBO block identifier.
     * @param func            Function used to rebuild the UBO data.
     * @param allowOverwrite  If false (default), overwriting an existing rebuild function
     *                        will trigger a debug assertion. Set to true only when an
     *                        overwrite is intentional.
     */
    void registerRebuildFunction(const SharedVboEnum block,
                                 RebuildFunction func,
                                 bool allowOverwrite = false)
    {
        if (m_rebuildFunctions[block]) {
            assert(allowOverwrite
                   && "UboManager::registerRebuildFunction: overwriting existing "
                      "rebuild function for UBO block");
        }
        m_rebuildFunctions[block] = std::move(func);
    }

    /**
     * @brief Unregisters a rebuild function for a UBO block.
     */
    void unregisterRebuildFunction(const SharedVboEnum block)
    {
        m_rebuildFunctions[block] = nullptr;
    }

    /**
     * @brief Checks if a UBO block is currently dirty/invalid.
     */
    NODISCARD bool isInvalid(const SharedVboEnum block) const
    {
        return !m_boundBuffers[block].has_value();
    }

    /**
     * @brief Rebuilds the UBO if it's invalid using the registered rebuild function.
     * @return The bound buffer ID, or 0 if failed.
     */
    ALLOW_DISCARD GLuint updateIfInvalid(Functions &gl, const SharedVboEnum block)
    {
        if (const auto bound = m_boundBuffers[block]) {
            return *bound;
        }

        const auto &func = m_rebuildFunctions[block];
        if (!func) {
            const char *name = Functions::getUniformBlockName(block);
            MMLOG_ERROR() << "UboManager::updateIfInvalid: UBO block '" << name
                          << "' is invalid and no rebuild function is registered";
            throw std::runtime_error("UBO block '" + std::string(name)
                                     + "' is invalid and no rebuild function is registered");
        }

        func(gl);

        if (const auto bound = m_boundBuffers[block]) {
            return *bound;
        }

        const char *name = Functions::getUniformBlockName(block);
        MMLOG_ERROR() << "UboManager::updateIfInvalid: rebuild function failed to call "
                         "update() for block '"
                      << name << "'";
        throw std::runtime_error("Rebuild function for block '" + std::string(name)
                                 + "' failed to call update()");
    }

    /**
     * @brief Uploads data to the UBO and marks it as valid.
     * Also binds it to its assigned point.
     * @return The bound buffer ID.
     *
     * Overload for bulk vector data.
     */
    template<typename T, typename A>
    ALLOW_DISCARD GLuint update(Functions &gl,
                                const SharedVboEnum block,
                                const std::vector<T, A> &data)
    {
        VBO &vbo = getOrCreateVbo(gl, block);
        gl.setVbo(GL_UNIFORM_BUFFER, vbo.get(), data, BufferUsageEnum::DYNAMIC_DRAW);
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
    ALLOW_DISCARD GLuint update(Functions &gl, const SharedVboEnum block, const T &data)
    {
        VBO &vbo = getOrCreateVbo(gl, block);
        gl.setVbo(GL_UNIFORM_BUFFER, vbo.get(), data, BufferUsageEnum::DYNAMIC_DRAW);
        return bind_internal(gl, block, vbo.get());
    }

    /**
     * @brief Type-safe upload to a UBO.
     * Enforces the correct data structure for the given block identifier.
     * Also updates the shadow copy.
     */
    template<SharedVboEnum Block>
    ALLOW_DISCARD GLuint update(Functions &gl, const BlockType_t<Block> &data)
    {
        get<Block>() = data;
        return update(gl, Block, data);
    }

    /**
     * @brief Syncs the entire shadow copy of a block to the GPU.
     */
    template<SharedVboEnum Block>
    ALLOW_DISCARD GLuint sync(Functions &gl)
    {
        return update(gl, Block, get<Block>());
    }

    /**
     * @brief Syncs multiple specific fields of a block to the GPU in a single bind.
     * @param gl       Legacy functions.
     * @param members  Pointers to the members in the block struct.
     */
    template<SharedVboEnum Block, typename T, typename... Us>
    void syncFields(Functions &gl, Us T::*...members)
    {
        using BlockType = BlockType_t<Block>;
        static_assert(std::is_same_v<T, BlockType>, "Members must belong to the correct block type");
        static_assert(std::is_standard_layout_v<BlockType>,
                      "Block type must have standard layout for offset calculation");

        // If the block has never been fully uploaded, fall back to a full sync.
        // glBufferSubData requires pre-allocated GPU storage from a prior glBufferData call.
        if (isInvalid(Block)) {
            sync<Block>(gl);
            return;
        }

        const auto &blockData = get<Block>();
        VBO &vbo = getOrCreateVbo(gl, Block);
        gl.glBindBuffer(GL_UNIFORM_BUFFER, vbo.get());

        (gl.glBufferSubData(GL_UNIFORM_BUFFER,
                            static_cast<GLintptr>(
                                reinterpret_cast<std::uintptr_t>(&(blockData.*members))
                                - reinterpret_cast<std::uintptr_t>(&blockData)),
                            static_cast<GLsizeiptr>(sizeof(Us)),
                            &(blockData.*members)),
         ...);

        gl.glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Ensure it's bound to the correct point.
        std::ignore = bind_internal(gl, Block, vbo.get());
    }

    /**
     * @brief Syncs a specific field of a block to the GPU.
     * @param gl      Legacy functions.
     * @param member  Pointer to the member in the block struct.
     */
    template<SharedVboEnum Block, typename T, typename U>
    void syncField(Functions &gl, U T::*member)
    {
        syncFields<Block>(gl, member);
    }

    /**
     * @brief Binds the UBO to its assigned point.
     * If invalid and a rebuild function is registered, it will be updated first.
     */
    void bind(Functions &gl, const SharedVboEnum block)
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
    NODISCARD VBO &getOrCreateVbo(Functions &gl, const SharedVboEnum block)
    {
        const auto sharedVbo = gl.getSharedVbos().get(block);
        VBO &vbo = deref(sharedVbo);

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
     * Note: This implementation explicitly assumes that SharedVboEnum values
     * are 0-based, contiguous, and directly correspond to UBO binding indices in
     * shader blocks.
     */
    NODISCARD GLuint bind_internal(Functions &gl, const SharedVboEnum block, const GLuint buffer)
    {
        const auto bindingIndex = getUboBindingIndex(block);
        assert(static_cast<std::size_t>(bindingIndex) < m_boundBuffers.size());

        auto &bound = m_boundBuffers[block];
        if (!bound.has_value() || bound.value() != buffer) {
            gl.glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex, buffer);
            bound = buffer;
        }
        return buffer;
    }
};

} // namespace Legacy
