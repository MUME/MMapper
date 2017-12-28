#ifndef ROOMFILTER_H
#define ROOMFILTER_H

#include "roomadmin.h"
#include "component.h"

#include "intermediatenode.h"
#include "map.h"

enum pattern_kinds {PAT_UNK, PAT_DESC, PAT_NAME, PAT_NOTE, PAT_EXITS, PAT_ALL};

class RoomFilter
{
public:
    RoomFilter() {};
    RoomFilter(const QString &pattern, const Qt::CaseSensitivity &cs,
               const char kind) : pattern(pattern), cs(cs), kind(kind) { }

    // Return false on parse failure
    static bool parseRoomFilter(const QString &line, RoomFilter &output);
    static const char *parse_help;

    const bool filter(const Room *r) const;

protected:
    QString pattern;
    Qt::CaseSensitivity cs;
    char kind;
};

#endif
