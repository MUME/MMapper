#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <QString>
#include <QWidget>
#include <QtCore>

#include "ui_pathmachinepage.h"

class QObject;

class PathmachinePage : public QWidget, private Ui::PathmachinePage
{
    Q_OBJECT

public:
    explicit PathmachinePage(QWidget *parent = nullptr);

public slots:
    void slot_loadConfig();
    void slot_acceptBestRelativeDoubleSpinBoxValueChanged(double);
    void slot_acceptBestAbsoluteDoubleSpinBoxValueChanged(double);
    void slot_newRoomPenaltyDoubleSpinBoxValueChanged(double);
    void slot_correctPositionBonusDoubleSpinBoxValueChanged(double);
    void slot_multipleConnectionsPenaltyDoubleSpinBoxValueChanged(double);
    void slot_maxPathsValueChanged(int);
    void slot_matchingToleranceSpinBoxValueChanged(int);
};
