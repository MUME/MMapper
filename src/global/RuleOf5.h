#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
