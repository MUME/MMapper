// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "groupaction.h"

#include "CGroup.h"
#include "CGroupChar.h"

#include <cassert>
#include <utility>

GroupAction::GroupAction() = default;
GroupAction::~GroupAction() = default;

/**
 * @brief AddCharacter::AddCharacter
 * @param Variant map representing the character
 */
AddCharacter::AddCharacter(const QVariantMap &map)
    : map(map)
{}

void AddCharacter::virt_exec()
{
    assert(group);
    group->addChar(map);
}

/**
 * @brief RemoveCharacter::RemoveCharacter
 * @param Variant map representing which character to delete
 */
RemoveCharacter::RemoveCharacter(const QVariantMap &map)
    : RemoveCharacter(CGroupChar::getNameFromUpdateChar(map))
{}

/**
 * @brief RemoveCharacter::RemoveCharacter
 * @param name of the character to delete
 */
RemoveCharacter::RemoveCharacter(QByteArray name)
    : name(std::move(name))
{}

void RemoveCharacter::virt_exec()
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

void UpdateCharacter::virt_exec()
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

void RenameCharacter::virt_exec()
{
    assert(group);
    group->renameChar(map);
}

/**
 * @brief ResetCharacters::ResetCharacters
 */
ResetCharacters::ResetCharacters() = default;

void ResetCharacters::virt_exec()
{
    assert(group);
    group->resetChars();
}
