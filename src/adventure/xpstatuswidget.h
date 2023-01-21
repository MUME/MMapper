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
    void slot_updatedChar(const QString charName);
    void slot_updatedXP(const double initialXP, const double currentXP);

private:
    void update();

    AdventureTracker &m_adventureTracker;

    QString m_charName = "";
    double m_xpInitial = 0.0, m_xpCurrent = 0.0;
};
