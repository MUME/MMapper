#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/Badge.h"
#include "../global/EnumIndexedArray.h"
#include "../map/mmapper2room.h"
#include "../opengl/OpenGLTypes.h"
#include "RoadIndex.h"

#include <functional>
#include <memory>

#include <QOpenGLTexture>
#include <QString>
#include <QtGui/qopengl.h>

// currently forward declared in OpenGLTypes.h
// so it can define SharedMMTexture
class NODISCARD MMTexture final : public std::enable_shared_from_this<MMTexture>
{
private:
    QOpenGLTexture m_qt_texture;
    MMTextureId m_id = INVALID_MM_TEXTURE_ID;
    bool m_forbidUpdates = false;

public:
    NODISCARD static std::shared_ptr<MMTexture> alloc(const QString &name)
    {
        return std::make_shared<MMTexture>(Badge<MMTexture>{}, name);
    }
    NODISCARD static std::shared_ptr<MMTexture> alloc(
        const QOpenGLTexture::Target target,
        const std::function<void(QOpenGLTexture &)> &init,
        const bool forbidUpdates)
    {
        return std::make_shared<MMTexture>(Badge<MMTexture>{}, target, init, forbidUpdates);
    }

public:
    MMTexture() = delete;
    MMTexture(Badge<MMTexture>, const QString &name);
    MMTexture(Badge<MMTexture>,
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
    NODISCARD QOpenGLTexture::Target target() const { return get()->target(); }
    NODISCARD bool canBeUpdated() const { return !m_forbidUpdates; }

    NODISCARD SharedMMTexture getShared() { return shared_from_this(); }
    NODISCARD MMTexture *getRaw() { return this; }

    NODISCARD MMTextureId getId() const
    {
        assert(m_id != INVALID_MM_TEXTURE_ID);
        return m_id;
    }

    // only called by MapCanvas::initTextures() and GLFont::init();
    // don't forget to call OpenGL::setTextureLookup(), too.
    void setId(const MMTextureId id)
    {
        assert(m_id == INVALID_MM_TEXTURE_ID);
        m_id = id;
    }

    void clearId()
    {
        assert(m_id != INVALID_MM_TEXTURE_ID);
        m_id = INVALID_MM_TEXTURE_ID;
    }
};

template<typename E>
using texture_array = EnumIndexedArray<SharedMMTexture, E>;

template<RoadTagEnum Tag>
struct NODISCARD road_texture_array : private texture_array<RoadIndexMaskEnum>
{
    using base = texture_array<RoadIndexMaskEnum>;
    using base::index_type;
    using base::SIZE;
    using base::value_type;
    decltype(auto) operator[](TaggedRoadIndex<Tag> x) { return base::operator[](x.index); }
    using base::operator[];
    using base::begin;
    using base::end;
    using base::for_each;
    using base::size;
};

using TextureArrayNESW = EnumIndexedArray<SharedMMTexture, ExitDirEnum, NUM_EXITS_NESW>;
using TextureArrayNESWUD = EnumIndexedArray<SharedMMTexture, ExitDirEnum, NUM_EXITS_NESWUD>;

// X(_Type, _Name)
#define XFOREACH_MAPCANVAS_TEXTURES(X) \
    X(texture_array<RoomTerrainEnum>, terrain) \
    X(road_texture_array<RoadTagEnum::ROAD>, road) \
    X(road_texture_array<RoadTagEnum::TRAIL>, trail) \
    X(texture_array<RoomMobFlagEnum>, mob) \
    X(texture_array<RoomLoadFlagEnum>, load) \
    X(TextureArrayNESW, wall) \
    X(TextureArrayNESW, dotted_wall) \
    X(TextureArrayNESWUD, stream_in) \
    X(TextureArrayNESWUD, stream_out) \
    X(TextureArrayNESWUD, door) \
    X(SharedMMTexture, char_arrows) \
    X(SharedMMTexture, char_room_sel) \
    X(SharedMMTexture, exit_climb_down) \
    X(SharedMMTexture, exit_climb_up) \
    X(SharedMMTexture, exit_down) \
    X(SharedMMTexture, exit_up) \
    X(SharedMMTexture, no_ride) \
    X(SharedMMTexture, room_sel) \
    X(SharedMMTexture, room_sel_distant) \
    X(SharedMMTexture, room_sel_move_bad) \
    X(SharedMMTexture, room_sel_move_good) \
    X(SharedMMTexture, room_highlight)

struct NODISCARD MapCanvasTextures final
{
#define X_DECL(_Type, _Name) _Type _Name;
    XFOREACH_MAPCANVAS_TEXTURES(X_DECL)
#undef X_DECL

private:
    template<typename Callback>
    static void apply_callback(SharedMMTexture &tex, Callback &&callback)
    {
        callback(tex);
    }

    template<typename T, typename Callback>
    static void apply_callback(T &x, Callback &&callback)
    {
        x.for_each(callback);
    }

public:
    template<typename Callback>
    void for_each(Callback &&callback)
    {
#define X_EACH(_Type, _Name) apply_callback(_Name, callback);
        XFOREACH_MAPCANVAS_TEXTURES(X_EACH)
#undef X_EACH
    }

    void destroyAll();
};

namespace mctp {

namespace detail {
// converts from EnumIndexedArray<SharedMMTexture, ...> to EnumIndexedArray<MMTextureId, ...>
template<typename T>
auto typeHack(const T &)
    -> std::enable_if_t<std::is_same_v<typename T::value_type, SharedMMTexture>,
                        EnumIndexedArray<MMTextureId, typename T::index_type, T::SIZE>>;
template<typename T>
struct NODISCARD Proxy
{
    using type = decltype(typeHack(std::declval<T>()));
};

template<>
struct NODISCARD Proxy<SharedMMTexture>
{
    using type = MMTextureId;
};

template<typename T>
using proxy_t = typename Proxy<T>::type;
} // namespace detail

struct NODISCARD MapCanvasTexturesProxy final
{
#define X_DECL_PROXY(_Type, _Name) mctp::detail::proxy_t<_Type> _Name;
    XFOREACH_MAPCANVAS_TEXTURES(X_DECL_PROXY)
#undef X_DECL_PROXY
};

NODISCARD extern MapCanvasTexturesProxy getProxy(const MapCanvasTextures &mct);

} // namespace mctp

NODISCARD extern MMTextureId allocateTextureId();
