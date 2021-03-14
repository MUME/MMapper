#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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

class AbstractMapStorage : public QObject
{
    Q_OBJECT

public:
    explicit AbstractMapStorage(MapData &, QString, QFile *, QObject *parent = nullptr);
    explicit AbstractMapStorage(MapData &, QString, QObject *parent = nullptr);
    ~AbstractMapStorage() override;

    virtual bool canLoad() const = 0;
    virtual bool canSave() const = 0;

    virtual void newData() = 0;
    virtual bool loadData() = 0;
    virtual bool mergeData() = 0;
    virtual bool saveData(bool baseMapOnly) = 0;
    ProgressCounter &getProgressCounter() const;

signals:
    void sig_log(const QString &, const QString &);
    void sig_onDataLoaded();
    void sig_onDataSaved();
    void sig_onNewData();

protected:
    QFile *m_file = nullptr;
    MapData &m_mapData;
    QString m_fileName;

private:
    // REVISIT: This could be probably be converted to a regular member,
    // unless there's some reason you can't nest QObjects inside one another.
    //
    // It needs to be private if it's a unique_ptr, but it might as well
    // be public if it's a regular member. If so, rename it to remove the
    // m_ protected/private member prefix, and remove getProgressCounter().
    std::unique_ptr<ProgressCounter> m_progressCounter;
};
