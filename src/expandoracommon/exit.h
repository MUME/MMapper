/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef ABSTRACTEXIT_H
#define ABSTRACTEXIT_H

#include <cassert>
#include <set>
#include <QVector>
#include <QVariant>

#include "global/range.h"

class Exit : public QVector<QVariant>
{

protected:
    std::set<uint> incoming;
    std::set<uint> outgoing;

public:
    Exit(uint numProps = 0) : QVector<QVariant>(static_cast<int>(numProps)) {}

public:
    auto inSize() const
    {
        return incoming.size();
    }
    bool inIsEmpty() const
    {
        return inSize() == 0;
    }
    bool inIsUnique() const
    {
        return inSize() == 1;
    }
    uint inFirst() const
    {
        assert(!inIsEmpty());
        return *incoming.begin();
    }
    auto inRange() const
    {
        return make_range(inBegin(), inEnd());
    }

public:
    auto outSize() const
    {
        return outgoing.size();
    }
    bool outIsEmpty() const
    {
        return outSize() == 0;
    }
    bool outIsUnique() const
    {
        return outSize() == 1;
    }
    uint outFirst() const
    {
        assert(!outIsEmpty());
        return *outgoing.begin();
    }
    auto outRange() const
    {
        return make_range(outBegin(), outEnd());
    }

public:
    auto getRange(bool out) const
    {
        return out ? outRange() : inRange();
    }

private:
    std::set<uint>::const_iterator inBegin() const
    {
        return incoming.begin();
    }
    std::set<uint>::const_iterator outBegin() const
    {
        return outgoing.begin();
    }

    std::set<uint>::const_iterator inEnd() const
    {
        return incoming.end();
    }
    std::set<uint>::const_iterator outEnd() const
    {
        return outgoing.end();
    }

public:
    void addIn(uint from)
    {
        incoming.insert(from);
    }
    void addOut(uint to)
    {
        outgoing.insert(to);
    }
    void removeIn(uint from)
    {
        incoming.erase(from);
    }
    void removeOut(uint to)
    {
        outgoing.erase(to);
    }
    bool containsIn(uint from) const
    {
        return incoming.find(from) != incoming.end();
    }
    bool containsOut(uint to) const
    {
        return outgoing.find(to) != outgoing.end();
    }
    void removeAll()
    {
        incoming.clear();
        outgoing.clear();
    }
};

#endif
