// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "pathmachinepage.h"

#include <QDoubleSpinBox>

#include "../configuration/configuration.h"

PathmachinePage::PathmachinePage(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    connect(acceptBestRelativeDoubleSpinBox,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(acceptBestRelativeDoubleSpinBoxValueChanged(double)));
    connect(acceptBestAbsoluteDoubleSpinBox,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(acceptBestAbsoluteDoubleSpinBoxValueChanged(double)));
    connect(newRoomPenaltyDoubleSpinBox,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(newRoomPenaltyDoubleSpinBoxValueChanged(double)));
    connect(correctPositionBonusDoubleSpinBox,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(correctPositionBonusDoubleSpinBoxValueChanged(double)));
    connect(multipleConnectionsPenaltyDoubleSpinBox,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(multipleConnectionsPenaltyDoubleSpinBoxValueChanged(double)));

    connect(maxPaths, SIGNAL(valueChanged(int)), this, SLOT(maxPathsValueChanged(int)));
    connect(matchingToleranceSpinBox,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(matchingToleranceSpinBoxValueChanged(int)));
}

void PathmachinePage::loadConfig()
{
    const auto &settings = getConfig().pathMachine;
    acceptBestRelativeDoubleSpinBox->setValue(settings.acceptBestRelative);
    acceptBestAbsoluteDoubleSpinBox->setValue(settings.acceptBestAbsolute);
    newRoomPenaltyDoubleSpinBox->setValue(settings.newRoomPenalty);
    correctPositionBonusDoubleSpinBox->setValue(settings.correctPositionBonus);
    maxPaths->setValue(settings.maxPaths);
    matchingToleranceSpinBox->setValue(settings.matchingTolerance);
    multipleConnectionsPenaltyDoubleSpinBox->setValue(settings.multipleConnectionsPenalty);
}

void PathmachinePage::acceptBestRelativeDoubleSpinBoxValueChanged(const double val)
{
    setConfig().pathMachine.acceptBestRelative = val;
}

void PathmachinePage::acceptBestAbsoluteDoubleSpinBoxValueChanged(const double val)
{
    setConfig().pathMachine.acceptBestAbsolute = val;
}

void PathmachinePage::newRoomPenaltyDoubleSpinBoxValueChanged(const double val)
{
    setConfig().pathMachine.newRoomPenalty = val;
}

void PathmachinePage::correctPositionBonusDoubleSpinBoxValueChanged(const double val)
{
    setConfig().pathMachine.correctPositionBonus = val;
}

void PathmachinePage::multipleConnectionsPenaltyDoubleSpinBoxValueChanged(const double val)
{
    setConfig().pathMachine.multipleConnectionsPenalty = val;
}

void PathmachinePage::maxPathsValueChanged(const int val)
{
    setConfig().pathMachine.maxPaths = val;
}

void PathmachinePage::matchingToleranceSpinBoxValueChanged(const int val)
{
    setConfig().pathMachine.matchingTolerance = val;
}
