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

#ifndef _PATTERNS_H_
#define _PATTERNS_H_

#include <QtGui>
#include <QtCore>

class Patterns {
	
    static QRegExp m_rx;
    
public:
	static bool matchMoveCancelPatterns(QString&);
	static bool matchMoveForcePatterns(QString&);
	static bool matchNoDescriptionPatterns(QString&);
	static bool matchDynamicDescriptionPatterns(QString&);
	static bool matchPasswordPatterns(QByteArray&);
	static bool matchExitsPatterns(QString&);
    static bool matchScoutPatterns(QString&);
    static bool matchPromptPatterns(QByteArray&);
    static bool matchLoginPatterns(QByteArray&);
    static bool matchMenuPromptPatterns(QByteArray&);

    static bool matchPattern(QString pattern, QString& str);
    static bool matchPattern(QByteArray pattern, QByteArray& str);
};

#endif
