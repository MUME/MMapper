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
#include <QFont>
#include <QString>
#include <QStringList>
#include <QtCore>
#include <QtGlobal>

#include "../pandoragroup/mmapper2group.h"

enum class MapMode { PLAY, MAP, OFFLINE };
enum class Platform { Unknown, Win32, Mac, Linux };

static inline constexpr Platform getCurrentPlatform()
{
#if defined(Q_OS_WIN)
    return Platform::Win32;
#elif defined(Q_OS_MAC)
    return Platform::Mac;
#elif defined(Q_OS_LINUX)
    return Platform::Linux;
#else
    return Platform::Unknown;
#endif
}
static constexpr const Platform CURRENT_PLATFORM = getCurrentPlatform();

class Configuration
{
public:
    void read();
    void write() const;
    bool isChanged() const;

public:
    void setFirstRun(bool value)
    {
        general.firstRun = value;
        change();
    }
    void setWindowGeometry(QByteArray geometry)
    {
        general.windowGeometry = geometry;
        change();
    }
    void setWindowState(QByteArray state)
    {
        general.windowState = state;
        change();
    }
    void setAlwaysOnTop(bool b)
    {
        general.alwaysOnTop = b;
        change();
    }

    struct GeneralSettings
    {
        bool firstRun = false;
        QByteArray windowGeometry{};
        QByteArray windowState{};
        bool alwaysOnTop = false;
        MapMode mapMode = MapMode::PLAY;
        bool noSplash = false;
        bool noLaunchPanel = false;
        bool proxyThreaded = false;

    private:
        friend class Configuration;
        void read(QSettings &conf);
        void write(QSettings &conf) const;

    } general{};
    struct ConnectionSettings
    {
        QString remoteServerName{}; /// Remote host and port settings
        quint16 remotePort = 0u;
        quint16 localPort = 0u; /// Port to bind to on local machine
        bool tlsEncryption = false;

    private:
        friend class Configuration;
        void read(QSettings &conf);
        void write(QSettings &conf) const;
    } connection{};

    bool m_autoLog = false;  // enables log to file
    QString m_logFileName{}; // file name to log

    struct ParserSettings
    {
        QString roomNameColor{}; // ANSI room name color
        QString roomDescColor{}; // ANSI room descriptions color
        bool removeXmlTags = false;

        QStringList moveForcePatternsList{}; // string wildcart patterns, that force new move command
        QStringList noDescriptionPatternsList{};

        QByteArray promptPattern{};
        QByteArray loginPattern{};
        QByteArray passwordPattern{};
        QByteArray menuPromptPattern{};

        bool utf8Charset = false;

    private:
        friend class Configuration;
        void read(QSettings &conf);
        void write(QSettings &conf) const;
    } parser;

    struct MumeClientProtocolSettings
    {
        bool IAC_prompt_parser = false;
        bool remoteEditing = false;
        bool internalRemoteEditor = false;
        QString externalRemoteEditorCommand{};

    private:
        friend class Configuration;
        void read(QSettings &conf);
        void write(QSettings &conf) const;
    } mumeClientProtocol{};

    struct MumeNativeSettings
    {
        /* serialized */
        bool emulatedExits = false;
        bool showHiddenExitFlags = false;
        bool showNotes = false;

    private:
        friend class Configuration;
        void read(QSettings &conf);
        void write(QSettings &conf) const;
    } mumeNative{};

    struct CanvasSettings
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
        friend class Configuration;
        void read(QSettings &conf);
        void write(QSettings &conf) const;
    } canvas{};

    struct AutoLoadSettings
    {
        bool autoLoadMap = false;
        QString fileName{};
        QString lastMapDirectory{};

    private:
        friend class Configuration;
        void read(QSettings &conf);
        void write(QSettings &conf) const;
    } autoLoad{};

    struct PathMachineSettings
    {
        qreal acceptBestRelative = 0.0;
        qreal acceptBestAbsolute = 0.0;
        qreal newRoomPenalty = 0.0;
        qreal multipleConnectionsPenalty = 0.0;
        qreal correctPositionBonus = 0.0;
        quint32 maxPaths = 0u;
        quint32 matchingTolerance = 0u;

    private:
        friend class Configuration;
        void read(QSettings &conf);
        void write(QSettings &conf) const;
    } pathMachine{};

    struct GroupManagerSettings
    {
        GroupManagerState state = GroupManagerState::Off;
        int localPort = 0;
        int remotePort = 0;
        QByteArray host{};
        QByteArray charName{};
        bool shareSelf = false;
        QColor color{};
        bool rulesWarning = false;

    private:
        friend class Configuration;
        void read(QSettings &conf);
        void write(QSettings &conf) const;
    } groupManager{};

    struct MumeClockSettings
    {
        int startEpoch = 0;
        bool display = false;

    private:
        friend class Configuration;
        void read(QSettings &conf);
        void write(QSettings &conf) const;
    } mumeClock{};

    struct IntegratedMudClientSettings
    {
        QFont font{};
        QColor foregroundColor{};
        QColor backgroundColor{};
        int columns = 0;
        int rows = 0;
        int linesOfScrollback = 0;
        int linesOfInputHistory = 0;
        int tabCompletionDictionarySize = 0;
        bool clearInputOnEnter = false;
        bool autoResizeTerminal = false;

    private:
        friend class Configuration;
        void read(QSettings &conf);
        void write(QSettings &conf) const;
    } integratedClient{};

public:
    Configuration(Configuration &&) = delete;
    Configuration(const Configuration &) = delete;
    Configuration &operator=(Configuration &&) = delete;
    Configuration &operator=(const Configuration &) = delete;

private:
    explicit Configuration();
    bool configurationChanged = false;
    void change() { configurationChanged = true; }

    friend Configuration &Config();

public:
    QByteArray readIntegratedMudClientGeometry();
    void writeIntegratedMudClientGeometry(const QByteArray &geometry) const;

public:
    QByteArray readInfoMarksEditDlgGeometry();
    void writeInfoMarksEditDlgGeometry(const QByteArray &geometry) const;

public:
    QByteArray readRoomEditAttrGeometry();
    void writeRoomEditAttrDlgGeometry(const QByteArray &geometry) const;
};

/// Returns a reference to the application configuration object.
Configuration &Config();

#endif
