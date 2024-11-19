// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Consts.h"

static_assert(char_consts::C_BACKSPACE == 'H' - 'A' + 1);
static_assert(char_consts::C_CTRL_U == 'U' - 'A' + 1);
static_assert(char_consts::C_CTRL_W == 'W' - 'A' + 1);
static_assert(char_consts::C_SPACE == 0x20);
static_assert(char_consts::C_NBSP == static_cast<char>(0xA0));

static_assert(string_consts::SV_SPACE.size() == 1);
static_assert(string_consts::SV_TWO_SPACES.size() == 2);
static_assert(string_consts::SV_TWO_SPACES.front() == char_consts::C_SPACE);
static_assert(string_consts::SV_TWO_SPACES.back() == char_consts::C_SPACE);

static_assert(string_consts::SV_CRLF.size() == 2);
static_assert(string_consts::SV_CRLF.front() == char_consts::C_CARRIAGE_RETURN);
static_assert(string_consts::SV_CRLF.back() == char_consts::C_NEWLINE);
