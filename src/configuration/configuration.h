#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Array.h"
#include "../global/ConfigConsts-Computed.h"
#include "../global/ConfigConsts.h"
#include "../global/ConfigEnums.h"
#include "../global/Consts.h"
#include "../global/FixedPoint.h"
#include "../global/NamedColors.h"
#include "../global/RuleOf5.h"
#include "../global/Signal2.h"
#include "GroupConfig.h"
#include "NamedConfig.h"

#include <memory>
#include <string_view>

#include <QByteArray>
#include <QColor>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtCore>
#include <QtGlobal>

#undef TRANSPARENT // Bad dog, Microsoft; bad dog!!!

#define SUBGROUP() \
    friend class Configuration; \
    void read(const QSettings &conf); \
    void write(QSettings &conf) const

class NODISCARD Configuration final
{
public:
    void read();
    void write() const;
    void reset();

    void readFrom(QSettings &conf);
    void writeTo(QSettings &conf) const;

public:
    struct NODISCARD GeneralSettings final
    {
    private:
        ChangeMonitor m_changeMonitor;
        ThemeEnum m_theme = ThemeEnum::System;

    public:
        explicit GeneralSettings() = default;
        ~GeneralSettings() = default;
        DELETE_CTORS_AND_ASSIGN_OPS(GeneralSettings);

    public:
        NODISCARD ThemeEnum getTheme() const { return m_theme; }
        void setTheme(const ThemeEnum theme)
        {
            m_theme = theme;
            m_changeMonitor.notifyAll();
        }

        void registerChangeCallback(const ChangeMonitor::Lifetime &lifetime,
                                    const ChangeMonitor::Function &callback)
        {
            return m_changeMonitor.registerChangeCallback(lifetime, callback);
        }

    public:
        bool firstRun = false;
        QByteArray windowGeometry;
        QByteArray windowState;
        bool alwaysOnTop = false;
        bool showStatusBar = true;
        bool showScrollBars = true;
        bool showMenuBar = true;
        MapModeEnum mapMode = MapModeEnum::PLAY;
        bool checkForUpdate = true;
        CharacterEncodingEnum characterEncoding = CharacterEncodingEnum::LATIN1;

    private:
        SUBGROUP();
    } general;

    struct NODISCARD ConnectionSettings final
    {
        QString remoteServerName; /// Remote host and port settings
        uint16_t remotePort = 0u;
        uint16_t localPort = 0u; /// Port to bind to on local machine
        bool tlsEncryption = false;
        bool proxyConnectionStatus = false;
        bool proxyListensOnAnyInterface = false;

    private:
        SUBGROUP();
    } connection;

    struct NODISCARD ParserSettings final
    {
        QString roomNameColor; // ANSI room name color
        QString roomDescColor; // ANSI room descriptions color
        char prefixChar = char_consts::C_UNDERSCORE;
        bool encodeEmoji = true;
        bool decodeEmoji = true;

    private:
        SUBGROUP();
    } parser;

    struct NODISCARD MumeClientProtocolSettings final
    {
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

#define XFOREACH_CANVAS_NAMED_COLOR_OPTIONS(X) \
    X(backgroundColor, BACKGROUND) \
    X(connectionNormalColor, CONNECTION_NORMAL) \
    X(roomDarkColor, ROOM_DARK) \
    X(roomDarkLitColor, ROOM_NO_SUNDEATH)

    struct CanvasNamedColorOptions;
    struct NODISCARD ResolvedCanvasNamedColorOptions final
    {
#define X_DECL(_id, _name) Color _id;
        XFOREACH_CANVAS_NAMED_COLOR_OPTIONS(X_DECL)
#undef X_DECL
        void setFrom(const CanvasNamedColorOptions &);
    };

    struct NODISCARD CanvasNamedColorOptions
    {
#define X_DECL(_id, _name) XNamedColor _id{NamedColorEnum::_name};
        XFOREACH_CANVAS_NAMED_COLOR_OPTIONS(X_DECL)
#undef X_DECL

        NODISCARD std::shared_ptr<const ResolvedCanvasNamedColorOptions> clone() const
        {
            auto pResult = std::make_shared<ResolvedCanvasNamedColorOptions>();
            auto &result = deref(pResult);
            result.setFrom(*this);
            return pResult;
        }
    };

    struct NODISCARD CanvasSettings final : public CanvasNamedColorOptions
    {
        NamedConfig<int> antialiasingSamples{"ANTIALIASING_SAMPLES", 0};
        NamedConfig<bool> trilinearFiltering{"TRILINEAR_FILTERING", true};
        NamedConfig<bool> showMissingMapId{"SHOW_MISSING_MAPID", false};
        NamedConfig<bool> showUnsavedChanges{"SHOW_UNSAVED_CHANGES", false};
        NamedConfig<bool> showUnmappedExits{"SHOW_UNMAPPED_EXITS", false};
        bool drawUpperLayersTextured = false;
        bool drawDoorNames = false;
        bool softwareOpenGL = false;
        QString resourcesDirectory;

        // not saved yet:
        bool drawCharBeacons = true;
        float charBeaconScaleCutoff = 0.4f;
        float doorNameScaleCutoff = 0.4f;
        float infomarkScaleCutoff = 0.25f;
        float extraDetailScaleCutoff = 0.15f;

        MMapper::Array<int, 3> mapRadius{100, 100, 100};

        struct NODISCARD Advanced final
        {
            NamedConfig<bool> use3D{"MMAPPER_3D", true};
            NamedConfig<bool> autoTilt{"MMAPPER_AUTO_TILT", true};
            NamedConfig<bool> printPerfStats{"MMAPPER_GL_PERFSTATS", IS_DEBUG_BUILD};

            // 5..90 degrees
            FixedPoint<1> fov{50, 900, 765};
            // 0..90 degrees
            FixedPoint<1> verticalAngle{0, 900, 450};
            // -180..180 degrees
            FixedPoint<1> horizontalAngle{-1800, 1800, 0};
            // 1..10 rooms
            FixedPoint<1> layerHeight{10, 100, 15};

        public:
            void registerChangeCallback(const ChangeMonitor::Lifetime &lifetime,
                                        const ChangeMonitor::Function &callback);

            Advanced();
        } advanced;

    private:
        SUBGROUP();
    } canvas;

    struct NODISCARD NamedColorOptions;
    struct NODISCARD ResolvedNamedColorOptions final
    {
#define X_DECL(_id, _name) XNamedColor _id{NamedColorEnum::_id};
        XFOREACH_NAMED_COLOR_OPTIONS(X_DECL)
#undef X_DECL

        void setFrom(const NamedColorOptions &);
    };

    struct NODISCARD NamedColorOptions
    {
#define X_DECL(_id, _name) XNamedColor _id{NamedColorEnum::_id};
        XFOREACH_NAMED_COLOR_OPTIONS(X_DECL)
#undef X_DECL
        NamedColorOptions() = default;
        void resetToDefaults();

        NODISCARD std::shared_ptr<const ResolvedNamedColorOptions> clone() const
        {
            auto pResult = std::make_shared<ResolvedNamedColorOptions>();
            auto &result = deref(pResult);
            result.setFrom(*this);
            return pResult;
        }
    };

    struct NODISCARD ColorSettings final : public NamedColorOptions
    {
        // TODO: save color settings
        // TODO: record which named colors require a full map update.

    private:
        SUBGROUP();
    } colorSettings;

    struct NODISCARD AccountSettings final
    {
        QString accountName;
        bool accountPassword = false;
        bool rememberLogin = false;

    private:
        SUBGROUP();
    } account;

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
        QColor color;
        QColor npcColor;
        bool npcColorOverride = false;
        bool npcSortBottom = false;
        bool npcHide = false;

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

    struct NODISCARD AdventurePanelSettings final
    {
    private:
        ChangeMonitor m_changeMonitor;
        bool m_displayXPStatus = false;

    public:
        explicit AdventurePanelSettings() = default;
        ~AdventurePanelSettings() = default;
        DELETE_CTORS_AND_ASSIGN_OPS(AdventurePanelSettings);

    public:
        NODISCARD bool getDisplayXPStatus() const { return m_displayXPStatus; }
        void setDisplayXPStatus(const bool display)
        {
            m_displayXPStatus = display;
            m_changeMonitor.notifyAll();
        }

        void registerChangeCallback(const ChangeMonitor::Lifetime &lifetime,
                                    const ChangeMonitor::Function &callback)
        {
            return m_changeMonitor.registerChangeCallback(lifetime, callback);
        }

    private:
        SUBGROUP();
    } adventurePanel;

    struct NODISCARD IntegratedMudClientSettings final
    {
        QString font;
        QColor foregroundColor;
        QColor backgroundColor;
        QString commandSeparator;
        int columns = 0;
        int rows = 0;
        int linesOfScrollback = 0;
        int linesOfInputHistory = 0;
        int tabCompletionDictionarySize = 0;
        bool clearInputOnEnter = false;
        bool autoResizeTerminal = false;
        int linesOfPeekPreview = 0;
        bool audibleBell = false;
        bool visualBell = false;
        bool useCommandSeparator = false;

    private:
        SUBGROUP();
    } integratedClient;

    struct NODISCARD RoomPanelSettings final
    {
        QByteArray geometry;

    private:
        SUBGROUP();
    } roomPanel;

    struct NODISCARD InfomarksDialog final
    {
        QByteArray geometry;

    private:
        SUBGROUP();
    } infomarksDialog;

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

    GroupConfig hotkeys;

public:
    DELETE_CTORS_AND_ASSIGN_OPS(Configuration);

private:
    Configuration();
    friend Configuration &setConfig();
};

#undef SUBGROUP

/// Must be called before you can call setConfig() or getConfig().
/// Please don't try to cheat it. Only call this function from main().
void setEnteredMain();
/// Returns a reference to the application configuration object.
NODISCARD Configuration &setConfig();
NODISCARD const Configuration &getConfig();

using SharedCanvasNamedColorOptions
    = std::shared_ptr<const Configuration::ResolvedCanvasNamedColorOptions>;
using SharedNamedColorOptions = std::shared_ptr<const Configuration::ResolvedNamedColorOptions>;

const Configuration::ResolvedCanvasNamedColorOptions &getCanvasNamedColorOptions();
const Configuration::ResolvedNamedColorOptions &getNamedColorOptions();

struct NODISCARD ThreadLocalNamedColorRaii final
{
    explicit ThreadLocalNamedColorRaii(SharedCanvasNamedColorOptions canvasNamedColorOptions,
                                       SharedNamedColorOptions namedColorOptions);
    ~ThreadLocalNamedColorRaii();
};
