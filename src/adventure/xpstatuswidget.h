#pragma once
#include "adventure/adventuretracker.h"
#include <QLabel>

class XPStatusWidget : public QLabel
{
    Q_OBJECT
public:
    XPStatusWidget(AdventureTracker &at);

public slots:
    void slot_updatedChar(const QString charName);
    void slot_updatedXP(const double initialXP, const double currentXP);

private:
    void update();

    AdventureTracker &m_adventureTracker;

    QString m_charName, m_xpSessionSummary;
};
