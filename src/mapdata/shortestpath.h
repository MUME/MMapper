#ifndef SHORTESTPATH_H
#define SHORTESTPATH_H

#include "abstractparser.h"
#include "mmapper2exit.h"
#include "roomadmin.h"
#include <QSet>

class SPNode
{
public:
    const Room *r;
    int parent;
    double dist;
    ExitDirection lastdir;
    SPNode() {}
    SPNode(const Room *r, const int parent, double dist, ExitDirection lastdir) : r(r), parent(parent),
        dist(dist), lastdir(lastdir) {}
};

class ShortestPathRecipient
{
public:
    virtual void receiveShortestPath(RoomAdmin *admin, QVector<SPNode> spnodes, int endpoint) = 0;
    virtual ~ShortestPathRecipient() {}
};


#endif
