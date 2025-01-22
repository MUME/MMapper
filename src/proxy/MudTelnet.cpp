// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "MudTelnet.h"

#include "../clock/mumeclock.h"
#include "../configuration/configuration.h"
#include "../display/MapCanvasConfig.h"
#include "../global/Consts.h"
#include "../global/LineUtils.h"
#include "../global/TextUtils.h"
#include "../global/Version.h"
#include "../mpi/mpifilter.h"
#include "GmcpUtils.h"

#include <charconv>
#include <list>
#include <map>
#include <optional>
#include <sstream>
#include <string_view>

#include <QByteArray>
#include <QOperatingSystemVersion>
#include <QSysInfo>

namespace { // anonymous

constexpr const auto GAME_YEAR = "GAME YEAR";
constexpr const auto GAME_MONTH = "GAME MONTH";
constexpr const auto GAME_DAY = "GAME DAY";
constexpr const auto GAME_HOUR = "GAME HOUR";

NODISCARD std::optional<std::string> getMajorMinor()
{
    const auto major = QOperatingSystemVersion::current().majorVersion();
    if (major < 0) {
        return std::nullopt;
    }
    const auto minor = QOperatingSystemVersion::current().minorVersion();
    if (minor < 0) {
        return std::to_string(major);
    }
    return std::to_string(major) + "." + std::to_string(minor);
}

NODISCARD std::string getOsName()
{
#define X_CASE(X) \
    case (PlatformEnum::X): \
        return #X

    switch (CURRENT_PLATFORM) {
        X_CASE(Linux);
        X_CASE(Mac);
        X_CASE(Windows);
        X_CASE(Unknown);
    }
    std::abort();
#undef X_CASE
}

NODISCARD std::string getOs()
{
    if (auto ver = getMajorMinor()) {
        return getOsName() + ver.value();
    }
    return getOsName();
}

NODISCARD TelnetTermTypeBytes addTerminalTypeSuffix(const std::string_view prefix)
{
    // It's probably required to be ASCII.
    const auto arch = mmqt::toStdStringUtf8(QSysInfo::currentCpuArchitecture().toUtf8());

    std::ostringstream ss;
    ss << prefix << "/MMapper-" << getMMapperVersion() << "/"
       << MapCanvasConfig::getCurrentOpenGLVersion() << "/" << getOs() << "/" << arch;
    auto str = std::move(ss).str();

    return TelnetTermTypeBytes{mmqt::toQByteArrayUtf8(str)};
}

using OptStdString = std::optional<std::string>;
struct MsspMap final
{
private:
    // REVISIT: why is this a list? always prefer vector over list, unless there's a good reason.
    // (And the good reason needs to be documented.)
    std::map<std::string, std::list<std::string>> m_map;

public:
    // Parse game time from MSSP
    NODISCARD OptStdString lookup(const std::string &key) const
    {
        const auto it = m_map.find(key);
        if (it == m_map.end()) {
            MMLOG_WARNING() << "MSSP missing key " << key;
            return std::nullopt;
        }
        const auto &elements = it->second;
        if (elements.empty()) {
            MMLOG_WARNING() << "MSSP empty key " << key;
            return std::nullopt;
        }
        // REVISIT: protocols that allow duplicates usually declare that the LAST one is correct,
        // but we're taking the first one here.
        return elements.front();
    }

public:
    NODISCARD static auto parseMssp(const TelnetMsspBytes &data, const bool debug)
    {
        MsspMap result;
        auto &map = result.m_map;

        std::optional<std::string> varName = std::nullopt;
        std::list<std::string> vals;

        enum class NODISCARD MSSPStateEnum {
            ///
            BEGIN,
            /// VAR
            IN_VAR,
            /// VAL
            IN_VAL
        } state
            = MSSPStateEnum::BEGIN;

        AppendBuffer buffer;

        const auto addValue([&map, &vals, &varName, &buffer, debug]() {
            // Put it into the map.
            if (debug) {
                qDebug() << "MSSP received value" << buffer << "for variable"
                         << mmqt::toQByteArrayRaw(varName.value());
            }

            vals.push_back(buffer.getQByteArray().toStdString());
            map[varName.value()] = vals;

            buffer.clear();
        });

        for (const char c : data) {
            switch (state) {
            case MSSPStateEnum::BEGIN:
                if (c != TNSB_MSSP_VAR) {
                    continue;
                }
                state = MSSPStateEnum::IN_VAR;
                break;

            case MSSPStateEnum::IN_VAR:
                switch (c) {
                case TNSB_MSSP_VAR:
                case TN_IAC:
                case 0:
                    continue;

                case TNSB_MSSP_VAL: {
                    if (buffer.isEmpty()) {
                        if (debug) {
                            qDebug() << "MSSP received variable without any name; ignoring it";
                        }
                        continue;
                    }

                    if (debug) {
                        qDebug() << "MSSP received variable" << buffer;
                    }

                    varName = buffer.getQByteArray().toStdString();
                    state = MSSPStateEnum::IN_VAL;

                    vals.clear(); // Which means this is a new value, so clear the list.
                    buffer.clear();
                } break;

                default:
                    buffer.append(static_cast<uint8_t>(c));
                }
                break;

            case MSSPStateEnum::IN_VAL: {
                assert(varName.has_value());

                switch (c) {
                case TN_IAC:
                case 0:
                    continue;

                case TNSB_MSSP_VAR:
                    state = MSSPStateEnum::IN_VAR;
                    FALLTHROUGH;
                case TNSB_MSSP_VAL:
                    addValue();
                    break;

                default:
                    buffer.append(static_cast<uint8_t>(c));
                    break;
                }
                break;
            }
            }
        }

        if (varName.has_value() && !buffer.isEmpty()) {
            addValue();
        }

        return result;
    }
};

} // namespace

MudTelnetOutputs::~MudTelnetOutputs() = default;

MudTelnet::MudTelnet(MudTelnetOutputs &outputs)
    : AbstractTelnet(TextCodecStrategyEnum::FORCE_UTF_8, addTerminalTypeSuffix("unknown"))
    , m_outputs{outputs}
{
    // RFC 2066 states we can provide many character sets but we force UTF-8 when
    // communicating with MUME
    resetGmcpModules();
}

void MudTelnet::onDisconnected()
{
    // Reset Telnet options but retain GMCP modules
    reset();
}

void MudTelnet::onAnalyzeMudStream(const TelnetIacBytes &data)
{
    onReadInternal(data);
}

void MudTelnet::onSubmitMpiToMud(const RawBytes &bytes)
{
    assert(isMpiMessage(bytes));
    submitOverTelnet(bytes, false);
}

NODISCARD static bool isOneLineCrlf(const QString &s)
{
    if (s.isEmpty() || s.back() != char_consts::C_NEWLINE) {
        return false;
    }
    QStringView sv = s;
    sv.chop(1);
    return !sv.empty() && sv.back() == char_consts::C_CARRIAGE_RETURN
           && !sv.contains(char_consts::C_NEWLINE);
}

void MudTelnet::submitOneLine(const QString &s)
{
    assert(isOneLineCrlf(s));
    if (hasMpiPrefix(s)) {
        // It would be useful to send feedback to the user.
        const auto mangled = char_consts::C_SPACE + s;
        qWarning() << "mangling command that contains MPI prefix" << mangled;
        submitOverTelnet(mangled, false);
        return;
    }

    submitOverTelnet(s, false);
}

void MudTelnet::onSendToMud(const QString &s)
{
    if (s.isEmpty()) {
        assert(false);
        return;
    }

    if (m_lineBuffer.isEmpty() && isOneLineCrlf(s)) {
        submitOneLine(s);
        return;
    }

    // fallback: buffering
    mmqt::foreachLine(s, [this](QStringView line, const bool hasNewline) {
        if (hasNewline && !line.empty() && line.back() == char_consts::C_CARRIAGE_RETURN) {
            line.chop(1);
        }

        m_lineBuffer += line;
        if (!hasNewline) {
            return;
        }

        const auto oneline = std::exchange(m_lineBuffer, {}) + string_consts::S_CRLF;
        submitOneLine(oneline);
    });
}

void MudTelnet::onGmcpToMud(const GmcpMessage &msg)
{
    const auto &hisOptionState = getOptions().hisOptionState;

    // Remember Core.Supports.[Add|Set|Remove] modules
    if (msg.getJson()
        && (msg.isCoreSupportsAdd() || msg.isCoreSupportsSet() || msg.isCoreSupportsRemove())
        && msg.getJsonDocument().has_value() && msg.getJsonDocument()->isArray()) {
        // Handle the various messages
        const auto &array = msg.getJsonDocument()->array();
        if (msg.isCoreSupportsSet()) {
            resetGmcpModules();
        }
        for (const auto &e : array) {
            if (!e.isString()) {
                continue;
            }
            const auto &moduleStr = e.toString();
            try {
                const GmcpModule mod(mmqt::toStdStringUtf8(moduleStr));
                receiveGmcpModule(mod, !msg.isCoreSupportsRemove());

            } catch (const std::exception &e) {
                qWarning() << "Module" << moduleStr
                           << (msg.isCoreSupportsRemove() ? "remove" : "add")
                           << "error because:" << e.what();
            }
        }

        // Send it now if GMCP has been negotiated
        if (hisOptionState[OPT_GMCP]) {
            sendCoreSupports();
        }
        return;
    }

    if (msg.isExternalDiscordHello()) {
        m_receivedExternalDiscordHello = true;
    }

    if (!hisOptionState[OPT_GMCP]) {
        qDebug() << "MUME did not request GMCP yet";
        return;
    }

    sendGmcpMessage(msg);
}

void MudTelnet::onRelayNaws(const int width, const int height)
{
    // remember the size - we'll need it if NAWS is currently disabled but will
    // be enabled. Also remember it if no connection exists at the moment;
    // we won't be called again when connecting
    m_currentNaws.width = width;
    m_currentNaws.height = height;

    if (getOptions().myOptionState[OPT_NAWS]) {
        // only if we have negotiated this option
        sendWindowSizeChanged(width, height);
    }
}

void MudTelnet::onRelayTermType(const TelnetTermTypeBytes &terminalType)
{
    // Append the MMapper version suffix to the terminal type
    setTerminalType(addTerminalTypeSuffix(terminalType.getQByteArray().constData()));
    if (getOptions().myOptionState[OPT_TERMINAL_TYPE]) {
        sendTerminalType(getTerminalType());
    }
}

void MudTelnet::virt_sendToMapper(const RawBytes &data, const bool goAhead)
{
    if (getDebug()) {
        qDebug() << "MudTelnet::virt_sendToMapper" << data;
    }

    m_outputs.onAnalyzeMudStream(data, goAhead);
}

void MudTelnet::virt_receiveEchoMode(const bool toggle)
{
    m_outputs.onRelayEchoMode(toggle);
}

void MudTelnet::virt_receiveGmcpMessage(const GmcpMessage &msg)
{
    if (getDebug()) {
        qDebug() << "Receiving GMCP from MUME" << msg.toRawBytes();
    }

    m_outputs.onRelayGmcpFromMudToUser(msg);
}

void MudTelnet::virt_receiveMudServerStatus(const TelnetMsspBytes &ba)
{
    parseMudServerStatus(ba);
    m_outputs.onSendMSSPToUser(ba);
}

void MudTelnet::virt_onGmcpEnabled()
{
    if (getDebug()) {
        qDebug() << "Requesting GMCP from MUME";
    }

    sendGmcpMessage(
        GmcpMessage(GmcpMessageTypeEnum::CORE_HELLO,
                    GmcpJson{QString(R"({ "client": "MMapper", "version": "%1" })")
                                 .arg(GmcpUtils::escapeGmcpStringData(getMMapperVersion()))}));

    // Request GMCP modules that might have already been sent by the local client
    sendCoreSupports();

    if (m_receivedExternalDiscordHello) {
        sendGmcpMessage(GmcpMessage{GmcpMessageTypeEnum::EXTERNAL_DISCORD_HELLO});
    }
}

void MudTelnet::virt_sendRawData(const TelnetIacBytes &data)
{
    m_outputs.onSendToSocket(data);
}

void MudTelnet::receiveGmcpModule(const GmcpModule &mod, const bool enabled)
{
    if (enabled) {
        m_gmcp.insert(mod);
    } else {
        m_gmcp.erase(mod);
    }
}

void MudTelnet::resetGmcpModules()
{
    m_gmcp.clear();

    // Following modules are enabled by default
    receiveGmcpModule(GmcpModule{GmcpModuleTypeEnum::CHAR, GmcpModuleVersion{1}}, true);
    receiveGmcpModule(GmcpModule{GmcpModuleTypeEnum::EVENT, GmcpModuleVersion{1}}, true);
    receiveGmcpModule(GmcpModule{GmcpModuleTypeEnum::EXTERNAL_DISCORD, GmcpModuleVersion{1}}, true);
    receiveGmcpModule(GmcpModule{GmcpModuleTypeEnum::ROOM_CHARS, GmcpModuleVersion{1}}, true);
    receiveGmcpModule(GmcpModule{GmcpModuleTypeEnum::ROOM, GmcpModuleVersion{1}}, true);
}

void MudTelnet::sendCoreSupports()
{
    if (m_gmcp.empty()) {
        qWarning() << "No GMCP modules can be requested";
        return;
    }

    std::ostringstream oss;
    oss << "[ ";
    bool comma = false;
    for (const GmcpModule &mod : m_gmcp) {
        if (comma) {
            oss << ", ";
        }
        oss << char_consts::C_DQUOTE << mod.toStdString() << char_consts::C_DQUOTE;
        comma = true;
    }
    oss << " ]";
    const std::string set = oss.str();

    if (getDebug()) {
        qDebug() << "Sending GMCP Core.Supports to MUME" << mmqt::toQByteArrayUtf8(set);
    }
    sendGmcpMessage(GmcpMessage(GmcpMessageTypeEnum::CORE_SUPPORTS_SET, GmcpJson{set}));
}

void MudTelnet::parseMudServerStatus(const TelnetMsspBytes &data)
{
    const auto map = MsspMap::parseMssp(data, getDebug());

    // REVISIT: try to read minute, in case mume ever supports it?
    const OptStdString yearStr = map.lookup(GAME_YEAR);
    const OptStdString monthStr = map.lookup(GAME_MONTH);
    const OptStdString dayStr = map.lookup(GAME_DAY);
    const OptStdString hourStr = map.lookup(GAME_HOUR);

    MMLOG() << "MSSP game time received with"
            << " year:" << yearStr.value_or("unknown") << " month:" << monthStr.value_or("unknown")
            << " day:" << dayStr.value_or("unknown") << " hour:" << hourStr.value_or("unknown");

    if (!yearStr.has_value() || !monthStr.has_value() || !dayStr.has_value()
        || !hourStr.has_value()) {
        MMLOG_WARNING() << "missing one or more MSSP keys";
        return;
    }

    using OptInt = std::optional<int>;
    auto my_stoi = [](const OptStdString &optString) -> OptInt {
        if (!optString.has_value()) {
            return std::nullopt;
        }
        const auto &s = optString.value();
        const auto beg = s.data();
        // NOLINTNEXTLINE (pointer arithmetic)
        const auto end = beg + static_cast<ptrdiff_t>(s.size());
        int result = 0;
        auto [ptr, ec] = std::from_chars(beg, end, result);
        if (ec != std::errc{} || (ptr != nullptr && *ptr != char_consts::C_NUL)) {
            return std::nullopt;
        }
        return result;
    };

    const OptInt year = my_stoi(yearStr);
    const OptInt month = MumeClock::getMumeMonth(mmqt::toQStringUtf8(monthStr.value()));
    const OptInt day = my_stoi(dayStr);
    const OptInt hour = my_stoi(hourStr);

    if (!year.has_value() || !month.has_value() || !day.has_value() || !hour.has_value()) {
        MMLOG_WARNING() << "invalid date values";
        return;
    }

    const auto msspTime = MsspTime{year.value(), month.value(), day.value(), hour.value()};

    auto warn_if_invalid = [](const std::string_view what, const int n, const int lo, const int hi) {
        if (n < lo || n > hi) {
            MMLOG_WARNING() << "invalid " << what << ": " << n;
        }
    };

    // MUME's official start is 2850, and the end is 3018 at the start of the fellowship.
    // However, the historical average reset time has been around 3023 (about a RL month late).
    //
    // (note: 3018 - 2850 = 168 game years = 1008 RL days = ~2.76 RL years, and
    //        3023 - 2850 = 173 game years = 1038 RL days = ~2.84 RL years).
    //
    // Let's err on the side of caution in case someone forgets to reset the time.
    const int max_rl_years = 6;
    const int mud_years_per_rl_year = MUME_MINUTES_PER_HOUR;
    const int max_year = MUME_START_YEAR + mud_years_per_rl_year * max_rl_years;

    // TODO: stronger validation of the integers here;
    warn_if_invalid("year", msspTime.year, MUME_START_YEAR, max_year);
    warn_if_invalid("month", msspTime.month, 0, MUME_MONTHS_PER_YEAR - 1);
    warn_if_invalid("day", msspTime.day, 0, MUME_DAYS_PER_MONTH - 1);
    warn_if_invalid("hour", msspTime.hour, 0, MUME_MINUTES_PER_HOUR - 1);

    m_outputs.onSendGameTimeToClock(msspTime);
}
