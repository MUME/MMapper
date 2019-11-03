// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "UserTelnet.h"
#include "../configuration/configuration.h"

UserTelnet::UserTelnet(QObject *const parent)
    : AbstractTelnet(TextCodec(TextCodecStrategyEnum::AUTO_SELECT_CODEC), false, parent)
{}

void UserTelnet::onConnected()
{
    reset();
    // Negotiate options
    requestTelnetOption(TN_DO, OPT_TERMINAL_TYPE);
    requestTelnetOption(TN_DO, OPT_NAWS);
    requestTelnetOption(TN_DO, OPT_CHARSET);
}

void UserTelnet::onAnalyzeUserStream(const QByteArray &data)
{
    onReadInternal(data);
}

void UserTelnet::onSendToUser(const QByteArray &ba, const bool goAhead)
{
    // MMapper internally represents all data as Latin-1
    QString temp = QString::fromLatin1(ba);

    // Switch codec if RFC 2066 was not negotiated and the configuration was altered
    const CharacterEncodingEnum configEncoding = getConfig().general.characterEncoding;
    if (!hisOptionState[OPT_CHARSET] && configEncoding != textCodec.getEncoding()) {
        textCodec.setEncoding(configEncoding);
    }

    // Convert from unicode into the client requested encoding
    QByteArray outdata = textCodec.fromUnicode(temp);

    submitOverTelnet(outdata, goAhead);
}

void UserTelnet::sendToMapper(const QByteArray &data, const bool goAhead)
{
    // MMapper requires all data to be Latin-1 internally
    // REVISIT: This will break things if the client is handling MPI itself
    QByteArray outdata = textCodec.toUnicode(data).toLatin1();
    emit analyzeUserStream(outdata, goAhead);
}

void UserTelnet::onRelayEchoMode(const bool isDisabled)
{
    sendTelnetOption(isDisabled ? TN_WONT : TN_WILL, OPT_ECHO);
    myOptionState[OPT_ECHO] = !isDisabled;
    announcedState[OPT_ECHO] = true;
}

void UserTelnet::receiveTerminalType(const QByteArray &data)
{
    emit relayTermType(data);
}

void UserTelnet::receiveWindowSize(const int x, const int y)
{
    emit relayNaws(x, y);
}

/** Send out the data. Does not double IACs, this must be done
            by caller if needed. This function is suitable for sending
            telnet sequences. */
void UserTelnet::sendRawData(const QByteArray &data)
{
    sentBytes += data.length();
    emit sendToSocket(data);
}
