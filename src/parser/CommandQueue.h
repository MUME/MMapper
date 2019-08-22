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

#include <QQueue>

#include "CommandId.h"

class CommandQueue : private QQueue<CommandIdType>
{
private:
    using base = QQueue<CommandIdType>;

public:
    using QQueue<CommandIdType>::QQueue;

public:
    QByteArray toByteArray() const;
    CommandQueue &operator=(const QByteArray &dirs);

public:
    using base::begin;
    using base::end;
    using base::head;
    using base::isEmpty;

public:
    using base::append;
    using base::clear;
    using base::dequeue;
    using base::enqueue;
    using base::prepend;
};
