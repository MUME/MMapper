#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <functional>
#include <memory>
#include <QOpenGLTexture>
#include <QString>
#include <QtGui/qopengl.h>

#include "../global/EnumIndexedArray.h"
#include "../mapdata/mmapper2room.h"
#include "../opengl/OpenGLTypes.h"
#include "RoadIndex.h"

// currently forward declared in OpenGLTypes.h
// so it can define SharedMMTexture
class NODISCARD MMTexture final : public std::enable_shared_from_this<MMTexture>
{
private:
    struct NODISCARD this_is_private final
    {
        explicit this_is_private(int) {}
    };

private:
    // REVISIT: can we store the actual QOpenGLTexture in this object?
    QOpenGLTexture m_qt_texture;
    int m_priority = -1;
    bool m_forbidUpdates = false;

public:
    NODISCARD static std::shared_ptr<MMTexture> alloc(const QString &name)
    {
        return std::make_shared<MMTexture>(this_is_private{0}, name);
    }
    NODISCARD static std::shared_ptr<MMTexture> alloc(
        const QOpenGLTexture::Target target,
        const std::function<void(QOpenGLTexture &)> &init,
        const bool forbidUpdates)
    {
        return std::make_shared<MMTexture>(this_is_private{0}, target, init, forbidUpdates);
    }

public:
    MMTexture() = delete;
    MMTexture(this_is_private, const QString &name);
    MMTexture(this_is_private,
              const QOpenGLTexture::Target target,
              const std::function<void(QOpenGLTexture &)> &init,
              const bool forbidUpdates)
        : m_qt_texture{target}
        , m_forbidUpdates{forbidUpdates}
    {
        init(m_qt_texture);
    }
    DELETE_CTORS_AND_ASSIGN_OPS(MMTexture);

public:
    NODISCARD QOpenGLTexture *get() { return &m_qt_texture; }
    NODISCARD const QOpenGLTexture *get() const { return &m_qt_texture; }
    QOpenGLTexture *operator->() { return get(); }

    void bind() { get()->bind(); }
    void bind(GLuint x) { get()->bind(x); }
    void release(GLuint x) { get()->release(x); }
    NODISCARD GLuint textureId() const { return get()->textureId(); }
    NODISCARD auto target() const { return get()->target(); }
    NODISCARD bool canBeUpdated() const { return !m_forbidUpdates; }

    NODISCARD SharedMMTexture getShared() { return shared_from_this(); }
    NODISCARD MMTexture *getRaw() { return this; }

    NODISCARD int getPriority() const { return m_priority; }
    void setPriority(const int priority) { m_priority = priority; }
};

template<typename E>
using texture_array = EnumIndexedArray<SharedMMTexture, E>;

template<RoadTagEnum Tag>
struct NODISCARD road_texture_array : private texture_array<RoadIndexMaskEnum>
{
    using base = texture_array<RoadIndexMaskEnum>;
    decltype(auto) operator[](TaggedRoadIndex<Tag> x) { return base::operator[](x.index); }
    using base::operator[];
    using base::begin;
    using base::end;
    using base::for_each;
    using base::size;
};

struct NODISCARD MapCanvasTextures final
{
    texture_array<RoomTerrainEnum> terrain;
    road_texture_array<RoadTagEnum::ROAD> road;
    road_texture_array<RoadTagEnum::TRAIL> trail;
    texture_array<RoomMobFlagEnum> mob;
    texture_array<RoomLoadFlagEnum> load;
    EnumIndexedArray<SharedMMTexture, ExitDirEnum, NUM_EXITS_NESW> wall;
    EnumIndexedArray<SharedMMTexture, ExitDirEnum, NUM_EXITS_NESW> dotted_wall;
    EnumIndexedArray<SharedMMTexture, ExitDirEnum, NUM_EXITS_NESWUD> stream_in;
    EnumIndexedArray<SharedMMTexture, ExitDirEnum, NUM_EXITS_NESWUD> stream_out;
    EnumIndexedArray<SharedMMTexture, ExitDirEnum, NUM_EXITS_NESWUD> door;
    SharedMMTexture char_arrows;
    SharedMMTexture char_room_sel;
    SharedMMTexture exit_climb_down;
    SharedMMTexture exit_climb_up;
    SharedMMTexture exit_down;
    SharedMMTexture exit_up;
    SharedMMTexture no_ride;
    SharedMMTexture room_sel;
    SharedMMTexture room_sel_distant;
    SharedMMTexture room_sel_move_bad;
    SharedMMTexture room_sel_move_good;
    SharedMMTexture update;

    template<typename Callback>
    void for_each(Callback &&callback)
    {
        terrain.for_each(callback);
        road.for_each(callback);
        trail.for_each(callback);
        mob.for_each(callback);
        load.for_each(callback);
        wall.for_each(callback);
        dotted_wall.for_each(callback);
        door.for_each(callback);
        stream_in.for_each(callback);
        stream_out.for_each(callback);
        callback(char_arrows);
        callback(char_room_sel);
        callback(exit_climb_down);
        callback(exit_climb_up);
        callback(exit_down);
        callback(exit_up);
        callback(no_ride);
        callback(room_sel);
        callback(room_sel_distant);
        callback(room_sel_move_bad);
        callback(room_sel_move_good);
        callback(update);
    }

    void destroyAll();
};
