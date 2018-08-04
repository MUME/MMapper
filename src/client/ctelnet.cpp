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

#include "ctelnet.h"

#include <limits>
#include <QApplication>
#include <QByteArray>
#include <QHostAddress>
#include <QMessageLogContext>
#include <QObject>
#include <QString>

#include "../configuration/configuration.h"
#include "../global/io.h"
#include "../global/utils.h"

QString telnetCommandName(uint8_t cmd)
{
#define CASE(x) \
    case TN_##x: \
        return #x
    switch (cmd) {
        CASE(SE);
        CASE(NOP);
        CASE(DM);
        CASE(B);
        CASE(IP);
        CASE(AO);
        CASE(AYT);
        CASE(EC);
        CASE(EL);
        CASE(GA);
        CASE(SB);
        CASE(WILL);
        CASE(WONT);
        CASE(DO);
        CASE(DONT);
        CASE(IAC);
    }
    return QString::asprintf("%u", cmd);
#undef CASE
}

static QString telnetOptionName(uint8_t opt)
{
#define CASE(x) \
    case OPT_##x: \
        return #x
    switch (opt) {
        CASE(ECHO);
        CASE(SUPPRESS_GA);
        CASE(STATUS);
        CASE(TIMING_MARK);
        CASE(TERMINAL_TYPE);
        CASE(NAWS);
        CASE(CHARSET);
    }
    return QString::asprintf("%u", opt);
#undef CASE
}
static QString telnetSubnegName(uint8_t opt)
{
#define CASE(x) \
    case TNSB_##x: \
        return #x
    switch (opt) {
        CASE(IS);
        CASE(SEND); // TODO: conflict between SEND and REQUEST
        CASE(ACCEPTED);
        CASE(REJECTED);
        CASE(TTABLE_IS);
        CASE(TTABLE_REJECTED);
        CASE(TTABLE_ACK);
        CASE(TTABLE_NAK);
    }
    return QString::asprintf("%u", opt);
#undef CASE
}

bool containsIAC(const QByteArray &arr)
{
    for (auto c : arr) {
        if (static_cast<uint8_t>(c) == TN_IAC)
            return true;
    }
    return false;
}

struct TelnetFormatter final : public QByteArray
{
    void addRaw(const uint8_t byte)
    {
        auto &s = static_cast<QByteArray &>(*this);
        s += byte;
    }

    void addEscaped(const uint8_t byte)
    {
        addRaw(byte);
        if (byte == TN_IAC) {
            addRaw(byte);
        }
    }

    void addTwoByteEscaped(const uint16_t n)
    {
        // network order is big-endian
        addEscaped(static_cast<uint8_t>(n >> 8));
        addEscaped(static_cast<uint8_t>(n));
    }

    void addClampedTwoByteEscaped(const int n)
    {
        static constexpr const auto lo = static_cast<int>(std::numeric_limits<uint16_t>::min());
        static constexpr const auto hi = static_cast<int>(std::numeric_limits<uint16_t>::max());
        static_assert(lo == 0, "");
        static_assert(hi == 65535, "");
        addTwoByteEscaped(static_cast<uint16_t>(clamp(n, lo, hi)));
    }

    void addEscapedBytes(const QByteArray &s)
    {
        for (auto c : s) {
            addEscaped(static_cast<uint8_t>(c));
        }
    }

    void addCommand(uint8_t cmd)
    {
        addRaw(TN_IAC);
        addRaw(cmd);
    }

    void addSubnegBegin(uint8_t opt)
    {
        addCommand(TN_SB);
        addRaw(opt);
    }

    void addSubnegEnd() { addCommand(TN_SE); }
};

cTelnet::cTelnet(QObject *const parent)
    : QObject(parent)
{
    /** MMapper Telnet */
    termType = QString("MMapper %1").arg(MMAPPER_VERSION);

    encoding = Config().parser.utf8Charset ? UTF_8_ENCODING : LATIN_1_ENCODING;
    reset();

    setupEncoding();

    connect(&socket, &QAbstractSocket::connected, this, &cTelnet::onConnected);
    connect(&socket, &QAbstractSocket::disconnected, this, &cTelnet::onDisconnected);
    connect(&socket, &QIODevice::readyRead, this, &cTelnet::onReadyRead);
    connect(&socket,
            SIGNAL(error(QAbstractSocket::SocketError)),
            this,
            SLOT(onError(QAbstractSocket::SocketError)));
}

cTelnet::~cTelnet()
{
    socket.disconnectFromHost();
    socket.deleteLater();
}

void cTelnet::connectToHost()
{
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.abort();
    }
    socket.connectToHost(QHostAddress::LocalHost, Config().connection.localPort);
    socket.waitForConnected(3000);
}

void cTelnet::onConnected()
{
    qDebug() << "* Telnet detected socket connect!";
    socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);

    // MUME opts to not send DO CHARSET due to older, broken clients
    sendTelnetOption(TN_WILL, OPT_CHARSET);

    emit connected();
}

void cTelnet::disconnectFromHost()
{
    socket.disconnectFromHost();
}

void cTelnet::onDisconnected()
{
    reset();
    emit disconnected();
}

void cTelnet::onError(QAbstractSocket::SocketError error)
{
    if (error == QAbstractSocket::RemoteHostClosedError) {
        // The connection closing isn't an error
        return;
    }
    QString err = socket.errorString();
    socket.abort();
    emit socketError(err);
}

void cTelnet::setupEncoding()
{
    inCoder.reset();
    outCoder.reset();

    qDebug() << "* Switching to" << encoding << "encoding";

    Config().parser.utf8Charset
        = (QString::compare(QString(encoding), UTF_8_ENCODING, Qt::CaseInsensitive) == 0);

    // MUME can understand US-ASCII, ISO-8859-1, or UTF-8
    auto textCodec = QTextCodec::codecForName(encoding);
    if (textCodec == nullptr) {
        qWarning() << "* Falling back to ISO-8859-1 because" << encoding << "was not available";
        encoding = LATIN_1_ENCODING;
        textCodec = QTextCodec::codecForName(encoding);
    }

    // MUME can output US-ASCII, LATIN1, or UTF-8
    inCoder = std::unique_ptr<QTextDecoder>{textCodec->makeDecoder()};
    outCoder = std::unique_ptr<QTextEncoder>{textCodec->makeEncoder()};
}

void cTelnet::reset()
{
    myOptionState.fill(false);
    hisOptionState.fill(false);
    announcedState.fill(false);
    heAnnouncedState.fill(false);

    //reset telnet status
    iac = iac2 = insb = false;
    command.clear();
    sentbytes = 0;
    emit echoModeChanged(true);
}

void cTelnet::sendToMud(const QString &data)
{
    QByteArray outdata = outCoder->fromUnicode(data);

    // IAC byte must be doubled
    if (containsIAC(outdata)) {
        TelnetFormatter d;
        d.addEscapedBytes(outdata);
        outdata = d;
    }

    //data ready, send it
    sendRawData(outdata);
}

void cTelnet::sendRawData(const QByteArray &data)
{
    //update counter
    sentbytes += data.length();
    socket.write(data, data.length());
}

void cTelnet::windowSizeChanged(int x, int y)
{
    //remember the size - we'll need it if NAWS is currently disabled but will
    //be enabled. Also remember it if no connection exists at the moment;
    //we won't be called again when connecting
    current.x = x;
    current.y = y;

    // REVISIT: Should we attempt to rate-limit this to avoid spamming dozens of NAWS
    // messages per second when the user adjusts the window size?
    if (myOptionState[OPT_NAWS]) { //only if we have negotiated this option
        // RFC 1073 specifies IAC SB NAWS WIDTH[1] WIDTH[0] HEIGHT[1] HEIGHT[0] IAC SE
        TelnetFormatter s;
        s.addSubnegBegin(OPT_NAWS);
        // RFC855 specifies that option parameters with a byte value of 255 must be doubled
        s.addClampedTwoByteEscaped(x);
        s.addClampedTwoByteEscaped(y);
        s.addSubnegEnd();
        sendRawData(s);
    }
}

void cTelnet::sendTelnetOption(unsigned char type, unsigned char option)
{
    qDebug() << "* Sending Telnet Command: " << telnetCommandName(type) << telnetOptionName(option);
    QByteArray s;
    s += TN_IAC;
    s += type;
    s += option;
    sendRawData(s);
}

void cTelnet::processTelnetCommand(const QByteArray &command)
{
    const unsigned char ch = command[1];
    unsigned char option;

    switch (command.length()) {
    case 2:
        if (ch != TN_GA)
            qDebug() << "* Processing Telnet Command:" << telnetCommandName(ch);

        switch (ch) {
        case TN_AYT:
            sendRawData("I'm here! Please be more patient!\r\n");
            //well, this should never be executed, as the response would probably
            //be treated as a command. But that's server's problem, not ours...
            //If the server wasn't capable of handling this, it wouldn't have
            //sent us the AYT command, would it? Impatient server = bad server.
            //Let it suffer! ;-)
            break;
        case TN_GA:
            recvdGA = true;
            //signal will be emitted later
            break;
        };
        break;

    case 3:
        qDebug() << "* Processing Telnet Command:" << telnetCommandName(ch)
                 << telnetOptionName(command[2]);

        switch (ch) {
        case TN_WILL:
            //server wants to enable some option (or he sends a timing-mark)...
            option = command[2];

            heAnnouncedState[option] = true;
            if (!hisOptionState[option])
            //only if this is not set; if it's set, something's wrong wth the server
            //(according to telnet specification, option announcement may not be
            //unless explicitly requested)
            {
                if ((option == OPT_SUPPRESS_GA) || (option == OPT_STATUS)
                    || (option == OPT_TERMINAL_TYPE) || (option == OPT_NAWS) || (option == OPT_ECHO)
                    || (option == OPT_CHARSET))
                //these options are supported
                {
                    sendTelnetOption(TN_DO, option);
                    hisOptionState[option] = true;
                    if (option == OPT_ECHO) {
                        emit echoModeChanged(false);
                    }
                } else {
                    sendTelnetOption(TN_DONT, option);
                    hisOptionState[option] = false;
                }
            }
            break;
        case TN_WONT:
            //server refuses to enable some option...
            option = command[2];
            //send DONT if needed (see RFC 854 for details)
            if (hisOptionState[option] || (!heAnnouncedState[option])) {
                sendTelnetOption(TN_DONT, option);
                hisOptionState[option] = false;
                if (option == OPT_ECHO) {
                    emit echoModeChanged(true);
                }
            }
            heAnnouncedState[option] = true;
            break;
        case TN_DO:
            //server wants us to enable some option
            option = command[2];
            if (option == OPT_TIMING_MARK) {
                //send WILL TIMING_MARK
                sendTelnetOption(TN_WILL, option);
            } else if (!myOptionState[option])
            //only if the option is currently disabled
            {
                if ((option == OPT_SUPPRESS_GA) || (option == OPT_STATUS)
                    || (option == OPT_TERMINAL_TYPE) || (option == OPT_NAWS)
                    || (option == OPT_CHARSET)) {
                    sendTelnetOption(TN_WILL, option);
                    myOptionState[option] = true;
                    announcedState[option] = true;
                } else {
                    sendTelnetOption(TN_WONT, option);
                    myOptionState[option] = false;
                    announcedState[option] = true;
                }
            }
            if (option == OPT_NAWS) { //NAWS here - window size info must be sent
                windowSizeChanged(current.x, current.y);
            } else if (myOptionState[OPT_CHARSET] && option == OPT_CHARSET) {
                QString myCharacterSet = Config().parser.utf8Charset ? UTF_8_ENCODING
                                                                     : LATIN_1_ENCODING;
                QByteArray s;
                s += TN_IAC;
                s += TN_SB;
                s += OPT_CHARSET;
                s += TNSB_REQUEST;
                s += ";";
                // RFC 2066 states we can provide many character sets but we will only give MUME our single preference
                s += myCharacterSet;
                s += TN_IAC;
                s += TN_SE;
                sendRawData(s);
                // TODO: RFC 2066 states to queue all subsequent data until ACCEPTED / REJECTED
            }
            break;
        case TN_DONT:
            //only respond if value changed or if this option has not been announced yet
            option = command[2];
            if (myOptionState[option] || (!announcedState[option])) {
                sendTelnetOption(TN_WONT, option);
                announcedState[option] = true;
            }
            myOptionState[option] = false;
            break;
        };
        break;

    case 4:
        qDebug() << "* Processing Telnet Command:" << telnetCommandName(ch)
                 << telnetOptionName(command[2]) << telnetSubnegName(command[3]);

        switch (ch) {
        case TN_SB:
            //subcommand - we analyze and respond...
            option = command[2];
            switch (option) {
            case OPT_STATUS:
                //see OPT_TERMINAL_TYPE for explanation why I'm doing this
                if (true /*myOptionState[OPT_STATUS]*/) {
                    if (command[3] == TNSB_SEND)
                    //request to send all enabled commands; if server sends his
                    //own list of commands, we just ignore it (well, he shouldn't
                    //send anything, as we do not request anything, but there are
                    //so many servers out there, that you can never be sure...)
                    {
                        QByteArray s;
                        s += TN_IAC;
                        s += TN_SB;
                        s += OPT_STATUS;
                        s += TNSB_IS;
                        for (int i = 0; i < 256; i++) {
                            if (myOptionState[i]) {
                                s += TN_WILL;
                                s += static_cast<unsigned char>(i);
                            }
                            if (hisOptionState[i]) {
                                s += TN_DO;
                                s += static_cast<unsigned char>(i);
                            }
                        }
                        s += TN_IAC;
                        s += TN_SE;
                        sendRawData(s);
                    }
                }
                break;
            case OPT_TERMINAL_TYPE:
                if (myOptionState[OPT_TERMINAL_TYPE]) {
                    if (command[3] == TNSB_SEND)
                    //server wants us to send terminal type; he can send his own type
                    //too, but we just ignore it, as we have no use for it...
                    {
                        TelnetFormatter s;
                        s.addSubnegBegin(OPT_TERMINAL_TYPE);
                        // RFC855 specifies that option parameters with a byte value of 255 must be doubled
                        s.addEscaped(TNSB_IS); /* NOTE: "IS" will never actually be escaped */
                        s.addEscapedBytes(termType.toLatin1());
                        s.addSubnegEnd();
                        sendRawData(s);
                    }
                }
                break;
                //other cmds should not arrive, as they were not negotiated.
                //if they do, they are merely ignored
            case OPT_CHARSET:
                if (myOptionState[OPT_CHARSET]) {
                    option = command[3];
                    int iacPos = command.indexOf(TN_IAC, 3);
                    switch (option) {
                    case TNSB_REQUEST:
                        // MMapper does not support [TTABLE]
                        if (iacPos > 6 && command[4] != '[') {
                            bool accepted = false;
                            // TODO: Add US-ASCII to Parser dropdown
                            QString myCharacterSet = Config().parser.utf8Charset ? UTF_8_ENCODING
                                                                                 : LATIN_1_ENCODING;

                            // Split remainder into delim and IAC
                            // IAC SB CHARSET REQUEST <sep> <charsets> IAC SE
                            unsigned char sep = command[4];
                            auto characterSets = command.mid(5, iacPos - 5).split(sep);
                            for (auto characterSet : characterSets) {
                                QString hisCharacterSet(characterSet);
                                if (QString::compare(myCharacterSet,
                                                     hisCharacterSet,
                                                     Qt::CaseInsensitive)
                                    == 0) {
                                    accepted = true;
                                    encoding = std::move(characterSet);
                                    setupEncoding();

                                    // Reply to server that we accepted this encoding
                                    QByteArray s;
                                    s += TN_IAC;
                                    s += TN_SB;
                                    s += OPT_CHARSET;
                                    s += TNSB_ACCEPTED;
                                    s += characterSet;
                                    s += TN_IAC;
                                    s += TN_SE;
                                    sendRawData(s);
                                    break;
                                }
                            }
                            if (accepted) {
                                break;
                            }
                        }
                        {
                            // Reject invalid requests or if we did not find any supported codecs
                            QByteArray s;
                            s += TN_IAC;
                            s += TN_SB;
                            s += OPT_CHARSET;
                            s += TNSB_REJECTED;
                            s += TN_IAC;
                            s += TN_SE;
                            sendRawData(s);
                        }
                        break;
                    case TNSB_ACCEPTED:
                        if (iacPos > 5) {
                            // IAC SB CHARSET ACCEPTED <charset> IAC SE
                            auto characterSet = command.mid(4, iacPos - 4);
                            encoding = characterSet;
                            setupEncoding();
                            // TODO: RFC 2066 states to stop queueing data
                        }
                        break;
                    case TNSB_REJECTED:
                        // TODO: RFC 2066 states to stop queueing data
                        break;
                    case TNSB_TTABLE_IS:
                        // We never request a [TTABLE] so this should not happen
                        abort();
                    }
                }
                break;
            };
            break;
        };
        break;
    case 1:
        break;
    };
    //other commands are simply ignored (NOP and such, see .h file for list)
}

void cTelnet::onReadyRead()
{
    io::readAllAvailable(socket, buffer, [this](const QByteArray &byteArray) {
        onReadInternal(byteArray);
    });
}

void cTelnet::sendToUserAndClear(QByteArray &data)
{
    emit sendToUser(inCoder->toUnicode(data));
    data.clear();
}

void cTelnet::onReadInternal(const QByteArray &data)
{
    //now we have the data, but we cannot forward it to next stage of processing,
    //because the data contains telnet commands
    //so we parse the text and process all telnet commands:

    //clear the GO-AHEAD flag
    recvdGA = false;

    auto cleanData = QByteArray{};
    cleanData.reserve(data.size());

    for (char i : data) {
        onReadInternal2(cleanData, i);

        if (recvdGA) {
            sendToUserAndClear(cleanData); // with GO-AHEAD
            recvdGA = false;
        }
    }

    //some data left to send - do it now!
    if (!cleanData.isEmpty()) {
        sendToUserAndClear(cleanData); // without GO-AHEAD
    }
}

/*
 * normal telnet state
 * -------------------
 * x                                # forward 0-254
 * IAC IAC                          # forward 255
 * IAC (WILL | WONT | DO | DONT) x  # negotiate 0-255 (w/ 255 = EXOPL)
 * IAC SB                           # begins subnegotiation
 * IAC SE                           # (error)
 * IAC x                            # exec command
 *
 * within a subnegotiation
 * -----------------------
 * x          # appends 0-254 to option payload
 * IAC IAC    # appends 255 to option payload
 * IAC SE     # ends subnegotiation
 * IAC SB     # (error)
 * IAC x      # exec command
 *
 * NOTE: RFC 855 refers to IAC SE as a command rather than a delimiter,
 * so that implies you're still supposed to process "commands" (e.g. IAC GA)!
 *
 * So if you receive "IAC SB IAC WILL ECHO f o o IAC IAC b a r IAC SE"
 * then you process will(ECHO) followed by the subnegotiation(f o o 255 b a r).
 */
void cTelnet::onReadInternal2(QByteArray &cleanData, const uint8_t c)
{
    if (iac || iac2 || insb || (c == TN_IAC)) {
        //there are many possibilities here:
        //1. this is IAC, previous character was regular data
        if (!(iac || iac2 || insb) && (c == TN_IAC)) {
            iac = true;
            command.append(c);
        }
        //2. seq. of two IACs
        else if (iac && (c == TN_IAC) && (!insb)) {
            iac = false;
            cleanData.append(c);
            command.clear();
        }
        //3. IAC DO/DONT/WILL/WONT
        else if (iac && (!insb)
                 && ((c == TN_WILL) || (c == TN_WONT) || (c == TN_DO) || (c == TN_DONT))) {
            iac = false;
            iac2 = true;
            command.append(c);
        }
        //4. IAC DO/DONT/WILL/WONT <command code>
        else if (iac2) {
            iac2 = false;
            command.append(c);
            processTelnetCommand(command);
            command.clear();
        }
        //5. IAC SB
        else if (iac && (!insb) && (c == TN_SB)) {
            iac = false;
            insb = true;
            command.append(c);
        }
        //6. IAC SE without IAC SB - error - ignored
        else if (iac && (!insb) && (c == TN_SE)) {
            command.clear();
            iac = false;
        }
        //7. inside IAC SB
        else if (insb) {
            command.append(c);
            if (iac && //IAC SE - end of subcommand
                (c == TN_SE)) {
                processTelnetCommand(command);
                command.clear();
                iac = false;
                insb = false;
            }
            if (iac) {
                iac = false;
            } else if (c == TN_IAC) {
                iac = true;
            }
        }
        //8. IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
        else {
            iac = false;
            command.append(c);
            processTelnetCommand(command);
            //this could have set receivedGA to true; we'll handle that later
            // (at the end of this function)
            command.clear();
        }
    } else { //plaintext
        //everything except CRLF is okay; CRLF is replaced by LF(\n) (CR ignored)

        switch (c) {
        case '\a': // BEL
            QApplication::beep();
            break;
        default:
            cleanData.append(c);
        };
    }
}
