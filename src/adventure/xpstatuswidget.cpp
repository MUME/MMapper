#include "xpstatuswidget.h"
#include "adventuretracker.h"
#include "adventurewidget.h"

XPStatusWidget::XPStatusWidget(AdventureTracker &at)
    : m_adventureTracker{at}
{
    connect(&m_adventureTracker,
            &AdventureTracker::sig_updatedXP,
            this,
            &XPStatusWidget::slot_updatedXP);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_updatedChar,
            this,
            &XPStatusWidget::slot_updatedChar);
}

void XPStatusWidget::slot_updatedChar(const QString charName)
{
    m_charName = charName;
    update();
}

void XPStatusWidget::slot_updatedXP(const double initialXP, const double currentXP)
{
    m_xpSessionSummary = AdventureWidget::formatXPGained(currentXP - initialXP);
    update();
}

void XPStatusWidget::update()
{
    setText(QString("%1 Session: %2 XP").arg(m_charName).arg(m_xpSessionSummary));
    repaint();
}
