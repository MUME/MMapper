#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <QString>
#include <QtCore>

#include "../expandoracommon/parseevent.h"
#include "pathmachine.h"

class Configuration;
class MapData;
class ParseEvent;
class QObject;
class QElapsedTimer;

/**
@author alve,,,
*/
class Mmapper2PathMachine final : public PathMachine
{
    Q_OBJECT

private:
    QElapsedTimer time;

public:
    explicit Mmapper2PathMachine(MapData *mapData, QObject *parent);

public slots:
    void slot_handleParseEvent(const SigParseEvent &);

signals:
    void sig_log(const QString &, const QString &);
};
