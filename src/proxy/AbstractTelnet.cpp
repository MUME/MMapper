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

#include "AbstractTelnet.h"

#include <limits>
#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QString>

#include "../global/utils.h"
#include "TextCodec.h"

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

AbstractTelnet::AbstractTelnet(TextCodec textCodec, bool debug, QObject *const parent)
    : QObject(parent)
    , textCodec{std::move(textCodec)}
    , debug{debug}
{
    reset();
}

void AbstractTelnet::reset()
{
    myOptionState.fill(false);
    hisOptionState.fill(false);
    announcedState.fill(false);
    heAnnouncedState.fill(false);

    //reset telnet status
    termType = QString("MMapper-%1").arg(MMAPPER_VERSION);
    state = TelnetState::FORWARD;
    commandBuffer.clear();
    subnegBuffer.clear();
    sentBytes = 0;
}

void AbstractTelnet::submitOverTelnet(const QByteArray &data, bool goAhead)
{
    QByteArray outdata = data;

    // IAC byte must be doubled
    if (containsIAC(outdata)) {
        TelnetFormatter d;
        d.addEscapedBytes(outdata);
        outdata = d;
    }

    // Add IAC GA unless they are suppressed
    if (goAhead && !hisOptionState[OPT_SUPPRESS_GA]) {
        outdata += TN_IAC;
        outdata += TN_GA;
    }

    //data ready, send it
    sendRawData(outdata);
}

void AbstractTelnet::sendWindowSizeChanged(const int x, const int y)
{
    if (debug)
        qDebug() << "Sending NAWS" << x << y;
    // RFC 1073 specifies IAC SB NAWS WIDTH[1] WIDTH[0] HEIGHT[1] HEIGHT[0] IAC SE
    TelnetFormatter s;
    s.addSubnegBegin(OPT_NAWS);
    // RFC855 specifies that option parameters with a byte value of 255 must be doubled
    s.addClampedTwoByteEscaped(x);
    s.addClampedTwoByteEscaped(y);
    s.addSubnegEnd();
    sendRawData(s);
}

void AbstractTelnet::sendTelnetOption(unsigned char type, unsigned char option)
{
    if (debug)
        qDebug() << "* Sending Telnet Command: " << telnetCommandName(type)
                 << telnetOptionName(option);
    QByteArray s;
    s += TN_IAC;
    s += type;
    s += option;
    sendRawData(s);
}

void AbstractTelnet::requestTelnetOption(unsigned char type, unsigned char option)
{
    myOptionState[option] = true;
    announcedState[option] = true;
    sendTelnetOption(type, option);
}

void AbstractTelnet::sendCharsetRequest(const QStringList &myCharacterSets)
{
    if (debug)
        qDebug() << "Requesting charsets" << myCharacterSets;
    static const auto delimeter = ";";
    TelnetFormatter s;
    s.addSubnegBegin(OPT_CHARSET);
    s.addRaw(TNSB_REQUEST);
    for (QString characterSet : myCharacterSets) {
        s.addEscapedBytes(delimeter);
        s.addEscapedBytes(characterSet.toLocal8Bit());
    }
    s.addSubnegEnd();
    sendRawData(s);
}

void AbstractTelnet::sendTerminalType(const QByteArray &terminalType)
{
    if (debug)
        qDebug() << "Sending Terminal Type:" << terminalType;
    TelnetFormatter s;
    s.addSubnegBegin(OPT_TERMINAL_TYPE);
    // RFC855 specifies that option parameters with a byte value of 255 must be doubled
    s.addEscaped(TNSB_IS); /* NOTE: "IS" will never actually be escaped */
    s.addEscapedBytes(terminalType);
    s.addSubnegEnd();
    sendRawData(s);
}

void AbstractTelnet::sendCharsetRejected()
{
    TelnetFormatter s;
    s.addSubnegBegin(OPT_CHARSET);
    s.addRaw(TNSB_REJECTED);
    s.addSubnegEnd();
    sendRawData(s);
}

void AbstractTelnet::sendCharsetAccepted(const QByteArray &characterSet)
{
    if (debug)
        qDebug() << "Accepted Charset" << characterSet;
    TelnetFormatter s;
    s.addSubnegBegin(OPT_CHARSET);
    s.addRaw(TNSB_ACCEPTED);
    s.addEscapedBytes(characterSet);
    s.addSubnegEnd();
    sendRawData(s);
}

void AbstractTelnet::sendOptionStatus()
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

void AbstractTelnet::sendAreYouThere()
{
    sendRawData("I'm here! Please be more patient!\r\n");
    //well, this should never be executed, as the response would probably
    //be treated as a command. But that's server's problem, not ours...
    //If the server wasn't capable of handling this, it wouldn't have
    //sent us the AYT command, would it? Impatient server = bad server.
    //Let it suffer! ;-)
}

void AbstractTelnet::sendTerminalTypeRequest()
{
    TelnetFormatter s;
    s.addSubnegBegin(OPT_TERMINAL_TYPE);
    s.addEscaped(TNSB_SEND);
    s.addSubnegEnd();
    sendRawData(s);
}

void AbstractTelnet::processTelnetCommand(const QByteArray &command)
{
    const unsigned char ch = command[1];
    unsigned char option;

    switch (command.length()) {
    case 1:
        break;
    case 2:
        if (ch != TN_GA && debug)
            qDebug() << "* Processing Telnet Command:" << telnetCommandName(ch);

        switch (ch) {
        case TN_AYT:
            sendAreYouThere();
            break;
        case TN_GA:
            recvdGA = true; //signal will be emitted later
            break;
        };
        break;

    case 3:
        if (debug)
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
                if (!myOptionState[option])
                //only if the option is currently disabled
                {
                    if ((option == OPT_SUPPRESS_GA) || (option == OPT_STATUS)
                        || (option == OPT_TERMINAL_TYPE) || (option == OPT_NAWS)
                        || (option == OPT_ECHO) || (option == OPT_CHARSET))
                    //these options are supported
                    {
                        sendTelnetOption(TN_DO, option);
                        hisOptionState[option] = true;
                        if (option == OPT_ECHO) {
                            receiveEchoMode(false);
                        }
                    } else {
                        sendTelnetOption(TN_DONT, option);
                        hisOptionState[option] = false;
                    }
                } else if (myOptionState[option] && option == OPT_TERMINAL_TYPE) {
                    sendTerminalTypeRequest();
                }
            } else if (debug) {
                qDebug() << "His option" << telnetOptionName(option) << "was already enabled";
            }
            break;
        case TN_WONT:
            //server refuses to enable some option...
            option = command[2];
            if (!myOptionState[option])
            //only if the option is currently disabled
            {
                //send DONT if needed (see RFC 854 for details)
                if (hisOptionState[option] || (!heAnnouncedState[option])) {
                    sendTelnetOption(TN_DONT, option);
                    hisOptionState[option] = false;
                    if (option == OPT_ECHO) {
                        receiveEchoMode(true);
                    }
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
                    || (option == OPT_CHARSET) || (option == OPT_ECHO)) {
                    sendTelnetOption(TN_WILL, option);
                    myOptionState[option] = true;
                    announcedState[option] = true;
                } else {
                    sendTelnetOption(TN_WONT, option);
                    myOptionState[option] = false;
                    announcedState[option] = true;
                }
            } else if (debug) {
                qDebug() << "My option" << telnetOptionName(option) << "was already enabled";
            }
            if (myOptionState[OPT_NAWS] && option == OPT_NAWS) {
                //NAWS here - window size info must be sent
                // REVISIT: Should we attempt to rate-limit this to avoid spamming dozens of NAWS
                // messages per second when the user adjusts the window size?
                sendWindowSizeChanged(current.x, current.y);

            } else if (myOptionState[OPT_CHARSET] && option == OPT_CHARSET) {
                sendCharsetRequest(textCodec.supportedEncodings());
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

    default:
        //other cmds should not arrive, as they were not negotiated.
        //if they do, they are merely ignored
        break;
    };
    //other commands are simply ignored (NOP and such, see .h file for list)
}

void AbstractTelnet::processTelnetSubnegotiation(const QByteArray &payload)
{
    if (debug && payload.length() >= 2) {
        qDebug() << "* Processing Telnet Subnegotiation:" << telnetOptionName(payload[0])
                 << telnetSubnegName(payload[1]);
    }

    //subnegotiation - we analyze and respond...
    switch (payload[0]) {
    case OPT_STATUS:
        //see OPT_TERMINAL_TYPE for explanation why I'm doing this
        if (true /*myOptionState[OPT_STATUS]*/) {
            if (payload[1] == TNSB_SEND)
            //request to send all enabled commands; if server sends his
            //own list of commands, we just ignore it (well, he shouldn't
            //send anything, as we do not request anything, but there are
            //so many servers out there, that you can never be sure...)
            {
                sendOptionStatus();
            }
        }
        break;
    case OPT_TERMINAL_TYPE:
        if (myOptionState[OPT_TERMINAL_TYPE]) {
            switch (payload[1]) {
            case TNSB_SEND:
                //server wants us to send terminal type
                sendTerminalType(termType.toLocal8Bit());
                break;
            case TNSB_IS:
                // Extract sender's terminal type
                // TERMINAL_TYPE IS <...>
                receiveTerminalType(payload.mid(2));
                break;
            }
        }
        break;
    case OPT_CHARSET:
        if (myOptionState[OPT_CHARSET]) {
            switch (payload[1]) {
            case TNSB_REQUEST:
                // MMapper does not support [TTABLE]
                if (payload.length() >= 4 && payload[2] != '[') {
                    bool accepted = false;
                    // Split remainder into delim and IAC
                    // CHARSET REQUEST <sep> <charsets>
                    const auto sep = payload[2];
                    const auto characterSets = payload.mid(3).split(sep);
                    for (const auto characterSet : characterSets) {
                        if (textCodec.supports(characterSet)) {
                            accepted = true;
                            textCodec.setEncodingForName(characterSet);

                            // Reply to server that we accepted this encoding
                            sendCharsetAccepted(characterSet);
                            break;
                        }
                    }
                    if (accepted) {
                        break;
                    } else if (debug) {
                        qDebug() << "Rejected encoding" << characterSets;
                    }
                }
                // Reject invalid requests or if we did not find any supported codecs
                sendCharsetRejected();
                break;
            case TNSB_ACCEPTED:
                if (payload.length() > 3) {
                    // CHARSET ACCEPTED <charset>
                    auto characterSet = payload.mid(2);
                    textCodec.setEncodingForName(characterSet);
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
    case OPT_NAWS:
        if (myOptionState[OPT_NAWS]) {
            // NAWS <16-bit value> <16-bit value>
            if (payload.length() == 5) {
                static constexpr const auto lo = static_cast<int>(
                    std::numeric_limits<uint8_t>::min());
                static constexpr const auto hi = static_cast<int>(
                    std::numeric_limits<uint8_t>::max());
                static_assert(lo == 0, "");
                static_assert(hi == 255, "");
                const int x1 = static_cast<int>(payload[1]);
                const int x2 = static_cast<int>(payload[2]);
                const int y1 = static_cast<int>(payload[3]);
                const int y2 = static_cast<int>(payload[4]);
                if (isClamped(x1, lo, hi) && isClamped(x2, lo, hi) && isClamped(y1, lo, hi)
                    && isClamped(y2, lo, hi)) {
                    const auto x = static_cast<uint16_t>((x1 << 8) + x2);
                    const auto y = static_cast<uint16_t>((y1 << 8) + y2);
                    receiveWindowSize(x, y);
                    break;
                }
            }
            qWarning() << "Corrupted NAWS received" << payload;
        }
        break;

    default:
        //other subnegs should not arrive and if they do, they are merely ignored
        break;
    }
}

void AbstractTelnet::onReadInternal(const QByteArray &data)
{
    //now we have the data, but we cannot forward it to next stage of processing,
    //because the data contains telnet commands
    //so we parse the text and process all telnet commands:

    //clear the GO-AHEAD flag
    recvdGA = false;

    auto cleanData = QByteArray{};
    cleanData.reserve(data.size());

    for (int j = 0; j < data.length(); j++) {
        char i = data.at(j);
        onReadInternal2(cleanData, i);

        if (recvdGA) {
            sendToMapper(cleanData, recvdGA); // with GO-AHEAD
            cleanData.clear();
            recvdGA = false;
        }
    }

    //some data left to send - do it now!
    if (!cleanData.isEmpty()) {
        sendToMapper(cleanData, recvdGA); // without GO-AHEAD
        cleanData.clear();
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
 * x                                # appends 0-254 to option payload
 * IAC IAC                          # appends 255 to option payload
 * IAC (WILL | WONT | DO | DONT) x  # negotiate 0-255 (w/ 255 = EXOPL)
 * IAC SB                           # (error)
 * IAC SE                           # ends subnegotiation
 * IAC x                            # exec command
 *
 * NOTE: RFC 855 refers to IAC SE as a command rather than a delimiter,
 * so that implies you're still supposed to process "commands" (e.g. IAC GA)!
 *
 * So if you receive "IAC SB IAC WILL ECHO f o o IAC IAC b a r IAC SE"
 * then you process will(ECHO) followed by the subnegotiation(f o o 255 b a r).
 */
void AbstractTelnet::onReadInternal2(QByteArray &cleanData, const uint8_t c)
{
    switch (state) {
    case TelnetState::FORWARD:
        if (c == TN_IAC) {
            // this is IAC, previous character was regular data
            state = TelnetState::IAC;
            commandBuffer.append(c);
        } else {
            // plaintext
            cleanData.append(c);
        }
        break;
    case TelnetState::IAC:
        // seq. of two IACs
        if (c == TN_IAC) {
            state = TelnetState::FORWARD;
            cleanData.append(c);
            commandBuffer.clear();
        }
        // IAC DO/DONT/WILL/WONT
        else if ((c == TN_WILL) || (c == TN_WONT) || (c == TN_DO) || (c == TN_DONT)) {
            state = TelnetState::COMMAND;
            commandBuffer.append(c);
        }
        // IAC SB
        else if (c == TN_SB) {
            state = TelnetState::SUBNEG;
            commandBuffer.clear();
        }
        // IAC SE without IAC SB - error - ignored
        else if (c == TN_SE) {
            state = TelnetState::FORWARD;
            commandBuffer.clear();
        }
        // IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
        else {
            state = TelnetState::FORWARD;
            commandBuffer.append(c);
            processTelnetCommand(commandBuffer);
            //this could have set receivedGA to true; we'll handle that later
            // (at the end of this function)
            commandBuffer.clear();
        }
        break;
    case TelnetState::COMMAND:
        // IAC DO/DONT/WILL/WONT <command code>
        state = TelnetState::FORWARD;
        commandBuffer.append(c);
        processTelnetCommand(commandBuffer);
        commandBuffer.clear();
        break;

    case TelnetState::SUBNEG:
        if (c == TN_IAC) {
            // this is IAC, previous character was option payload
            state = TelnetState::SUBNEG_IAC;
            commandBuffer.append(c);
        } else {
            //option payload
            subnegBuffer.append(c);
        }
        break;
    case TelnetState::SUBNEG_IAC:
        // seq. of two IACs
        if (c == TN_IAC) {
            state = TelnetState::FORWARD;
            subnegBuffer.append(c);
            commandBuffer.clear();
        }
        // IAC DO/DONT/WILL/WONT
        else if ((c == TN_WILL) || (c == TN_WONT) || (c == TN_DO) || (c == TN_DONT)) {
            state = TelnetState::SUBNEG_COMMAND;
            commandBuffer.append(c);
        }
        // IAC SE - end of subcommand
        else if (c == TN_SE) {
            state = TelnetState::FORWARD;
            processTelnetSubnegotiation(subnegBuffer);
            commandBuffer.clear();
            subnegBuffer.clear();
        }
        // IAC SB within IAC SB - error - ignored
        else if (c == TN_SB) {
            state = TelnetState::FORWARD;
            commandBuffer.clear();
            subnegBuffer.clear();
        }
        // IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
        else {
            state = TelnetState::SUBNEG;
            commandBuffer.append(c);
            processTelnetCommand(commandBuffer);
            //this could have set receivedGA to true; we'll handle that later
            // (at the end of this function)
            commandBuffer.clear();
        }
        break;
    case TelnetState::SUBNEG_COMMAND:
        // IAC DO/DONT/WILL/WONT <command code>
        state = TelnetState::SUBNEG;
        commandBuffer.append(c);
        processTelnetCommand(commandBuffer);
        commandBuffer.clear();
        break;
    }
}
