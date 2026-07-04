// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "EnumIndexedArray.h"

#include "enums.h"
#include "logging.h"

#include <array>
#include <cstdint>
#include <string>

namespace {
enum class NODISCARD AbcEnum : uint8_t { A, B, C };
} // namespace

DEFINE_ENUM_COUNT(AbcEnum, 3)

namespace {

template<typename T>
using AbcArrayView = EnumIndexedArray<T, AbcEnum>;

using Names = EnumIndexedArray<std::string, AbcEnum>;
static_assert(concepts::IsEnumIndexedArray<Names>);

using concepts::IsEnumIndexedArrayOfStrings;
static_assert(!IsEnumIndexedArrayOfStrings<AbcArrayView<int>>);
static_assert(IsEnumIndexedArrayOfStrings<AbcArrayView<std::string>>);
static_assert(IsEnumIndexedArrayOfStrings<AbcArrayView<std::string_view>>);
static_assert(IsEnumIndexedArrayOfStrings<AbcArrayView<const char *>>);

} // namespace
