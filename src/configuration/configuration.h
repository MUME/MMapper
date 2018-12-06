#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara),
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

#ifndef MMAPPER_CONFIGURATION_H
#define MMAPPER_CONFIGURATION_H

#include <QByteArray>
#include <QColor>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QtCore>
#include <QtGlobal>

#include "../pandoragroup/mmapper2group.h"

enum class MapMode { PLAY, MAP, OFFLINE };
enum class Platform { Unknown, Win32, Mac, Linux };
enum class CharacterEncoding { LATIN1, UTF8, ASCII };

static inline constexpr Platform getCurrentPlatform()
{
#if defined(Q_OS_WIN)
    return Platform::Win32;
#elif defined(Q_OS_MAC)
    return Platform::Mac;
#elif defined(Q_OS_LINUX)
    return Platform::Linux;
#else
    throw std::runtime_error("unsupported platform");
#endif
}
static constexpr const Platform CURRENT_PLATFORM = getCurrentPlatform();

#if defined(MMAPPER_NO_OPENSSL) && MMAPPER_NO_OPENSSL
static constexpr const bool NO_OPEN_SSL = true;
#else
static constexpr const bool NO_OPEN_SSL = false;
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

        QStringList moveForcePatternsList{}; // string wildcart patterns, that force new move command
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
        qreal acceptBestRelative = 0.0;
        qreal acceptBestAbsolute = 0.0;
        qreal newRoomPenalty = 0.0;
        qreal multipleConnectionsPenalty = 0.0;
        qreal correctPositionBonus = 0.0;
        qint32 maxPaths = 0;
        qint32 matchingTolerance = 0;

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
    Configuration(Configuration &&) = delete;
    Configuration(const Configuration &) = delete;
    Configuration &operator=(Configuration &&) = delete;
    Configuration &operator=(const Configuration &) = delete;

private:
    explicit Configuration();
    friend Configuration &setConfig();
};

/// Returns a reference to the application configuration object.
Configuration &setConfig();
const Configuration &getConfig();

#undef SUBGROUP
#endif
