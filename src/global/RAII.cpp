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

#include "RAII.h"

#include <cassert>
#include <utility>

RAIIBool::RAIIBool(bool &b)
    : ref{b}
{
    assert(!ref);
    ref = true;
}

RAIIBool::RAIIBool(RAIIBool &&rhs)
    : ref{rhs.ref}
    , moved{std::exchange(rhs.moved, true)}
{}

RAIIBool::~RAIIBool()
{
    if (!std::exchange(moved, true))
        ref = false;
}
