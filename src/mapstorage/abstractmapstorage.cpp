/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "abstractmapstorage.h"

#include <stdexcept>
#include <utility>
#include <QObject>
#include <QString>

#include "../global/NullPointerException.h"
#include "progresscounter.h"

AbstractMapStorage::AbstractMapStorage(MapData &mapdata,
                                       QString filename,
                                       QFile *const file,
                                       QObject *const parent)
    : QObject(parent)
    , m_file(file)
    , m_mapData(mapdata)
    , m_fileName(std::move(filename))
    , m_progressCounter(new ProgressCounter(this))
{}

AbstractMapStorage::AbstractMapStorage(MapData &mapdata, QString filename, QObject *const parent)
    : QObject(parent)
    , m_mapData(mapdata)
    , m_fileName(std::move(filename))
    , m_progressCounter(new ProgressCounter(this))
{}

AbstractMapStorage::~AbstractMapStorage() = default;

ProgressCounter &AbstractMapStorage::getProgressCounter() const
{
    // This will always exist, so it should be safe to just dereference it,
    // but let's not tempt UB.
    if (auto *const pc = m_progressCounter.get())
        return *pc;
    throw NullPointerException();
}
