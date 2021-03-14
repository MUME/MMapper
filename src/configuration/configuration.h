#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <string_view>
#include <QByteArray>
#include <QColor>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QtCore>
#include <QtGlobal>

#include "../global/Array.h"
#include "../global/Debug.h"
#include "../global/FixedPoint.h"
#include "../global/NamedColors.h"
#include "../global/RuleOf5.h"
#include "../pandoragroup/mmapper2group.h"
#include "NamedConfig.h"

#undef TRANSPARENT // Bad dog, Microsoft; bad dog!!!

enum class NODISCARD MapModeEnum { PLAY, MAP, OFFLINE };
enum class NODISCARD PlatformEnum { Unknown, Windows, Mac, Linux };
enum class NODISCARD CharacterEncodingEnum { LATIN1, UTF8, ASCII };
enum class NODISCARD EnvironmentEnum { Unknown, Env32Bit, Env64Bit };
enum class NODISCARD RestrictMapEnum { Never, OnlyInMapMode, Always };
enum class NODISCARD AutoLoggerEnum { KeepForever, DeleteDays, DeleteSize };

// Do not call this directly; use CURRENT_PLATFORM.
NODISCARD static inline constexpr PlatformEnum getCurrentPlatform()
{
#if defined(Q_OS_WIN)
    return PlatformEnum::Windows;
#elif defined(Q_OS_MAC)
    return PlatformEnum::Mac;
#elif defined(Q_OS_LINUX)
    return PlatformEnum::Linux;
#else
    throw std::runtime_error("unsupported platform");
#endif
}
static constexpr const PlatformEnum CURRENT_PLATFORM = getCurrentPlatform();

// Do not call this directly; use CURRENT_ENVIRONMENT.
NODISCARD static inline constexpr EnvironmentEnum getCurrentEnvironment()
{
#if Q_PROCESSOR_WORDSIZE == 4
    return EnvironmentEnum::Env32Bit;
#elif Q_PROCESSOR_WORDSIZE == 8
    return EnvironmentEnum::Env64Bit;
#else
    throw std::runtime_error("unsupported environment");
#endif
}
static constexpr const EnvironmentEnum CURRENT_ENVIRONMENT = getCurrentEnvironment();

#if defined(MMAPPER_NO_OPENSSL) && MMAPPER_NO_OPENSSL
static constexpr const bool NO_OPEN_SSL = true;
#else
static constexpr const bool NO_OPEN_SSL = false;
#endif

#if defined(MMAPPER_NO_UPDATER) && MMAPPER_NO_UPDATER
static constexpr const bool NO_UPDATER = true;
#else
static constexpr const bool NO_UPDATER = false;
#endif

#if defined(MMAPPER_NO_MAP) && MMAPPER_NO_MAP
static constexpr const bool NO_MAP_RESOURCE = true;
#else
static constexpr const bool NO_MAP_RESOURCE = false;
#endif

#if defined(MMAPPER_NO_ZLIB) && MMAPPER_NO_ZLIB
static constexpr const bool NO_ZLIB = true;
#else
static constexpr const bool NO_ZLIB = false;
#endif

#define SUBGROUP() \
    friend class Configuration; \
    void read(QSettings &conf); \
    void write(QSettings &conf) const

class NODISCARD Configuration final
{
public:
    void read();
    void write() const;
    void reset();

public:
    struct NODISCARD GeneralSettings final
    {
        bool firstRun = false;
        QByteArray windowGeometry;
        QByteArray windowState;
        bool alwaysOnTop = false;
        MapModeEnum mapMode = MapModeEnum::PLAY;
        bool noSplash = false;
        bool noClientPanel = false;
        bool checkForUpdate = true;
        CharacterEncodingEnum characterEncoding = CharacterEncodingEnum::LATIN1;

    private:
        SUBGROUP();
    } general{};

    struct NODISCARD ConnectionSettings final
    {
        QString remoteServerName; /// Remote host and port settings
        quint16 remotePort = 0u;
        quint16 localPort = 0u; /// Port to bind to on local machine
        bool tlsEncryption = false;
        bool proxyThreaded = false;
        bool proxyConnectionStatus = false;
        bool proxyListensOnAnyInterface = false;

    private:
        SUBGROUP();
    } connection;

    struct NODISCARD ParserSettings final
    {
        QString roomNameColor; // ANSI room name color
        QString roomDescColor; // ANSI room descriptions color
        bool removeXmlTags = false;
        char prefixChar = '_';
        QStringList noDescriptionPatternsList;

    private:
        SUBGROUP();
    } parser;

    struct NODISCARD MumeClientProtocolSettings final
    {
        bool remoteEditing = false;
        bool internalRemoteEditor = false;
        QString externalRemoteEditorCommand;

    private:
        SUBGROUP();
    } mumeClientProtocol;

    struct NODISCARD MumeNativeSettings final
    {
        /* serialized */
        bool emulatedExits = false;
        bool showHiddenExitFlags = false;
        bool showNotes = false;

    private:
        SUBGROUP();
    } mumeNative;

    static constexpr const std::string_view BACKGROUND_NAME = "background";
    static constexpr const std::string_view CONNECTION_NORMAL_NAME = "connection-normal";
    static constexpr const std::string_view ROOM_DARK_NAME = "room-dark";
    static constexpr const std::string_view ROOM_NO_SUNDEATH_NAME = "room-no-sundeath";

    struct NODISCARD CanvasSettings final
    {
        bool showUpdated = false;
        bool drawNotMappedExits = false;
        bool drawUpperLayersTextured = false;
        bool drawDoorNames = false;
        XNamedColor backgroundColor{BACKGROUND_NAME};
        XNamedColor connectionNormalColor{CONNECTION_NORMAL_NAME};
        XNamedColor roomDarkColor{ROOM_DARK_NAME};
        XNamedColor roomDarkLitColor{ROOM_NO_SUNDEATH_NAME};
        int antialiasingSamples = 0;
        bool trilinearFiltering = false;
        bool softwareOpenGL = false;

        // not saved yet:
        bool drawCharBeacons = true;
        float charBeaconScaleCutoff = 0.4f;
        float doorNameScaleCutoff = 0.4f;
        float infomarkScaleCutoff = 0.25f;
        float extraDetailScaleCutoff = 0.15f;

        MMapper::Array<int, 3> mapRadius{100, 100, 100};
        RestrictMapEnum useRestrictedMap = RestrictMapEnum::OnlyInMapMode;

        struct NODISCARD Advanced final
        {
            NamedConfig<bool> use3D{"MMAPPER_3D", true};
            NamedConfig<bool> autoTilt{"MMAPPER_AUTO_TILT", true};
            NamedConfig<bool> printPerfStats{"MMAPPER_GL_PERFSTATS", IS_DEBUG_BUILD};

            // 5..90 degrees
            FixedPoint<1> fov{50, 900, 765};
            // 0..90 degrees
            FixedPoint<1> verticalAngle{0, 900, 450};
            // -45..45 degrees
            FixedPoint<1> horizontalAngle{-450, 450, 0};
            // 1..10 rooms
            FixedPoint<1> layerHeight{10, 100, 15};

        public:
            NODISCARD ConnectionSet registerChangeCallback(ChangeMonitor::Function callback);

            Advanced();
        } advanced;

    private:
        SUBGROUP();
    } canvas;

    struct NODISCARD ColorSettings final
    {
        // TODO: save color settings
        // TODO: record which named colors require a full map update.
        XNamedColor BACKGROUND{BACKGROUND_NAME};

        XNamedColor INFOMARK_COMMENT{"infomark-comment"};
        XNamedColor INFOMARK_HERB{"infomark-herb"};
        XNamedColor INFOMARK_MOB{"infomark-mob"};
        XNamedColor INFOMARK_OBJECT{"infomark-object"};
        XNamedColor INFOMARK_RIVER{"infomark-river"};
        XNamedColor INFOMARK_ROAD{"infomark-road"};

        XNamedColor CONNECTION_NORMAL{CONNECTION_NORMAL_NAME};
        XNamedColor ROOM_DARK{ROOM_DARK_NAME};
        XNamedColor ROOM_NO_SUNDEATH{ROOM_NO_SUNDEATH_NAME};

        XNamedColor STREAM{"stream"};

        XNamedColor TRANSPARENT{".transparent"};

        XNamedColor VERTICAL_COLOR_CLIMB{"vertical-climb"};
        XNamedColor VERTICAL_COLOR_REGULAR_EXIT{"vertical-regular"};

        XNamedColor WALL_COLOR_BUG_WALL_DOOR{"wall-bug-wall-door"};
        XNamedColor WALL_COLOR_CLIMB{"wall-climb"};
        XNamedColor WALL_COLOR_FALL_DAMAGE{"wall-fall-damage"};
        XNamedColor WALL_COLOR_GUARDED{"wall-guarded"};
        XNamedColor WALL_COLOR_NO_FLEE{"wall-no-flee"};
        XNamedColor WALL_COLOR_NO_MATCH{"wall-no-match"};
        XNamedColor WALL_COLOR_NOT_MAPPED{"wall-not-mapped"};
        XNamedColor WALL_COLOR_RANDOM{"wall-random"};
        XNamedColor WALL_COLOR_REGULAR_EXIT{"wall-regular-exit"};
        XNamedColor WALL_COLOR_SPECIAL{"wall-special"};

        ColorSettings() = default;
        void resetToDefaults();

    private:
        SUBGROUP();
    } colorSettings;

    struct NODISCARD AutoLoadSettings final
    {
        bool autoLoadMap = false;
        QString fileName;
        QString lastMapDirectory;

    private:
        SUBGROUP();
    } autoLoad;

    struct NODISCARD AutoLogSettings final
    {
        QString autoLogDirectory;
        bool autoLog = false;
        AutoLoggerEnum cleanupStrategy = AutoLoggerEnum::DeleteDays;
        int deleteWhenLogsReachDays = 0;
        int deleteWhenLogsReachBytes = 0;
        bool askDelete = false;
        int rotateWhenLogsReachBytes = 0;

    private:
        SUBGROUP();
    } autoLog;

    struct NODISCARD PathMachineSettings final
    {
        double acceptBestRelative = 0.0;
        double acceptBestAbsolute = 0.0;
        double newRoomPenalty = 0.0;
        double multipleConnectionsPenalty = 0.0;
        double correctPositionBonus = 0.0;
        int maxPaths = 0;
        int matchingTolerance = 0;

    private:
        SUBGROUP();
    } pathMachine;

    struct NODISCARD GroupManagerSettings final
    {
        GroupManagerStateEnum state = GroupManagerStateEnum::Off;
        quint16 localPort = 0u;
        quint16 remotePort = 0u;
        QByteArray host;
        QByteArray charName;
        bool shareSelf = false;
        QColor color;
        bool rulesWarning = false;
        QByteArray certificate;
        QByteArray privateKey;
        QStringList authorizedSecrets;
        bool requireAuth = false;
        QByteArray geometry;
        QMap<QString, QVariant> secretMetadata;
        QString groupTellColor; // ANSI color
        bool useGroupTellAnsi256Color = false;
        bool lockGroup = false;
        bool autoStart = false;

    private:
        SUBGROUP();
    } groupManager;

    struct NODISCARD MumeClockSettings final
    {
        int64_t startEpoch = 0;
        bool display = false;

    private:
        SUBGROUP();
    } mumeClock;

    struct NODISCARD IntegratedMudClientSettings final
    {
        QString font;
        QColor foregroundColor;
        QColor backgroundColor;
        int columns = 0;
        int rows = 0;
        int linesOfScrollback = 0;
        int linesOfInputHistory = 0;
        int tabCompletionDictionarySize = 0;
        bool clearInputOnEnter = false;
        bool autoResizeTerminal = false;

    private:
        SUBGROUP();
    } integratedClient;

    struct NODISCARD InfoMarksDialog final
    {
        QByteArray geometry;

    private:
        SUBGROUP();
    } infoMarksDialog;

    struct NODISCARD RoomEditDialog final
    {
        QByteArray geometry;

    private:
        SUBGROUP();
    } roomEditDialog;

    struct NODISCARD FindRoomsDialog final
    {
        QByteArray geometry;

    private:
        SUBGROUP();
    } findRoomsDialog;

public:
    DELETE_CTORS_AND_ASSIGN_OPS(Configuration);

private:
    Configuration();
    friend Configuration &setConfig();
};

/// Must be called before you can call setConfig() or getConfig().
/// Please don't try to cheat it. Only call this function from main().
void setEnteredMain();
/// Returns a reference to the application configuration object.
NODISCARD Configuration &setConfig();
NODISCARD const Configuration &getConfig();

#undef SUBGROUP
