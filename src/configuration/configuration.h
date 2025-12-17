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

// Forward declaration for InfomarkClassEnum
enum class InfomarkClassEnum : uint8_t;

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

public:
    struct NODISCARD GeneralSettings final
    {
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
    } general{};

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
        bool enableYellFallbackParsing = true; // Parse yells from game text when GMCP unavailable

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

    static constexpr const std::string_view BACKGROUND_NAME = "background";
    static constexpr const std::string_view CONNECTION_NORMAL_NAME = "connection-normal";
    static constexpr const std::string_view ROOM_DARK_NAME = "room-dark";
    static constexpr const std::string_view ROOM_NO_SUNDEATH_NAME = "room-no-sundeath";

#define XFOREACH_CANVAS_NAMED_COLOR_OPTIONS(X) \
    X(backgroundColor, BACKGROUND_NAME) \
    X(connectionNormalColor, CONNECTION_NORMAL_NAME) \
    X(roomDarkColor, ROOM_DARK_NAME) \
    X(roomDarkLitColor, ROOM_NO_SUNDEATH_NAME)

    struct NODISCARD CanvasNamedColorOptions
    {
#define X_DECL(_id, _name) XNamedColor _id{_name};
        XFOREACH_CANVAS_NAMED_COLOR_OPTIONS(X_DECL)
#undef X_DECL

        NODISCARD std::shared_ptr<const CanvasNamedColorOptions> clone() const
        {
            auto pResult = std::make_shared<CanvasNamedColorOptions>();
            auto &result = deref(pResult);
#define X_CLONE(_id, _name) result._id = this->_id.getColor();
            XFOREACH_CANVAS_NAMED_COLOR_OPTIONS(X_CLONE)
#undef X_CLONE
            return pResult;
        }
    };

    struct NODISCARD CanvasSettings final : public CanvasNamedColorOptions
    {
        NamedConfig<bool> showMissingMapId{"SHOW_MISSING_MAPID", false};
        NamedConfig<bool> showUnsavedChanges{"SHOW_UNSAVED_CHANGES", false};
        NamedConfig<bool> showUnmappedExits{"SHOW_UNMAPPED_EXITS", false};
        bool drawUpperLayersTextured = false;
        bool drawDoorNames = false;
        int antialiasingSamples = 0;
        bool trilinearFiltering = false;
        bool softwareOpenGL = false;
        QString resourcesDirectory;
        TextureSetEnum textureSet = TextureSetEnum::MODERN;
        bool enableSeasonalTextures = true;
        float layerTransparency = 1.0f;  // 0.0 = only focused layer, 1.0 = maximum transparency
        bool enableRadialTransparency = true;  // Enable radial transparency zones on upper layers

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

            // Background image settings
            bool useBackgroundImage = false;
            QString backgroundImagePath;
            int backgroundFitMode = 0;  // BackgroundFitModeEnum::FIT
            float backgroundOpacity = 1.0f;
            float backgroundFocusedScale = 1.0f;  // Scale factor for FOCUSED mode (0.1 to 10.0)
            float backgroundFocusedOffsetX = 0.0f;  // X offset for FOCUSED mode (-1000 to 1000)
            float backgroundFocusedOffsetY = 0.0f;  // Y offset for FOCUSED mode (-1000 to 1000)

            // 5..90 degrees
            FixedPoint<1> fov{50, 900, 765};
            // 0..90 degrees
            FixedPoint<1> verticalAngle{0, 900, 450};
            // -180..180 degrees (full rotation)
            FixedPoint<1> horizontalAngle{-1800, 1800, 0};
            // 1..10 rooms
            FixedPoint<1> layerHeight{10, 100, 15};

        public:
            void registerChangeCallback(const ChangeMonitor::Lifetime &lifetime,
                                        const ChangeMonitor::Function &callback);

            Advanced();
        } advanced;

        struct NODISCARD VisibilityFilter final
        {
            NamedConfig<bool> generic{"VISIBLE_MARKER_GENERIC", true};
            NamedConfig<bool> herb{"VISIBLE_MARKER_HERB", true};
            NamedConfig<bool> river{"VISIBLE_MARKER_RIVER", true};
            NamedConfig<bool> place{"VISIBLE_MARKER_PLACE", true};
            NamedConfig<bool> mob{"VISIBLE_MARKER_MOB", true};
            NamedConfig<bool> comment{"VISIBLE_MARKER_COMMENT", true};
            NamedConfig<bool> road{"VISIBLE_MARKER_ROAD", true};
            NamedConfig<bool> object{"VISIBLE_MARKER_OBJECT", true};
            NamedConfig<bool> action{"VISIBLE_MARKER_ACTION", true};
            NamedConfig<bool> locality{"VISIBLE_MARKER_LOCALITY", true};
            NamedConfig<bool> connections{"VISIBLE_CONNECTIONS", true};

        public:
            NODISCARD bool isVisible(InfomarkClassEnum markerClass) const;
            void setVisible(InfomarkClassEnum markerClass, bool visible);
            NODISCARD bool isConnectionsVisible() const { return connections.get(); }
            void setConnectionsVisible(bool visible) { connections.set(visible); }
            void showAll();
            void hideAll();
            void registerChangeCallback(const ChangeMonitor::Lifetime &lifetime,
                                       const ChangeMonitor::Function &callback);

            VisibilityFilter();
        } visibilityFilter;

    private:
        SUBGROUP();
    } canvas;

    struct NODISCARD Hotkeys final
    {
        // File operations
        NamedConfig<QString> fileOpen{"HOTKEY_FILE_OPEN", "Ctrl+O"};
        NamedConfig<QString> fileSave{"HOTKEY_FILE_SAVE", "Ctrl+S"};
        NamedConfig<QString> fileReload{"HOTKEY_FILE_RELOAD", "Ctrl+R"};
        NamedConfig<QString> fileQuit{"HOTKEY_FILE_QUIT", "Ctrl+Q"};

        // Edit operations
        NamedConfig<QString> editUndo{"HOTKEY_EDIT_UNDO", "Ctrl+Z"};
        NamedConfig<QString> editRedo{"HOTKEY_EDIT_REDO", "Ctrl+Y"};
        NamedConfig<QString> editPreferences{"HOTKEY_EDIT_PREFERENCES", "Ctrl+P"};
        NamedConfig<QString> editPreferencesAlt{"HOTKEY_EDIT_PREFERENCES_ALT", "Esc"};
        NamedConfig<QString> editFindRooms{"HOTKEY_EDIT_FIND_ROOMS", "Ctrl+F"};
        NamedConfig<QString> editRoom{"HOTKEY_EDIT_ROOM", "Ctrl+E"};

        // View operations
        NamedConfig<QString> viewZoomIn{"HOTKEY_VIEW_ZOOM_IN", ""};
        NamedConfig<QString> viewZoomOut{"HOTKEY_VIEW_ZOOM_OUT", ""};
        NamedConfig<QString> viewZoomReset{"HOTKEY_VIEW_ZOOM_RESET", "Ctrl+0"};
        NamedConfig<QString> viewLayerUp{"HOTKEY_VIEW_LAYER_UP", ""};
        NamedConfig<QString> viewLayerDown{"HOTKEY_VIEW_LAYER_DOWN", ""};
        NamedConfig<QString> viewLayerReset{"HOTKEY_VIEW_LAYER_RESET", ""};

        // View toggles
        NamedConfig<QString> viewRadialTransparency{"HOTKEY_VIEW_RADIAL_TRANSPARENCY", ""};
        NamedConfig<QString> viewStatusBar{"HOTKEY_VIEW_STATUS_BAR", ""};
        NamedConfig<QString> viewScrollBars{"HOTKEY_VIEW_SCROLL_BARS", ""};
        NamedConfig<QString> viewMenuBar{"HOTKEY_VIEW_MENU_BAR", ""};
        NamedConfig<QString> viewAlwaysOnTop{"HOTKEY_VIEW_ALWAYS_ON_TOP", ""};

        // Side panels
        NamedConfig<QString> panelLog{"HOTKEY_PANEL_LOG", "Ctrl+L"};
        NamedConfig<QString> panelClient{"HOTKEY_PANEL_CLIENT", ""};
        NamedConfig<QString> panelGroup{"HOTKEY_PANEL_GROUP", ""};
        NamedConfig<QString> panelRoom{"HOTKEY_PANEL_ROOM", ""};
        NamedConfig<QString> panelAdventure{"HOTKEY_PANEL_ADVENTURE", ""};
        NamedConfig<QString> panelDescription{"HOTKEY_PANEL_DESCRIPTION", ""};
        NamedConfig<QString> panelComms{"HOTKEY_PANEL_COMMS", ""};

        // Mouse modes
        NamedConfig<QString> modeMoveMap{"HOTKEY_MODE_MOVE_MAP", ""};
        NamedConfig<QString> modeRaypick{"HOTKEY_MODE_RAYPICK", ""};
        NamedConfig<QString> modeSelectRooms{"HOTKEY_MODE_SELECT_ROOMS", ""};
        NamedConfig<QString> modeSelectMarkers{"HOTKEY_MODE_SELECT_MARKERS", ""};
        NamedConfig<QString> modeSelectConnection{"HOTKEY_MODE_SELECT_CONNECTION", ""};
        NamedConfig<QString> modeCreateMarker{"HOTKEY_MODE_CREATE_MARKER", ""};
        NamedConfig<QString> modeCreateRoom{"HOTKEY_MODE_CREATE_ROOM", ""};
        NamedConfig<QString> modeCreateConnection{"HOTKEY_MODE_CREATE_CONNECTION", ""};
        NamedConfig<QString> modeCreateOnewayConnection{"HOTKEY_MODE_CREATE_ONEWAY_CONNECTION", ""};

        // Room operations
        NamedConfig<QString> roomCreate{"HOTKEY_ROOM_CREATE", ""};
        NamedConfig<QString> roomMoveUp{"HOTKEY_ROOM_MOVE_UP", ""};
        NamedConfig<QString> roomMoveDown{"HOTKEY_ROOM_MOVE_DOWN", ""};
        NamedConfig<QString> roomMergeUp{"HOTKEY_ROOM_MERGE_UP", ""};
        NamedConfig<QString> roomMergeDown{"HOTKEY_ROOM_MERGE_DOWN", ""};
        NamedConfig<QString> roomDelete{"HOTKEY_ROOM_DELETE", "Del"};
        NamedConfig<QString> roomConnectNeighbors{"HOTKEY_ROOM_CONNECT_NEIGHBORS", ""};
        NamedConfig<QString> roomMoveToSelected{"HOTKEY_ROOM_MOVE_TO_SELECTED", ""};
        NamedConfig<QString> roomUpdateSelected{"HOTKEY_ROOM_UPDATE_SELECTED", ""};

    private:
        SUBGROUP();
    } hotkeys;

    struct NODISCARD CommsSettings final
    {
        // Colors for each communication type
        NamedConfig<QColor> tellColor{"COMMS_TELL_COLOR", QColor(Qt::cyan)};
        NamedConfig<QColor> whisperColor{"COMMS_WHISPER_COLOR", QColor(135, 206, 250)};  // Light sky blue
        NamedConfig<QColor> groupColor{"COMMS_GROUP_COLOR", QColor(Qt::green)};
        NamedConfig<QColor> askColor{"COMMS_ASK_COLOR", QColor(Qt::yellow)};
        NamedConfig<QColor> sayColor{"COMMS_SAY_COLOR", QColor(Qt::white)};
        NamedConfig<QColor> emoteColor{"COMMS_EMOTE_COLOR", QColor(Qt::magenta)};
        NamedConfig<QColor> socialColor{"COMMS_SOCIAL_COLOR", QColor(255, 182, 193)};  // Light pink
        NamedConfig<QColor> yellColor{"COMMS_YELL_COLOR", QColor(Qt::red)};
        NamedConfig<QColor> narrateColor{"COMMS_NARRATE_COLOR", QColor(255, 165, 0)};  // Orange
        NamedConfig<QColor> prayColor{"COMMS_PRAY_COLOR", QColor(173, 216, 230)};  // Light blue
        NamedConfig<QColor> shoutColor{"COMMS_SHOUT_COLOR", QColor(139, 0, 0)};  // Dark red
        NamedConfig<QColor> singColor{"COMMS_SING_COLOR", QColor(144, 238, 144)};  // Light green
        NamedConfig<QColor> backgroundColor{"COMMS_BG_COLOR", QColor(Qt::black)};

        // Talker colors (based on GMCP Comm.Channel talker-type)
        NamedConfig<QColor> talkerYouColor{"COMMS_TALKER_YOU_COLOR", QColor(255, 215, 0)};  // Gold
        NamedConfig<QColor> talkerPlayerColor{"COMMS_TALKER_PLAYER_COLOR", QColor(Qt::white)};
        NamedConfig<QColor> talkerNpcColor{"COMMS_TALKER_NPC_COLOR", QColor(192, 192, 192)};  // Silver/Gray
        NamedConfig<QColor> talkerAllyColor{"COMMS_TALKER_ALLY_COLOR", QColor(0, 255, 0)};  // Bright green
        NamedConfig<QColor> talkerNeutralColor{"COMMS_TALKER_NEUTRAL_COLOR", QColor(255, 255, 0)};  // Yellow
        NamedConfig<QColor> talkerEnemyColor{"COMMS_TALKER_ENEMY_COLOR", QColor(255, 0, 0)};  // Red

        // Font styling options
        NamedConfig<bool> yellAllCaps{"COMMS_YELL_ALL_CAPS", true};
        NamedConfig<bool> whisperItalic{"COMMS_WHISPER_ITALIC", true};
        NamedConfig<bool> emoteItalic{"COMMS_EMOTE_ITALIC", true};

        // Display options
        NamedConfig<bool> showTimestamps{"COMMS_SHOW_TIMESTAMPS", false};
        NamedConfig<bool> saveLogOnExit{"COMMS_SAVE_LOG_ON_EXIT", false};
        NamedConfig<QString> logDirectory{"COMMS_LOG_DIR", ""};

        // Tab muting (acts as a filter)
        NamedConfig<bool> muteDirectTab{"COMMS_MUTE_DIRECT", false};
        NamedConfig<bool> muteLocalTab{"COMMS_MUTE_LOCAL", false};
        NamedConfig<bool> muteGlobalTab{"COMMS_MUTE_GLOBAL", false};

    private:
        SUBGROUP();
    } comms;

#define XFOREACH_NAMED_COLOR_OPTIONS(X) \
    X(BACKGROUND, BACKGROUND_NAME) \
    X(CONNECTION_NORMAL, CONNECTION_NORMAL_NAME) \
    X(HIGHLIGHT_NEEDS_SERVER_ID, "highlight-needs-server-id") \
    X(HIGHLIGHT_UNSAVED, "highlight-unsaved") \
    X(HIGHLIGHT_TEMPORARY, "highlight-temporary") \
    X(INFOMARK_COMMENT, "infomark-comment") \
    X(INFOMARK_HERB, "infomark-herb") \
    X(INFOMARK_MOB, "infomark-mob") \
    X(INFOMARK_OBJECT, "infomark-object") \
    X(INFOMARK_RIVER, "infomark-river") \
    X(INFOMARK_ROAD, "infomark-road") \
    X(ROOM_DARK, ROOM_DARK_NAME) \
    X(ROOM_NO_SUNDEATH, ROOM_NO_SUNDEATH_NAME) \
    X(STREAM, "stream") \
    X(TRANSPARENT, ".transparent") \
    X(VERTICAL_COLOR_CLIMB, "vertical-climb") \
    X(VERTICAL_COLOR_REGULAR_EXIT, "vertical-regular") \
    X(WALL_COLOR_BUG_WALL_DOOR, "wall-bug-wall-door") \
    X(WALL_COLOR_CLIMB, "wall-climb") \
    X(WALL_COLOR_FALL_DAMAGE, "wall-fall-damage") \
    X(WALL_COLOR_GUARDED, "wall-guarded") \
    X(WALL_COLOR_NO_FLEE, "wall-no-flee") \
    X(WALL_COLOR_NO_MATCH, "wall-no-match") \
    X(WALL_COLOR_NOT_MAPPED, "wall-not-mapped") \
    X(WALL_COLOR_RANDOM, "wall-random") \
    X(WALL_COLOR_REGULAR_EXIT, "wall-regular-exit") \
    X(WALL_COLOR_SPECIAL, "wall-special")

    struct NODISCARD NamedColorOptions
    {
#define X_DECL(_id, _name) XNamedColor _id{_name};
        XFOREACH_NAMED_COLOR_OPTIONS(X_DECL)
#undef X_DECL
        NamedColorOptions() = default;
        void resetToDefaults();

        NODISCARD std::shared_ptr<const NamedColorOptions> clone() const
        {
            auto pResult = std::make_shared<NamedColorOptions>();
            auto &result = deref(pResult);
#define X_CLONE(_id, _name) result._id = this->_id.getColor();
            XFOREACH_NAMED_COLOR_OPTIONS(X_CLONE)
#undef X_CLONE
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
        NamedConfig<bool> gmcpBroadcast{"GMCP_BROADCAST_CLOCK", true};  // Enable GMCP clock broadcasting
        NamedConfig<int> gmcpBroadcastInterval{"GMCP_BROADCAST_INTERVAL", 2500};  // Update interval in milliseconds (default: 2.5 seconds = 1 MUME minute)

        MumeClockSettings();

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
        int columns = 0;
        int rows = 0;
        int linesOfScrollback = 0;
        int linesOfInputHistory = 0;
        int tabCompletionDictionarySize = 0;
        bool clearInputOnEnter = false;
        bool autoResizeTerminal = false;
        int linesOfPeekPreview = 0;

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

using SharedCanvasNamedColorOptions = std::shared_ptr<const Configuration::CanvasNamedColorOptions>;
using SharedNamedColorOptions = std::shared_ptr<const Configuration::NamedColorOptions>;

const Configuration::CanvasNamedColorOptions &getCanvasNamedColorOptions();
const Configuration::NamedColorOptions &getNamedColorOptions();

struct NODISCARD ThreadLocalNamedColorRaii final
{
    explicit ThreadLocalNamedColorRaii(SharedCanvasNamedColorOptions canvasNamedColorOptions,
                                       SharedNamedColorOptions namedColorOptions);
    ~ThreadLocalNamedColorRaii();
};
