#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef MMAPPER_RANDOM_H
#define MMAPPER_RANDOM_H

#include <random>
#include <stdexcept>

#include "RuleOf5.h"

class RandomEngine final : public std::mt19937
{
private:
    explicit RandomEngine();

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

#endif // MMAPPER_RANDOM_H
