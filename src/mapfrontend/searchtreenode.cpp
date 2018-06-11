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

#include "searchtreenode.h"
#include "intermediatenode.h"
#include "parseevent.h"
#include "property.h"

#include <cstring>
#include <string>

SearchTreeNode::byte_array SearchTreeNode::from_string(const char *s)
{
    if (s == nullptr) {
        return byte_array{};
    }
    const auto beg = reinterpret_cast<const uint8_t *>(s);
    return byte_array{beg, beg + std::strlen(s)};
}

SearchTreeNode::byte_array SearchTreeNode::skip(const SearchTreeNode::byte_array &input,
                                                const size_t count)
{
    const auto avail = input.size();
    if (count >= avail) {
        return SearchTreeNode::byte_array{};
    }
    return SearchTreeNode::byte_array{input.begin() + count, input.end()};
}

SearchTreeNode::SearchTreeNode(ParseEvent &event)
{
    if (auto curr = event.current()) {
        if (const char *rest = curr->rest()) {
            // Property::rest() can return "", but "" + 1 is garbage.
            // A crash is better than data corruption.
            assert(rest[0] != '\0');
            // we copy the string so that we can remove rooms independently of tree nodes
            // note: original code does not explain why it skips the first character
            myChars = from_string(rest + 1);
        }
    }
}

SearchTreeNode::SearchTreeNode(byte_array in_bytes, TinyList in_children)
    : children{std::move(in_children)}
    , myChars{std::move(in_bytes)}
{}

SearchTreeNode::SearchTreeNode() = default;

SearchTreeNode::~SearchTreeNode() = default;

void SearchTreeNode::getRooms(RoomOutStream &stream, ParseEvent &event)
{
    SearchTreeNode *selectedChild = nullptr;
    Property *currentProperty = event.current();

    const auto size = static_cast<int>(myChars.size());
    for (auto i = 0; i < size && myChars[i] != '\0'; i++) {
        if (currentProperty->next() != myChars[i]) {
            for (; i > 0; i--) {
                currentProperty->prev();
            }
            return;
        }
    }
    selectedChild = children.get(currentProperty->next());

    if (selectedChild == nullptr) {
        // This test makes zero sense. I think someone meant to copy/paste
        // the above code and lost some characters on accident, but I'm
        // keeping the original behavior because I don't understand it.
        //
        // In the initial revision:
        // above: for (int i = 0; myChars[i] != 0; i++) {
        // here:  for (int i = 1; i < myChars[i] != 0; i++) ...
        //
        // I suspect they intended to type something like:
        // - for (int i = 1; i < myChars[i] != 0; i++) ...
        // + for (int i = 1; i < len && myChars[i] != 0; i++) ...
        //
        // But even still, I'm not convinced that the rewindings here
        // and above don't contain off-by-at-least-one errors.
        for (int i = 1; i < size && i < static_cast<int>(myChars[i]); i++) {
            currentProperty->prev();
        }
        return; // no such room
    }
    selectedChild->getRooms(stream, event); // the last character of name is 0,
    // at position 0 there is a roomCollection, if we have rooms here
    // else there is 0, so name[depth] should work.
}

void SearchTreeNode::setChild(char position, SearchTreeNode *node)
{
    children.put(position, node);
}

RoomCollection *SearchTreeNode::insertRoom(ParseEvent &event)
{
    SearchTreeNode *selectedChild = nullptr;
    Property *currentProperty = event.current();
    char c = currentProperty->next();

    for (size_t i = 0u; i < myChars.size() && myChars[i] != '\0'; i++, c = currentProperty->next()) {
        if (c != myChars[i]) {
            // we have to split, as we encountered a difference in the strings ...
            // first build the lower part of this node
            selectedChild = new SearchTreeNode(skip(myChars, i + 1u),
                                               std::exchange(children, TinyList{}));

            // and update the upper part of this node
            children.put(myChars[i], selectedChild);

            if (c == '\0') {
                selectedChild = new IntermediateNode(event); // then build the branch
            } else {
                selectedChild = new SearchTreeNode(event);
            }

            children.put(c, selectedChild); // and update again

            // the string is separated in [myChars][0][selectedChildChars][0] so we don't have to copy anything
            //
            // NOTE: it's unclear if this requires us to retain data after a null-character,
            // (e.g. "foo\0bar"), so we use a byte_array instead of a std::string to be safe.
            myChars[i] = '\0';

            return selectedChild->insertRoom(event);
        }
    }

    // we reached the end of our string and can pass the event to the next node (or create it)
    selectedChild = children.get(c);
    if (selectedChild == nullptr) {
        if (c != 0) {
            selectedChild = new SearchTreeNode(event);
        } else {
            selectedChild = new IntermediateNode(event);
        }
        children.put(c, selectedChild);
    }
    return selectedChild->insertRoom(event);
}

/**
 * checking if another property needs to be skipped is done in the intermediate nodes
 */
void SearchTreeNode::skipDown(RoomOutStream &stream, ParseEvent &event)
{
    for (auto selectedChild : children) {
        if (selectedChild != nullptr) {
            // caution: this will slice a derived class; it would be better to use
            // "virtual std::unique_ptr<ParseEvent> clone() const;"
            ParseEvent copy(event);
            selectedChild->skipDown(stream, copy);
        }
    }
}
