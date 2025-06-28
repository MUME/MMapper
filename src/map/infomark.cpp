// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "infomark.h"

#include "../global/Charset.h"
#include "../global/ImmUnorderedMap.h"

#include <memory>
#include <stdexcept>
#include <vector>

template<typename T>
void maybeModify(T &ours, T value)
{
    if (ours != value) {
        ours = value;
    }
}

void RawInfomark::setPosition1(Coordinate pos)
{
    if (Type == InfomarkTypeEnum::TEXT) {
        // See comment in setPosition2()
        Position2 = pos;
    }
    maybeModify(Position1, std::move(pos));
}

void RawInfomark::setPosition2(Coordinate pos)
{
    if (Type == InfomarkTypeEnum::TEXT) {
        // Text Infomarks utilize Position1 exclusively
        return;
    }
    maybeModify(Position2, std::move(pos));
}

// REVISIT: consider rounding rotation to 45 degrees, since that's all the dialog can handle?
void RawInfomark::setRotationAngle(const int rotationAngle)
{
    // mod twice avoids separate case for negative.
    int adjusted = (rotationAngle % 360 + 360) % 360;
    maybeModify(RotationAngle, std::move(adjusted));
}

void RawInfomark::setText(InfomarkText text)
{
    maybeModify(Text, std::move(text));
}

void RawInfomark::setType(InfomarkTypeEnum type)
{
    maybeModify(Type, std::move(type));
}

void RawInfomark::setClass(InfomarkClassEnum markClass)
{
    maybeModify(Class, std::move(markClass));
}

struct NODISCARD InfomarkDb::Pimpl final
{
    struct NODISCARD Data final
    {
    public:
#define X_DEFINE_VECTOR(_Type, _Prop, _OptInit) ImmUnorderedMap<InfomarkId, _Type> _Prop##_map;
        XFOREACH_INFOMARK_PROPERTY(X_DEFINE_VECTOR)
#undef X_DEFINE_VECTOR

    public:
        NODISCARD bool operator==(const Data &rhs) const
        {
#define X_DEFINE_VECTOR(_Type, _Prop, _OptInit) \
    if (_Prop##_map != rhs._Prop##_map) { \
        return false; \
    }
            XFOREACH_INFOMARK_PROPERTY(X_DEFINE_VECTOR)
#undef X_DEFINE_VECTOR
            return true;
        }
        NODISCARD bool operator!=(const Data &rhs) const
        {
            return !(rhs == *this);
        }
    };

    InfomarkId m_next;
    ImmInfomarkIdSet m_set;
    Data m_data;

#define X_DEFINE_GETTERS(_Type, _Prop, _OptInit) \
    NODISCARD const _Type &get##_Prop(const InfomarkId id) const \
    { \
        if (const _Type *const ptr = m_data._Prop##_map.find(id)) { \
            return *ptr; \
        } \
        throw std::runtime_error("invalid InfomarkId"); \
    } \
    NODISCARD bool set##_Prop(const InfomarkId id, const _Type &val) \
    { \
        if (const _Type *const ptr = m_data._Prop##_map.find(id); ptr == nullptr || *ptr != val) { \
            auto &ref = m_data._Prop##_map; \
            ref.set(id, val); \
            return true; \
        } \
        return false; \
    }
    XFOREACH_INFOMARK_PROPERTY(X_DEFINE_GETTERS)
#undef X_DEFINE_GETTERS

public:
    NODISCARD const ImmInfomarkIdSet &getIdSet() const
    {
        return m_set;
    }

public:
    void updateMarker(const InfomarkId id, const RawInfomark &im)
    {
        if (!m_set.contains(id)) {
            throw std::runtime_error("invalid infomark id");
        }

#define X_SET_VALUE(_Type, _Prop, _OptInit) std::ignore = set##_Prop(id, im.get##_Prop());
        XFOREACH_INFOMARK_PROPERTY(X_SET_VALUE)
#undef X_SET_VALUE
    }

    NODISCARD InfomarkId addMarker(const RawInfomark &im)
    {
        auto id = m_next;
        m_next = m_next.next();
        m_set.insert(id);
#define X_SET_VALUE(_Type, _Prop, _OptInit) \
    { \
        auto &ref = m_data._Prop##_map; \
        ref.set(id, im.get##_Prop()); \
    }
        XFOREACH_INFOMARK_PROPERTY(X_SET_VALUE)
#undef X_SET_VALUE
        return id;
    }

    void removeMarker(const InfomarkId id)
    {
        if (!m_set.contains(id)) {
            throw std::runtime_error("invalid infomark id");
        }
        m_set.remove(id);
#define X_SET_VALUE(_Type, _Prop, _OptInit) \
    { \
        auto &ref = m_data._Prop##_map; \
        ref.erase(id); \
    }
        XFOREACH_INFOMARK_PROPERTY(X_SET_VALUE)
#undef X_SET_VALUE
    }

    NODISCARD RawInfomark getRawCopy(const InfomarkId id) const
    {
        RawInfomark result;
#define X_COPY_VALUE(_Type, _Prop, _OptInit) result._Prop = get##_Prop(id);
        XFOREACH_INFOMARK_PROPERTY(X_COPY_VALUE)
#undef X_COPY_VALUE
        return result;
    }

    NODISCARD bool operator==(const Pimpl &rhs) const
    {
        return m_next == rhs.m_next && m_set == rhs.m_set && m_data == rhs.m_data;
    }
    NODISCARD bool operator!=(const Pimpl &rhs) const
    {
        return !(rhs == *this);
    }

    NODISCARD auto clone() const
    {
        return std::make_shared<Pimpl>(*this);
    }
};

InfomarkDb::InfomarkDb()
    : m_pimpl{std::make_shared<Pimpl>()}
{}

#define X_DEFINE_GETTERS(_Type, _Prop, _OptInit) \
    const _Type &InfomarkDb::get##_Prop(const InfomarkId id) const \
    { \
        return m_pimpl->get##_Prop(id); \
    }
XFOREACH_INFOMARK_PROPERTY(X_DEFINE_GETTERS)
#undef X_DEFINE_GETTERS

const ImmInfomarkIdSet &InfomarkDb::getIdSet() const
{
    return m_pimpl->getIdSet();
}

InfomarkId InfomarkDb::addMarker(const RawInfomark &im)
{
    auto tmp = m_pimpl->clone();
    auto result = tmp->addMarker(im);

    /* replace our guts */
    m_pimpl = tmp;
    return result;
}

void InfomarkDb::updateMarker(const InfomarkId id, const RawInfomark &im)
{
    auto tmp = m_pimpl->clone();
    tmp->updateMarker(id, im);

    /* replace our guts */
    m_pimpl = tmp;
}

void InfomarkDb::updateMarkers(const std::vector<InfomarkChange> &updates)
{
    auto tmp = m_pimpl->clone();

    for (const InfomarkChange &update : updates) {
        tmp->updateMarker(update.id, update.mark);
    }

    /* replace our guts */
    m_pimpl = tmp;
}

void InfomarkDb::removeMarker(const InfomarkId id)
{
    auto tmp = m_pimpl->clone();
    tmp->removeMarker(id);

    /* replace our guts */
    m_pimpl = tmp;
}

RawInfomark InfomarkDb::getRawCopy(const InfomarkId id) const
{
    return m_pimpl->getRawCopy(id);
}

NODISCARD InfomarkHandle InfomarkDb::find(const InfomarkId id) const
{
    if (getIdSet().contains(id)) {
        return InfomarkHandle{*this, id};
    }
    return InfomarkHandle{*this, INVALID_INFOMARK_ID};
}

bool InfomarkDb::operator==(const InfomarkDb &rhs) const
{
    return m_pimpl == rhs.m_pimpl || deref(m_pimpl) == deref(rhs.m_pimpl);
}

#define X_DEFINE_GETTERS(_Type, _Prop, _OptInit) \
    const _Type &InfomarkHandle::get##_Prop() const \
    { \
        return m_db.get##_Prop(m_id); \
    }
XFOREACH_INFOMARK_PROPERTY(X_DEFINE_GETTERS)
#undef X_DEFINE_GETTERS

InfomarkText makeInfomarkText(std::string text)
{
    if (!isValidUtf8(text)) {
        throw std::runtime_error("wrong encoding");
    }
    // TODO: add sanitizer here
    return InfomarkText{std::move(text)};
}

namespace mmqt {
InfomarkText makeInfomarkText(QString text)
{
    return ::makeInfomarkText(toStdStringUtf8(text));
}
} // namespace mmqt

bool tags::InfomarkTextTag::isValid(std::string_view /*sv*/)
{
    return true;
}
bool tags::InfomarkIdTag::isValid(std::string_view /*sv*/)
{
    return true;
}
