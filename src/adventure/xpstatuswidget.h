#pragma once

#include "adventure/adventuretracker.h"
#include <QLabel>
#include <QPushButton>

class XPStatusWidget : public QPushButton
{
    Q_OBJECT
public:
    XPStatusWidget(AdventureTracker &at, QWidget *parent = nullptr);

public slots:
    void slot_endedSession(const QString charName);
    void slot_updatedChar(const QString charName);
    void slot_updatedXP(const double xpInitial, const double xpCurrent);

private:
    void resetSession();
    void update();

    AdventureTracker &m_adventureTracker;

    QString m_charName;
    double m_xpInitial, m_xpCurrent;
};
