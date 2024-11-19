#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

// These macros only exists to keep clang-format from losing its mind
// when it encounters c++11 attributes.

/* This is an empty macro that might be replaced with "[[discard]]" or similar at some point
 * in the future once the attribute is added to the standard. This placeholder serves to document
 * that the code expects the user will have a valid reason to discard the result (e.g. side-effect
 * causing functions like printf() that return something useless to most callers). */
#define ALLOW_DISCARD

/* This is the default for most functions, but it serves to document cases where exceptions are
 * possible in classes that users might expect to be purely noexcept. The main reasons for making
 * this a macro is to avoiding polluting the results when grepping for noexcept, and to reduce
 * the mental burden of having to read a double-negative. */
#define CAN_THROW noexcept(false)

#define DEPRECATED [[deprecated]]
#define DEPRECATED_MSG(_msg) [[deprecated(_msg)]]
#define FALLTHROUGH [[fallthrough]]

#if __cplusplus >= 202000L
#define IMPLICIT explicit(false)
#else
#define IMPLICIT
#endif

#define MAYBE_UNUSED [[maybe_unused]]
#define NODISCARD [[nodiscard]]
#define NORETURN [[noreturn]]

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define PRETTY_FUNCTION __FUNCSIG__
#else
#define PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif

#if defined(__clang__) && !defined(Q_MOC_RUN)
// This macro allows classes to be defined NODISCARD for the regular compiler
// but omit the attribute because the MOC compiler can't understand it.
#define NODISCARD_QOBJECT NODISCARD
#else
// This macro allows classes to be defined NODISCARD for the regular compiler
// but omit the attribute because the MOC compiler can't understand it.
#define NODISCARD_QOBJECT
#endif
