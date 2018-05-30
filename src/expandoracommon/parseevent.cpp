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

#include "parseevent.h"
#include "property.h"

ParseEvent::ParseEvent(const ParseEvent &other)
    : ParseEventBase()
    , optional(other.optional)
    , moveType(other.moveType)
    , numSkipped(other.numSkipped)
{
    for (unsigned int i = 0; i < other.size(); ++i) {
        push_back(new Property(*other[i]));
    }
    pos = other.pos;
}

ParseEvent &ParseEvent::operator=(const ParseEvent &other)
{
    if (&other == this) {
        return *this;
    }
    moveType = other.moveType;
    pos = other.pos;
    optional = other.optional;
    numSkipped = other.numSkipped;
    for (auto &property : *this) {
        delete property;
    }
    for (unsigned int i = 0; i < other.size(); ++i) {
        push_back(new Property(*other[i]));
    }
    return *this;
}

ParseEvent::~ParseEvent()
{
    for (auto &property : *this) {
        delete property;
    }
}

void ParseEvent::reset()
{
    ParseEventBase::reset();
    for (auto &property : *this) {
        property->reset();
    }
}

void ParseEvent::countSkipped()
{
    numSkipped = 0;
    for (auto &property : *this) {
        if (property->isSkipped()) {
            numSkipped++;
        }
    }
}
