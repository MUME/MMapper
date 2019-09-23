// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "property.h"

#include <stdexcept>

Property::Property(std::string s)
    : m_data{std::move(s)}
{}

Property::~Property() = default;
