// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "MudTelnet.h"

#include "../display/MapCanvasConfig.h"
#include "../global/Consts.h"
#include "../global/Version.h"
#include "GmcpUtils.h"

#include <list>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include <QByteArray>
#include <QSysInfo>

NODISCARD static QByteArray addTerminalTypeSuffix(const std::string_view prefix)
{
    const auto get_os_string = []() {
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Linux)
            return "Linux";
        else if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac)
            return "Mac";
        else if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows)
            return "Windows";
        else {
            assert(CURRENT_PLATFORM == PlatformEnum::Unknown);
            return "Unknown";
        }
    };
    const auto arch = QSysInfo::currentCpuArchitecture().toLatin1();

    std::ostringstream ss;
    ss << prefix << "/MMapper-" << getMMapperVersion() << "/"
       << MapCanvasConfig::getCurrentOpenGLVersion() << "/" << get_os_string() << "/"
       << arch.constData();
    return mmqt::toQByteArrayLatin1(ss.str());
}

MudTelnet::MudTelnet(QObject *const parent)
    : AbstractTelnet(TextCodecStrategyEnum::FORCE_LATIN_1, parent, addTerminalTypeSuffix("unknown"))
{
    // RFC 2066 states we can provide many character sets but we force Latin-1 when
    // communicating with MUME
    resetGmcpModules();
}

void MudTelnet::slot_onDisconnected()
{
    // Reset Telnet options but retain GMCP modules
    reset();
}

void MudTelnet::slot_onAnalyzeMudStream(const QByteArray &data)
{
    onReadInternal(data);
}

void MudTelnet::slot_onSendToMud(const QByteArray &ba)
{
    // Bytes are already Latin-1 so we just send it to MUME
    submitOverTelnet(mmqt::toStdStringViewLatin1(ba), false);
}

void MudTelnet::slot_onGmcpToMud(const GmcpMessage &msg)
{
    // Remember Core.Supports.[Add|Set|Remove] modules
    if (msg.getJson()
        && (msg.isCoreSupportsAdd() || msg.isCoreSupportsSet() || msg.isCoreSupportsRemove())
        && msg.getJsonDocument().has_value() && msg.getJsonDocument()->isArray()) {
        // Handle the various messages
        const auto &array = msg.getJsonDocument()->array();
        if (msg.isCoreSupportsSet())
            resetGmcpModules();
        for (const auto &e : array) {
            if (!e.isString())
                continue;
            const auto &moduleStr = e.toString();
            try {
                const GmcpModule mod(mmqt::toStdStringLatin1(moduleStr));
                receiveGmcpModule(mod, !msg.isCoreSupportsRemove());

            } catch (const std::exception &e) {
                qWarning() << "Module" << moduleStr
                           << (msg.isCoreSupportsRemove() ? "remove" : "add")
                           << "error because:" << e.what();
            }
        }

        // Send it now if GMCP has been negotiated
        if (hisOptionState[OPT_GMCP])
            sendCoreSupports();
        return;
    }

    if (msg.isExternalDiscordHello())
        receivedExternalDiscordHello = true;

    if (!hisOptionState[OPT_GMCP]) {
        qDebug() << "MUME did not request GMCP yet";
        return;
    }

    sendGmcpMessage(msg);
}

void MudTelnet::slot_onRelayNaws(const int x, const int y)
{
    // remember the size - we'll need it if NAWS is currently disabled but will
    // be enabled. Also remember it if no connection exists at the moment;
    // we won't be called again when connecting
    current.x = x;
    current.y = y;

    if (myOptionState[OPT_NAWS]) {
        // only if we have negotiated this option
        sendWindowSizeChanged(x, y);
    }
}

void MudTelnet::slot_onRelayTermType(const QByteArray &terminalType)
{
    // Append the MMapper version suffix to the terminal type
    setTerminalType(addTerminalTypeSuffix(terminalType.constData()));
    if (myOptionState[OPT_TERMINAL_TYPE]) {
        sendTerminalType(getTerminalType());
    }
}

void MudTelnet::virt_sendToMapper(const QByteArray &data, bool goAhead)
{
    emit sig_analyzeMudStream(data, goAhead);
}

void MudTelnet::virt_receiveEchoMode(bool toggle)
{
    emit sig_relayEchoMode(toggle);
}

void MudTelnet::virt_receiveGmcpMessage(const GmcpMessage &msg)
{
    if (debug)
        qDebug() << "Receiving GMCP from MUME" << msg.toRawBytes();

    emit sig_relayGmcp(msg);
}

void MudTelnet::virt_receiveMudServerStatus(const QByteArray &ba)
{
    parseMudServerStatus(ba);
    emit sig_sendMSSPToUser(ba);
}

void MudTelnet::virt_onGmcpEnabled()
{
    if (debug)
        qDebug() << "Requesting GMCP from MUME";

    sendGmcpMessage(
        GmcpMessage(GmcpMessageTypeEnum::CORE_HELLO,
                    GmcpJson{QString(R"({ "client": "MMapper", "version": "%1" })")
                                 .arg(GmcpUtils::escapeGmcpStringData(getMMapperVersion()))}));

    // Request GMCP modules that might have already been sent by the local client
    sendCoreSupports();

    if (receivedExternalDiscordHello)
        sendGmcpMessage(GmcpMessage{GmcpMessageTypeEnum::EXTERNAL_DISCORD_HELLO});
}

/** Send out the data. Does not double IACs, this must be done
            by caller if needed. This function is suitable for sending
            telnet sequences. */
void MudTelnet::virt_sendRawData(const std::string_view data)
{
    sentBytes += data.length();
    emit sig_sendToSocket(mmqt::toQByteArrayLatin1(data));
}

void MudTelnet::receiveGmcpModule(const GmcpModule &mod, const bool enabled)
{
    if (enabled)
        gmcp.insert(mod);
    else
        gmcp.erase(mod);
}

void MudTelnet::resetGmcpModules()
{
    gmcp.clear();

    // Following modules are enabled by default
    receiveGmcpModule(GmcpModule{GmcpModuleTypeEnum::CHAR, GmcpModuleVersion{1}}, true);
    receiveGmcpModule(GmcpModule{GmcpModuleTypeEnum::EVENT, GmcpModuleVersion{1}}, true);
    receiveGmcpModule(GmcpModule{GmcpModuleTypeEnum::EXTERNAL_DISCORD, GmcpModuleVersion{1}}, true);
    receiveGmcpModule(GmcpModule{GmcpModuleTypeEnum::ROOM_CHARS, GmcpModuleVersion{1}}, true);
}

void MudTelnet::sendCoreSupports()
{
    if (gmcp.empty()) {
        qWarning() << "No GMCP modules can be requested";
        return;
    }

    std::ostringstream oss;
    oss << "[ ";
    bool comma = false;
    for (const GmcpModule &mod : gmcp) {
        if (comma)
            oss << ", ";
        oss << char_consts::C_DQUOTE << mod.toStdString() << char_consts::C_DQUOTE;
        comma = true;
    }
    oss << " ]";
    const std::string set = oss.str();

    if (debug)
        qDebug() << "Sending GMCP Core.Supports to MUME" << mmqt::toQByteArrayLatin1(set);

    sendGmcpMessage(GmcpMessage(GmcpMessageTypeEnum::CORE_SUPPORTS_SET, GmcpJson{set}));
}

void MudTelnet::parseMudServerStatus(const QByteArray &data)
{
    std::map<std::string, std::list<std::string>> map;

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

    const auto addValue([&map, &vals, &varName, &buffer, this]() {
        // Put it into the map.
        if (debug)
            qDebug() << "MSSP received value" << mmqt::toQByteArrayLatin1(buffer.toStdString())
                     << "for variable" << mmqt::toQByteArrayLatin1(varName.value());

        vals.push_back(buffer.toStdString());
        map[varName.value()] = vals;

        buffer.clear();
    });

    for (int i = 0; i < data.size(); i++) {
        switch (state) {
        case MSSPStateEnum::BEGIN:
            if (data.at(i) != TNSB_MSSP_VAR)
                continue;
            state = MSSPStateEnum::IN_VAR;
            break;

        case MSSPStateEnum::IN_VAR:
            switch (data.at(i)) {
            case TNSB_MSSP_VAR:
            case TN_IAC:
            case 0:
                continue;

            case TNSB_MSSP_VAL: {
                if (buffer.isEmpty()) {
                    if (debug)
                        qDebug() << "MSSP received variable without any name; ignoring it";
                    continue;
                }

                if (debug)
                    qDebug() << "MSSP received variable"
                             << mmqt::toQByteArrayLatin1(buffer.toStdString());

                varName = buffer.toStdString();
                state = MSSPStateEnum::IN_VAL;

                vals.clear(); // Which means this is a new value, so clear the list.
                buffer.clear();
            } break;

            default:
                buffer.append(static_cast<uint8_t>(data.at(i)));
            }
            break;

        case MSSPStateEnum::IN_VAL: {
            assert(varName.has_value());

            switch (data.at(i)) {
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
                buffer.append(static_cast<uint8_t>(data.at(i)));
                break;
            }
            break;

        } break;
        }
    }

    if (varName.has_value() && !buffer.isEmpty())
        addValue();

    // Parse game time from MSSP
    const auto firstElement(
        [](const std::list<std::string> &elements) -> std::optional<std::string> {
            return elements.empty() ? std::nullopt : std::optional<std::string>{elements.front()};
        });
    const auto yearStr = firstElement(map["GAME YEAR"]);
    const auto monthStr = firstElement(map["GAME MONTH"]);
    const auto dayStr = firstElement(map["GAME DAY"]);
    const auto hourStr = firstElement(map["GAME HOUR"]);

    qInfo() << "MSSP game time received with"
            << "year:" << mmqt::toQByteArrayLatin1(yearStr.value_or("unknown"))
            << "month:" << mmqt::toQByteArrayLatin1(monthStr.value_or("unknown"))
            << "day:" << mmqt::toQByteArrayLatin1(dayStr.value_or("unknown"))
            << "hour:" << mmqt::toQByteArrayLatin1(hourStr.value_or("unknown"));

    if (yearStr.has_value() && monthStr.has_value() && dayStr.has_value() && hourStr.has_value()) {
        const int year = stoi(yearStr.value());
        const int day = stoi(dayStr.value());
        const int hour = stoi(hourStr.value());
        emit sig_sendGameTimeToClock(year, monthStr.value(), day, hour);
    }
}
