#pragma once
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

#include <memory>
#include <QObject>
#include <QString>
#include <QtCore>

#include "progresscounter.h"

class InfoMark;
class MapData;
class ProgressCounter;
class QFile;
class Room;
class RoomSaveFilter;

class AbstractMapStorage : public QObject
{
    Q_OBJECT

public:
    explicit AbstractMapStorage(MapData &, QString, QFile *, QObject *parent = nullptr);
    explicit AbstractMapStorage(MapData &, QString, QObject *parent = nullptr);
    ~AbstractMapStorage();

    virtual bool canLoad() const = 0;
    virtual bool canSave() const = 0;

    virtual void newData() = 0;
    virtual bool loadData() = 0;
    virtual bool mergeData() = 0;
    virtual bool saveData(bool baseMapOnly = false) = 0;
    ProgressCounter &getProgressCounter() const;

signals:
    void log(const QString &, const QString &);
    void onDataLoaded();
    void onDataSaved();
    void onNewData();

protected:
    QFile *m_file = nullptr;
    MapData &m_mapData;
    QString m_fileName{};

private:
    // REVISIT: This could be probably be converted to a regular member,
    // unless there's some reason you can't nest QObjects inside one another.
    //
    // It needs to be private if it's a unique_ptr, but it might as well
    // be public if it's a regular member. If so, rename it to remove the
    // m_ protected/private member prefix, and remove getProgressCounter().
    std::unique_ptr<ProgressCounter> m_progressCounter{};
};
