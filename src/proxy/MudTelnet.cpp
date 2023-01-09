// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "MudTelnet.h"

#include <sstream>
#include <string>
#include <string_view>
#include <QByteArray>
#include <QSysInfo>

#include "../configuration/configuration.h"
#include "../display/MapCanvasConfig.h"
#include "../global/TextUtils.h"
#include "../global/Version.h"
#include "GmcpUtils.h"

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

    std::stringstream ss;
    ss << prefix << "/MMapper-" << getMMapperVersion() << "/"
       << MapCanvasConfig::getCurrentOpenGLVersion() << "/" << get_os_string() << "/"
       << arch.constData();
    return ::toQByteArrayLatin1(ss.str());
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
    submitOverTelnet(::toStdStringViewLatin1(ba), false);
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
                const GmcpModule module(moduleStr);
                receiveGmcpModule(module, !msg.isCoreSupportsRemove());

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

void MudTelnet::virt_onGmcpEnabled()
{
    if (debug)
        qDebug() << "Requesting GMCP from MUME";

    // Per MUME documentation, this must be the very first client GMCP message
    // https://mume.org/help/generic_mud_communication_protocol
    sendGmcpMessage(GmcpMessage(GmcpMessageTypeEnum::CORE_HELLO,
                                QString(R"({ "client": "MMapper", "version": "%1" })")
                                    .arg(GmcpUtils::escapeGmcpStringData(getMMapperVersion()))));

    // Request GMCP modules that might have already been sent by the local client
    sendCoreSupports();

    // Request that Narrates and Tells be emitted via GMCP
    sendGmcpMessage(GmcpMessage(GmcpMessageTypeEnum::COMM_CHANNEL_ENABLE, QString(R"("tells")")));
    sendGmcpMessage(GmcpMessage(GmcpMessageTypeEnum::COMM_CHANNEL_ENABLE, QString(R"("narrates")")));

    if (receivedExternalDiscordHello)
        sendGmcpMessage(GmcpMessage{GmcpMessageTypeEnum::EXTERNAL_DISCORD_HELLO});
}

/** Send out the data. Does not double IACs, this must be done
            by caller if needed. This function is suitable for sending
            telnet sequences. */
void MudTelnet::virt_sendRawData(const std::string_view data)
{
    sentBytes += data.length();
    emit sig_sendToSocket(::toQByteArrayLatin1(data));
}

void MudTelnet::receiveGmcpModule(const GmcpModule &module, const bool enabled)
{
    if (enabled)
        gmcp.insert(module);
    else
        gmcp.erase(module);
}

void MudTelnet::resetGmcpModules()
{
    gmcp.clear();

    // Following modules are enabled by default
    receiveGmcpModule(GmcpModule{GmcpModuleTypeEnum::CHAR, GmcpModuleVersion{1}}, true);
    receiveGmcpModule(GmcpModule{GmcpModuleTypeEnum::COMM_CHANNEL, GmcpModuleVersion{1}}, true);
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
    for (const GmcpModule &module : gmcp) {
        if (comma)
            oss << ", ";
        oss << '"' << module.toStdString() << '"';
        comma = true;
    }
    oss << " ]";
    const std::string set = oss.str();

    if (debug)
        qDebug() << "Sending GMCP Core.Supports to MUME" << ::toQByteArrayLatin1(set);

    sendGmcpMessage(GmcpMessage(GmcpMessageTypeEnum::CORE_SUPPORTS_SET, set));
}
