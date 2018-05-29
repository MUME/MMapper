/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef TELNETFILTER
#define TELNETFILTER

//#define TELNET_STREAM_DEBUG_INPUT_TO_FILE

#include <QByteArray>
#include <QObject>
#include <QQueue>

enum TelnetDataType { TDT_PROMPT, TDT_MENU_PROMPT, TDT_LOGIN, TDT_LOGIN_PASSWORD,
                      TDT_CRLF, TDT_LFCR, TDT_LF, TDT_TELNET, TDT_DELAY, TDT_SPLIT, TDT_UNKNOWN
                    };

struct IncomingData {
    IncomingData()
    {
        type = TDT_PROMPT;
    }
    QByteArray line;
    TelnetDataType type;
};

using TelnetIncomingDataQueue = QQueue<IncomingData>;

class TelnetFilter : public QObject
{

public:
    TelnetFilter(QObject *parent);
    ~TelnetFilter();

public slots:
    void analyzeMudStream( const QByteArray &ba );
    void analyzeUserStream( const QByteArray &ba );

signals:
    void parseNewMudInput(IncomingData &);
    void parseNewUserInput(IncomingData &);

    //telnet
    void sendToMud(const QByteArray &);
    void sendToUser(const QByteArray &);

private:

#ifdef TELNET_STREAM_DEBUG_INPUT_TO_FILE
    QDataStream *debugStream;
    QFile *file;
#endif

    Q_OBJECT
    void dispatchTelnetStream(const QByteArray &stream, IncomingData &m_incomingData,
                              TelnetIncomingDataQueue &que);

    IncomingData m_userIncomingData;
    IncomingData m_mudIncomingData;
    TelnetIncomingDataQueue m_mudIncomingQue;
    TelnetIncomingDataQueue m_userIncomingQue;
};

#endif

