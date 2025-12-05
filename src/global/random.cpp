// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "random.h"

#include <functional>

NODISCARD static auto expensiveSeed()
{
    return std::invoke(std::random_device{});
}

RandomEngine::RandomEngine()
    : std::mt19937{expensiveSeed()}
{}

RandomEngine &RandomEngine::getSingleton()
{
    static RandomEngine singleton;
    return singleton;
}

size_t getRandom(const size_t max)
{
    std::uniform_int_distribution<size_t> dist{0u, max}; // inclusive
    return dist(RandomEngine::getSingleton());
}
