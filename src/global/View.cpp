// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "View.h"

#include "../map/CommandId.h"
#include "../map/mmapper2room.h"
#include "EnumIndexedArray.h"

namespace {
using namespace concepts;

static_assert(IsView<View<int>>);

enum class AbcEnum : uint8_t { A, B, C };
constexpr size_t NUM_ABCS = 3;

template<typename T>
using AbcArray = EnumIndexedArray<T, AbcEnum, NUM_ABCS>;

static_assert(!IsEnumIndexedArray<std::array<int, NUM_ABCS>>);
static_assert(!IsEnumIndexedArray<std::vector<int>>);
static_assert(IsEnumIndexedArray<AbcArray<int>>);
static_assert(IsEnumIndexedArray<AbcArray<std::string>>);
static_assert(IsEnumIndexedArray<EnumIndexedArray<int, AbcEnum, NUM_ABCS>>);

} // namespace
