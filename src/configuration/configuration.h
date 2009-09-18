/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara),
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

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include <QtGui>
#include <QtCore>
#include "defs.h"

class Configuration {
  public:
    void read();
    void write() const;
    bool isChanged() { return configurationChanged; }

    bool m_firstRun, m_mpi;
    QPoint windowPosition;
    QSize windowSize;
    QByteArray windowState;
    bool alwaysOnTop;
    int m_mapMode; //0 play, 1 map
    void setFirstRun(bool value) { m_firstRun = value; change(); }
    void setWindowPosition(QPoint pos) {windowPosition = pos; change(); }
    void setWindowSize(QSize size) { windowSize = size; change(); }
    void setWindowState(QByteArray state) { windowState = state; change(); }
    void setAlwaysOnTop(bool b) { alwaysOnTop = b; change(); }

    QString   m_remoteServerName;         /// Remote host and port settings
    quint32   m_remotePort;
    quint32   m_localPort;         /// Port to bind to on local machine

    bool m_autoLog;         // enables log to file
    QString m_logFileName;  // file name to log
    bool m_autoLoadWorld;
    QString m_autoLoadFileName;

    QString m_roomNameColor; // ANSI room name color
    QString m_roomDescColor; // ANSI room descriptions color
    bool m_brief;
    bool m_emulatedExits;
    bool m_showUpdated;
    bool m_drawNotMappedExits;
    bool m_drawUpperLayersTextured;
    bool m_drawDoorNames;
    
    enum RoomDescriptionsParserType {RDPT_COLOR, RDPT_PARSER, RDPT_LINEBREAK};
    RoomDescriptionsParserType           m_roomDescriptionsParserType;
    quint16  m_minimumStaticLines;

    bool m_IAC_prompt_parser;
    bool m_removeXmlTags;

    QStringList m_moveCancelPatternsList; // string wildcart patterns, that cancel last move command
    QStringList m_moveForcePatternsList;  // string wildcart patterns, that force new move command
    QStringList m_noDescriptionPatternsList;
    QStringList m_dynamicDescriptionPatternsList;
    QString         m_exitsPattern;
    QString     m_scoutPattern;
    QByteArray  m_promptPattern;
    QByteArray  m_loginPattern;
    QByteArray  m_passwordPattern;
    QByteArray  m_menuPromptPattern;

    qreal m_acceptBestRelative;
    qreal m_acceptBestAbsolute;
    qreal m_newRoomPenalty;
    qreal m_multipleConnectionsPenalty;
    qreal m_correctPositionBonus;
    quint32 m_maxPaths;
    quint32 m_matchingTolerance;

    int m_groupManagerState;
    int m_groupManagerLocalPort;
    int m_groupManagerRemotePort;
    QByteArray m_groupManagerHost;
    QByteArray m_groupManagerCharName;
    bool m_showGroupManager;
    QRect m_groupManagerRect;
    QColor m_groupManagerColor;
    bool m_groupManagerRulesWarning;

  private:
    Configuration();
    Configuration(const Configuration&);

    bool configurationChanged;
    void change() { configurationChanged = true; }

    friend Configuration& Config();
};

/// Returns a reference to the application configuration object.
Configuration& Config();


#endif
