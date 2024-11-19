// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "ExitFieldVariant.h"

#include "../global/Charset.h"
#include "sanitizer.h"

DoorName makeDoorName(std::string doorName)
{
    return DoorName{sanitizer::sanitizeOneLine(std::move(doorName))};
}

namespace mmqt {
DoorName makeDoorName(const QString &doorName)
{
    return ::makeDoorName(toStdStringUtf8(doorName));
}
} // namespace mmqt

bool tags::DoorNameTag::isValid(std::string_view sv)
{
    return sanitizer::isSanitizedOneLine(sv);
}
