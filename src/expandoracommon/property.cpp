// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "property.h"

#include <stdexcept>

Property::Property(Property::TagSkip)
    : m_skipped{true}
{}

Property::~Property() = default;

const char *Property::rest() const
{
    if (m_skipped) {
        throw std::runtime_error("can't get a string from a SKIPPED property");
    }
    if (pos >= static_cast<uint32_t>(size())) {
        return "";
    }
    return this->c_str() + pos;
}
