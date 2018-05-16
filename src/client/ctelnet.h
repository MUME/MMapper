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

#ifndef CTELNET_H
#define CTELNET_H

#include <QObject>
#include <QTcpSocket>

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

//telnet command codes (prefixed with TN_ to prevent duplicit #defines
#define TN_SE (unsigned char) 240
#define TN_NOP (unsigned char) 241
#define TN_DM (unsigned char) 242
#define TN_B (unsigned char) 243
#define TN_IP (unsigned char) 244
#define TN_AO (unsigned char) 245
#define TN_AYT (unsigned char) 246
#define TN_EC (unsigned char) 247
#define TN_EL (unsigned char) 248
#define TN_GA (unsigned char) 249
#define TN_SB (unsigned char) 250
#define TN_WILL (unsigned char) 251
#define TN_WONT (unsigned char) 252
#define TN_DO (unsigned char) 253
#define TN_DONT (unsigned char) 254
#define TN_IAC (unsigned char) 255

//telnet option codes (supported options only)
#define OPT_ECHO (unsigned char) 1
#define OPT_SUPPRESS_GA (unsigned char) 3
#define OPT_STATUS (unsigned char) 5
#define OPT_TIMING_MARK (unsigned char) 6
#define OPT_TERMINAL_TYPE (unsigned char) 24
#define OPT_NAWS (unsigned char) 31

//telnet SB suboption types
#define TNSB_IS (char) 0
#define TNSB_SEND (unsigned char) 1

class QTextCodec;
class QTextDecoder;
class QTextEncoder;

class cTelnet : public QObject
{
    Q_OBJECT

public:
    cTelnet(QObject *parent = 0);
    ~cTelnet();

    void connectToHost();

    void disconnectFromHost();

public slots:
    /** Window size has changed - informs the server about it */
    void windowSizeChanged (int x, int y);

    /** Prepares data, doubles IACs, sends it using sendRawData. */
    void sendToMud (const QByteArray &data);

protected slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);

    /** Reads, parses telnet, and so forth */
    void onReadyRead ();

signals:
    /** Submits Telnet/text data back to the client */
    void sendToUser (const QString &data);

    /** toggles echo mode for passwords */
    void echoModeChanged (bool);

    void disconnected();
    void connected();
    void socketError(const QString &);

protected:
    void reset ();

    void setupEncoding ();

    /** Send out the data. Does not double IACs, this must be done
            by caller if needed. This function is suitable for sending
            telnet sequences. */
    void sendRawData (const QByteArray &data);

    /** processes a telnet command (IAC ...) */
    void processTelnetCommand (const QByteArray &command);

    /** send a telnet option */
    void sendTelnetOption (unsigned char type, unsigned char option);

private:
    QTcpSocket socket;
    char buffer[32769];

    QByteArray encoding;

    QTextDecoder *inCoder;
    QTextEncoder *outCoder;

    //iac: last char was IAC
    //iac2: last char was DO, DONT, WILL or WONT
    //insb: we're in IAC SB, waiting for IAC SE
    QByteArray command;
    bool iac, iac2, insb;

    /** current state of options on our side and on server side */
    bool myOptionState[256], hisOptionState[256];
    /** whether we have announced WILL/WON'T for that option (if we have, we don't
        respond to DO/DON'T sent by the server -- see implementation and RFC 854
        for more information... */
    bool announcedState[256];
    /** whether the server has already announced his WILL/WON'T */
    bool heAnnouncedState[256];
    /** whether we have tried to enable this option */
    bool triedToEnable[256];
    /** amount of bytes sent up to now */
    int sentbytes;
    /** have we received the GA signal? */
    bool recvdGA;
    bool echoMode;
    bool startupneg;
    /** current dimensions */
    int curX, curY;

    /* Terminal Type */
    QString termType;
};


#endif /* CTELNET_H */
