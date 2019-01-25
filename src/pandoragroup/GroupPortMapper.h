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

#ifndef GROUPPORTMAPPER_H
#define GROUPPORTMAPPER_H

#include "../global/RuleOf5.h"

#include <memory>
#include <QByteArray>

class GroupPortMapper
{
public:
    struct Pimpl;

private:
    std::unique_ptr<Pimpl> m_pimpl;

public:
    explicit GroupPortMapper();
    ~GroupPortMapper();
    DELETE_CTORS_AND_ASSIGN_OPS(GroupPortMapper);

    QByteArray tryGetExternalIp();
    bool tryAddPortMapping(const quint16 port);
    bool tryDeletePortMapping(const quint16 port);
};

#endif // GROUPPORTMAPPER_H
