// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Filenames.h"

#include "../clock/mumemoment.h"
#include "../configuration/configuration.h"
#include "../global/Consts.h"
#include "../global/EnumIndexedArray.h"
#include "../global/NullPointerException.h"
#include "../parser/AbstractParser-Commands.h"

#include <atomic>
#include <memory>
#include <mutex>

#include <QtCore/QDebug>
#include <QtCore/QFile>

// Thread-safe storage for current season
namespace {
std::atomic<MumeSeasonEnum> g_currentSeason{MumeSeasonEnum::SPRING};

NODISCARD const char *seasonToString(MumeSeasonEnum season)
{
    switch (season) {
    case MumeSeasonEnum::WINTER:
        return "Winter";
    case MumeSeasonEnum::SPRING:
        return "Spring";
    case MumeSeasonEnum::SUMMER:
        return "Summer";
    case MumeSeasonEnum::AUTUMN:
        return "Autumn";
    case MumeSeasonEnum::UNKNOWN:
    default:
        return "Spring"; // Default to Spring
    }
}

NODISCARD const char *textureSetToString(TextureSetEnum textureSet)
{
    switch (textureSet) {
    case TextureSetEnum::CLASSIC:
        return "Classic";
    case TextureSetEnum::MODERN:
        return "Modern";
    case TextureSetEnum::CUSTOM:
        return ""; // Custom uses resourcesDirectory directly
    default:
        return "Modern";
    }
}

} // namespace

void setCurrentSeason(MumeSeasonEnum season)
{
    g_currentSeason.store(season);
}

MumeSeasonEnum getCurrentSeason()
{
    return g_currentSeason.load();
}

// NOTE: This isn't used by the parser (currently only used for filenames).
// If we were going to use it for parsing, then we'd probably want to
// return special ArgRoadIndex that could match the direction combinations
// in any order (e.g. sort the word's letters using compass-ordering).
NODISCARD static const char *getFilenameSuffix(const RoadIndexMaskEnum x)
{
    assert(RoadIndexMaskEnum::NONE <= x && x <= RoadIndexMaskEnum::ALL);

    if (x == RoadIndexMaskEnum::NONE) {
        return "none";
    } else if (x == RoadIndexMaskEnum::ALL) {
        return "all";
    }

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
            *ptr = char_consts::C_NUL;
        }
        return arr;
    }();

    return names[x];
}

template<RoadTagEnum Tag>
NODISCARD static inline const char *getFilenameSuffix(const TaggedRoadIndex<Tag> &x)
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
NODISCARD static const char *getFilenameSuffix(const E x)
{
    return getParserCommandName(x).getCommand();
}

QString getResourceFilenameRaw(const QString &dir, const QString &name)
{
    const auto &config = getConfig().canvas;
    const auto textureSet = config.textureSet;
    const auto enableSeasonalTextures = config.enableSeasonalTextures;

    // Determine the base directory for the texture set
    QString setDir;
    QString customBasePath;

    if (textureSet == TextureSetEnum::CUSTOM) {
        // Use custom resources directory directly
        customBasePath = config.resourcesDirectory;
    } else {
        // Use Classic or Modern from bundled or custom resources
        setDir = textureSetToString(textureSet);
    }

    // Determine which season to use
    const char *seasonStr = nullptr;
    if (dir == "pixmaps" && enableSeasonalTextures) {
        // Only apply seasonal logic when seasonal textures are enabled
        seasonStr = seasonToString(getCurrentSeason());
    }

    // Try multiple paths with fallback logic
    QStringList pathsToTry;

    if (textureSet == TextureSetEnum::CUSTOM) {
        // Custom texture set paths
        const auto baseFilename = QString("/%1/%2").arg(dir, name);

        if (seasonStr != nullptr) {
            // Try: custom/pixmaps/[Season]/file.png
            pathsToTry << customBasePath + QString("/%1/%2/%3").arg(dir, seasonStr, name);
        }
        // Try: custom/pixmaps/file.png
        pathsToTry << customBasePath + baseFilename;

        // Fallback to Modern tileset for missing custom textures
        const QString modernDir = "Modern";

        // Try Modern with current season (if seasonal textures enabled)
        if (seasonStr != nullptr) {
            if (!config.resourcesDirectory.isEmpty()) {
                pathsToTry << config.resourcesDirectory + QString("/%1/%2/%3/%4")
                                                              .arg(dir, modernDir, seasonStr, name);
            }
            pathsToTry << QString(":/%1/%2/%3/%4").arg(dir, modernDir, seasonStr, name);
        }

        // Try Modern base folder
        if (!config.resourcesDirectory.isEmpty()) {
            pathsToTry << config.resourcesDirectory + QString("/%1/%2/%3")
                                                          .arg(dir, modernDir, name);
        }
        pathsToTry << QString(":/%1/%2/%3").arg(dir, modernDir, name);

        // Try Modern/Spring as final fallback for pixmaps
        if (dir == "pixmaps") {
            if (!config.resourcesDirectory.isEmpty()) {
                pathsToTry << config.resourcesDirectory + QString("/%1/%2/Spring/%3")
                                                              .arg(dir, modernDir, name);
            }
            pathsToTry << QString(":/%1/%2/Spring/%3").arg(dir, modernDir, name);
        }
    } else {
        // Classic or Modern texture sets
        if (seasonStr != nullptr) {
            // Try: resourcesDir/pixmaps/Modern/[Season]/file.png (custom location)
            if (!config.resourcesDirectory.isEmpty()) {
                pathsToTry << config.resourcesDirectory + QString("/%1/%2/%3/%4")
                                                              .arg(dir, setDir, seasonStr, name);
            }
            // Try: :pixmaps/Modern/[Season]/file.png (bundled)
            pathsToTry << QString(":/%1/%2/%3/%4").arg(dir, setDir, seasonStr, name);

            // Try: resourcesDir/pixmaps/Modern/file.png (fallback without season, custom)
            if (!config.resourcesDirectory.isEmpty()) {
                pathsToTry << config.resourcesDirectory + QString("/%1/%2/%3")
                                                              .arg(dir, setDir, name);
            }
            // Try: :pixmaps/Modern/file.png (fallback without season, bundled)
            pathsToTry << QString(":/%1/%2/%3").arg(dir, setDir, name);
        } else {
            // Seasonal textures disabled - try base folder first, then Spring as fallback
            if (!config.resourcesDirectory.isEmpty()) {
                pathsToTry << config.resourcesDirectory + QString("/%1/%2/%3")
                                                              .arg(dir, setDir, name);
            }
            pathsToTry << QString(":/%1/%2/%3").arg(dir, setDir, name);

            // For Modern tileset with seasonal disabled, fallback to Spring for pixmaps
            if (dir == "pixmaps" && textureSet == TextureSetEnum::MODERN) {
                if (!config.resourcesDirectory.isEmpty()) {
                    pathsToTry << config.resourcesDirectory + QString("/%1/%2/Spring/%3")
                                                                  .arg(dir, setDir, name);
                }
                pathsToTry << QString(":/%1/%2/Spring/%3").arg(dir, setDir, name);
            }
        }
    }

    // Try each path in order
    for (const auto &path : pathsToTry) {
        if (QFile{path}.exists()) {
            return path;
        }
    }

    // Final fallback to original bundled resource location (without texture set)
    const auto qrc = QString(":/%1/%2").arg(dir, name);
    if (!QFile{qrc}.exists()) {
        qWarning() << "WARNING: Resource not found:" << dir << "/" << name
                   << "(tried" << pathsToTry.size() << "locations)";
    }
    return qrc;
}

QString getPixmapFilenameRaw(const QString &name)
{
    return getResourceFilenameRaw("pixmaps", name);
}

template<typename E>
NODISCARD static QString getPixmapFilename(const char *const prefix, const E x)
{
    if (prefix == nullptr) {
        throw NullPointerException();
    }

    const char *const suffix = getFilenameSuffix(x);
    if (suffix == nullptr) {
        throw NullPointerException();
    }

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

NODISCARD static QString getIconFilenameRaw(const QString &name)
{
    return getResourceFilenameRaw("icons", name);
}

NODISCARD static const char *getFilenameSuffix(const CharacterPositionEnum position)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case CharacterPositionEnum::UPPER_CASE: \
        return #lower_case; \
    } while (false);
    switch (position) {
        XFOREACH_CHARACTER_POSITION(X_CASE)
    }
    return "";
#undef X_CASE
}
NODISCARD static const char *getFilenameSuffix(const CharacterAffectEnum affect)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case CharacterAffectEnum::UPPER_CASE: \
        return #lower_case; \
    } while (false);
    switch (affect) {
        XFOREACH_CHARACTER_AFFECT(X_CASE)
    }
    return "";
#undef X_CASE
}

template<typename E>
NODISCARD static QString getIconFilename(const char *const prefix, const E x)
{
    if (prefix == nullptr) {
        throw NullPointerException();
    }

    const char *const suffix = getFilenameSuffix(x);
    if (suffix == nullptr) {
        throw NullPointerException();
    }

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
