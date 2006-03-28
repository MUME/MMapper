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

#ifndef SEARCHTREENODE
#define SEARCHTREENODE
#include "tinylist.h"
#include "roomcollection.h"
#include "roomoutstream.h"



/**
 * keeps a substring of the properties, and a table of other RoomSearchNodes pointing to the possible following characters
 */

class SearchTreeNode {
	protected:
		bool ownerOfChars;
		TinyList<SearchTreeNode *> * children; 
		char * myChars;
	public:
		SearchTreeNode(ParseEvent * event, TinyList<SearchTreeNode *> * children = 0);
		SearchTreeNode(char * string = 0, TinyList<SearchTreeNode *> * children = 0);
		virtual ~SearchTreeNode();
		virtual void getRooms(RoomOutStream & stream, ParseEvent * event);
		virtual RoomCollection * insertRoom(ParseEvent * event);
		
		virtual void setChild(char, SearchTreeNode *);
		virtual void skipDown(RoomOutStream & stream, ParseEvent * event);
};



#endif
