#pragma once

#include "adventure/adventuretracker.h"
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>

class XPStatusWidget : public QPushButton
{
    Q_OBJECT
public:
    XPStatusWidget(AdventureTracker &at, QStatusBar *sb = nullptr, QWidget *parent = nullptr);

public slots:
    void slot_endedSession(const QString charName);
    void slot_updatedChar(const QString charName);
    void slot_updatedXP(const double xpInitial, const double xpCurrent);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    double calculateXPHourlyRate();
    void resetSession();
    void update();

    QStatusBar *m_statusBar;

    AdventureTracker &m_adventureTracker;

    QString m_charName;
    std::chrono::steady_clock::time_point m_startTimePoint;
    double m_xpInitial, m_xpCurrent;
};
