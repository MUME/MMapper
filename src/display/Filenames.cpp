/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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
#define CASE2(UPPER, s) \
    do { \
    case CharacterPosition::UPPER: \
        return s; \
    } while (false)
    switch (position) {
        CASE2(STANDING, "standing");
        CASE2(FIGHTING, "fighting");
        CASE2(RESTING, "resting");
        CASE2(SITTING, "sitting");
        CASE2(SLEEPING, "sleeping");
        CASE2(INCAPACITATED, "incapacitated");
        CASE2(DEAD, "dead");
    case CharacterPosition::UNDEFINED:
        break;
    }
    return "";
#undef CASE2
}
static const char *getFilenameSuffix(const CharacterAffect affect)
{
#define CASE2(UPPER, s) \
    do { \
    case CharacterAffect::UPPER: \
        return s; \
    } while (false)
    switch (affect) {
        CASE2(BLIND, "blind");
        CASE2(BASHED, "bashed");
        CASE2(SLEPT, "slept");
        CASE2(POISONED, "poisoned");
    }
    return "";
#undef CASE2
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
    return getIconFilename("group", x);
}
QString getIconFilename(const CharacterAffect x)
{
    return getIconFilename("group", x);
}
