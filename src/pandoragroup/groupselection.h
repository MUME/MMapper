#pragma once
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

#ifndef GROUPSELECTION_H
#define GROUPSELECTION_H

#include <vector>

class CGroupChar;
class GroupRecipient;

class GroupAdmin
{
public:
    virtual void releaseCharacters(GroupRecipient *) = 0;
    virtual ~GroupAdmin() = default;
};

class GroupRecipient
{
public:
    virtual void receiveCharacters(GroupAdmin *, const std::vector<CGroupChar *>) = 0;
    virtual ~GroupRecipient() = default;
};

class GroupSelection : public GroupRecipient
{
private:
    GroupSelection(GroupSelection &&) = delete;
    GroupSelection(const GroupSelection &) = delete;
    GroupSelection &operator=(GroupSelection &&) = delete;
    GroupSelection &operator=(const GroupSelection &) = delete;

public:
    explicit GroupSelection(GroupAdmin *admin);
    virtual ~GroupSelection() = default;

    void receiveCharacters(GroupAdmin *, std::vector<CGroupChar *>) override;

public:
    auto at(int i) const { return chars.at(i); }
    auto begin() const { return chars.begin(); }
    auto cbegin() const { return chars.cbegin(); }
    auto cend() const { return chars.cend(); }
    auto end() const { return chars.end(); }
    auto size() const { return chars.size(); }

private:
    GroupAdmin *m_admin = nullptr;
    std::vector<CGroupChar *> chars;
};

#endif // GROUPSELECTION_H
