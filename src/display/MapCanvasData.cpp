// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "MapCanvasData.h"

#include <QPointF>

MapCanvasData::MapCanvasData(MapData *const mapData,
                             PrespammedPath *const prespammedPath,
                             QWidget &sizeWidget)
    : m_sizeWidget(sizeWidget)
    , m_data(mapData)
    , m_prespammedPath(prespammedPath)

{}

MapCanvasData::~MapCanvasData()
{
    delete std::exchange(m_connectionSelection, nullptr);
}

QVector3D MapCanvasData::project(const QVector3D &v) const
{
    return v.project(m_modelview, m_projection, this->rect());
}

QVector3D MapCanvasData::unproject(const QVector3D &v) const
{
    return v.unproject(m_modelview, m_projection, this->rect());
}

QVector3D MapCanvasData::unproject(const QMouseEvent *const event) const
{
    const auto x = static_cast<float>(event->pos().x());
    const auto y = static_cast<float>(height() - event->pos().y());
    return unproject(QVector3D{x, y, CAMERA_Z_DISTANCE});
}

MouseSel MapCanvasData::getUnprojectedMouseSel(const QMouseEvent *const event) const
{
    const QVector3D v = unproject(event);
    return MouseSel{Coordinate2f{v.x(), v.y()}, m_currentLayer};
}

void MapCanvasData::destroyAllGLObjects()
{
    m_textures.destroyAll();
    m_gllist.destroyAll();
}

void MapCanvasData::Textures::destroyAll()
{
    terrain.destroyAll();
    load.destroyAll();
    mob.destroyAll();
    trail.destroyAll();
    road.destroyAll();
    update.reset();
}
