#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <set>
#include <stack>
#include <vector>
#include <QCharRef>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#include "../expandoracommon/parseevent.h"
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/RoomFieldVariant.h"
#include "../mapdata/mmapper2exit.h"
#include "../mapdata/mmapper2room.h"

class AbstractRoomFactory;
class Map;
class MapFrontend;
class ParseTree;
class Room;
class RoomCollection;

class FrontendAccessor
{
public:
    virtual ~FrontendAccessor() = default;

    virtual void setFrontend(MapFrontend *in);

protected:
    MapFrontend *m_frontend = nullptr;

    Map &map();

    ParseTree &getParseTree();

    RoomIndex &roomIndex();
    Room *roomIndex(RoomId) const;

    RoomHomes &roomHomes();
    const SharedRoomCollection &roomHomes(RoomId) const;

    std::stack<RoomId> &unusedIds();

    AbstractRoomFactory *factory();
};

class AbstractAction : public virtual FrontendAccessor
{
public:
    virtual ~AbstractAction() override;
    virtual void preExec(RoomId) {}

    virtual void exec(RoomId id) = 0;

    virtual void insertAffected(RoomId id, std::set<RoomId> &affected) { affected.insert(id); }
};

class MapAction
{
    friend class MapFrontend;

public:
    virtual void schedule(MapFrontend *in) = 0;

    virtual ~MapAction();

protected:
    virtual void exec() = 0;

    virtual const std::set<RoomId> &getAffectedRooms() { return affectedRooms; }

    std::set<RoomId> affectedRooms{};
};

class SingleRoomAction final : public MapAction
{
public:
    explicit SingleRoomAction(AbstractAction *ex, RoomId id);

    void schedule(MapFrontend *in) override { executor->setFrontend(in); }

    virtual ~SingleRoomAction() override { delete executor; }

protected:
    virtual void exec() override
    {
        executor->preExec(id);
        executor->exec(id);
    }

    virtual const RoomIdSet &getAffectedRooms() override;

private:
    RoomId id = INVALID_ROOMID;
    AbstractAction *executor = nullptr;
};

class AddExit : public MapAction, public FrontendAccessor
{
public:
    void schedule(MapFrontend *in) override { setFrontend(in); }

    explicit AddExit(RoomId in_from, RoomId in_to, ExitDirection in_dir);

protected:
    virtual void exec() override;

    Room *tryExec();

    RoomId from;
    RoomId to;
    ExitDirection dir = ExitDirection::UNKNOWN;
};

class RemoveExit : public MapAction, public FrontendAccessor
{
public:
    explicit RemoveExit(RoomId from, RoomId to, ExitDirection dir);

    void schedule(MapFrontend *in) override { setFrontend(in); }

protected:
    virtual void exec() override;

    Room *tryExec();

    RoomId from = DEFAULT_ROOMID;
    RoomId to = DEFAULT_ROOMID;
    ExitDirection dir = ExitDirection::UNKNOWN;
};

class MakePermanent final : public AbstractAction
{
public:
    virtual void exec(RoomId RoomId) override;
};

class UpdateRoomField final : public virtual AbstractAction
{
public:
    explicit UpdateRoomField(const RoomFieldVariant &var);
#define X_DECLARE_CONSTRUCTORS(UPPER_CASE, CamelCase, Type) explicit UpdateRoomField(Type type);
    X_FOREACH_ROOM_FIELD(X_DECLARE_CONSTRUCTORS)
#undef X_DECLARE_CONSTRUCTORS

public:
    virtual void exec(RoomId id) override;

protected:
    RoomFieldVariant var;
};

class Update final : public virtual AbstractAction
{
public:
    explicit Update();

    explicit Update(const SigParseEvent &sigParseEvent);

    virtual void exec(RoomId id) override;

protected:
    ParseEvent props;
};

class ExitsAffecter : public AbstractAction
{
public:
    virtual void insertAffected(RoomId id, std::set<RoomId> &affected) override;
};

class Remove : public ExitsAffecter
{
protected:
    virtual void exec(RoomId id) override;
};
