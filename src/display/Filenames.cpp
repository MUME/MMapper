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
static const char *getFilenameSuffix(const RoadIndex x)
{
    assert(RoadIndex::NONE <= x && x <= RoadIndex::ALL);

    if (x == RoadIndex::NONE)
        return "none";
    else if (x == RoadIndex::ALL)
        return "all";

    struct CustomDeleter final
    {
        void operator()(char *ptr) const { std::free(ptr); }
    };
    using UniqueCString = std::unique_ptr<char, CustomDeleter>;

    static EnumIndexedArray<UniqueCString, RoadIndex> names{};

    // Make the write thread safe to avoid accidentally freeing pointers
    // we've already given away. (Probably overkill, but it won't hurt.)
    static std::mutex mutex{};
    std::lock_guard<std::mutex> lock{mutex};

    if (names[x] == nullptr) {
        char buf[8] = "";
        char *ptr = buf;

        if ((x & RoadIndex::NORTH) != RoadIndex::NONE) {
            *ptr++ = 'n';
        }
        if ((x & RoadIndex::EAST) != RoadIndex::NONE) {
            *ptr++ = 'e';
        }
        if ((x & RoadIndex::SOUTH) != RoadIndex::NONE) {
            *ptr++ = 's';
        }
        if ((x & RoadIndex::WEST) != RoadIndex::NONE) {
            *ptr++ = 'w';
        }

        assert(ptr > buf);
        assert(ptr <= buf + 4);
        *ptr = '\0';

        names[x] = UniqueCString{strdup(buf)};
    }

    assert(names[x] != nullptr);
    return names[x].get();
}

template<RoadIndexType Type>
static inline const char *getFilenameSuffix(TaggedRoadIndex<Type> x)
{
    return getFilenameSuffix(x.index);
}

// template<>
// const char *getFilenameSuffix(RoomTerrainType);
// template<>
// const char *getFilenameSuffix(RoomLoadFlag);
// template<>
// const char *getFilenameSuffix(RoomMobFlag);
template<typename E>
static const char *getFilenameSuffix(const E x)
{
    return getParserCommandName(x).getCommand();
}

static QString getResourceFilenameRaw(const QString dir, const QString name)
{
    const auto filename = QString(":/%1/%2").arg(dir).arg(name);
    if (!QFile{filename}.exists())
        qWarning() << "WARNING:" << dir << filename << "does not exist.";
    return filename;
}

QString getPixmapFilenameRaw(const QString name)
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

QString getPixmapFilename(const RoomTerrainType x)
{
    return getPixmapFilename("terrain", x);
}
QString getPixmapFilename(const RoomLoadFlag x)
{
    return getPixmapFilename("load", x);
}
QString getPixmapFilename(const RoomMobFlag x)
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

static QString getIconFilenameRaw(const QString name)
{
    return getResourceFilenameRaw("icons", name);
}

static const char *getFilenameSuffix(const CharacterPosition position)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case CharacterPosition::UPPER_CASE: \
        return #lower_case; \
    } while (false);
    switch (position) {
        X_FOREACH_CHARACTER_POSITION(X_CASE)
    }
    return "";
#undef X_CASE
}
static const char *getFilenameSuffix(const CharacterAffect affect)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case CharacterAffect::UPPER_CASE: \
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

QString getIconFilename(const CharacterPosition x)
{
    return getIconFilename("position", x);
}
QString getIconFilename(const CharacterAffect x)
{
    return getIconFilename("affect", x);
}
