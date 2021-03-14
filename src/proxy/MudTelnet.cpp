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
#include "../global/TextUtils.h"
#include "../global/Version.h"
#include "GmcpUtils.h"

NODISCARD static QByteArray addTerminalTypeSuffix(const std::string_view &prefix)
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
    ss << prefix << "/MMapper-" << getMMapperVersion() << "/" << get_os_string() << "/"
       << arch.constData();
    return ::toQByteArrayLatin1(ss.str());
}

MudTelnet::MudTelnet(QObject *const parent)
    : AbstractTelnet(TextCodecStrategyEnum::FORCE_LATIN_1, parent, addTerminalTypeSuffix("unknown"))
{
    // RFC 2066 states we can provide many character sets but we force Latin-1 when
    // communicating with MUME
}

void MudTelnet::slot_onConnected()
{
    reset();
    // MUME opts to not send DO CHARSET due to older, broken clients
    requestTelnetOption(TN_WILL, OPT_CHARSET);
    // Assume MUME will also opt to not send DO GMCP similar to CHARSET
    requestTelnetOption(TN_WILL, OPT_GMCP);
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
    if (myOptionState[OPT_GMCP])
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
    emit sig_relayGmcp(msg);
}

void MudTelnet::virt_onGmcpEnabled()
{
    sendGmcpMessage(GmcpMessage(GmcpMessageTypeEnum::CORE_HELLO,
                                QString(R"({ "client": "MMapper", "version": "%1" })")
                                    .arg(GmcpUtils::escapeGmcpStringData(getMMapperVersion()))));
}

/** Send out the data. Does not double IACs, this must be done
            by caller if needed. This function is suitable for sending
            telnet sequences. */
void MudTelnet::virt_sendRawData(const std::string_view &data)
{
    sentBytes += data.length();
    emit sig_sendToSocket(::toQByteArrayLatin1(data));
}
