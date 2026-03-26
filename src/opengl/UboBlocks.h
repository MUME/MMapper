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
#define XFOREACH_SHARED_VBO(X) \
    X(NamedColorsBlock, "NamedColorsBlock") \
    X(CameraBlock, "CameraBlock") \
    X(TimeBlock, "TimeBlock") \
    X(WeatherBlock, "WeatherBlock")

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
 * @brief Memory layout for the Camera uniform block.
 * Must match CameraBlock in shaders (std140 layout).
 */
struct NODISCARD CameraBlock final
{
    glm::mat4 viewProj{1.0f};  // 0-63
    glm::vec4 playerPos{0.0f}; // 64-79 (xyz, w=zScale)
};

/**
 * @brief Memory layout for the Time uniform block.
 * Must match TimeBlock in shaders (std140 layout).
 */
struct NODISCARD TimeBlock final
{
    glm::vec4 time{0.0f}; // 0-15 (x=time, y=delta, zw=unused)
};

/**
 * @brief Memory layout for the Weather uniform block.
 * Must match WeatherBlock in shaders (std140 layout).
 */
struct NODISCARD WeatherBlock final
{
    glm::vec4 intensities{0.0f}; // 0-15: rain, snow, clouds, fog (starts)
    glm::vec4 targets{0.0f};     // 16-31: rain, snow, clouds, fog (targets)
    glm::vec4 timeOfDay{0.0f};   // 32-47: startIdx, targetIdx, todStart, todTarget
    glm::vec4 config{0.0f};      // 48-63: weatherStartTime, todStartTime, duration, unused
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
