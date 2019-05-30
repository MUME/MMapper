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

#ifndef MMAPPER_RAII_H
#define MMAPPER_RAII_H

#include <algorithm>
#include <functional>

#include "RuleOf5.h"

class [[nodiscard]] RAIIBool final
{
private:
    bool &ref;
    bool moved = false;

public:
    explicit RAIIBool(bool &b);
    ~RAIIBool();

public:
    /* move ctor */
    RAIIBool(RAIIBool && rhs);
    DELETE_COPY_CTOR(RAIIBool);
    DELETE_ASSIGN_OPS(RAIIBool);
};

class [[nodiscard]] RAIICallback final
{
private:
    using Callback = std::function<void()>;
    Callback callback;
    bool moved = false;

public:
    /* move ctor */
    RAIICallback(RAIICallback && rhs);
    DELETE_COPY_CTOR(RAIICallback);
    DELETE_ASSIGN_OPS(RAIICallback);

public:
    explicit RAIICallback(Callback && callback);
    ~RAIICallback() noexcept(false);
};

#endif // MMAPPER_RAII_H
