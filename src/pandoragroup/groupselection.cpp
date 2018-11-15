/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "groupselection.h"
#include "CGroupChar.h"
#include <cassert>

GroupAdmin::~GroupAdmin() = default;
GroupRecipient::~GroupRecipient() = default;

GroupSelection::GroupSelection(GroupAdmin *admin)
    : m_admin(admin)
{}

GroupSelection::~GroupSelection() = default;

/**
 * @brief CGroupSelection::receiveCharacters
 * @param admin lock administrator
 * @param chars characters to insert
 */
void GroupSelection::receiveCharacters(GroupAdmin *const admin, std::vector<CGroupChar *> in_chars)
{
    assert(admin == m_admin);
    chars = std::move(in_chars);
}
