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

#ifndef SEARCHTREENODE
#define SEARCHTREENODE
#include "tinylist.h"

class ParseEvent;
class RoomCollection;
class RoomOutStream;

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
