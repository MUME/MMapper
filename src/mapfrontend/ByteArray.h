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

#ifndef MMAPPER_BYTE_ARRAY_H
#define MMAPPER_BYTE_ARRAY_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class ByteArray : public std::vector<uint8_t>
{
private:
    using base = std::vector<uint8_t>;

public:
    explicit ByteArray() = default;
    explicit ByteArray(const char *s);
    explicit ByteArray(const char *s, size_t len);
    explicit ByteArray(const char *begin, const char *end);
    explicit ByteArray(const base::const_iterator &begin, const base::const_iterator &end);

public:
    /* NOTE: This uses sloppy bounds checking that lets you ignore the end;
     * the function only exists for use with SearchTreeNode. */
    ByteArray skip(size_t count) const;

public:
    void append(const char *string);
    void append(const std::string &string);
};

#endif
