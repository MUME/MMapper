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

#include "mmapper2event.h"
#include "parseevent.h"
#include "property.h"

#define EV_NAME 0
#define EV_DESC 1
#define EV_PDESC 2
#define EV_EXITS 3
#define EV_PROMPT 4
#define EV_CROOM 5

namespace Mmapper2Event {

ParseEvent *createEvent(const CommandIdType &c,
                        const QString &roomName,
                        const QString &dynamicDesc,
                        const QString &staticDesc,
                        const ExitsFlagsType &exitFlags,
                        const PromptFlagsType &promptFlags,
                        const ConnectedRoomFlagsType &connectedRoomFlags)
{
    auto *event = new ParseEvent(c);
    std::deque<QVariant> &optional = event->getOptional();

    if (roomName.isNull()) {
        event->push_back(new SkipProperty());
    } else {
        event->push_back(new Property(roomName.toLatin1()));
    }
    optional.emplace_back(roomName);
    optional.emplace_back(dynamicDesc);

    if (staticDesc.isNull()) {
        event->push_back(new SkipProperty());
    } else {
        event->push_back(new Property(staticDesc.toLatin1()));
    }
    optional.emplace_back(staticDesc);
    optional.emplace_back(static_cast<uint>(exitFlags));

    if ((promptFlags & PROMPT_FLAGS_VALID) != 0) {
        char terrain = (promptFlags & TERRAIN_TYPE);
        event->push_back(new Property(QByteArray(1, terrain)));
    } else {
        event->push_back(new SkipProperty());
    }
    optional.emplace_back(static_cast<uint>(promptFlags));
    optional.emplace_back(static_cast<uint>(connectedRoomFlags));
    event->countSkipped();
    return event;
}

QString getRoomName(const ParseEvent &e)
{
    return e.getOptional()[EV_NAME].toString();
}

QString getRoomDesc(const ParseEvent &e)
{
    return e.getOptional()[EV_DESC].toString();
}

QString getParsedRoomDesc(const ParseEvent &e)
{
    return e.getOptional()[EV_PDESC].toString();
}

ExitsFlagsType getExitFlags(const ParseEvent &e)
{
    return e.getOptional()[EV_EXITS].toUInt();
}

PromptFlagsType getPromptFlags(const ParseEvent &e)
{
    return e.getOptional()[EV_PROMPT].toUInt();
}

ConnectedRoomFlagsType getConnectedRoomFlags(const ParseEvent &e)
{
    return e.getOptional()[EV_CROOM].toUInt();
}

} // namespace Mmapper2Event
