// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Filenames.h"

#include <memory>
#include <mutex>
#include <QtCore/QDebug>
#include <QtCore/QFile>

#include "../global/EnumIndexedArray.h"
#include "../global/NullPointerException.h"
#include "../parser/AbstractParser-Commands.h"

// NOTE: This isn't used by the parser (currently only used for filenames).
// If we were going to use it for parsing, then we'd probably want to
// return special ArgRoadIndex that could match the direction combinations
// in any order (e.g. sort the word's letters using compass-ordering).
static const char *getFilenameSuffix(const RoadIndexMaskEnum x)
{
    assert(RoadIndexMaskEnum::NONE <= x && x <= RoadIndexMaskEnum::ALL);

    if (x == RoadIndexMaskEnum::NONE)
        return "none";
    else if (x == RoadIndexMaskEnum::ALL)
        return "all";

    // note: static variable initialized by IIFE guarantees thread safety.
    static const auto names = []() {
        using CharArray = char[8];
        static_assert(sizeof(CharArray) >= NUM_COMPASS_DIRS + 1);
        EnumIndexedArray<CharArray, RoadIndexMaskEnum> arr{};
        for (size_t i = 0; i < NUM_ROAD_INDICES; ++i) {
            const auto e = static_cast<RoadIndexMaskEnum>(i);
            char *ptr = arr[e];
            if ((e & RoadIndexMaskEnum::NORTH) != RoadIndexMaskEnum::NONE) {
                *ptr++ = 'n';
            }
            if ((e & RoadIndexMaskEnum::EAST) != RoadIndexMaskEnum::NONE) {
                *ptr++ = 'e';
            }
            if ((e & RoadIndexMaskEnum::SOUTH) != RoadIndexMaskEnum::NONE) {
                *ptr++ = 's';
            }
            if ((e & RoadIndexMaskEnum::WEST) != RoadIndexMaskEnum::NONE) {
                *ptr++ = 'w';
            }
            *ptr = '\0';
        }
        return arr;
    }();

    return names[x];
}

template<RoadTagEnum Tag>
static inline const char *getFilenameSuffix(const TaggedRoadIndex<Tag> &x)
{
    return getFilenameSuffix(x.index);
}

// template<>
// const char *getFilenameSuffix(RoomTerrainEnum);
// template<>
// const char *getFilenameSuffix(RoomLoadFlagEnum);
// template<>
// const char *getFilenameSuffix(RoomMobFlagEnum);
template<typename E>
static const char *getFilenameSuffix(const E x)
{
    return getParserCommandName(x).getCommand();
}

QString getResourceFilenameRaw(const QString &dir, const QString &name)
{
    const auto filename = QString(":/%1/%2").arg(dir).arg(name);
    if (!QFile{filename}.exists())
        qWarning() << "WARNING:" << dir << filename << "does not exist.";
    return filename;
}

QString getPixmapFilenameRaw(const QString &name)
{
    return getResourceFilenameRaw("pixmaps", name);
}

template<typename E>
static QString getPixmapFilename(const char *const prefix, const E x)
{
    if (prefix == nullptr)
        throw NullPointerException();

    const char *const suffix = getFilenameSuffix(x);
    if (suffix == nullptr)
        throw NullPointerException();

    return getPixmapFilenameRaw(QString::asprintf("%s-%s.png", prefix, suffix));
}

QString getPixmapFilename(const RoomTerrainEnum x)
{
    return getPixmapFilename("terrain", x);
}
QString getPixmapFilename(const RoomLoadFlagEnum x)
{
    return getPixmapFilename("load", x);
}
QString getPixmapFilename(const RoomMobFlagEnum x)
{
    return getPixmapFilename("mob", x);
}
QString getPixmapFilename(const TaggedRoad x)
{
    return getPixmapFilename("road", x);
}
QString getPixmapFilename(const TaggedTrail x)
{
    return getPixmapFilename("trail", x);
}

static QString getIconFilenameRaw(const QString &name)
{
    return getResourceFilenameRaw("icons", name);
}

static const char *getFilenameSuffix(const CharacterPositionEnum position)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case CharacterPositionEnum::UPPER_CASE: \
        return #lower_case; \
    } while (false);
    switch (position) {
        X_FOREACH_CHARACTER_POSITION(X_CASE)
    }
    return "";
#undef X_CASE
}
static const char *getFilenameSuffix(const CharacterAffectEnum affect)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case CharacterAffectEnum::UPPER_CASE: \
        return #lower_case; \
    } while (false);
    switch (affect) {
        X_FOREACH_CHARACTER_AFFECT(X_CASE)
    }
    return "";
#undef X_CASE
}

template<typename E>
static QString getIconFilename(const char *const prefix, const E x)
{
    if (prefix == nullptr)
        throw NullPointerException();

    const char *const suffix = getFilenameSuffix(x);
    if (suffix == nullptr)
        throw NullPointerException();

    return getIconFilenameRaw(QString::asprintf("%s-%s.png", prefix, suffix));
}

QString getIconFilename(const CharacterPositionEnum x)
{
    return getIconFilename("position", x);
}
QString getIconFilename(const CharacterAffectEnum x)
{
    return getIconFilename("affect", x);
}
