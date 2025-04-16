// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "mmapper2character.h"

#include "../global/Charset.h"

#include <string_view>

namespace tags {
bool CharacterNameTag::isValid(const std::string_view sv)
{
    return charset::isValidUtf8(sv);
}
bool CharacterLabelTag::isValid(const std::string_view sv)
{
    return charset::isValidUtf8(sv);
}
bool CharacterRoomNameTag::isValid(const std::string_view sv)
{
    return charset::isValidUtf8(sv);
}
} // namespace tags
