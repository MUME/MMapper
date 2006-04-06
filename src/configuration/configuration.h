/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/


#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include <QtGui>
#include <QtCore>
#include "defs.h"

class Configuration {
  public:
    void read();
    void write() const;
    
    bool m_firstRun;

	int m_mapMode; //0 play, 1 map

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
    bool m_showUpdated;
    bool m_drawNotMappedExits;
    bool m_drawUpperLayersTextured;
    
    enum RoomDescriptionsParserType {RDPT_COLOR, RDPT_PARSER, RDPT_LINEBREAK};
    RoomDescriptionsParserType   	 m_roomDescriptionsParserType; 
    quint16  m_minimumStaticLines;
    
    bool m_IAC_prompt_parser;
    bool m_useXmlParser;
    
    
    QStringList m_moveCancelPatternsList; // string wildcart patterns, that cancel last move command
    QStringList m_moveForcePatternsList;  // string wildcart patterns, that force new move command
    QStringList m_noDescriptionPatternsList;
    QStringList m_dynamicDescriptionPatternsList;
	QString 	m_exitsPattern;
    QString 	m_scoutPattern;
    QByteArray 	m_promptPattern;
    QByteArray 	m_loginPattern;
    QByteArray 	m_passwordPattern;
    QByteArray 	m_menuPromptPattern;

    qreal m_acceptBestRelative;
    qreal m_acceptBestAbsolute;
    qreal m_newRoomPenalty;
	qreal m_multipleConnectionsPenalty;
    qreal m_correctPositionBonus;
    quint32 m_maxPaths;
    quint32 m_matchingTolerance;

  private:
    Configuration();
    Configuration(const Configuration&);

    friend Configuration& Config();
};

/// Returns a reference to the application configuration object.
Configuration& Config();


#endif
