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

#ifndef TELNETFILTER
#define TELNETFILTER

//#define TELNET_STREAM_DEBUG_INPUT_TO_FILE

#include <QtGui>
#include <QtCore>
#include <QtNetwork>

enum TelnetDataType { TDT_PROMPT, TDT_MENU_PROMPT, TDT_LOGIN, TDT_LOGIN_PASSWORD, TDT_CRLF, TDT_LFCR, TDT_LF, TDT_TELNET, TDT_DELAY, TDT_SPLIT, TDT_UNKNOWN };

struct IncomingData {
    QByteArray line;
    TelnetDataType type;
};

typedef QQueue<IncomingData> TelnetIncomingDataQueue;



class TelnetFilter : public QObject {
	
	public:
           TelnetFilter(QObject *parent);
           ~TelnetFilter();

	public slots:
		void analyzeMudStream( const char * input, int length );
		void analyzeUserStream( const char * input, int length );
		
		void setNormalMode();
		void setXmlMode();

	signals:
	  	void parseNewMudInput(IncomingData&);
	  	void parseNewUserInput(IncomingData&);
	  	void parseNewMudInputXml(IncomingData&);
	  	void parseNewUserInputXml(IncomingData&);

		//telnet
		void sendToMud(const QByteArray&);
		void sendToUser(const QByteArray&);
		
	private:

#ifdef TELNET_STREAM_DEBUG_INPUT_TO_FILE
	QDataStream *debugStream;
	QFile* file;
#endif

		Q_OBJECT
		void dispatchTelnetStream(QByteArray& stream, IncomingData &m_incomingData, TelnetIncomingDataQueue &que);

		static const QChar escChar;

   		IncomingData m_userIncomingData;
   		IncomingData m_mudIncomingData;
        TelnetIncomingDataQueue m_mudIncomingQue;        
        TelnetIncomingDataQueue m_userIncomingQue;        
        
       bool m_reconnecting;
       bool m_xmlModeAutoconfigured;
       bool m_xmlMode; 
        
};

#endif

