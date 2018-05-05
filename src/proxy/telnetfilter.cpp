/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "telnetfilter.h"
#include "patterns.h"
#include "configuration.h"

#define TC_SE  240  //End of subnegotiation parameters.  
#define TC_NOP  241  //No operation  
#define TC_DM  242  //Data mark. Indicates the position of a Synch event within the data stream. This should always be accompanied by a TCP urgent notification.  
#define TC_BRK  243  //Break. Indicates that the "break" or "attention" key was hit.  
#define TC_IP  244  //Suspend, interrupt or abort the process to which the NVT is connected.  
#define TC_AO  245  //Abort output. Allows the current process to run to completion but do not send its output to the user.  
#define TC_AYT  246  //Are you there. Send back to the NVT some visible evidence that the AYT was received.  
#define TC_EC  247  //Erase character. The receiver should delete the last preceding undeleted character from the data stream.  
#define TC_EL  248  //Erase line. Delete characters from the data stream back to but not including the previous CRLF.  
#define TC_GA  249  //Go ahead. Used, under certain circumstances, to tell the other end that it can transmit.  
#define TC_SB  250  //Subnegotiation of the indicated option follows.  
#define TC_WILL  251  //Indicates the desire to begin performing, or confirmation that you are now performing, the indicated option.  
#define TC_WONT  252  //Indicates the refusal to perform, or continue performing, the indicated option.  
#define TC_DO  253  //Indicates the request that the other party perform, or confirmation that you are expecting the other party to perform, the indicated option.  
#define TC_DONT  254  //Indicates the demand that the other party stop performing, or confirmation that you are no longer expecting the other party to perform, the indicated option.  
#define TC_IAC  255

#define ASCII_DEL 8

#define ASCII_CR 13
#define ASCII_LF 10

#define ESCAPE '\x1B'
const QChar TelnetFilter::s_escChar('\x1B');

TelnetFilter::TelnetFilter(QObject *parent)
    : QObject(parent)
{
#ifdef TELNET_STREAM_DEBUG_INPUT_TO_FILE
    QString fileName = "telnet_debug.dat";

    file = new QFile(fileName);

    if (!file->open(QFile::WriteOnly))
        return;

    debugStream = new QDataStream(file);
#endif
}

TelnetFilter::~TelnetFilter()
{
#ifdef TELNET_STREAM_DEBUG_INPUT_TO_FILE
    file->close();
#endif
}

void TelnetFilter::analyzeMudStream(const QByteArray &ba)
{
    dispatchTelnetStream(ba, m_mudIncomingData, m_mudIncomingQue);
    IncomingData data;
    while ( !m_mudIncomingQue.isEmpty() ) {
        data = m_mudIncomingQue.dequeue();
        //parse incoming lines in que
        emit parseNewMudInput(data/*m_mudIncomingQue*/);
    }
    return;
}

void TelnetFilter::analyzeUserStream(const QByteArray &ba)
{
    dispatchTelnetStream(ba, m_userIncomingData, m_userIncomingQue);

    //parse incoming lines in que
    IncomingData data;
    while ( !m_userIncomingQue.isEmpty() ) {
        data = m_userIncomingQue.dequeue();

        emit parseNewUserInput(data);
    }

    return;
}

void TelnetFilter::dispatchTelnetStream(const QByteArray &stream, IncomingData &m_incomingData,
                                        TelnetIncomingDataQueue &que)
{
    quint8 val1 = 0;
    quint8 val2 = 0;
    quint16 index = 0; //Here br

#ifdef TELNET_STREAM_DEBUG_INPUT_TO_FILE
    if (stream.contains("<")) {
        (*debugStream) << "***S***";
        (*debugStream) << stream;
        (*debugStream) << "***E***";
    }
#endif

    while (index < stream.size()) {
        val1 = (quint8) stream.at(index);
        switch (val1) {
        case ASCII_DEL:
            m_incomingData.line.append((char)ASCII_DEL);

            if (m_incomingData.type != TDT_TELNET) {
                m_incomingData.type = TDT_DELAY;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TDT_SPLIT;
            }
            index++;
            break;

        case TC_IAC: //IAC IAC  - should not happen tho, we use ASCII chars only!!!

            if ( m_incomingData.type != TDT_TELNET && !m_incomingData.line.isEmpty()) {
                if (Config().m_IAC_prompt_parser
                        && ((quint8) stream.at(index - 1)) == '>' /*&& ((quint8) stream.at(index+1)) == TC_GA*/) {
                    //prompt
                    m_incomingData.type = TDT_PROMPT;
                } else
                    m_incomingData.type = TDT_UNKNOWN;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
            }
            //start incoming telnet command
            m_incomingData.type = TDT_TELNET;
            m_incomingData.line.append((char)val1);
            index++;
            break;

        case TC_GA:

        case TC_NOP:
        case TC_DM:
        case TC_BRK:
        case TC_IP:
        case TC_AO:
        case TC_AYT:
        case TC_EC:
        case TC_EL:
        case TC_SE:
            if (m_incomingData.type == TDT_TELNET) {
                //end incoming telnet command
                m_incomingData.line.append((char)val1);
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
//              if (val1 == TC_GA && Config().m_IAC_prompt_parser)
//                  m_incomingData.type = TDT_PROMPT;
//              else
                m_incomingData.type = TDT_SPLIT;

            } else
                m_incomingData.line.append((char)val1);
            index++;
            break;

        case ASCII_CR:
            if (!m_incomingData.line.isEmpty() && m_incomingData.type != TDT_TELNET) {
                val2 = m_incomingData.line.right(1).at(0); //get last char
                switch (val2) {
                case ASCII_LF:
                    m_incomingData.line.append((char)ASCII_CR);
                    m_incomingData.type = TDT_LFCR;
                    {
                        que.enqueue(m_incomingData);
                    }
                    m_incomingData.line.clear();
                    m_incomingData.type = TDT_SPLIT;
                    index++;
                    break;

                default:
                    m_incomingData.line.append((char)ASCII_CR);
                    index++;
                    break;
                }
            } else {
                m_incomingData.line.append((char)ASCII_CR);
                index++;
                break;
            }
            break;


        case ASCII_LF:
            if (!m_incomingData.line.isEmpty() && m_incomingData.type != TDT_TELNET) {
                val2 = m_incomingData.line.right(1).at(0); //get last char
                switch (val2) {
                case ASCII_CR:
                    m_incomingData.line.append((char)ASCII_LF);
                    m_incomingData.type = TDT_CRLF;
                    {
                        que.enqueue(m_incomingData);
                    }
                    m_incomingData.line.clear();
                    m_incomingData.type = TDT_SPLIT;
                    index++;
                    break;

                default:
                    m_incomingData.line.append((char)ASCII_LF);
                    index++;
                    break;
                }
            } else {
                m_incomingData.line.append((char)ASCII_LF);
                index++;
                break;
            }
            break;

        default:
            if (m_incomingData.type == TDT_TELNET) {
                if (!m_incomingData.line.isEmpty()) {
                    val2 = m_incomingData.line.right(1).at(0); //get last char
                    switch (val2) {
                    case TC_DONT:
                    case TC_DO:
                    case TC_WONT:
                    case TC_WILL:
                        //end incoming telnet command
                        m_incomingData.line.append((char)val1);
                        {
                            que.enqueue(m_incomingData);
                        }
                        m_incomingData.line.clear();
                        m_incomingData.type = TDT_SPLIT;
                        index++;
                        break;

                    default:
                        m_incomingData.line.append((char)val1);
                        index++;
                        break;
                    }
                }
            } else {
                if (!m_incomingData.line.isEmpty() && m_incomingData.line.right(1).at(0) == ASCII_LF) {
                    m_incomingData.type = TDT_LF;
                    que.enqueue(m_incomingData);
                    m_incomingData.line.clear();
                    m_incomingData.type = TDT_SPLIT;
                }

                m_incomingData.line.append((char)val1);
                index++;
                break;
            }
            break;
        }
    }

    if ( !m_incomingData.line.isEmpty() && m_incomingData.type == TDT_SPLIT) {
        {
            if (m_incomingData.line.endsWith("prompt>")) {
                m_incomingData.type = TDT_PROMPT;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TDT_SPLIT;
            } else if (m_incomingData.line.endsWith(char(ASCII_LF))) {
                m_incomingData.type = TDT_LF;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TDT_SPLIT;
            } else if (Patterns::matchPromptPatterns(m_incomingData.line)) {
                m_incomingData.type = TDT_PROMPT;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TDT_SPLIT;
            } else if (Patterns::matchPasswordPatterns(m_incomingData.line)) {
                m_incomingData.type = TDT_LOGIN_PASSWORD;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TDT_SPLIT;
            } else if (Patterns::matchMenuPromptPatterns(m_incomingData.line)) {
                m_incomingData.type = TDT_MENU_PROMPT;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TDT_SPLIT;
            } else if (Patterns::matchLoginPatterns(m_incomingData.line)) {
                m_incomingData.type = TDT_LOGIN;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TDT_SPLIT;
            }
        }
    }
}
