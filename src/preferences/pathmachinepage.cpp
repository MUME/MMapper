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
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &PathmachinePage::slot_acceptBestRelativeDoubleSpinBoxValueChanged);
    connect(acceptBestAbsoluteDoubleSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &PathmachinePage::slot_acceptBestAbsoluteDoubleSpinBoxValueChanged);
    connect(newRoomPenaltyDoubleSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &PathmachinePage::slot_newRoomPenaltyDoubleSpinBoxValueChanged);
    connect(correctPositionBonusDoubleSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &PathmachinePage::slot_correctPositionBonusDoubleSpinBoxValueChanged);
    connect(multipleConnectionsPenaltyDoubleSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &PathmachinePage::slot_multipleConnectionsPenaltyDoubleSpinBoxValueChanged);

    connect(maxPaths,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &PathmachinePage::slot_maxPathsValueChanged);
    connect(matchingToleranceSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &PathmachinePage::slot_matchingToleranceSpinBoxValueChanged);
}

void PathmachinePage::slot_loadConfig()
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

void PathmachinePage::slot_acceptBestRelativeDoubleSpinBoxValueChanged(const double val)
{
    setConfig().pathMachine.acceptBestRelative = val;
}

void PathmachinePage::slot_acceptBestAbsoluteDoubleSpinBoxValueChanged(const double val)
{
    setConfig().pathMachine.acceptBestAbsolute = val;
}

void PathmachinePage::slot_newRoomPenaltyDoubleSpinBoxValueChanged(const double val)
{
    setConfig().pathMachine.newRoomPenalty = val;
}

void PathmachinePage::slot_correctPositionBonusDoubleSpinBoxValueChanged(const double val)
{
    setConfig().pathMachine.correctPositionBonus = val;
}

void PathmachinePage::slot_multipleConnectionsPenaltyDoubleSpinBoxValueChanged(const double val)
{
    setConfig().pathMachine.multipleConnectionsPenalty = val;
}

void PathmachinePage::slot_maxPathsValueChanged(const int val)
{
    setConfig().pathMachine.maxPaths = utils::clampNonNegative(val);
}

void PathmachinePage::slot_matchingToleranceSpinBoxValueChanged(const int val)
{
    setConfig().pathMachine.matchingTolerance = utils::clampNonNegative(val);
}
