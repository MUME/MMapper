#pragma once
/************************************************************************
**
** Authors:   Tomas Mecir <kmuddy@kmuddy.com>
**            Nils Schimmelmann <nschimme@gmail.com>
**
** Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com
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

/**
cTelnet handles the connection and telnet commands
However, as this is a MUD client and not a telnet, only a small subset of
telnet commands is handled.

List of direct telnet commands and their support in this class:
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
240 - SE (subcommand end) - supported
241 - NOP (no operation) - ignored ;-)
242 - DM (data mark) - NOT USED - Synch is not implemented (well,
  maybe I'll implement this later if reall necessary...)
243 - B (break) - not used (only client may use this, so the server won't bother with this)
244 - IP (interrupt process) - not applicable (only useful for shell processes)
245 - AO (about output) - not applicable (see IP)
246 - AYT (are you there) - I do not send this; "I'm here" is sent
  in an unlikely event that AYT is received
247 - EC (erase char) - not applicable (line editing fully handled by client)
248 - EL (erase line) - not applicable (line editing fully handled by client)
249 - GA (go ahead) - ignored; suppress-go-ahead is negotiated if possible
250 - SB (subcommand begin) - supported
251 - WILL - supported
252 - WON'T - supported
253 - DO - supported
254 - DON'T - supported
255 - IAC - supported

Extended telnet commands:
=-=-=-=-=-=-=-=-=-=-=-=-=
  these are handled with the IAC DO/DON'T/WILL/WON'T and SB/SE mechanism;
  see RFC 854 and RFCs listed here for further information.

a) FULLY SUPPORTED COMMANDS:
- STATUS (cmd 5, RFC 859) - sending/receiving option status
- TIMING-MARK (cmd 6, RFC 860) - timing marks, used by auto-mapper
- TERMINAL-TYPE (cmd 24, RFC 1091) - terminal type sending
- NAWS (cmd 31, RFC 1073) - negotiate about window size - used to inform
  the server about window size (cols x rows)

b) PARTIALLY SUPPORTED COMMANDS
- SUPPRESS-GO-AHEAD (cmd 3, RFC 858) - we try to suppress GA's if possible.
  If we fail, we just ignore all GA's, hoping for no problems...
- CHARSET (cmd 42, RFC 2066) - mechanism for passing character set

c) COMMANDS THAT ARE NOT SUPPORTED
The following commands are not supported at all - they are all disabled,
if the server asks us to enable it, we respond with DON'T or WON'T (this
is a standard behaviour described in RFC 854)
- TRANSMIT-BINARY (cmd 0, RFC 856) (see note below)
- ECHO (cmd 1, RFC 857) (see note below)
- EXTENDED-OPTIONS-LIST (cmd 255, RFC 861) (see note below)
- RCTE (cmd 7, RFC 726)
- NAOCRD (cmd 10, RFC 652)
- NAOHTS (cmd 11, RFC 653)
- NAOHTD (cmd 12, RFC 654)
- NAOFFD (cmd 13, RFC 655)
- NAOVTS (cmd 14, RFC 656)
- NAOVTD (cmd 15, RFC 657)
- NAOLFD (cmd 16, RFC 658)
- EXTEND-ASCII (cmd 17, RFC 698)
- LOGOUT (cmd 18, RFC 727)
- BM (cmd 19, RFC 735)
- DET (cmd 20, RFC 1043)
- SUPDUP (cmd 21, RFC 736)
- SUPDUP-OUTPUT (cmd 22, RFC 749)
- SEND-LOCATION (cmd 23, RFC 779) (well, maybe one day I'll support this)
- END-OF-RECORD (cmd 25, RFC 885)
- TUID (cmd 26, RFC 927)
- OUTMARK (cmd 27, RFC 933)
- TTYLOC (cmd 28, RFC 946)
- 3270-REGIME (cmd 29, RFC 1041)
- X.3-PAD (cmd 30, RFC 1053)
- TERMINAL-SPEED (cmd 32, RFC 1079)  (not needed, let the server decide; we can
  accept data at (almost) any speed, only limited by connection speed)
- TOGGLE-FLOW-CONTROL (cmd 33, RFC 1372) (maybe I'll support it, if it
  proves to be necessary/useful)
- LINEMODE (cmd 34, RFC 1184) - linemode is not needed, as we send the entire
  line either...
- NEW-ENVIRON (cmd 39, RFC 1572)
- TN3270E (cmd 40, RFC 1647)

Note: the first three commands are internet standards and each telnet application
should support them. However, this is NOT a telnet application and those
commands have no use for a MUD client, so I leave those commands unsupported.
Who knows, maybe one day I'll change my opinion and implement them (or somebody
else does it ;))

  *@author Tomas Mecir
*/

#ifndef ABSTRACTTELNET_H
#define ABSTRACTTELNET_H

#include "TextCodec.h"
#include <array>
#include <cstdint>
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QtCore>

//telnet command codes (prefixed with TN_ to prevent duplicit #defines
static constexpr const uint8_t TN_SE = 240;
static constexpr const uint8_t TN_NOP = 241;
static constexpr const uint8_t TN_DM = 242;
static constexpr const uint8_t TN_B = 243;
static constexpr const uint8_t TN_IP = 244;
static constexpr const uint8_t TN_AO = 245;
static constexpr const uint8_t TN_AYT = 246;
static constexpr const uint8_t TN_EC = 247;
static constexpr const uint8_t TN_EL = 248;
static constexpr const uint8_t TN_GA = 249;
static constexpr const uint8_t TN_SB = 250;
static constexpr const uint8_t TN_WILL = 251;
static constexpr const uint8_t TN_WONT = 252;
static constexpr const uint8_t TN_DO = 253;
static constexpr const uint8_t TN_DONT = 254;
static constexpr const uint8_t TN_IAC = 255;

//telnet option codes (supported options only)
static constexpr const uint8_t OPT_ECHO = 1;
static constexpr const uint8_t OPT_SUPPRESS_GA = 3;
static constexpr const uint8_t OPT_STATUS = 5;
static constexpr const uint8_t OPT_TIMING_MARK = 6;
static constexpr const uint8_t OPT_TERMINAL_TYPE = 24;
static constexpr const uint8_t OPT_NAWS = 31;
static constexpr const uint8_t OPT_CHARSET = 42;

//telnet SB suboption types
static constexpr const uint8_t TNSB_IS = 0;
static constexpr const uint8_t TNSB_SEND = 1;
static constexpr const uint8_t TNSB_REQUEST = 1;
static constexpr const uint8_t TNSB_ACCEPTED = 2;
static constexpr const uint8_t TNSB_REJECTED = 3;
static constexpr const uint8_t TNSB_TTABLE_IS = 4;
static constexpr const uint8_t TNSB_TTABLE_REJECTED = 5;
static constexpr const uint8_t TNSB_TTABLE_ACK = 6;
static constexpr const uint8_t TNSB_TTABLE_NAK = 7;

class AbstractTelnet : public QObject
{
    Q_OBJECT

public:
    explicit AbstractTelnet(TextCodec textCodec, bool debug = false, QObject *parent = nullptr);

    QByteArray getTerminalType() const { return termType; }
    uint64_t getSentBytes() const { return sentBytes; }

protected:
    void sendCharsetRequest(const QStringList &myCharacterSet);

    void sendTerminalType(const QByteArray &terminalType);

    void sendCharsetRejected();

    void sendCharsetAccepted(const QByteArray &characterSet);

    void sendOptionStatus();

    void sendAreYouThere();

    void sendWindowSizeChanged(int, int);

    void sendTerminalTypeRequest();

    void requestTelnetOption(unsigned char type, unsigned char subnegBuffer);

    /** Prepares data, doubles IACs, sends it using sendRawData. */
    void submitOverTelnet(const QByteArray &data, bool goAhead);

    virtual void sendToMapper(const QByteArray &, bool goAhead) = 0;

    virtual void receiveEchoMode(bool) {}

    virtual void receiveTerminalType(QByteArray) {}

    virtual void receiveWindowSize(int, int) {}

    /** Send out the data. Does not double IACs, this must be done
            by caller if needed. This function is suitable for sending
            telnet sequences. */
    virtual void sendRawData(const QByteArray &data) = 0;

    /** send a telnet option */
    void sendTelnetOption(unsigned char type, unsigned char subnegBuffer);

    void reset();

    void onReadInternal(const QByteArray &);

    void setTerminalType(const QByteArray &terminalType = "unknown");

    TextCodec textCodec{};

    using OptionArray = std::array<bool, 256>;

    /** current state of options on our side and on server side */
    OptionArray myOptionState{};
    OptionArray hisOptionState{};
    /** whether we have announced WILL/WON'T for that option (if we have, we don't
        respond to DO/DON'T sent by the server -- see implementation and RFC 854
        for more information... */
    OptionArray announcedState{};
    /** whether the server has already announced his WILL/WON'T */
    OptionArray heAnnouncedState{};

    /** current dimensions for NAWS */
    struct
    {
        int x = 80, y = 24;
    } current{};

    /* Terminal Type */
    QByteArray termType{};

    /** amount of bytes sent up to now */
    uint64_t sentBytes = 0;

private:
    void onReadInternal2(QByteArray &, uint8_t);

    /** processes a telnet command (IAC ...) */
    void processTelnetCommand(const QByteArray &command);

    /** processes a telnet subcommand payload */
    void processTelnetSubnegotiation(const QByteArray &payload);

    QByteArray commandBuffer{};
    QByteArray subnegBuffer{};
    enum class TelnetState {
        /// normal input
        NORMAL,
        /// received IAC
        IAC,
        /// received IAC <WILL|WONT|DO|DONT>
        COMMAND,
        /// received IAC SB
        SUBNEG,
        /// received IAC SB ... IAC
        SUBNEG_IAC,
        /// received IAC SB ... IAC <WILL|WONT|DO|DONT>
        SUBNEG_COMMAND
    } state;

    /** have we received the GA signal? */
    bool recvdGA = false;

    bool debug = false;
};

#endif /* ABSTRACTTELNET_H */
