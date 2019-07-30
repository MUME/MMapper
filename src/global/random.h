#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <random>
#include <stdexcept>

#include "RuleOf5.h"

class RandomEngine final : public std::mt19937
{
private:
    RandomEngine();

public:
    ~RandomEngine() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(RandomEngine);

public:
    static RandomEngine &getSingleton();
};

// returns a uniformly-distributed random number in [0..max], inclusive
size_t getRandom(size_t max);

template<typename T>
decltype(auto) chooseRandomElement(T &container)
{
    if (container.empty())
        throw std::invalid_argument("container");
    const auto dir = getRandom(container.size() - 1u);
    return container[dir];
}
