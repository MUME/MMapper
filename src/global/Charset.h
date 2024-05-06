#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../configuration/configuration.h" // TODO: move CharacterEncodingEnum somewhere

#include <iosfwd>
#include <string>

void latin1ToUtf8(std::ostream &os, char c);
void latin1ToUtf8(std::ostream &os, const std::string_view sv);
// Converts input string_view sv from latin1 to the specified encoding
// by writing "raw bytes" to the output stream os.
void convertFromLatin1(std::ostream &os,
                       const CharacterEncodingEnum encoding,
                       const std::string_view sv);
