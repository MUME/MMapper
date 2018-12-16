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

#ifndef MMAPPER_RULEOF5_H
#define MMAPPER_RULEOF5_H

#define DTOR(T) ~T()
#define MOVE_CTOR(T) T(T &&)
#define COPY_CTOR(T) T(const T &)
#define MOVE_ASSIGN_OP(T) T &operator=(T &&)
#define COPY_ASSIGN_OP(T) T &operator=(const T &)

#define DEFAULT_MOVE_CTOR(T) MOVE_CTOR(T) = default
#define DEFAULT_COPY_CTOR(T) COPY_CTOR(T) = default
#define DEFAULT_MOVE_ASSIGN_OP(T) MOVE_ASSIGN_OP(T) = default
#define DEFAULT_COPY_ASSIGN_OP(T) COPY_ASSIGN_OP(T) = default

#define DEFAULT_CTORS(T) \
    DEFAULT_MOVE_CTOR(T); \
    DEFAULT_COPY_CTOR(T)

#define DEFAULT_ASSIGN_OPS(T) \
    DEFAULT_MOVE_ASSIGN_OP(T); \
    DEFAULT_COPY_ASSIGN_OP(T)

#define DEFAULT_CTORS_AND_ASSIGN_OPS(T) \
    DEFAULT_CTORS(T); \
    DEFAULT_ASSIGN_OPS(T)

#define DEFAULT_RULE_OF_5(T) \
    DTOR(T) = default; \
    DEFAULT_CTORS_AND_ASSIGN_OPS(T)

#define DELETE_MOVE_CTOR(T) MOVE_CTOR(T) = delete
#define DELETE_COPY_CTOR(T) COPY_CTOR(T) = delete
#define DELETE_MOVE_ASSIGN_OP(T) MOVE_ASSIGN_OP(T) = delete
#define DELETE_COPY_ASSIGN_OP(T) COPY_ASSIGN_OP(T) = delete

#define DELETE_CTORS(T) \
    DELETE_MOVE_CTOR(T); \
    DELETE_COPY_CTOR(T)

#define DELETE_ASSIGN_OPS(T) \
    DELETE_MOVE_ASSIGN_OP(T); \
    DELETE_COPY_ASSIGN_OP(T)

#define DELETE_CTORS_AND_ASSIGN_OPS(T) \
    DELETE_CTORS(T); \
    DELETE_ASSIGN_OPS(T)

#define DELETE_RULE_OF_5(T) \
    DTOR(T) = delete; \
    DELETE_CTORS_AND_ASSIGN_OPS(T)

#endif // MMAPPER_RULEOF5_H
