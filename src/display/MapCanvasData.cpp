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
    return MouseSel{vec2f{v.x(), v.y()}, m_currentLayer};
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
