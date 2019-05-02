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

#ifndef MMPMAPSTORAGE_H
#define MMPMAPSTORAGE_H

#include <QString>
#include <QtCore>

#include "abstractmapstorage.h"

class MapData;
class QObject;
class QXmlStreamWriter;

/*! \brief MMP export for other clients
 *
 * This saves to a XML file following the MMP Specification defined at:
 * https://wiki.mudlet.org/w/Standards:MMP
 */
class MmpMapStorage : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit MmpMapStorage(MapData &, const QString &, QFile *, QObject *parent = nullptr);
    ~MmpMapStorage() override;

public:
    explicit MmpMapStorage() = delete;

private:
    virtual bool canLoad() const override { return false; }
    virtual bool canSave() const override { return true; }

    virtual void newData() override;
    virtual bool loadData() override;
    virtual bool saveData(bool baseMapOnly) override;
    virtual bool mergeData() override;

private:
    void saveRoom(const Room &room, QXmlStreamWriter &stream);
};

#endif // MMPMAPSTORAGE_H
