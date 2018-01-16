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

#include <assert.h>

/**
 * @brief AddCharacter::AddCharacter
 * @param blob of XML representing the character
 */
AddCharacter::AddCharacter(QDomNode blob) :
    GroupAction(),
    blob(blob)
{
}

void AddCharacter::exec()
{
    assert(group);
    group->addChar(blob);
}

/**
 * @brief RemoveCharacter::RemoveCharacter
 * @param blob of XML representing which character to delete
 */
RemoveCharacter::RemoveCharacter(QDomNode blob) :
    RemoveCharacter(CGroupChar::getNameFromXML(blob))
{
}

/**
 * @brief RemoveCharacter::RemoveCharacter
 * @param name of the character to delete
 */
RemoveCharacter::RemoveCharacter(const QByteArray &name) :
    GroupAction(),
    name(name)
{
}

void RemoveCharacter::exec()
{
    assert(group);
    group->removeChar(name);
}

/**
 * @brief UpdateCharacter::UpdateCharacter
 * @param blob of XML with which to update the character with
 */
UpdateCharacter::UpdateCharacter(QDomNode blob) :
    GroupAction(),
    blob(blob)
{
}

void UpdateCharacter::exec()
{
    assert(group);
    group->updateChar(blob);
}

/**
 * @brief RenameCharacter::RenameCharacter
 * @param blob of XML with the new name of the character
 */
RenameCharacter::RenameCharacter(QDomNode blob) :
    GroupAction(),
    blob(blob)
{
}

void RenameCharacter::exec()
{
    assert(group);
    group->renameChar(blob);
}

/**
 * @brief ResetCharacters::ResetCharacters
 */
ResetCharacters::ResetCharacters() :
    GroupAction()
{
}

void ResetCharacters::exec()
{
    assert(group);
    group->resetChars();
}
