#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QByteArray>
#include <QColor>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QtCore>
#include <QtGlobal>

#include "../global/RuleOf5.h"
#include "../pandoragroup/mmapper2group.h"

enum class MapMode { PLAY, MAP, OFFLINE };
enum class Platform { Unknown, Windows, Mac, Linux };
enum class CharacterEncoding { LATIN1, UTF8, ASCII };
enum class Environment { Unknown, Env32Bit, Env64Bit };

// Do not call this directly; use CURRENT_PLATFORM.
static inline constexpr Platform getCurrentPlatform()
{
#if defined(Q_OS_WIN)
    return Platform::Windows;
#elif defined(Q_OS_MAC)
    return Platform::Mac;
#elif defined(Q_OS_LINUX)
    return Platform::Linux;
#else
    throw std::runtime_error("unsupported platform");
#endif
}
static constexpr const Platform CURRENT_PLATFORM = getCurrentPlatform();

// Do not call this directly; use CURRENT_ENVIRONMENT.
static inline constexpr Environment getCurrentEnvironment()
{
#if Q_PROCESSOR_WORDSIZE == 4
    return Environment::Env32Bit;
#elif Q_PROCESSOR_WORDSIZE == 8
    return Environment::Env64Bit;
#else
    throw std::runtime_error("unsupported environment");
#endif
}
static constexpr const Environment CURRENT_ENVIRONMENT = getCurrentEnvironment();

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

#define SUBGROUP() \
    friend class Configuration; \
    void read(QSettings &conf); \
    void write(QSettings &conf) const

class Configuration final
{
public:
    void read();
    void write() const;
    void reset();

public:
    struct GeneralSettings final
    {
        bool firstRun = false;
        QByteArray windowGeometry{};
        QByteArray windowState{};
        bool alwaysOnTop = false;
        MapMode mapMode = MapMode::PLAY;
        bool noSplash = false;
        bool noLaunchPanel = false;
        bool checkForUpdate = true;
        CharacterEncoding characterEncoding = CharacterEncoding::LATIN1;

    private:
        SUBGROUP();
    } general{};

    struct ConnectionSettings final
    {
        QString remoteServerName{}; /// Remote host and port settings
        quint16 remotePort = 0u;
        quint16 localPort = 0u; /// Port to bind to on local machine
        bool tlsEncryption = false;
        bool proxyThreaded = false;
        bool proxyConnectionStatus = false;

    private:
        SUBGROUP();
    } connection{};

    struct ParserSettings final
    {
        QString roomNameColor{}; // ANSI room name color
        QString roomDescColor{}; // ANSI room descriptions color
        bool removeXmlTags = false;
        char prefixChar = '_';
        QStringList noDescriptionPatternsList{};

    private:
        SUBGROUP();
    } parser;

    struct MumeClientProtocolSettings final
    {
        bool remoteEditing = false;
        bool internalRemoteEditor = false;
        QString externalRemoteEditorCommand{};

    private:
        SUBGROUP();
    } mumeClientProtocol{};

    struct MumeNativeSettings final
    {
        /* serialized */
        bool emulatedExits = false;
        bool showHiddenExitFlags = false;
        bool showNotes = false;

    private:
        SUBGROUP();
    } mumeNative{};

    struct CanvasSettings final
    {
        bool showUpdated = false;
        bool drawNotMappedExits = false;
        bool drawNoMatchExits = false;
        bool drawUpperLayersTextured = false;
        bool drawDoorNames = false;
        QColor backgroundColor{};
        QColor roomDarkColor{};
        QColor roomDarkLitColor{};
        int antialiasingSamples = 0;
        bool trilinearFiltering = false;
        bool softwareOpenGL = false;

    private:
        SUBGROUP();
    } canvas{};

    struct AutoLoadSettings final
    {
        bool autoLoadMap = false;
        QString fileName{};
        QString lastMapDirectory{};

    private:
        SUBGROUP();
    } autoLoad{};

    struct PathMachineSettings final
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
    } pathMachine{};

    struct GroupManagerSettings final
    {
        GroupManagerState state = GroupManagerState::Off;
        quint16 localPort = 0u;
        quint16 remotePort = 0u;
        QByteArray host{};
        QByteArray charName{};
        bool shareSelf = false;
        QColor color{};
        bool rulesWarning = false;
        QByteArray certificate{};
        QByteArray privateKey{};
        QStringList authorizedSecrets{};
        bool requireAuth = false;
        QByteArray geometry{};
        QMap<QString, QVariant> secretMetadata{};
        QString groupTellColor{}; // ANSI color
        bool useGroupTellAnsi256Color = false;
        bool lockGroup = false;
        bool autoStart = false;

    private:
        SUBGROUP();
    } groupManager{};

    struct MumeClockSettings final
    {
        int64_t startEpoch = 0;
        bool display = false;

    private:
        SUBGROUP();
    } mumeClock{};

    struct IntegratedMudClientSettings final
    {
        QString font{};
        QColor foregroundColor{};
        QColor backgroundColor{};
        int columns = 0;
        int rows = 0;
        int linesOfScrollback = 0;
        int linesOfInputHistory = 0;
        int tabCompletionDictionarySize = 0;
        bool clearInputOnEnter = false;
        bool autoResizeTerminal = false;
        QByteArray geometry{};

    private:
        SUBGROUP();
    } integratedClient{};

    struct InfoMarksDialog final
    {
        QByteArray geometry{};

    private:
        SUBGROUP();
    } infoMarksDialog{};

    struct RoomEditDialog final
    {
        QByteArray geometry{};

    private:
        SUBGROUP();
    } roomEditDialog{};

public:
    DELETE_CTORS_AND_ASSIGN_OPS(Configuration);

private:
    Configuration();
    friend Configuration &setConfig();
};

/// Returns a reference to the application configuration object.
Configuration &setConfig();
const Configuration &getConfig();

#undef SUBGROUP
