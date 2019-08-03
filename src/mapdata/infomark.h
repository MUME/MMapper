#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <memory>
#include <QDateTime>

#include "../expandoracommon/coordinate.h"

static constexpr const auto INFOMARK_SCALE = 100;

using InfoMarkName = QString;
using InfoMarkText = QString;
enum class InfoMarkTypeEnum { TEXT, LINE, ARROW };
enum class InfoMarkClassEnum {
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

class InfoMark final : public std::enable_shared_from_this<InfoMark>
{
private:
    struct this_is_private final
    {
        explicit this_is_private(int) {}
    };

public:
    static std::shared_ptr<InfoMark> alloc()
    {
        return std::make_shared<InfoMark>(this_is_private{0});
    }

public:
    explicit InfoMark(this_is_private) {}
    ~InfoMark() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(InfoMark);

public:
    const InfoMarkName &getName() const { return m_name; }
    const InfoMarkText &getText() const { return m_text; }
    InfoMarkTypeEnum getType() const { return m_type; }
    InfoMarkClassEnum getClass() const { return m_class; }
    const Coordinate &getPosition1() const { return m_pos1; }
    const Coordinate &getPosition2() const { return m_pos2; }
    float getRotationAngle() const { return m_rotationAngle; }
    const MarkerTimeStamp &getTimeStamp() const { return m_timeStamp; }

    void setPosition1(const Coordinate &pos) { m_pos1 = pos; }
    void setPosition2(const Coordinate &pos) { m_pos2 = pos; }
    void setRotationAngle(float rotationAngle) { m_rotationAngle = rotationAngle; }
    void setName(InfoMarkName name) { m_name = name; }
    void setText(InfoMarkText text) { m_text = text; }
    void setType(InfoMarkTypeEnum type) { m_type = type; }
    void setClass(InfoMarkClassEnum markClass) { m_class = markClass; }

    void setTimeStamp(MarkerTimeStamp timeStamp) { m_timeStamp = timeStamp; }

private:
    InfoMarkName m_name;
    InfoMarkText m_text;
    InfoMarkTypeEnum m_type = InfoMarkTypeEnum::TEXT;
    InfoMarkClassEnum m_class = InfoMarkClassEnum::GENERIC;

    MarkerTimeStamp m_timeStamp;

    Coordinate m_pos1;
    Coordinate m_pos2;
    float m_rotationAngle = 0.0; // in degrees
};
