#include "roomfilter.h"
#include "mmapper2exit.h"
#include "mmapper2room.h"
#include <cassert>
#include <cerrno>

const char *RoomFilter::parse_help =
    "Parse error; format is: [-(name|desc|dyndesc|note|exits|all)] pattern\r\n";


bool RoomFilter::parseRoomFilter(const QString &line, RoomFilter &output)
{
    QString pattern = line;
    char kind = PAT_NAME;
    if (line.size() >= 2 && line[0] == '-') {
        QString kindstr = line.section(" ", 0, 0);
        if (kindstr == "-desc" || kindstr.startsWith("-d") ) {
            kind = PAT_DESC;
        } else if (kindstr == "-dyndesc" || kindstr.startsWith("-dy") ) {
            kind = PAT_DYNDESC;
        } else if (kindstr == "-name") {
            kind = PAT_NAME;
        } else if (kindstr == "-exits" || kindstr == "-e") {
            kind = PAT_EXITS;
        } else if (kindstr == "-note" || kindstr == "-n") {
            kind = PAT_NOTE;
        } else if (kindstr == "-all" || kindstr == "-a") {
            kind = PAT_ALL;
        } else {
            return false;
        }

        pattern = line.section(" ", 1);
        if (pattern.isEmpty()) {
            return false;
        }
    }
    output = RoomFilter(pattern, Qt::CaseInsensitive, kind);
    return true;
}

bool RoomFilter::filter(const Room *r) const
{
    if (kind == PAT_ALL) {
        for (const auto &elt : (*r)) {
            if (elt.toString().contains(pattern, cs)) {
                return 1;
            }
        }
    } else if (kind == PAT_DESC) {
        return Mmapper2Room::getDescription(r).contains(pattern, cs);
    } else if (kind == PAT_DYNDESC) {
        return Mmapper2Room::getDynamicDescription(r).contains(pattern, cs);
    } else if (kind == PAT_NAME) {
        return Mmapper2Room::getName(r).contains(pattern, cs);
    } else if (kind == PAT_NOTE) {
        return Mmapper2Room::getNote(r).contains(pattern, cs);
    } else if (kind == PAT_EXITS) {
        ExitsList exits = r->getExitsList();
        for (const auto &e : exits) {
            if (Mmapper2Exit::getDoorName(e).contains(pattern, cs)) {
                return 1;
            }
        }
    }
    return 0;
}
