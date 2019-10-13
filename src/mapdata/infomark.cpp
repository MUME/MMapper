// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "infomark.h"

InfoMarkModificationTracker::~InfoMarkModificationTracker() = default;

void InfoMarkModificationTracker::notifyModified(InfoMark &mark, InfoMarkUpdateFlags updateFlags)
{
    virt_onNotifyModified(mark, updateFlags);
}

InfoMark::InfoMark(InfoMark::this_is_private, InfoMarkModificationTracker &tracker)
    : m_tracker{tracker}
{}

void InfoMark::setModified(InfoMarkUpdateFlags updateFlags)
{
    m_tracker.notifyModified(*this, updateFlags);
}

template<typename T>
inline bool maybeModify(T &ours, T &&value)
{
    if (ours == value)
        return false;

    ours = std::forward<T>(value);
    return true;
}

void InfoMark::setPosition1(Coordinate pos)
{
    if (m_fields.Type == InfoMarkTypeEnum::TEXT) {
        // See comment in setPosition2()
        m_fields.Position2 = pos;
    }
    if (maybeModify(m_fields.Position1, std::move(pos)))
        setModified(InfoMarkUpdateFlags{InfoMarkUpdateEnum::CoordinatePosition1});
}

void InfoMark::setPosition2(Coordinate pos)
{
    if (m_fields.Type == InfoMarkTypeEnum::TEXT) {
        // Text InfoMarks utilize Position1 exclusively
        return;
    }
    if (maybeModify(m_fields.Position2, std::move(pos)))
        setModified(InfoMarkUpdateFlags{InfoMarkUpdateEnum::CoordinatePosition2});
}

// REVISIT: consider rounding rotation to 45 degrees, since that's all the dialog can handle?
void InfoMark::setRotationAngle(float rotationAngle)
{
    if (qFloatDistance(m_fields.RotationAngle, rotationAngle) != 0) {
        m_fields.RotationAngle = std::move(rotationAngle);
        setModified(InfoMarkUpdateFlags{InfoMarkUpdateEnum::RotationAngle});
    }
}

void InfoMark::setName(InfoMarkName name)
{
    if (maybeModify(m_fields.Name, std::move(name)))
        setModified(InfoMarkUpdateFlags{InfoMarkUpdateEnum::InfoMarkName});
}

void InfoMark::setText(InfoMarkText text)
{
    if (maybeModify(m_fields.Text, std::move(text)))
        setModified(InfoMarkUpdateFlags{InfoMarkUpdateEnum::InfoMarkText});
}

void InfoMark::setType(InfoMarkTypeEnum type)
{
    if (maybeModify(m_fields.Type, std::move(type)))
        setModified(InfoMarkUpdateFlags{InfoMarkUpdateEnum::InfoMarkType});
}

void InfoMark::setClass(InfoMarkClassEnum markClass)
{
    if (maybeModify(m_fields.Class, std::move(markClass)))
        setModified(InfoMarkUpdateFlags{InfoMarkUpdateEnum::InfoMarkClass});
}

void InfoMark::setTimeStamp(MarkerTimeStamp time)
{
    if (maybeModify(m_fields.TimeStamp, std::move(time)))
        setModified(InfoMarkUpdateFlags{InfoMarkUpdateEnum::MarkerTimeStamp});
}
