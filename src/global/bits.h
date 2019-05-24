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

#ifndef MMAPPER_BITS_H
#define MMAPPER_BITS_H

#include <cstdint>

#define BIT32_FOREACH(x) \
    x(1) x(2) x(3) x(4) x(5) x(6) x(7) x(8) x(9) x(10) x(11) x(12) x(13) x(14) x(15) x(16) x(17) \
        x(18) x(19) x(20) x(21) x(22) x(23) x(24) x(25) x(26) x(27) x(28) x(29) x(30) x(31) x(32)

/* TODO: eliminate bitxx constants.
 * They're unnecessary, and they're also confusing because they're 1-based. */
#define DEFINE_BITS(x) static constexpr const uint32_t bit##x = (1u << ((x) -1));
BIT32_FOREACH(DEFINE_BITS)
#define ADD(x) +1
static_assert(BIT32_FOREACH(ADD) == 32);
static_assert(bit1 == 1u);
static_assert(bit32 == 2147483648u);
#undef ADD
#undef DEFINE_BITS

#endif // MMAPPER_BITS_H
