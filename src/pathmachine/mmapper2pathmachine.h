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
class ParseEvent;
class QObject;
class QTime;

/**
@author alve,,,
*/
class Mmapper2PathMachine final : public PathMachine
{
    Q_OBJECT
public slots:
    void event(const SigParseEvent &) override;

public:
    explicit Mmapper2PathMachine(QObject *parent = nullptr);

signals:
    void log(const QString &, const QString &);

private:
    QTime time;
};
