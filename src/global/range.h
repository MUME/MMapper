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

#ifndef MMAPPER_RANGE_H
#define MMAPPER_RANGE_H

#include <iterator> // IWYU pragma: keep (std::rbegin)

template<typename It>
struct range
{
    It begin_;
    It end_;

    It begin() const { return begin_; }
    It end() const { return end_; }
};

template<typename It>
range<It> make_range(It begin, It end)
{
    return range<It>{begin, end};
}

template<typename T>
auto make_reverse_range(T &&container)
{
    return make_range(std::rbegin(container), std::rend(container));
}

#endif // MMAPPER_RANGE_H
