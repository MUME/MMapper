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
    void loadConfig();
    void acceptBestRelativeDoubleSpinBoxValueChanged(double);
    void acceptBestAbsoluteDoubleSpinBoxValueChanged(double);
    void newRoomPenaltyDoubleSpinBoxValueChanged(double);
    void correctPositionBonusDoubleSpinBoxValueChanged(double);
    void multipleConnectionsPenaltyDoubleSpinBoxValueChanged(double);
    void maxPathsValueChanged(int);
    void matchingToleranceSpinBoxValueChanged(int);
};
