#pragma once
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

#ifndef INFOMARK_H
#define INFOMARK_H

#include "../expandoracommon/coordinate.h"
#include <QDateTime>

static constexpr const auto INFOMARK_SCALE = 100;

using InfoMarkName = QString;
using InfoMarkText = QString;
enum class InfoMarkType { TEXT, LINE, ARROW };
enum class InfoMarkClass {
    GENERIC,
    HERB,
    RIVER,
    PLACE,
    MOB,
    COMMENT,
    ROAD,
    OBJECT,
    ACTION,
    LOCALITY
};

using MarkerTimeStamp = QDateTime;

class InfoMark
{
public:
    explicit InfoMark() = default;
    ~InfoMark() = default;

    const InfoMarkName &getName() const { return m_name; }
    const InfoMarkText &getText() const { return m_text; }
    InfoMarkType getType() const { return m_type; }
    InfoMarkClass getClass() const { return m_class; }
    const Coordinate &getPosition1() const { return m_pos1; }
    const Coordinate &getPosition2() const { return m_pos2; }
    double getRotationAngle() const { return m_rotationAngle; }
    const MarkerTimeStamp &getTimeStamp() const { return m_timeStamp; }

    void setPosition1(const Coordinate &pos) { m_pos1 = pos; }
    void setPosition2(const Coordinate &pos) { m_pos2 = pos; }
    void setRotationAngle(double rotationAngle) { m_rotationAngle = rotationAngle; }
    void setName(InfoMarkName name) { m_name = name; }
    void setText(InfoMarkText text) { m_text = text; }
    void setType(InfoMarkType type) { m_type = type; }
    void setClass(InfoMarkClass markClass) { m_class = markClass; }

    void setTimeStamp(MarkerTimeStamp timeStamp) { m_timeStamp = timeStamp; }

private:
    InfoMarkName m_name{};
    InfoMarkType m_type = InfoMarkType::TEXT;
    InfoMarkText m_text{};
    InfoMarkClass m_class = InfoMarkClass::GENERIC;

    MarkerTimeStamp m_timeStamp{};

    Coordinate m_pos1{};
    Coordinate m_pos2{};
    double m_rotationAngle = 0.0; // in degrees
};

#endif
