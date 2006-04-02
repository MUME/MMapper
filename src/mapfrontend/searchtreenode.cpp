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

#include "searchtreenode.h"
#include "intermediatenode.h"
#include <string>
using namespace std;


SearchTreeNode::SearchTreeNode(ParseEvent * event, TinyList<SearchTreeNode *> * in_children) : ownerOfChars(true) {
  const char * rest = event->current()->rest();
  
  myChars = new char[strlen(rest)];
  strcpy(myChars, rest+1); 	// we copy the string so that we can remove rooms independently of tree nodes
  if (in_children == 0) children = new TinyList<SearchTreeNode *>();
  else children = in_children;
}

SearchTreeNode::~SearchTreeNode() {
  if (ownerOfChars) delete myChars;
  for (uint i = 0; i < children->size(); ++i) {
    delete children->get(i);
  }
  delete children;
}

SearchTreeNode::SearchTreeNode(char * in_myChars, TinyList<SearchTreeNode *> * in_children) : ownerOfChars(false) {
  myChars = in_myChars; 
  if (in_children == 0) children = new TinyList<SearchTreeNode *>();
  else children = in_children;
}


void SearchTreeNode::getRooms(RoomOutStream & stream, ParseEvent * event) {
  SearchTreeNode * selectedChild = 0;
  Property * currentProperty = event->current();
  
  for (int i = 0; myChars[i] != 0; i++) {
    if (currentProperty->next() != myChars[i]) {
      for(; i > 0 ; i--) currentProperty->prev();
      return;
    }
  }
  selectedChild = children->get(currentProperty->next());

  if(selectedChild == 0) {
    for (int i = 1; i < myChars[i] != 0; i++) currentProperty->prev();
    return; // no such room
  }
  else selectedChild->getRooms(stream, event);	// the last character of name is 0, 
  // at position 0 there is a roomCollection, if we have rooms here
  // else there is 0, so name[depth] should work.
}

void SearchTreeNode::setChild(char position, SearchTreeNode * node) {
  children->put(position, node);
}

RoomCollection * SearchTreeNode::insertRoom(ParseEvent * event) {
  SearchTreeNode * selectedChild = 0;
  Property * currentProperty = event->current();
  char c = currentProperty->next();

  for (int i = 0; myChars[i] != 0; i++) {
    if (c != myChars[i]) {
      // we have to split, as we encountered a difference in the strings ...	
      selectedChild = new SearchTreeNode(myChars + i + 1, children);	// first build the lower part of this node	
      children = new TinyList<SearchTreeNode *>();	// and update the upper part of this node
      children->put(myChars[i], selectedChild);
		
      if (c == 0) selectedChild = new IntermediateNode(event);	// then build the branch
      else selectedChild = new SearchTreeNode(event);
				
      children->put(c, selectedChild); // and update again
      myChars[i] = 0; // the string is separated in [myChars][0][selectedChildChars][0] so we don't have to copy anything
		
      return selectedChild->insertRoom(event);
    }
    else c = currentProperty->next();
  }
	
  // we reached the end of our string and can pass the event to the next node (or create it)
  selectedChild = children->get(c);
  if (selectedChild == 0) {
    if (c != 0) selectedChild = new SearchTreeNode(event);
    else selectedChild = new IntermediateNode(event);
    children->put(c, selectedChild);
  }
  return selectedChild->insertRoom(event);
}
			
/**
 * checking if another property needs to be skipped is done in the intermediate nodes
 */
void SearchTreeNode::skipDown(RoomOutStream & stream, ParseEvent * event) {
  SearchTreeNode * selectedChild = 0;
  ParseEvent * copy = 0;
  for (unsigned int i = 0; i < 256; i++) {
    if ((selectedChild = children->get(i)) != 0) {
      copy = new ParseEvent(*event);
      selectedChild->skipDown(stream, copy);
      delete copy;
      copy = 0;
    }
  }
}
		


