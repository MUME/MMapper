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

#ifndef ABSTRACTEXIT_H
#define ABSTRACTEXIT_H

#include <set>
#include <QVector>
#include <QVariant>

class Exit : public QVector<QVariant> {
  
  protected:
    std::set<uint> incoming;
    std::set<uint> outgoing;
    
  public:
    Exit(uint numProps = 0) : QVector<QVariant>(numProps) {}
    std::set<uint>::const_iterator inBegin() const {return incoming.begin();}
    std::set<uint>::const_iterator outBegin() const {return outgoing.begin();}
    
    std::set<uint>::const_iterator inEnd() const {return incoming.end();}
    std::set<uint>::const_iterator outEnd() const {return outgoing.end();}
    
    void addIn(uint from) {incoming.insert(from);}
    void addOut(uint to) {outgoing.insert(to);}
    void removeIn(uint from) {incoming.erase(from);}
    void removeOut(uint to) {outgoing.erase(to);}
    bool containsIn(uint from) const {return incoming.find(from) != incoming.end();}
    bool containsOut(uint to) const {return outgoing.find(to) != outgoing.end();}
    void removeAll() {incoming.clear(); outgoing.clear();}
};

#endif
