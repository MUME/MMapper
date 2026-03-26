#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/NamedColors.h"
#include "../global/utils.h"

#include <array>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

#include <glm/glm.hpp>

namespace Legacy {

// X(EnumName, GL_String_Name)
/**
 * Note: SharedVboEnum values are implicitly used as UBO binding indices.
 * They must be 0-based and contiguous.
 */
#define XFOREACH_SHARED_VBO(X) X(NamedColorsBlock, "NamedColorsBlock")

#define X_ENUM(element, name) element,
enum class NODISCARD SharedVboEnum : uint8_t { XFOREACH_SHARED_VBO(X_ENUM) NUM_BLOCKS };
#undef X_ENUM

static constexpr size_t NUM_SHARED_VBOS = static_cast<size_t>(SharedVboEnum::NUM_BLOCKS);

inline constexpr const char *const SharedVboNames[] = {
#define X_NAME(EnumName, StringName) StringName,
    XFOREACH_SHARED_VBO(X_NAME)
#undef X_NAME
};

/**
 * @brief Memory layout for the NamedColors uniform block.
 * Must match NamedColorsBlock in shaders (std140 layout).
 */
struct NODISCARD NamedColorsBlock final
{
    std::array<glm::vec4, MAX_NAMED_COLORS> colors{};
};

template<SharedVboEnum Block>
struct BlockType;

#define X_TYPE(EnumName, StringName) \
    template<> \
    struct BlockType<SharedVboEnum::EnumName> \
    { \
        using type = EnumName; \
    };
XFOREACH_SHARED_VBO(X_TYPE)
#undef X_TYPE

#define X_ASSERT(EnumName, StringName) \
    static_assert(std::is_standard_layout_v<EnumName>, \
                  "UBO block " #EnumName " must have standard layout for offset calculations"); \
    static_assert(std::is_trivially_copyable_v<EnumName>, \
                  "UBO block " #EnumName " must be trivially copyable for buffer uploads");
XFOREACH_SHARED_VBO(X_ASSERT)
#undef X_ASSERT

template<std::size_t... Is>
auto MakeSharedVboBlocksHelper(std::index_sequence<Is...>)
    -> std::tuple<typename BlockType<static_cast<SharedVboEnum>(Is)>::type...>;

using SharedVboBlocks = decltype(MakeSharedVboBlocksHelper(
    std::make_index_sequence<NUM_SHARED_VBOS>{}));

} // namespace Legacy
