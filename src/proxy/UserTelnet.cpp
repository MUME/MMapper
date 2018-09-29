/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "UserTelnet.h"
#include "../configuration/configuration.h"

UserTelnet::UserTelnet(QObject *const parent)
    : AbstractTelnet(TextCodec(TextCodecStrategy::AUTO_SELECT_CODEC), false, parent)
{}

void UserTelnet::onConnected()
{
    // Negotiate options
    requestTelnetOption(TN_DO, OPT_TERMINAL_TYPE);
    requestTelnetOption(TN_DO, OPT_NAWS);
    requestTelnetOption(TN_DO, OPT_CHARSET);
}

void UserTelnet::onAnalyzeUserStream(const QByteArray &data)
{
    onReadInternal(data);
}

void UserTelnet::onSendToUser(const QByteArray &ba, bool goAhead)
{
    // MMapper internally represents all data as Latin-1
    QString temp = QString::fromLatin1(ba);

    // Switch codec if RFC 2066 was not negotiated and the configuration was altered
    const CharacterEncoding configEncoding = getConfig().general.characterEncoding;
    if (!hisOptionState[OPT_CHARSET] && configEncoding != textCodec.getEncoding()) {
        textCodec.setEncoding(configEncoding);
    }

    // Convert from unicode into the client requested encoding
    QByteArray outdata = textCodec.fromUnicode(temp);

    submitOverTelnet(outdata, goAhead);
}

void UserTelnet::sendToMapper(const QByteArray &data, bool goAhead)
{
    // MMapper requires all data to be Latin-1 internally
    // REVISIT: This will break things if the client is handling MPI itself
    QByteArray outdata = textCodec.toUnicode(data).toLatin1();
    emit analyzeUserStream(outdata, goAhead);
}

void UserTelnet::onRelayEchoMode(bool isDisabled)
{
    sendTelnetOption(isDisabled ? TN_WONT : TN_WILL, OPT_ECHO);
    myOptionState[OPT_ECHO] = !isDisabled;
    announcedState[OPT_ECHO] = true;
}

void UserTelnet::receiveTerminalType(QByteArray data)
{
    emit relayTermType(data);
}

void UserTelnet::receiveWindowSize(int x, int y)
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
