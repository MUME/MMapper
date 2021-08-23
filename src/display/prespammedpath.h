#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <QObject>
#include <QString>
#include <QtCore>

#include "../parser/CommandQueue.h"

class MapCanvas;
class MapData;

class PrespammedPath final : public QObject
{
    Q_OBJECT

private:
    CommandQueue m_queue;

public:
    explicit PrespammedPath(QObject *parent);
    ~PrespammedPath() final;

public:
    NODISCARD const CommandQueue &getQueue() const { return m_queue; }

signals:
    void sig_update();

public slots:
    void slot_setPath(CommandQueue);
};
