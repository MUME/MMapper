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

static QByteArray addTerminalTypeSuffix(const std::string_view &prefix)
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
    : AbstractTelnet(TextCodecStrategyEnum::FORCE_LATIN_1,
                     false,
                     parent,
                     addTerminalTypeSuffix("unknown"))
{
    // RFC 2066 states we can provide many character sets but we force Latin-1 when
    // communicating with MUME
}

void MudTelnet::onConnected()
{
    reset();
    // MUME opts to not send DO CHARSET due to older, broken clients
    sendTelnetOption(TN_WILL, OPT_CHARSET);
    announcedState[OPT_CHARSET] = true;
}

void MudTelnet::onAnalyzeMudStream(const QByteArray &data)
{
    onReadInternal(data);
}

void MudTelnet::onSendToMud(const QByteArray &ba)
{
    // Bytes are already Latin-1 so we just send it to MUME
    submitOverTelnet(ba, false);
}

void MudTelnet::onRelayNaws(const int x, const int y)
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

void MudTelnet::onRelayTermType(const QByteArray &terminalType)
{
    // Append the MMapper version suffix to the terminal type
    setTerminalType(addTerminalTypeSuffix(terminalType.constData()));
    if (myOptionState[OPT_TERMINAL_TYPE]) {
        sendTerminalType(getTerminalType());
    }
}

void MudTelnet::sendToMapper(const QByteArray &data, bool goAhead)
{
    emit analyzeMudStream(data, goAhead);
}

void MudTelnet::receiveEchoMode(bool toggle)
{
    emit relayEchoMode(toggle);
}

/** Send out the data. Does not double IACs, this must be done
            by caller if needed. This function is suitable for sending
            telnet sequences. */
void MudTelnet::sendRawData(const QByteArray &data)
{
    sentBytes += data.length();
    emit sendToSocket(data);
}
