#pragma once
#include "adventure/adventuretracker.h"
#include <QLabel>

class XPStatusWidget : public QLabel
{
    Q_OBJECT
public:
    XPStatusWidget(AdventureTracker &at);

public slots:
    void slot_updatedXP(const double currentXP);

private:
    AdventureTracker &m_adventureTracker;
};
