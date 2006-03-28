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

#ifndef INFOMARK_H
#define INFOMARK_H

#include "coordinate.h"
#include <QtCore>

typedef class QString InfoMarkName;
typedef class QString InfoMarkText;
enum InfoMarkType { MT_TEXT, MT_LINE, MT_ARROW };

typedef QDateTime MarkerTimeStamp;

class InfoMark {

public:
    InfoMark(){};
    ~InfoMark(){};

    InfoMarkName getName(){ return m_name; };
    InfoMarkText getText(){ return m_text; };
    InfoMarkType getType() { return m_type; };
    Coordinate & getPosition1() {return m_pos1;}
    Coordinate & getPosition2() {return m_pos2;}
    MarkerTimeStamp getTimeStamp() { return m_timeStamp; };

    void setPosition1(Coordinate & pos) {m_pos1 = pos;}
    void setPosition2(Coordinate & pos) {m_pos2 = pos;}
    void setName(InfoMarkName name) { m_name = name; };
    void setText(InfoMarkText text) { m_text = text; };
    void setType(InfoMarkType type ){ m_type = type; };
    
    void setTimeStamp(MarkerTimeStamp timeStamp) { m_timeStamp = timeStamp; };

private:
    InfoMarkName m_name;
    InfoMarkType m_type;
    InfoMarkText m_text;
    
    MarkerTimeStamp m_timeStamp;

    Coordinate m_pos1;
    Coordinate m_pos2;
};

#endif
