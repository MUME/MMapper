// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
