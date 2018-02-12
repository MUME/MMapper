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

#include <QDebug>
#include <QTextCodec>
#include <QApplication>
#include <QHostAddress>

#define LATIN_1_ENCODING "ISO 8859-1"

cTelnet::cTelnet(QObject *parent)
    : QObject(parent)
{
    /** MMapper Telnet */
    termType = QString("MMapper %1").arg(MMAPPER_VERSION);
    // set up encoding
    inCoder = 0;
    outCoder = 0;
    iac = iac2 = insb = false;
    command.clear();
    sentbytes = 0;
    curX = 80;
    curY = 24;
    startupneg = false;
    encoding = Config().m_utf8Charset ? "UTF-8" : LATIN_1_ENCODING;
    reset();
    setupEncoding();

    connect(&socket, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(&socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(&socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this,
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
    socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);
    socket.waitForConnected(3000);
}

void cTelnet::onConnected()
{
    qDebug() << "Telnet detected socket connect!";

    reset ();
    sentbytes = 0;

    //negotiate some telnet options, if allowed
    if (startupneg) {
        //NAWS (used to send info about window size)
        sendTelnetOption (TN_WILL, OPT_NAWS);
        //do not allow server to echo our text!
        sendTelnetOption (TN_DONT, OPT_ECHO);
        //we will send our terminal type
        sendTelnetOption (TN_WILL, OPT_TERMINAL_TYPE);
    }

    emit connected();
}

void cTelnet::disconnectFromHost()
{
    socket.disconnectFromHost();
}

void cTelnet::onDisconnected()
{
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

    // MUME can output ASCII, Latin-1, or UTF-8
    inCoder = QTextCodec::codecForName(encoding)->makeDecoder();

    // MUME only understands Latin-1 input
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
        triedToEnable[i] = false;
    }
    //reset telnet status
    iac = iac2 = insb = false;
    command.clear();
}

void cTelnet::sendToMud(const QByteArray &data)
{
    QByteArray outdata = outCoder->fromUnicode(data);

    // IAC byte must be doubled
    int len = outdata.length();
    bool gotIAC = false;
    for (int i = 0; i < len; i++)
        if ((unsigned char) outdata[i] == TN_IAC) {
            gotIAC = true;
            break;
        }
    if (gotIAC) {
        QByteArray d;
        // double IACs
        for (int i = 0; i < len; i++) {
            d.append(outdata.at(i));
            if ((unsigned char) outdata.at(i) == TN_IAC)
                d.append(outdata.at(i));  //double IAC
        }
        outdata = d;
    }

    //data ready, send it
    sendRawData(outdata);
}

void cTelnet::sendRawData (const QByteArray &data)
{
    //update counter
    sentbytes += data.length();
    socket.write(data, data.length());
}

void cTelnet::windowSizeChanged (int x, int y)
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
        x1 = (unsigned char) x / 256;
        x2 = (unsigned char) x % 256;
        y1 = (unsigned char) y / 256;
        y2 = (unsigned char) y % 256;
        //IAC must be doubled
        s += x1;
        if (x1 == TN_IAC)
            s += TN_IAC;
        s += x2;
        if (x2 == TN_IAC)
            s += TN_IAC;
        s += y1;
        if (y1 == TN_IAC)
            s += TN_IAC;
        s += y2;
        if (y2 == TN_IAC)
            s += TN_IAC;

        s += TN_IAC;
        s += TN_SE;
        sendRawData(s);
    }
}

void cTelnet::sendTelnetOption (unsigned char type, unsigned char option)
{
    qDebug() << "* Sending Telnet Command: " << type << " " << option;
    QByteArray s;
    s += TN_IAC;
    s += (unsigned char) type;
    s += (unsigned char) option;
    sendRawData(s);
}

void cTelnet::processTelnetCommand (const QByteArray &command)
{
    unsigned char ch = command[1];
    unsigned char option;

    switch (command.length()) {
    case 2:
        qDebug() << "* Processing Telnet Command:" << command[1];

        switch (ch) {
        case TN_AYT:
            sendRawData ("I'm here! Please be more patient!\r\n");
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
        qDebug() << "* Processing Telnet Command:" << command[1] << command[2];

        switch (ch) {
        case TN_WILL:
            //server wants to enable some option (or he sends a timing-mark)...
            option = command[2];

            heAnnouncedState[option] = true;
            if (triedToEnable[option]) {
                hisOptionState[option] = true;
                triedToEnable[option] = false;
            } else {
                if (!hisOptionState[option])
                    //only if this is not set; if it's set, something's wrong wth the server
                    //(according to telnet specification, option announcement may not be
                    //unless explicitly requested)
                {
                    if ((option == OPT_SUPPRESS_GA) || (option == OPT_STATUS) ||
                            (option == OPT_TERMINAL_TYPE) || (option == OPT_NAWS) ||
                            (option == OPT_ECHO))
                        //these options are supported
                    {
                        sendTelnetOption (TN_DO, option);
                        hisOptionState[option] = true;
                        // Echo mode support
                        if (option == OPT_ECHO) {
                            echoMode = false;
                            emit echoModeChanged(echoMode);
                        } else {
                            sendTelnetOption (TN_DONT, option);
                            hisOptionState[option] = false;
                        }
                    }
                }
            }
            break;
        case TN_WONT:
            //server refuses to enable some option...
            option = command[2];
            if (triedToEnable[option]) {
                hisOptionState[option] = false;
                triedToEnable[option] = false;
                heAnnouncedState[option] = true;
            } else {
                //send DONT if needed (see RFC 854 for details)
                if (hisOptionState[option] || (!heAnnouncedState[option])) {
                    sendTelnetOption (TN_DONT, option);
                    hisOptionState[option] = false;
                    if (option == OPT_ECHO) {
                        echoMode = true;
                        emit echoModeChanged(echoMode);
                    }
                }
                heAnnouncedState[option] = true;
            }
            break;
        case TN_DO:
            //server wants us to enable some option
            option = command[2];
            if (option == OPT_TIMING_MARK) {
                //send WILL TIMING_MARK
                sendTelnetOption (TN_WILL, option);
            } else if (!myOptionState[option])
                //only if the option is currently disabled
            {
                if ((option == OPT_SUPPRESS_GA) || (option == OPT_STATUS) ||
                        (option == OPT_TERMINAL_TYPE) || (option == OPT_NAWS)) {
                    sendTelnetOption (TN_WILL, option);
                    myOptionState[option] = true;
                    announcedState[option] = true;
                } else {
                    sendTelnetOption (TN_WONT, option);
                    myOptionState[option] = false;
                    announcedState[option] = true;
                }
            }
            if (option == OPT_NAWS)  //NAWS here - window size info must be sent
                windowSizeChanged (curX, curY);
            break;
        case TN_DONT:
            //only respond if value changed or if this option has not been announced yet
            option = command[2];
            if (myOptionState[option] || (!announcedState[option])) {
                sendTelnetOption (TN_WONT, option);
                announcedState[option] = true;
            }
            myOptionState[option] = false;
            break;
        };
        break;

    case 4:
        qDebug() << "* Processing Telnet Command:" << command[1] << command[2] << command[3];

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
                                s += (unsigned char) i;
                            }
                            if (hisOptionState[i]) {
                                s += TN_DO;
                                s += (unsigned char) i;
                            }
                        }
                        s += TN_IAC;
                        s += TN_SE;
                        sendRawData (s);
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
                        sendRawData (s);
                    }
                }
                break;
                //other cmds should not arrive, as they were not negotiated.
                //if they do, they are merely ignored
            };
            break;
        };
        break;
    };
    //other commands are simply ignored (NOP and such, see .h file for list)
}

void cTelnet::onReadyRead()
{
    int read;
    while (socket.bytesAvailable()) {
        read = socket.read(buffer, 8191);
        if (read != -1) {
            buffer[read] = 0;
        }
    }
    QByteArray data = QByteArray::fromRawData(buffer, read);
    QByteArray cleanData;

    //clear the GO-AHEAD flag
    recvdGA = false;

    //now we have the data, but we cannot forward it to next stage of processing,
    //because the data contains telnet commands
    //so we parse the text and process all telnet commands:

    for (unsigned int i = 0; i < (unsigned int) data.length(); i++) {
        if (iac || iac2 || insb ||
                ((unsigned char) data.at(i) == TN_IAC)) {
            //there are many possibilities here:
            //1. this is IAC, previous character was regular data
            if (! (iac || iac2 || insb) &&
                    ((unsigned char) data.at(i) == TN_IAC)) {
                iac = true;
                command.append(data.at(i));
            }
            //2. seq. of two IACs
            else if (iac && ((unsigned char) data.at(i) == TN_IAC)
                     && (!insb)) {
                iac = false;
                cleanData.append(data.at(i));
                command.clear();
            }
            //3. IAC DO/DONT/WILL/WONT
            else if (iac && (!insb) &&
                     (((unsigned char) data.at(i) == TN_WILL) ||
                      ((unsigned char) data.at(i) == TN_WONT) ||
                      ((unsigned char) data.at(i) == TN_DO)   ||
                      ((unsigned char) data.at(i) == TN_DONT))) {
                iac = false;
                iac2 = true;
                command.append(data.at(i));
            }
            //4. IAC DO/DONT/WILL/WONT <command code>
            else if (iac2) {
                iac2 = false;
                command.append(data.at(i));
                processTelnetCommand (command);
                command.clear();
            }
            //5. IAC SB
            else if (iac && (!insb) &&
                     ((unsigned char) data.at(i) == TN_SB)) {
                iac = false;
                insb = true;
                command.append(data.at(i));
            }
            //6. IAC SE without IAC SB - error - ignored
            else if (iac && (!insb) &&
                     ((unsigned char) data.at(i) == TN_SE)) {
                command.clear();
                iac = false;
            }
            //7. inside IAC SB
            else if (insb) {
                command.append(data.at(i));
                if (iac && //IAC SE - end of subcommand
                        ((unsigned char) data.at(i) == TN_SE)) {
                    processTelnetCommand (command);
                    command.clear();
                    iac = false;
                    insb = false;
                }
                if (iac)
                    iac = false;
                else if ((unsigned char) data.at(i) == TN_IAC)
                    iac = true;
            }
            //8. IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
            else {
                iac = false;
                command.append(data.at(i));
                processTelnetCommand (command);
                //this could have set receivedGA to true; we'll handle that later
                // (at the end of this function)
                command.clear();
            }
        } else { //plaintext
            //everything except CRLF is okay; CRLF is replaced by LF(\n) (CR ignored)

            switch (data.at(i)) {
            case '\a': // BEL
                QApplication::beep();
                break;
            default:
                cleanData.append(data.at(i));
            };
        }

        // TODO: do something about all that code duplication ...

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
