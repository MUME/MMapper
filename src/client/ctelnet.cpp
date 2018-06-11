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
#include "configuration/configuration.h"

#include <QApplication>
#include <QDebug>
#include <QHostAddress>
#include <QTextCodec>

#define LATIN_1_ENCODING "ISO-8859-1"

cTelnet::cTelnet(QObject *parent)
    : QObject(parent)
{
    /** MMapper Telnet */
    termType = QString("MMapper %1").arg(MMAPPER_VERSION);
    // set up encoding
    inCoder = nullptr;
    outCoder = nullptr;
    iac = iac2 = insb = false;
    command.clear();
    sentbytes = 0;
    curX = 80;
    curY = 24;
    encoding = Config().m_utf8Charset ? "UTF-8" : LATIN_1_ENCODING;
    reset();
    setupEncoding();

    connect(&socket, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(&socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(&socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(&socket,
            SIGNAL(error(QAbstractSocket::SocketError)),
            this,
            SLOT(onError(QAbstractSocket::SocketError)));
}

cTelnet::~cTelnet()
{
    delete inCoder;
    delete outCoder;
    socket.disconnectFromHost();
    socket.deleteLater();
}

void cTelnet::connectToHost()
{
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.abort();
    }
    socket.connectToHost(QHostAddress::LocalHost, Config().m_localPort);
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
    delete inCoder;
    delete outCoder;

    qDebug() << "* Switching to" << encoding << "encoding";

    Config().m_utf8Charset = (QString::compare(QString(encoding), "UTF-8", Qt::CaseInsensitive)
                              == 0);

    auto textCodec = QTextCodec::codecForName(encoding);
    if (textCodec == nullptr) {
        qWarning() << "* Falling back to LATIN-1 because" << encoding << "was not available";
        textCodec = QTextCodec::codecForName(LATIN_1_ENCODING);
    }

    // MUME can output US-ASCII, LATIN1, or UTF-8
    inCoder = textCodec->makeDecoder();

    // MUME only understands US-ASCII or LATIN1 input
    outCoder = QTextCodec::codecForName(LATIN_1_ENCODING)->makeEncoder();
}

void cTelnet::reset()
{
    //prepare option variables
    for (int i = 0; i < 256; i++) {
        myOptionState[i] = false;
        hisOptionState[i] = false;
        announcedState[i] = false;
        heAnnouncedState[i] = false;
    }
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
    int len = outdata.length();
    bool gotIAC = false;
    for (int i = 0; i < len; i++) {
        if ((unsigned char) outdata[i] == TN_IAC) {
            gotIAC = true;
            break;
        }
    }
    if (gotIAC) {
        QByteArray d;
        // double IACs
        for (int i = 0; i < len; i++) {
            d.append(outdata.at(i));
            if (static_cast<unsigned char>(outdata.at(i)) == TN_IAC) {
                d.append(outdata.at(i)); //double IAC
            }
        }
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
    curX = x;
    curY = y;
    if (myOptionState[OPT_NAWS]) { //only if we have negotiated this option
        QByteArray s;
        s += TN_IAC;
        s += TN_SB;
        s += OPT_NAWS;
        unsigned char x1, x2, y1, y2;
        x1 = static_cast<unsigned char>(x) / 256;
        x2 = static_cast<unsigned char>(x) % 256;
        y1 = static_cast<unsigned char>(y) / 256;
        y2 = static_cast<unsigned char>(y) % 256;
        //IAC must be doubled
        s += x1;
        if (x1 == TN_IAC) {
            s += TN_IAC;
        }
        s += x2;
        if (x2 == TN_IAC) {
            s += TN_IAC;
        }
        s += y1;
        if (y1 == TN_IAC) {
            s += TN_IAC;
        }
        s += y2;
        if (y2 == TN_IAC) {
            s += TN_IAC;
        }

        s += TN_IAC;
        s += TN_SE;
        sendRawData(s);
    }
}

void cTelnet::sendTelnetOption(unsigned char type, unsigned char option)
{
    qDebug() << "* Sending Telnet Command: " << type << " " << option;
    QByteArray s;
    s += TN_IAC;
    s += type;
    s += option;
    sendRawData(s);
}

void cTelnet::processTelnetCommand(const QByteArray &command)
{
    unsigned char ch = command[1];
    unsigned char option;

    switch (command.length()) {
    case 2:
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
        default:
            qDebug() << "* Processing Telnet Command:" << ch;
        };
        break;

    case 3:
        qDebug() << "* Processing Telnet Command:" << ch << static_cast<unsigned char>(command[2]);

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
                windowSizeChanged(curX, curY);
            } else if (myOptionState[OPT_CHARSET] && option == OPT_CHARSET) {
                QString myCharacterSet = Config().m_utf8Charset ? "UTF-8" : LATIN_1_ENCODING;
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

    default:
        qDebug() << "* Processing Telnet Command:" << ch << static_cast<unsigned char>(command[2])
                 << static_cast<unsigned char>(command[3]);

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
                        QByteArray s;
                        s += TN_IAC;
                        s += TN_SB;
                        s += OPT_TERMINAL_TYPE;
                        s += TNSB_IS;
                        s += termType.toLatin1().data();
                        s += TN_IAC;
                        s += TN_SE;
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
                            QString myCharacterSet = Config().m_utf8Charset ? "UTF-8"
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
    int read = socket.read(buffer, 32768);
    if (read != -1) {
        buffer[read] = 0;
    } else if (read == 0) {
        return;
    } else {
        buffer[read] = '\0';
    }
    QByteArray data = QByteArray::fromRawData(buffer, read);
    QByteArray cleanData;

    //clear the GO-AHEAD flag
    recvdGA = false;

    //now we have the data, but we cannot forward it to next stage of processing,
    //because the data contains telnet commands
    //so we parse the text and process all telnet commands:

    for (char i : data) {
        if (iac || iac2 || insb || (static_cast<unsigned char>(i) == TN_IAC)) {
            //there are many possibilities here:
            //1. this is IAC, previous character was regular data
            if (!(iac || iac2 || insb) && (static_cast<unsigned char>(i) == TN_IAC)) {
                iac = true;
                command.append(i);
            }
            //2. seq. of two IACs
            else if (iac && (static_cast<unsigned char>(i) == TN_IAC) && (!insb)) {
                iac = false;
                cleanData.append(i);
                command.clear();
            }
            //3. IAC DO/DONT/WILL/WONT
            else if (iac && (!insb)
                     && ((static_cast<unsigned char>(i) == TN_WILL)
                         || (static_cast<unsigned char>(i) == TN_WONT)
                         || (static_cast<unsigned char>(i) == TN_DO)
                         || (static_cast<unsigned char>(i) == TN_DONT))) {
                iac = false;
                iac2 = true;
                command.append(i);
            }
            //4. IAC DO/DONT/WILL/WONT <command code>
            else if (iac2) {
                iac2 = false;
                command.append(i);
                processTelnetCommand(command);
                command.clear();
            }
            //5. IAC SB
            else if (iac && (!insb) && (static_cast<unsigned char>(i) == TN_SB)) {
                iac = false;
                insb = true;
                command.append(i);
            }
            //6. IAC SE without IAC SB - error - ignored
            else if (iac && (!insb) && (static_cast<unsigned char>(i) == TN_SE)) {
                command.clear();
                iac = false;
            }
            //7. inside IAC SB
            else if (insb) {
                command.append(i);
                if (iac && //IAC SE - end of subcommand
                    (static_cast<unsigned char>(i) == TN_SE)) {
                    processTelnetCommand(command);
                    command.clear();
                    iac = false;
                    insb = false;
                }
                if (iac) {
                    iac = false;
                } else if (static_cast<unsigned char>(i) == TN_IAC) {
                    iac = true;
                }
            }
            //8. IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
            else {
                iac = false;
                command.append(i);
                processTelnetCommand(command);
                //this could have set receivedGA to true; we'll handle that later
                // (at the end of this function)
                command.clear();
            }
        } else { //plaintext
            //everything except CRLF is okay; CRLF is replaced by LF(\n) (CR ignored)

            switch (i) {
            case '\a': // BEL
                QApplication::beep();
                break;
            default:
                cleanData.append(i);
            };
        }

        // TODO(nschimme): do something about all that code duplication ...

        //we've just received the GA signal - higher layers shall be informed about it
        if (recvdGA) {
            //forward data for further processing
            QString unicodeData;
            unicodeData = inCoder->toUnicode(cleanData);
            emit sendToUser(unicodeData); // with GO-AHEAD

            //clean the flag, and the data (so that we don't send it multiple times)
            cleanData.clear();
            recvdGA = false;
        }
    }

    //some data left to send - do it now!
    if (!cleanData.isEmpty()) {
        QString unicodeData = inCoder->toUnicode(cleanData);
        emit sendToUser(unicodeData); // without GO-AHEAD

        // clean the buffer
        cleanData.clear();
    }
}
