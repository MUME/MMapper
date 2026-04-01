// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Filenames.h"

#include "../configuration/configuration.h"
#include "../global/ConfigConsts-Computed.h"
#include "../global/Consts.h"
#include "../global/EnumIndexedArray.h"
#include "../global/NullPointerException.h"
#include "../parser/AbstractParser-Commands.h"

#include <memory>
#include <mutex>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>

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
    static const auto names = std::invoke([]() {
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
    });

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

/**
 * @brief Returns the base path to the sideloaded assets directory.
 *
 * This function returns an absolute path on desktop platforms and a relative path
 * on WebAssembly (as assets are served from the web root).
 *
 * @return The assets path with a trailing slash.
 */
QString getAssetsPath()
{
    static const QString assetsDirName = QStringLiteral("assets");

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        return assetsDirName + "/";
    }

    static const QString assetsPath = []() {
        const QDir appDir(QCoreApplication::applicationDirPath());

        if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac) {
            // Check for assets in the bundle's Resources directory
            QDir bundleAssets(appDir);
            bundleAssets.cdUp();
            bundleAssets.cd("Resources");
            bundleAssets.cd(assetsDirName);
            if (bundleAssets.exists()) {
                return bundleAssets.absolutePath() + "/";
            }
        } else if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
            if (appDir.exists(assetsDirName)) {
                return appDir.absoluteFilePath(assetsDirName) + "/";
            }
        } else if constexpr (CURRENT_PLATFORM == PlatformEnum::Linux) {
            // Check for assets in share/mmapper/assets (standard Linux install)
            QDir linuxAssets(appDir);
            linuxAssets.cdUp();
            linuxAssets.cd("share/mmapper");
            linuxAssets.cd(assetsDirName);
            if (linuxAssets.exists()) {
                return linuxAssets.absolutePath() + "/";
            }
        }

        // Fallback: check next to the binary
        if (appDir.exists(assetsDirName)) {
            return appDir.absoluteFilePath(assetsDirName) + "/";
        }

        // Final fallback: return an absolute path even if it doesn't exist
        return appDir.absoluteFilePath(assetsDirName) + "/";
    }();

    return assetsPath;
}

QString getResourceFilenameRaw(const QString &dir, const QString &name)
{
    const auto filename = QString("/%1/%2").arg(dir, name);

    // Check if the user provided a custom resource
    auto custom = getConfig().canvas.resourcesDirectory + filename;
    if (QFile{custom}.exists()) {
        return custom;
    }

    // Check the system assets directory
    auto assetPath = getAssetsPath() + dir + "/" + name;
    if (QFile{assetPath}.exists()) {
        return assetPath;
    }

    // Fallback to the qrc resource
    const auto qrc = ":" + filename;
    if (!QFile{qrc}.exists()) {
        qWarning() << "WARNING:" << dir << filename << "does not exist.";
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
