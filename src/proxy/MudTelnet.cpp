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

#include "MudTelnet.h"

MudTelnet::MudTelnet(QObject *const parent)
    : AbstractTelnet(TextCodec(TextCodecStrategy::FORCE_LATIN_1), false, parent)
{
    // RFC 2066 states we can provide many character sets but we force Latin-1 when
    // communicating with MUME
}

void MudTelnet::onConnected()
{
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

void MudTelnet::onRelayNaws(int x, int y)
{
    //remember the size - we'll need it if NAWS is currently disabled but will
    //be enabled. Also remember it if no connection exists at the moment;
    //we won't be called again when connecting
    current.x = x;
    current.y = y;

    if (myOptionState[OPT_NAWS]) {
        // only if we have negotiated this option
        sendWindowSizeChanged(x, y);
    }
}

void MudTelnet::onRelayTermType(QByteArray terminalType)
{
    termType = terminalType.append(QString("/MMapper-%1").arg(MMAPPER_VERSION));
    if (myOptionState[OPT_TERMINAL_TYPE]) {
        sendTerminalType(terminalType);
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
