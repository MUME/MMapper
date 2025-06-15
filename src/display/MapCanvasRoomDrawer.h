#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Array.h"
#include "../global/Timer.h"
#include "../map/ExitDirection.h"
#include "../map/coordinate.h"
#include "../map/infomark.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../opengl/Font.h"
#include "Connections.h"
#include "IMapBatchesFinisher.h"
#include "Infomarks.h"
#include "MapBatches.h"
#include "MapCanvasData.h"
#include "RoadIndex.h"

#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QColor>
#include <QtCore>

class OpenGL;
class QOpenGLTexture;
struct MapCanvasTextures;

using RoomVector = std::vector<RoomHandle>;
using LayerToRooms = std::map<int, RoomVector>;

struct NODISCARD RemeshCookie final
{
private:
    std::optional<FutureSharedMapBatchFinisher> m_opt_future;
    bool m_ignored = false;

private:
    // NOTE: If you think you want to make this public, then you're really looking for setIgnored().
    void reset() { *this = RemeshCookie{}; }

public:
    // This means you've decided to load a completely new map, so you're not interested
    // in the results. It SHOULD NOT be used just because you got another mesh request.
    void setIgnored()
    {
        //
        m_ignored = true;
    }

public:
    NODISCARD bool isPending() const { return m_opt_future.has_value(); }

    // Don't call this unless isPending() is true.
    // returns true if get() will return without blocking
    NODISCARD bool isReady() const
    {
        const FutureSharedMapBatchFinisher &future = m_opt_future.value();
        return future.valid()
               && future.wait_for(std::chrono::nanoseconds(0)) != std::future_status::timeout;
    }

private:
    static void reportException()
    {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception &ex) {
            qWarning() << "Exception: " << ex.what();
        } catch (...) {
            qWarning() << "Unknown exception";
        }
    }

public:
    // Don't call this unless isPending() is true.
    // NOTE: This can throw an exception thrown by the async function!
    NODISCARD SharedMapBatchFinisher get()
    {
        DECL_TIMER(t, __FUNCTION__);
        FutureSharedMapBatchFinisher &future = m_opt_future.value();

        SharedMapBatchFinisher pFinisher;
        try {
            pFinisher = future.get();
        } catch (...) {
            reportException();
            pFinisher.reset();
        }

        if (m_ignored) {
            pFinisher.reset();
        }

        reset();
        return pFinisher;
    }

public:
    // Don't call this if isPending() is true, unless you called set_ignored().
    void set(FutureSharedMapBatchFinisher future)
    {
        if (m_opt_future && !m_ignored) {
            // If this happens, you should wait until the old one is finished first,
            // or we're going to end up with tons of in-flight future meshes.
            throw std::runtime_error("replaced existing future");
        }

        m_opt_future.emplace(std::move(future));
        m_ignored = false;
    }
};

struct NODISCARD Batches final
{
    RemeshCookie remeshCookie;
    std::optional<MapBatches> next_mapBatches;
    std::optional<MapBatches> mapBatches;
    std::optional<BatchedInfomarksMeshes> infomarksMeshes;
    struct NODISCARD FlashState final
    {
    private:
        std::chrono::steady_clock::time_point lastChange = getTime();
        bool on = false;

        static std::chrono::steady_clock::time_point getTime()
        {
            return std::chrono::steady_clock::now();
        }

    public:
        NODISCARD bool tick()
        {
            const auto flash_interval = std::chrono::milliseconds(100);
            const auto now = getTime();
            if (now >= lastChange + flash_interval) {
                lastChange = now;
                on = !on;
            }
            return on;
        }
    };
    FlashState pendingUpdateFlashState;

    Batches() = default;
    ~Batches() = default;
    DEFAULT_MOVES_DELETE_COPIES(Batches);

    NODISCARD bool isInProgress() const;

    void resetExistingMeshesButKeepPendingRemesh()
    {
        mapBatches.reset();
        infomarksMeshes.reset();
    }

    void ignorePendingRemesh()
    {
        //
        remeshCookie.setIgnored();
    }

    void resetExistingMeshesAndIgnorePendingRemesh()
    {
        resetExistingMeshesButKeepPendingRemesh();
        ignorePendingRemesh();
    }
};

NODISCARD FutureSharedMapBatchFinisher
generateMapDataFinisher(const mctp::MapCanvasTexturesProxy &textures,
                        const std::shared_ptr<const FontMetrics> &font,
                        const Map &map);

extern void finish(const IMapBatchesFinisher &finisher,
                   std::optional<MapBatches> &batches,
                   OpenGL &gl,
                   GLFont &font);
