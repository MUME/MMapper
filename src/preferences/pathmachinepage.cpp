/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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
    setConfig().pathMachine.maxPaths = static_cast<uint32_t>(val);
}

void PathmachinePage::matchingToleranceSpinBoxValueChanged(const int val)
{
    setConfig().pathMachine.matchingTolerance = static_cast<uint32_t>(val);
}
