// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "configuration.h"

#include "../global/utils.h"

#include <cassert>
#include <mutex>
#include <optional>
#include <thread>

#include <QByteArray>
#include <QChar>
#include <QDir>
#include <QHostInfo>
#include <QSslSocket>
#include <QString>
#include <QStringList>

#undef TRANSPARENT // Bad dog, Microsoft; bad dog!!!

namespace { // anonymous

std::thread::id g_thread{};
std::atomic_bool g_config_enteredMain{false};

thread_local SharedCanvasNamedColorOptions tl_canvas_named_color_options;
thread_local SharedNamedColorOptions tl_named_color_options;

NODISCARD const char *getPlatformEditor()
{
    switch (CURRENT_PLATFORM) {
    case PlatformEnum::Windows:
        return "notepad";

    case PlatformEnum::Mac:
        return "open -W -n -t";

    case PlatformEnum::Linux:
        // add .txt extension and use xdg-open instead?
        // or if xdg-open doesn't exist, then you can
        // look for gnome-open, mate-open, etc.
        return "gedit";

    case PlatformEnum::Unknown:
    default:
        return "";
    }
}

} // namespace

Configuration::Configuration()
{
    read(); // read the settings or set them to the default values
}

/*
 * TODO: Make a dialog asking if the user wants to import settings
 * from an older version of MMapper, and then change the organization name
 * to reflect that it's an open source project that's not Caligor's
 * personal project anymore.
 *
 * Also, don't use space, because it will be a file name on disk.
 */
#define ConstString static constexpr const char *const
ConstString SETTINGS_ORGANIZATION = "MUME";
ConstString OLD_SETTINGS_ORGANIZATION = "Caligor soft";
ConstString SETTINGS_APPLICATION = "MMapper2";
ConstString SETTINGS_FIRST_TIME_KEY = "General/Run first time";

class NODISCARD Settings final
{
private:
    static constexpr const char *const MMAPPER_PROFILE_PATH = "MMAPPER_PROFILE_PATH";

private:
    std::optional<QSettings> m_settings;

private:
    NODISCARD static bool isValid(const QFile &file)
    {
        const QFileInfo info{file};
        return !info.isDir() && info.exists() && info.isReadable() && info.isWritable();
    }

    NODISCARD static bool isValid(const QString &fileName)
    {
        const QFile file{fileName};
        return isValid(file);
    }

    static void tryCopyOldSettings();

private:
    void initSettings();

public:
    DELETE_CTORS_AND_ASSIGN_OPS(Settings);
    Settings() { initSettings(); }
    ~Settings() = default;
    explicit operator QSettings &()
    {
        if (!m_settings) {
            throw std::runtime_error("object does not exist");
        }
        return m_settings.value();
    }
};

void Settings::initSettings()
{
    if (m_settings) {
        throw std::runtime_error("object already exists");
    }

    // NOTE: mutex guards read/write access to g_path from multiple threads,
    // since the static variable can be set to nullptr on spurious failure.
    static std::mutex g_mutex;
    std::lock_guard<std::mutex> lock{g_mutex};

    static auto g_path = qgetenv(MMAPPER_PROFILE_PATH);

    if (g_path != nullptr) {
        // NOTE: QMessageLogger quotes QString by default, but doesn't quote const char*.
        const QString pathString{g_path};

        static std::once_flag attempt_flag;
        std::call_once(attempt_flag, [&pathString] {
            qInfo() << "Attempting to use settings from" << pathString
                    << "(specified by environment variable" << QString{MMAPPER_PROFILE_PATH}
                    << ")...";
        });

        if (!isValid(pathString)) {
            qWarning() << "Falling back to default settings path because" << pathString
                       << "is not a writable file.";
            g_path = nullptr;
        } else {
            try {
                m_settings.emplace(pathString, QSettings::IniFormat);
            } catch (...) {
                qInfo() << "Exception loading settings for " << pathString
                        << "; falling back to default settings...";
                g_path = nullptr;
            }
        }
    }

    if (!m_settings) {
        tryCopyOldSettings();
        m_settings.emplace(SETTINGS_ORGANIZATION, SETTINGS_APPLICATION);
    }

    static std::once_flag success_flag;
    std::call_once(success_flag, [this] {
        auto &&info = qInfo();
        info << "Using settings from" << QString{static_cast<QSettings &>(*this).fileName()};
        if (g_path == nullptr) {
            info << "(Hint: Environment variable" << QString{MMAPPER_PROFILE_PATH}
                 << "overrides the default).";
        } else {
            info << ".";
        }
    });
}

//
// NOTES:
//
// * Avoiding using global QSettings& getSettings() because the QSettings object
//   maintains some state, so it's better to construct a new one each time.
//
// * Declaring an object via macro instead of just calling a function is necessary
//   because we have two object construction paths, and we attempt to access the
//   value after its construction, so we can't use RVO to avoid copy/move
//   construction of the QSettings object.
//
// * Using a separate reference because macros use "conf.beginGroup()", but
//   "operator T&" is never selected when used with (non-existent) "operator.".
//   Instead, we could use "conf->beginGroup()" with "QSettings* operator->()"
//   to avoid needing to declare a QSettings& reference.

#define SETTINGS(conf) \
    Settings settings; \
    QSettings &conf = static_cast<QSettings &>(settings)

ConstString GRP_ADVENTURE_PANEL = "Adventure Panel";
ConstString GRP_ACCOUNT = "Account";
ConstString GRP_AUTO_LOAD_WORLD = "Auto load world";
ConstString GRP_AUTO_LOG = "Auto log";
ConstString GRP_CANVAS = "Canvas";
ConstString GRP_CONNECTION = "Connection";
ConstString GRP_FINDROOMS_DIALOG = "FindRooms Dialog";
ConstString GRP_GENERAL = "General";
ConstString GRP_GROUP_MANAGER = "Group Manager";
ConstString GRP_INFOMARKS_DIALOG = "InfoMarks Dialog";
ConstString GRP_INTEGRATED_MUD_CLIENT = "Integrated Mud Client";
ConstString GRP_MUME_CLIENT_PROTOCOL = "Mume client protocol";
ConstString GRP_MUME_CLOCK = "Mume Clock";
ConstString GRP_MUME_NATIVE = "Mume native";
ConstString GRP_PARSER = "Parser";
ConstString GRP_PATH_MACHINE = "Path Machine";
ConstString GRP_ROOM_PANEL = "Room Panel";
ConstString GRP_ROOMEDIT_DIALOG = "RoomEdit Dialog";

ConstString KEY_ABSOLUTE_PATH_ACCEPTANCE = "absolute path acceptance";
ConstString KEY_ACCOUNT_NAME = "account name";
ConstString KEY_ACCOUNT_PASSWORD = "account password";
ConstString KEY_ALWAYS_ON_TOP = "Always On Top";
ConstString KEY_SHOW_STATUS_BAR = "Show Status Bar";
ConstString KEY_SHOW_SCROLL_BARS = "Show Scroll Bars";
ConstString KEY_SHOW_MENU_BAR = "Show Menu Bar";
ConstString KEY_AUTO_LOAD = "Auto load";
ConstString KEY_AUTO_RESIZE_TERMINAL = "Auto resize terminal";
ConstString KEY_BACKGROUND_COLOR = "Background color";
ConstString KEY_CHARACTER_ENCODING = "Character encoding";
ConstString KEY_CHECK_FOR_UPDATE = "Check for update";
ConstString KEY_CLEAR_INPUT_ON_ENTER = "Clear input on enter";
ConstString KEY_COLUMNS = "Columns";
ConstString KEY_COMMAND_PREFIX_CHAR = "Command prefix character";
ConstString KEY_CONNECTION_NORMAL_COLOR = "Connection normal color";
ConstString KEY_CORRECT_POSITION_BONUS = "correct position bonus";
ConstString KEY_DISPLAY_XP_STATUS = "Display XP status bar widget";
ConstString KEY_DISPLAY_CLOCK = "Display clock";
ConstString KEY_DRAW_DOOR_NAMES = "Draw door names";
ConstString KEY_DRAW_NOT_MAPPED_EXITS = "Draw not mapped exits";
ConstString KEY_DRAW_UPPER_LAYERS_TEXTURED = "Draw upper layers textured";
ConstString KEY_EMOJI_ENCODE = "encode emoji";
ConstString KEY_EMOJI_DECODE = "decode emoji";
ConstString KEY_EMULATED_EXITS = "Emulated Exits";
ConstString KEY_EXTERNAL_EDITOR_COMMAND = "External editor command";
ConstString KEY_FILE_NAME = "File name";
ConstString KEY_GROUP_YOUR_COLOR = "color";
ConstString KEY_GROUP_NPC_COLOR = "npc color";
ConstString KEY_GROUP_NPC_COLOR_OVERRIDE = "npc color override";
ConstString KEY_GROUP_NPC_SORT_BOTTOM = "npc sort bottom";
ConstString KEY_GROUP_NPC_HIDE = "npc hide";
ConstString KEY_GROUP_SHOW_MAP_TOKENS = "show map tokens";
ConstString KEY_GROUP_SHOW_NPC_GHOSTS = "show npc ghosts";
ConstString KEY_GROUP_SHOW_TOKENS = "show tokens";
ConstString KEY_GROUP_TOKEN_ICON_SIZE = "token icon size";
ConstString KEY_GROUP_TOKEN_OVERRIDES = "token overrides";
ConstString KEY_AUTO_LOG = "Auto log";
ConstString KEY_AUTO_LOG_ASK_DELETE = "Auto log ask before deleting";
ConstString KEY_AUTO_LOG_CLEANUP_STRATEGY = "Auto log cleanup strategy";
ConstString KEY_AUTO_LOG_DELETE_AFTER_DAYS = "Auto log delete after X days";
ConstString KEY_AUTO_LOG_DELETE_AFTER_BYTES = "Auto log delete after X bytes";
ConstString KEY_AUTO_LOG_DIRECTORY = "Auto log directory";
ConstString KEY_AUTO_LOG_ROTATE_SIZE_BYTES = "Auto log rotate after X bytes";
ConstString KEY_FONT = "Font";
ConstString KEY_FOREGROUND_COLOR = "Foreground color";
ConstString KEY_3D_CANVAS = "canvas.advanced.use3D";
ConstString KEY_3D_AUTO_TILT = "canvas.advanced.autoTilt";
ConstString KEY_3D_PERFSTATS = "canvas.advanced.printPerfStats";
ConstString KEY_3D_FOV = "canvas.advanced.fov";
ConstString KEY_3D_VERTICAL_ANGLE = "canvas.advanced.verticalAngle";
ConstString KEY_3D_HORIZONTAL_ANGLE = "canvas.advanced.horizontalAngle";
ConstString KEY_3D_LAYER_HEIGHT = "canvas.advanced.layerHeight";
ConstString KEY_LAST_MAP_LOAD_DIRECTORY = "Last map load directory";
ConstString KEY_LINES_OF_INPUT_HISTORY = "Lines of input history";
ConstString KEY_LINES_OF_PEEK_PREVIEW = "Lines of peek preview";
ConstString KEY_LINES_OF_SCROLLBACK = "Lines of scrollback";
ConstString KEY_PROXY_LOCAL_PORT = "Local port number";
ConstString KEY_MAP_MODE = "Map Mode";
ConstString KEY_MAXIMUM_NUMBER_OF_PATHS = "maximum number of paths";
ConstString KEY_MULTIPLE_CONNECTIONS_PENALTY = "multiple connections penalty";
ConstString KEY_MUME_START_EPOCH = "Mume start epoch";
ConstString KEY_NUMBER_OF_ANTI_ALIASING_SAMPLES = "Number of anti-aliasing samples";
ConstString KEY_PROXY_CONNECTION_STATUS = "Proxy connection status";
ConstString KEY_PROXY_LISTENS_ON_ANY_INTERFACE = "Proxy listens on any interface";
ConstString KEY_RELATIVE_PATH_ACCEPTANCE = "relative path acceptance";
ConstString KEY_RESOURCES_DIRECTORY = "canvas.resourcesDir";
ConstString KEY_MUME_REMOTE_PORT = "Remote port number";
ConstString KEY_REMEMBER_LOGIN = "remember login";
ConstString KEY_ROOM_CREATION_PENALTY = "room creation penalty";
ConstString KEY_ROOM_DARK_COLOR = "Room dark color";
ConstString KEY_ROOM_DARK_LIT_COLOR = "Room dark lit color";
ConstString KEY_ROOM_DESC_ANSI_COLOR = "Room desc ansi color";
ConstString KEY_ROOM_MATCHING_TOLERANCE = "room matching tolerance";
ConstString KEY_ROOM_NAME_ANSI_COLOR = "Room name ansi color";
ConstString KEY_ROWS = "Rows";
ConstString KEY_RUN_FIRST_TIME = "Run first time";
ConstString KEY_SERVER_NAME = "Server name";
ConstString KEY_SHOW_HIDDEN_EXIT_FLAGS = "Show hidden exit flags";
ConstString KEY_SHOW_NOTES = "Show notes";
ConstString KEY_SHOW_UNSAVED_CHANGES = "Show unsaved changes";
ConstString KEY_SHOW_MISSING_MAP_ID = "Show missing map id";
ConstString KEY_TAB_COMPLETION_DICTIONARY_SIZE = "Tab completion dictionary size";
ConstString KEY_TLS_ENCRYPTION = "TLS encryption";
ConstString KEY_USE_INTERNAL_EDITOR = "Use internal editor";
ConstString KEY_USE_TRILINEAR_FILTERING = "Use trilinear filtering";
ConstString KEY_WINDOW_GEOMETRY = "Window Geometry";
ConstString KEY_WINDOW_STATE = "Window State";

void Settings::tryCopyOldSettings()
{
    QSettings sNew(SETTINGS_ORGANIZATION, SETTINGS_APPLICATION);
    if (!sNew.allKeys().contains(SETTINGS_FIRST_TIME_KEY)) {
        const QSettings sOld(OLD_SETTINGS_ORGANIZATION, SETTINGS_APPLICATION);
        if (!sOld.allKeys().isEmpty()) {
            qInfo() << "Copying old config" << sOld.fileName() << "to" << sNew.fileName() << "...";
            for (const QString &key : sOld.allKeys()) {
                sNew.setValue(key, sOld.value(key));
            }
        }
    }
    // News 2340, changing domain from fire.pvv.org to mume.org:
    sNew.beginGroup(GRP_CONNECTION);
    if (sNew.value(KEY_SERVER_NAME, "").toString().contains("pvv.org")) {
        sNew.setValue(KEY_SERVER_NAME, "mume.org");
    }
    sNew.endGroup();
}

NODISCARD static bool isValidAnsi(const QString &input)
{
    static constexpr const auto MAX = static_cast<uint32_t>(std::numeric_limits<uint8_t>::max());

    if (!input.startsWith("[") || !input.endsWith("m")) {
        return false;
    }

    for (const auto &part : input.mid(1, input.length() - 2).split(";")) {
        for (const QChar c : part) {
            if (!c.isDigit()) {
                return false;
            }
        }
        bool ok = false;
        const auto n = part.toUInt(&ok, 10);
        if (!ok || n > MAX) {
            return false;
        }
    }

    return true;
}

NODISCARD static bool isValidMapMode(const MapModeEnum mode)
{
    switch (mode) {
    case MapModeEnum::PLAY:
    case MapModeEnum::MAP:
    case MapModeEnum::OFFLINE:
        return true;
    }
    return false;
}

NODISCARD static bool isValidCharacterEncoding(const CharacterEncodingEnum encoding)
{
    switch (encoding) {
    case CharacterEncodingEnum::ASCII:
    case CharacterEncodingEnum::LATIN1:
    case CharacterEncodingEnum::UTF8:
        return true;
    }
    return false;
}

NODISCARD static bool isValidAutoLoggerState(const AutoLoggerEnum strategy)
{
    switch (strategy) {
    case AutoLoggerEnum::DeleteDays:
    case AutoLoggerEnum::DeleteSize:
    case AutoLoggerEnum::KeepForever:
        return true;
    }
    return false;
}

NODISCARD static QString sanitizeAnsi(const QString &input, const QString &defaultValue)
{
    assert(isValidAnsi(defaultValue));
    if (isValidAnsi(input)) {
        return input;
    }

    if (!input.isEmpty()) {
        qWarning() << "invalid ansi code: " << input;
    }

    return defaultValue;
}

NODISCARD static MapModeEnum sanitizeMapMode(const uint32_t input)
{
    const auto mode = static_cast<MapModeEnum>(input);
    if (isValidMapMode(mode)) {
        return mode;
    }

    qWarning() << "invalid MapMode:" << input;
    return MapModeEnum::PLAY;
}

NODISCARD static CharacterEncodingEnum sanitizeCharacterEncoding(const uint32_t input)
{
    const auto encoding = static_cast<CharacterEncodingEnum>(input);
    if (isValidCharacterEncoding(encoding)) {
        return encoding;
    }

    qWarning() << "invalid CharacterEncodingEnum:" << input;
    return CharacterEncodingEnum::LATIN1;
}

NODISCARD static AutoLoggerEnum sanitizeAutoLoggerState(const int input)
{
    const auto state = static_cast<AutoLoggerEnum>(input);
    if (isValidAutoLoggerState(state)) {
        return state;
    }

    qWarning() << "invalid AutoLoggerEnum:" << input;
    return AutoLoggerEnum::DeleteDays;
}

NODISCARD static uint16_t sanitizeUint16(const int input, const uint16_t defaultValue)
{
    static constexpr const auto MIN = static_cast<int>(std::numeric_limits<uint16_t>::min());
    static constexpr const auto MAX = static_cast<int>(std::numeric_limits<uint16_t>::max());

    if (isClamped(input, MIN, MAX)) {
        return static_cast<uint16_t>(input);
    }

    qWarning() << "invalid uint16: " << input;
    return defaultValue;
}

#define GROUP_CALLBACK(callback, name, ref) \
    do { \
        conf.beginGroup(name); \
        ref.callback(conf); \
        conf.endGroup(); \
    } while (false)

#define FOREACH_CONFIG_GROUP(callback) \
    do { \
        GROUP_CALLBACK(callback, GRP_GENERAL, general); \
        GROUP_CALLBACK(callback, GRP_CONNECTION, connection); \
        GROUP_CALLBACK(callback, GRP_CANVAS, canvas); \
        GROUP_CALLBACK(callback, GRP_ACCOUNT, account); \
        GROUP_CALLBACK(callback, GRP_AUTO_LOAD_WORLD, autoLoad); \
        GROUP_CALLBACK(callback, GRP_AUTO_LOG, autoLog); \
        GROUP_CALLBACK(callback, GRP_PARSER, parser); \
        GROUP_CALLBACK(callback, GRP_MUME_CLIENT_PROTOCOL, mumeClientProtocol); \
        GROUP_CALLBACK(callback, GRP_MUME_NATIVE, mumeNative); \
        GROUP_CALLBACK(callback, GRP_PATH_MACHINE, pathMachine); \
        GROUP_CALLBACK(callback, GRP_GROUP_MANAGER, groupManager); \
        GROUP_CALLBACK(callback, GRP_MUME_CLOCK, mumeClock); \
        GROUP_CALLBACK(callback, GRP_ADVENTURE_PANEL, adventurePanel); \
        GROUP_CALLBACK(callback, GRP_INTEGRATED_MUD_CLIENT, integratedClient); \
        GROUP_CALLBACK(callback, GRP_INFOMARKS_DIALOG, infomarksDialog); \
        GROUP_CALLBACK(callback, GRP_ROOMEDIT_DIALOG, roomEditDialog); \
        GROUP_CALLBACK(callback, GRP_ROOM_PANEL, roomPanel); \
        GROUP_CALLBACK(callback, GRP_FINDROOMS_DIALOG, findRoomsDialog); \
    } while (false)

void Configuration::read()
{
    // reset to defaults before reading colors that might override them
    colorSettings.resetToDefaults();

    SETTINGS(conf);
    FOREACH_CONFIG_GROUP(read);

    // This logic only runs once on a MMapper fresh install (or factory reset)
    // Subsequent MMapper starts will always read "firstRun" as false
    if (general.firstRun) {
        // New users get the 3D canvas but old users do not
        canvas.advanced.use3D.set(true);

        // New users get autologger turned on by default
        autoLog.autoLog = true;
    }

    assert(canvas.backgroundColor == colorSettings.BACKGROUND);
    assert(canvas.roomDarkColor == colorSettings.ROOM_DARK);
    assert(canvas.roomDarkLitColor == colorSettings.ROOM_NO_SUNDEATH);

    assert(colorSettings.TRANSPARENT.isInitialized()
           && colorSettings.TRANSPARENT.getColor().isTransparent());
    assert(colorSettings.BACKGROUND.isInitialized()
           && !colorSettings.BACKGROUND.getColor().isTransparent());
}

void Configuration::write() const
{
    SETTINGS(conf);
    FOREACH_CONFIG_GROUP(write);
}

void Configuration::reset()
{
    {
        // Purge old organization settings first to prevent them from being migrated
        QSettings oldConf(OLD_SETTINGS_ORGANIZATION, SETTINGS_APPLICATION);
        oldConf.clear();
    }
    {
        // Purge new organization settings
        SETTINGS(conf);
        conf.clear();
    }

    // Reload defaults
    read();
}

#undef FOREACH_CONFIG_GROUP
#undef GROUP_CALLBACK

ConstString DEFAULT_MMAPPER_SUBDIR = "/MMapper";
ConstString DEFAULT_LOGS_SUBDIR = "/Logs";
ConstString DEFAULT_RESOURCES_SUBDIR = "/Resources";

NODISCARD static QString getDefaultDirectory()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).absolutePath();
}

void Configuration::GeneralSettings::read(const QSettings &conf)
{
    firstRun = conf.value(KEY_RUN_FIRST_TIME, true).toBool();
    /*
     * REVISIT: It's basically impossible to verify that this state is valid,
     * because we have no idea what it contains!
     *
     * This setting is inherently non-portable between OSes
     * (and possibly even window managers), so it doesn't belong here!
     *
     * If we're going to save it, then we should probably least checksum it
     * (or better yet sign it), and record the OS config, so that we won't
     * try to apply Windows settings to Mac, or Gnome settings to KDE, etc?
     */
    windowGeometry = conf.value(KEY_WINDOW_GEOMETRY).toByteArray();
    windowState = conf.value(KEY_WINDOW_STATE).toByteArray();
    alwaysOnTop = conf.value(KEY_ALWAYS_ON_TOP, false).toBool();
    showStatusBar = conf.value(KEY_SHOW_STATUS_BAR, true).toBool();
    showScrollBars = conf.value(KEY_SHOW_SCROLL_BARS, true).toBool();
    showMenuBar = conf.value(KEY_SHOW_MENU_BAR, true).toBool();
    mapMode = sanitizeMapMode(
        conf.value(KEY_MAP_MODE, static_cast<uint32_t>(MapModeEnum::PLAY)).toUInt());
    checkForUpdate = conf.value(KEY_CHECK_FOR_UPDATE, true).toBool();
    characterEncoding = sanitizeCharacterEncoding(
        conf.value(KEY_CHARACTER_ENCODING, static_cast<uint32_t>(CharacterEncodingEnum::LATIN1))
            .toUInt());
}

void Configuration::ConnectionSettings::read(const QSettings &conf)
{
    static constexpr const int DEFAULT_PORT = 4242;

    remoteServerName = conf.value(KEY_SERVER_NAME, "mume.org").toString();
    remotePort = sanitizeUint16(conf.value(KEY_MUME_REMOTE_PORT, DEFAULT_PORT).toInt(),
                                static_cast<uint16_t>(DEFAULT_PORT));
    localPort = sanitizeUint16(conf.value(KEY_PROXY_LOCAL_PORT, DEFAULT_PORT).toInt(),
                               static_cast<uint16_t>(DEFAULT_PORT));
    tlsEncryption = QSslSocket::supportsSsl() ? conf.value(KEY_TLS_ENCRYPTION, true).toBool()
                                              : false;
    proxyConnectionStatus = conf.value(KEY_PROXY_CONNECTION_STATUS, false).toBool();
    proxyListensOnAnyInterface = conf.value(KEY_PROXY_LISTENS_ON_ANY_INTERFACE, false).toBool();
}

// closest well-known color is "Outer Space"
static constexpr const std::string_view DEFAULT_BGCOLOR = "#2E3436";
// closest well-known color is "Dusty Gray"
static constexpr const std::string_view DEFAULT_DARK_COLOR = "#A19494";
// closest well-known color is "Cold Turkey"
static constexpr const std::string_view DEFAULT_NO_SUNDEATH_COLOR = "#D4C7C7";

void Configuration::CanvasSettings::read(const QSettings &conf)
{
    // REVISIT: Consider just using the "current" value of the named color object,
    // since we can assume they're initialized before the values are read.
    const auto lookupColor = [&conf](const char *const key, std::string_view def) {
        // NOTE: string_view isn't guaranteed to be null-terminated,
        // but wow... this is a complicated way of passing the exact same value.
        const auto qdef = QColor(QString(std::string{def}.c_str())).name();
        return Color(QColor(conf.value(key, qdef).toString()));
    };

    resourcesDirectory = conf.value(KEY_RESOURCES_DIRECTORY,
                                    getDefaultDirectory()
                                        .append(DEFAULT_MMAPPER_SUBDIR)
                                        .append(DEFAULT_RESOURCES_SUBDIR))
                             .toString();
    showMissingMapId.set(conf.value(KEY_SHOW_MISSING_MAP_ID, true).toBool());
    showUnsavedChanges.set(conf.value(KEY_SHOW_UNSAVED_CHANGES, true).toBool());
    showUnmappedExits.set(conf.value(KEY_DRAW_NOT_MAPPED_EXITS, true).toBool());
    drawUpperLayersTextured = conf.value(KEY_DRAW_UPPER_LAYERS_TEXTURED, false).toBool();
    drawDoorNames = conf.value(KEY_DRAW_DOOR_NAMES, true).toBool();
    backgroundColor = lookupColor(KEY_BACKGROUND_COLOR, DEFAULT_BGCOLOR);
    connectionNormalColor = lookupColor(KEY_CONNECTION_NORMAL_COLOR, Colors::white.toHex());
    roomDarkColor = lookupColor(KEY_ROOM_DARK_COLOR, DEFAULT_DARK_COLOR);
    roomDarkLitColor = lookupColor(KEY_ROOM_DARK_LIT_COLOR, DEFAULT_NO_SUNDEATH_COLOR);
    antialiasingSamples = conf.value(KEY_NUMBER_OF_ANTI_ALIASING_SAMPLES, 0).toInt();
    trilinearFiltering = conf.value(KEY_USE_TRILINEAR_FILTERING, true).toBool();
    advanced.use3D.set(conf.value(KEY_3D_CANVAS, false).toBool());
    advanced.autoTilt.set(conf.value(KEY_3D_AUTO_TILT, true).toBool());
    advanced.printPerfStats.set(conf.value(KEY_3D_PERFSTATS, IS_DEBUG_BUILD).toBool());
    advanced.fov.set(conf.value(KEY_3D_FOV, 765).toInt());
    advanced.verticalAngle.set(conf.value(KEY_3D_VERTICAL_ANGLE, 450).toInt());
    advanced.horizontalAngle.set(conf.value(KEY_3D_HORIZONTAL_ANGLE, 0).toInt());
    advanced.layerHeight.set(conf.value(KEY_3D_LAYER_HEIGHT, 15).toInt());
}

void Configuration::AccountSettings::read(const QSettings &conf)
{
    accountName = conf.value(KEY_ACCOUNT_NAME, "").toString();
    accountPassword = conf.value(KEY_ACCOUNT_PASSWORD, false).toBool();
    rememberLogin = NO_QTKEYCHAIN ? false : conf.value(KEY_REMEMBER_LOGIN, false).toBool();
}

void Configuration::AutoLoadSettings::read(const QSettings &conf)
{
    autoLoadMap = conf.value(KEY_AUTO_LOAD, true).toBool();
    fileName = conf.value(KEY_FILE_NAME, "").toString();
    lastMapDirectory = conf.value(KEY_LAST_MAP_LOAD_DIRECTORY,
                                  getDefaultDirectory().append(DEFAULT_MMAPPER_SUBDIR))
                           .toString();
}

void Configuration::AutoLogSettings::read(const QSettings &conf)
{
    autoLogDirectory = conf.value(KEY_AUTO_LOG_DIRECTORY,
                                  getDefaultDirectory()
                                      .append(DEFAULT_MMAPPER_SUBDIR)
                                      .append(DEFAULT_LOGS_SUBDIR))
                           .toString();
    autoLog = conf.value(KEY_AUTO_LOG, false).toBool();
    rotateWhenLogsReachBytes = conf.value(KEY_AUTO_LOG_ROTATE_SIZE_BYTES, 10 * 1000000)
                                   .toInt(); // 10 Megabytes
    askDelete = conf.value(KEY_AUTO_LOG_ASK_DELETE, false).toBool();
    cleanupStrategy = sanitizeAutoLoggerState(
        conf.value(KEY_AUTO_LOG_CLEANUP_STRATEGY, static_cast<int>(AutoLoggerEnum::DeleteDays))
            .toInt());
    deleteWhenLogsReachDays = conf.value(KEY_AUTO_LOG_DELETE_AFTER_DAYS, 30).toInt();
    deleteWhenLogsReachBytes = conf.value(KEY_AUTO_LOG_DELETE_AFTER_BYTES, 100 * 1000000).toInt();
}

void Configuration::ParserSettings::read(const QSettings &conf)
{
    static constexpr const char *const ANSI_GREEN = "[32m";
    static constexpr const char *const ANSI_RESET = "[0m";

    roomNameColor = sanitizeAnsi(conf.value(KEY_ROOM_NAME_ANSI_COLOR, ANSI_GREEN).toString(),
                                 QString(ANSI_GREEN));
    roomDescColor = sanitizeAnsi(conf.value(KEY_ROOM_DESC_ANSI_COLOR, ANSI_RESET).toString(),
                                 QString(ANSI_RESET));
    prefixChar = conf.value(KEY_COMMAND_PREFIX_CHAR, QChar::fromLatin1(char_consts::C_UNDERSCORE))
                     .toChar()
                     .toLatin1();
    encodeEmoji = conf.value(KEY_EMOJI_ENCODE, true).toBool();
    decodeEmoji = conf.value(KEY_EMOJI_DECODE, true).toBool();
}

void Configuration::MumeClientProtocolSettings::read(const QSettings &conf)
{
    internalRemoteEditor = conf.value(KEY_USE_INTERNAL_EDITOR, true).toBool();
    externalRemoteEditorCommand = conf.value(KEY_EXTERNAL_EDITOR_COMMAND, getPlatformEditor())
                                      .toString();
}

void Configuration::MumeNativeSettings::read(const QSettings &conf)
{
    emulatedExits = conf.value(KEY_EMULATED_EXITS, true).toBool();
    showHiddenExitFlags = conf.value(KEY_SHOW_HIDDEN_EXIT_FLAGS, true).toBool();
    showNotes = conf.value(KEY_SHOW_NOTES, true).toBool();
}

void Configuration::PathMachineSettings::read(const QSettings &conf)
{
    acceptBestRelative = conf.value(KEY_RELATIVE_PATH_ACCEPTANCE, 25).toDouble();
    acceptBestAbsolute = conf.value(KEY_ABSOLUTE_PATH_ACCEPTANCE, 6).toDouble();
    newRoomPenalty = conf.value(KEY_ROOM_CREATION_PENALTY, 5).toDouble();
    correctPositionBonus = conf.value(KEY_CORRECT_POSITION_BONUS, 5).toDouble();
    multipleConnectionsPenalty = conf.value(KEY_MULTIPLE_CONNECTIONS_PENALTY, 2.0).toDouble();
    maxPaths = utils::clampNonNegative(conf.value(KEY_MAXIMUM_NUMBER_OF_PATHS, 1000).toInt());
    matchingTolerance = utils::clampNonNegative(conf.value(KEY_ROOM_MATCHING_TOLERANCE, 8).toInt());
}

void Configuration::GroupManagerSettings::read(const QSettings &conf)
{
    color = QColor(conf.value(KEY_GROUP_YOUR_COLOR, "#FFFF00").toString());
    npcColor = QColor(conf.value(KEY_GROUP_NPC_COLOR, QColor(Qt::lightGray)).toString());
    npcColorOverride = conf.value(KEY_GROUP_NPC_COLOR_OVERRIDE, false).toBool();
    npcHide = conf.value(KEY_GROUP_NPC_HIDE, false).toBool();
    npcSortBottom = conf.value(KEY_GROUP_NPC_SORT_BOTTOM, false).toBool();
    showTokens = conf.value(KEY_GROUP_SHOW_TOKENS, true).toBool();
    showMapTokens = conf.value(KEY_GROUP_SHOW_MAP_TOKENS, true).toBool();
    tokenIconSize = conf.value(KEY_GROUP_TOKEN_ICON_SIZE, 32).toInt();
    showNpcGhosts = conf.value(KEY_GROUP_SHOW_NPC_GHOSTS, true).toBool();

    tokenOverrides.clear();
    QSettings &rw = const_cast<QSettings &>(conf);
    rw.beginGroup(KEY_GROUP_TOKEN_OVERRIDES);
    const QStringList keys = rw.childKeys();
    for (const QString &k : keys)
        tokenOverrides.insert(k, rw.value(k).toString());
    rw.endGroup();
}

void Configuration::MumeClockSettings::read(const QSettings &conf)
{
    // NOTE: old values might be stored as int32
    startEpoch = conf.value(KEY_MUME_START_EPOCH, 1517443173).toLongLong();
    display = conf.value(KEY_DISPLAY_CLOCK, true).toBool();
}

void Configuration::AdventurePanelSettings::read(const QSettings &conf)
{
    m_displayXPStatus = conf.value(KEY_DISPLAY_XP_STATUS, true).toBool();
}

void Configuration::IntegratedMudClientSettings::read(const QSettings &conf)
{
    font = conf.value(KEY_FONT, "").toString();
    backgroundColor = conf.value(KEY_BACKGROUND_COLOR, QColor(Qt::black).name()).toString();
    foregroundColor = conf.value(KEY_FOREGROUND_COLOR, QColor(Qt::lightGray).name()).toString();
    columns = conf.value(KEY_COLUMNS, 80).toInt();
    rows = conf.value(KEY_ROWS, 24).toInt();
    linesOfScrollback = conf.value(KEY_LINES_OF_SCROLLBACK, 10000).toInt();
    linesOfInputHistory = conf.value(KEY_LINES_OF_INPUT_HISTORY, 100).toInt();
    tabCompletionDictionarySize = conf.value(KEY_TAB_COMPLETION_DICTIONARY_SIZE, 100).toInt();
    clearInputOnEnter = conf.value(KEY_CLEAR_INPUT_ON_ENTER, false).toBool();
    autoResizeTerminal = conf.value(KEY_AUTO_RESIZE_TERMINAL, true).toBool();
    linesOfPeekPreview = conf.value(KEY_LINES_OF_PEEK_PREVIEW, 7).toInt();
}

void Configuration::RoomPanelSettings::read(const QSettings &conf)
{
    geometry = conf.value(KEY_WINDOW_GEOMETRY).toByteArray();
}

void Configuration::InfomarksDialog::read(const QSettings &conf)
{
    geometry = conf.value(KEY_WINDOW_GEOMETRY).toByteArray();
}

void Configuration::RoomEditDialog::read(const QSettings &conf)
{
    geometry = conf.value(KEY_WINDOW_GEOMETRY).toByteArray();
}

void Configuration::FindRoomsDialog::read(const QSettings &conf)
{
    geometry = conf.value(KEY_WINDOW_GEOMETRY).toByteArray();
}

void Configuration::GeneralSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_RUN_FIRST_TIME, false);
    conf.setValue(KEY_WINDOW_GEOMETRY, windowGeometry);
    conf.setValue(KEY_WINDOW_STATE, windowState);
    conf.setValue(KEY_ALWAYS_ON_TOP, alwaysOnTop);
    conf.setValue(KEY_SHOW_STATUS_BAR, showStatusBar);
    conf.setValue(KEY_SHOW_SCROLL_BARS, showScrollBars);
    conf.setValue(KEY_SHOW_MENU_BAR, showMenuBar);
    conf.setValue(KEY_MAP_MODE, static_cast<uint32_t>(mapMode));
    conf.setValue(KEY_CHECK_FOR_UPDATE, checkForUpdate);
    conf.setValue(KEY_CHARACTER_ENCODING, static_cast<uint32_t>(characterEncoding));
}

void Configuration::ConnectionSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_SERVER_NAME, remoteServerName);
    conf.setValue(KEY_MUME_REMOTE_PORT, static_cast<int>(remotePort));
    conf.setValue(KEY_PROXY_LOCAL_PORT, static_cast<int>(localPort));
    conf.setValue(KEY_TLS_ENCRYPTION, tlsEncryption);
    conf.setValue(KEY_PROXY_CONNECTION_STATUS, proxyConnectionStatus);
    conf.setValue(KEY_PROXY_LISTENS_ON_ANY_INTERFACE, proxyListensOnAnyInterface);
}

NODISCARD static auto getQColorName(const XNamedColor &color)
{
    return color.getColor().getQColor().name();
}

void Configuration::CanvasSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_RESOURCES_DIRECTORY, resourcesDirectory);
    conf.setValue(KEY_SHOW_MISSING_MAP_ID, showMissingMapId.get());
    conf.setValue(KEY_SHOW_UNSAVED_CHANGES, showUnsavedChanges.get());
    conf.setValue(KEY_DRAW_NOT_MAPPED_EXITS, showUnmappedExits.get());
    conf.setValue(KEY_DRAW_UPPER_LAYERS_TEXTURED, drawUpperLayersTextured);
    conf.setValue(KEY_DRAW_DOOR_NAMES, drawDoorNames);
    conf.setValue(KEY_BACKGROUND_COLOR, getQColorName(backgroundColor));
    conf.setValue(KEY_ROOM_DARK_COLOR, getQColorName(roomDarkColor));
    conf.setValue(KEY_ROOM_DARK_LIT_COLOR, getQColorName(roomDarkLitColor));
    conf.setValue(KEY_CONNECTION_NORMAL_COLOR, getQColorName(connectionNormalColor));
    conf.setValue(KEY_NUMBER_OF_ANTI_ALIASING_SAMPLES, antialiasingSamples);
    conf.setValue(KEY_USE_TRILINEAR_FILTERING, trilinearFiltering);
    conf.setValue(KEY_3D_CANVAS, advanced.use3D.get());
    conf.setValue(KEY_3D_AUTO_TILT, advanced.autoTilt.get());
    conf.setValue(KEY_3D_PERFSTATS, advanced.printPerfStats.get());
    conf.setValue(KEY_3D_FOV, advanced.fov.get());
    conf.setValue(KEY_3D_VERTICAL_ANGLE, advanced.verticalAngle.get());
    conf.setValue(KEY_3D_HORIZONTAL_ANGLE, advanced.horizontalAngle.get());
    conf.setValue(KEY_3D_LAYER_HEIGHT, advanced.layerHeight.get());
}

void Configuration::AccountSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_ACCOUNT_NAME, accountName);
    conf.setValue(KEY_ACCOUNT_PASSWORD, accountPassword);
    conf.setValue(KEY_REMEMBER_LOGIN, rememberLogin);
}

void Configuration::AutoLoadSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_AUTO_LOAD, autoLoadMap);
    conf.setValue(KEY_FILE_NAME, fileName);
    conf.setValue(KEY_LAST_MAP_LOAD_DIRECTORY, lastMapDirectory);
}

void Configuration::AutoLogSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_AUTO_LOG, autoLog);
    conf.setValue(KEY_AUTO_LOG_CLEANUP_STRATEGY, static_cast<int>(cleanupStrategy));
    conf.setValue(KEY_AUTO_LOG_DIRECTORY, autoLogDirectory);
    conf.setValue(KEY_AUTO_LOG_ROTATE_SIZE_BYTES, rotateWhenLogsReachBytes);
    conf.setValue(KEY_AUTO_LOG_ASK_DELETE, askDelete);
    conf.setValue(KEY_AUTO_LOG_DELETE_AFTER_DAYS, deleteWhenLogsReachDays);
    conf.setValue(KEY_AUTO_LOG_DELETE_AFTER_BYTES, deleteWhenLogsReachBytes);
}

void Configuration::ParserSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_ROOM_NAME_ANSI_COLOR, roomNameColor);
    conf.setValue(KEY_ROOM_DESC_ANSI_COLOR, roomDescColor);
    conf.setValue(KEY_COMMAND_PREFIX_CHAR, QChar::fromLatin1(prefixChar));
    conf.setValue(KEY_EMOJI_ENCODE, encodeEmoji);
    conf.setValue(KEY_EMOJI_DECODE, decodeEmoji);
}

void Configuration::MumeNativeSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_EMULATED_EXITS, emulatedExits);
    conf.setValue(KEY_SHOW_HIDDEN_EXIT_FLAGS, showHiddenExitFlags);
    conf.setValue(KEY_SHOW_NOTES, showNotes);
}

void Configuration::MumeClientProtocolSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_USE_INTERNAL_EDITOR, internalRemoteEditor);
    conf.setValue(KEY_EXTERNAL_EDITOR_COMMAND, externalRemoteEditorCommand);
}

void Configuration::PathMachineSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_RELATIVE_PATH_ACCEPTANCE, acceptBestRelative);
    conf.setValue(KEY_ABSOLUTE_PATH_ACCEPTANCE, acceptBestAbsolute);
    conf.setValue(KEY_ROOM_CREATION_PENALTY, newRoomPenalty);
    conf.setValue(KEY_CORRECT_POSITION_BONUS, correctPositionBonus);
    conf.setValue(KEY_MAXIMUM_NUMBER_OF_PATHS, utils::clampNonNegative(maxPaths));
    conf.setValue(KEY_ROOM_MATCHING_TOLERANCE, utils::clampNonNegative(matchingTolerance));
    conf.setValue(KEY_MULTIPLE_CONNECTIONS_PENALTY, multipleConnectionsPenalty);
}

void Configuration::GroupManagerSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_GROUP_YOUR_COLOR, color.name());
    conf.setValue(KEY_GROUP_NPC_COLOR, npcColor);
    conf.setValue(KEY_GROUP_NPC_COLOR_OVERRIDE, npcColorOverride);
    conf.setValue(KEY_GROUP_NPC_HIDE, npcHide);
    conf.setValue(KEY_GROUP_NPC_SORT_BOTTOM, npcSortBottom);
    conf.setValue(KEY_GROUP_SHOW_TOKENS, showTokens);
    conf.setValue(KEY_GROUP_SHOW_MAP_TOKENS, showMapTokens);
    conf.setValue(KEY_GROUP_TOKEN_ICON_SIZE, tokenIconSize);
    conf.setValue(KEY_GROUP_SHOW_NPC_GHOSTS, showNpcGhosts);

    conf.beginGroup(KEY_GROUP_TOKEN_OVERRIDES);
    conf.remove(""); // wipe old map entries
    for (auto it = tokenOverrides.cbegin(); it != tokenOverrides.cend(); ++it)
        conf.setValue(it.key(), it.value());
    conf.endGroup();
}

void Configuration::MumeClockSettings::write(QSettings &conf) const
{
    // Note: There's no QVariant(int64_t) constructor.
    conf.setValue(KEY_MUME_START_EPOCH, static_cast<qlonglong>(startEpoch));
    conf.setValue(KEY_DISPLAY_CLOCK, display);
}

void Configuration::AdventurePanelSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_DISPLAY_XP_STATUS, m_displayXPStatus);
}

void Configuration::IntegratedMudClientSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_FONT, font);
    conf.setValue(KEY_BACKGROUND_COLOR, backgroundColor.name());
    conf.setValue(KEY_FOREGROUND_COLOR, foregroundColor.name());
    conf.setValue(KEY_COLUMNS, columns);
    conf.setValue(KEY_ROWS, rows);
    conf.setValue(KEY_LINES_OF_SCROLLBACK, linesOfScrollback);
    conf.setValue(KEY_LINES_OF_INPUT_HISTORY, linesOfInputHistory);
    conf.setValue(KEY_TAB_COMPLETION_DICTIONARY_SIZE, tabCompletionDictionarySize);
    conf.setValue(KEY_CLEAR_INPUT_ON_ENTER, clearInputOnEnter);
    conf.setValue(KEY_AUTO_RESIZE_TERMINAL, autoResizeTerminal);
    conf.setValue(KEY_LINES_OF_PEEK_PREVIEW, linesOfPeekPreview);
}

void Configuration::RoomPanelSettings::write(QSettings &conf) const
{
    conf.setValue(KEY_WINDOW_GEOMETRY, geometry);
}

void Configuration::InfomarksDialog::write(QSettings &conf) const
{
    conf.setValue(KEY_WINDOW_GEOMETRY, geometry);
}

void Configuration::RoomEditDialog::write(QSettings &conf) const
{
    conf.setValue(KEY_WINDOW_GEOMETRY, geometry);
}

void Configuration::FindRoomsDialog::write(QSettings &conf) const
{
    conf.setValue(KEY_WINDOW_GEOMETRY, geometry);
}

Configuration &setConfig()
{
    assert(g_config_enteredMain);
    assert(g_thread == std::this_thread::get_id());
    static Configuration conf;
    return conf;
}

const Configuration &getConfig()
{
    return setConfig();
}

void Configuration::NamedColorOptions::resetToDefaults()
{
    assert(Colors::black.getRGB() == 0 && Colors::black.getRGBA() != 0);
    static const auto fromHashHex = [](std::string_view sv) {
        assert(sv.length() == 7 && sv[0] == char_consts::C_POUND_SIGN);
        return Color::fromHex(sv.substr(1));
    };

    static const Color background = fromHashHex(DEFAULT_BGCOLOR);
    static const Color darkRoom = fromHashHex(DEFAULT_DARK_COLOR);
    static const Color noSundeath = fromHashHex(DEFAULT_NO_SUNDEATH_COLOR);
    static const Color road{140, 83, 58};     // closest well-known color is "Affair";
    static const Color special{204, 25, 204}; // closest well-known color is "Red Violet"
    static const Color noflee{123, 63, 0};    // closest well-known color is "Cinnamon"
    static const Color water{76, 216, 255};   // closest well-known color is "Malibu"

    BACKGROUND = background;
    CONNECTION_NORMAL = Colors::white;
    HIGHLIGHT_UNSAVED = Colors::cyan;
    HIGHLIGHT_TEMPORARY = Colors::red;
    HIGHLIGHT_NEEDS_SERVER_ID = Colors::yellow;
    INFOMARK_COMMENT = Colors::gray75;
    INFOMARK_HERB = Colors::green;
    INFOMARK_MOB = Colors::red;
    INFOMARK_OBJECT = Colors::yellow;
    INFOMARK_RIVER = water;
    INFOMARK_ROAD = road;
    ROOM_DARK = darkRoom;
    ROOM_NO_SUNDEATH = noSundeath;
    STREAM = water;
    TRANSPARENT = Color(0, 0, 0, 0);
    VERTICAL_COLOR_CLIMB = Colors::webGray;
    VERTICAL_COLOR_REGULAR_EXIT = Colors::white;
    WALL_COLOR_BUG_WALL_DOOR = Colors::red20;
    WALL_COLOR_CLIMB = Colors::gray70;
    WALL_COLOR_FALL_DAMAGE = Colors::cyan;
    WALL_COLOR_GUARDED = Colors::yellow;
    WALL_COLOR_NO_FLEE = noflee;
    WALL_COLOR_NO_MATCH = Colors::blue;
    WALL_COLOR_NOT_MAPPED = Colors::darkOrange1;
    WALL_COLOR_RANDOM = Colors::red;
    WALL_COLOR_REGULAR_EXIT = Colors::black;
    WALL_COLOR_SPECIAL = special;
}

Configuration::CanvasSettings::Advanced::Advanced()
{
    for (NamedConfig<bool> *const it : {&use3D, &autoTilt, &printPerfStats}) {
        const char *const name = it->getName().c_str();
        qInfo() << "Checking environment variable" << name;
        if (std::optional<bool> opt = utils::getEnvBool(name)) {
            const bool &val = opt.value();
            qInfo() << "Using value" << val << "from environment variable" << name;
            it->set(val);
        }
    }

    if ((false)) {
        for (const auto *it : {&fov, &verticalAngle, &horizontalAngle, &layerHeight}) {
            static_cast<void>(it);
        }
    }
}

void Configuration::CanvasSettings::Advanced::registerChangeCallback(
    const ChangeMonitor::Lifetime &lifetime, const ChangeMonitor::Function &callback)
{
    /* copied for each; consider changing this to be std::shared_ptr<Function> */
    use3D.registerChangeCallback(lifetime, callback);
    autoTilt.registerChangeCallback(lifetime, callback);
    printPerfStats.registerChangeCallback(lifetime, callback);
    fov.registerChangeCallback(lifetime, callback);
    verticalAngle.registerChangeCallback(lifetime, callback);
    horizontalAngle.registerChangeCallback(lifetime, callback);
    layerHeight.registerChangeCallback(lifetime, callback);
}

void setEnteredMain()
{
    g_thread = std::this_thread::get_id();
    g_config_enteredMain = true;
}

const Configuration::NamedColorOptions &getNamedColorOptions()
{
    assert(g_config_enteredMain);
    if (g_thread == std::this_thread::get_id()) {
        return getConfig().colorSettings;
    }

    return deref(tl_named_color_options);
}

const Configuration::CanvasNamedColorOptions &getCanvasNamedColorOptions()
{
    assert(g_config_enteredMain);
    if (g_thread == std::this_thread::get_id()) {
        return getConfig().canvas;
    }

    return deref(tl_canvas_named_color_options);
}

ThreadLocalNamedColorRaii::ThreadLocalNamedColorRaii(
    SharedCanvasNamedColorOptions canvasNamedColorOptions, SharedNamedColorOptions namedColorOptions)
{
    assert(g_config_enteredMain);
    assert(g_thread != std::this_thread::get_id());
    tl_canvas_named_color_options = std::move(canvasNamedColorOptions);
    tl_named_color_options = std::move(namedColorOptions);
}

ThreadLocalNamedColorRaii::~ThreadLocalNamedColorRaii()
{
    tl_canvas_named_color_options.reset();
    tl_named_color_options.reset();
}
