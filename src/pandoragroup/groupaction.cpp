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

#include "groupaction.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include <cassert>
#include <utility>

/**
 * @brief AddCharacter::AddCharacter
 * @param Variant map representing the character
 */
AddCharacter::AddCharacter(const QVariantMap &map)
    : map(map)
{}

void AddCharacter::exec()
{
    assert(group);
    group->addChar(map);
}

/**
 * @brief RemoveCharacter::RemoveCharacter
 * @param Variant map representing which character to delete
 */
RemoveCharacter::RemoveCharacter(const QVariantMap &map)
    : RemoveCharacter(CGroupChar::getNameFromVariantMap(map))
{}

/**
 * @brief RemoveCharacter::RemoveCharacter
 * @param name of the character to delete
 */
RemoveCharacter::RemoveCharacter(QByteArray name)
    : name(std::move(name))
{}

void RemoveCharacter::exec()
{
    assert(group);
    group->removeChar(name);
}

/**
 * @brief UpdateCharacter::UpdateCharacter
 * @param Variant map with which to update the character with
 */
UpdateCharacter::UpdateCharacter(const QVariantMap &map)
    : map(map)
{}

void UpdateCharacter::exec()
{
    assert(group);
    group->updateChar(map);
}

/**
 * @brief RenameCharacter::RenameCharacter
 * @param Variant map with the new name of the character
 */
RenameCharacter::RenameCharacter(const QVariantMap &map)
    : map(map)
{}

void RenameCharacter::exec()
{
    assert(group);
    group->renameChar(map);
}

/**
 * @brief ResetCharacters::ResetCharacters
 */
ResetCharacters::ResetCharacters() = default;

void ResetCharacters::exec()
{
    assert(group);
    group->resetChars();
}
