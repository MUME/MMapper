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
#include "configuration.h"

PathmachinePage::PathmachinePage(QWidget *parent)
        : QWidget(parent)
{
    setupUi(this);

    connect ( acceptBestRelativeDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(acceptBestRelativeDoubleSpinBoxValueChanged(double)) );
    connect ( acceptBestAbsoluteDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(acceptBestAbsoluteDoubleSpinBoxValueChanged(double)) );
    connect ( newRoomPenaltyDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(newRoomPenaltyDoubleSpinBoxValueChanged(double)) );
    connect ( correctPositionBonusDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(correctPositionBonusDoubleSpinBoxValueChanged(double)) );
    connect ( multipleConnectionsPenaltyDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(multipleConnectionsPenaltyDoubleSpinBoxValueChanged(double)) );

    connect ( maxPaths, SIGNAL(valueChanged(int)), this, SLOT(maxPathsValueChanged(int)) );
    connect ( matchingToleranceSpinBox, SIGNAL(valueChanged(int)), this, SLOT(matchingToleranceSpinBoxValueChanged(int)) );

    acceptBestRelativeDoubleSpinBox->setValue(Config().m_acceptBestRelative);
    acceptBestAbsoluteDoubleSpinBox->setValue(Config().m_acceptBestAbsolute);
    newRoomPenaltyDoubleSpinBox->setValue(Config().m_newRoomPenalty);
    correctPositionBonusDoubleSpinBox->setValue(Config().m_correctPositionBonus);
    maxPaths->setValue(Config().m_maxPaths);
    matchingToleranceSpinBox->setValue(Config().m_matchingTolerance);
    multipleConnectionsPenaltyDoubleSpinBox->setValue(Config().m_multipleConnectionsPenalty);
}

void PathmachinePage::acceptBestRelativeDoubleSpinBoxValueChanged(double val)
{
  Config().m_acceptBestRelative = val;
}

void PathmachinePage::acceptBestAbsoluteDoubleSpinBoxValueChanged(double val)
{
  Config().m_acceptBestAbsolute = val;
}

void PathmachinePage::newRoomPenaltyDoubleSpinBoxValueChanged(double val)
{
  Config().m_newRoomPenalty = val;
}

void PathmachinePage::correctPositionBonusDoubleSpinBoxValueChanged(double val)
{
  Config().m_correctPositionBonus = val;
}

void PathmachinePage::multipleConnectionsPenaltyDoubleSpinBoxValueChanged(double val)
{
  Config().m_multipleConnectionsPenalty = val;
}

void PathmachinePage::maxPathsValueChanged(int val)
{
  Config().m_maxPaths = val;
}

void PathmachinePage::matchingToleranceSpinBoxValueChanged(int val)
{
  Config().m_matchingTolerance = val;
}

