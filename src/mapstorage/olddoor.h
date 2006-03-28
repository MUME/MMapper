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

#ifndef DOOR_H
#define DOOR_H

#define DF_HIDDEN     bit1
#define DF_NEEDKEY    bit2
#define DF_NOBLOCK    bit3
#define DF_NOBREAK    bit4
#define DF_NOPICK     bit5
#define DF_DELAYED    bit6
#define DF_RESERVED1  bit7
#define DF_RESERVED2  bit8
typedef class QString DoorName;
typedef quint8 DoorFlags;

class Door {

public:
    Door (DoorName name = "", DoorFlags flags = 0){ m_name = name; m_flags = flags; }
    DoorFlags getFlags() const { return m_flags; };
    DoorName getName() const { return m_name; };
    void setFlags(DoorFlags flags) { m_flags = flags; };
    void setName(const DoorName & name) { m_name = name; };

private:
    DoorName  m_name;
    DoorFlags m_flags;
};

#endif
