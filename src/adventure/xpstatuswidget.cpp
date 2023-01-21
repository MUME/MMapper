#include "xpstatuswidget.h"
#include "adventuretracker.h"
#include "adventurewidget.h"

XPStatusWidget::XPStatusWidget(AdventureTracker &at, QWidget *parent)
    : QPushButton(parent)
    , m_adventureTracker{at}
{
    setFlat(true);
    setMaximumHeight(22);
    setToolTip("Click to open the Adventure Journal");

    resetSession();
    update();

    connect(&m_adventureTracker,
            &AdventureTracker::sig_endedSession,
            this,
            &XPStatusWidget::slot_endedSession);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_updatedXP,
            this,
            &XPStatusWidget::slot_updatedXP);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_updatedChar,
            this,
            &XPStatusWidget::slot_updatedChar);
}

void XPStatusWidget::resetSession()
{
    m_charName = "";
    m_xpInitial = 0.0;
    m_xpCurrent = 0.0;
}

void XPStatusWidget::slot_endedSession(const QString charName)
{
    resetSession();
    // we do NOT call update() here to leave the last session info visible
}

void XPStatusWidget::slot_updatedChar(const QString charName)
{
    m_charName = charName;
    update();
}

void XPStatusWidget::slot_updatedXP(const double xpInitial, const double xpCurrent)
{
    m_xpInitial = xpInitial;
    m_xpCurrent = xpCurrent;
    update();
}

void XPStatusWidget::update()
{
    double xpGained = m_xpCurrent - m_xpInitial;

    if (xpGained > 0.0) {
        auto s = AdventureWidget::formatXPGained(xpGained);
        setText(QString("%1 Session: %2 XP").arg(m_charName).arg(s));
        show();
        repaint();

    } else {
        setText("");
        hide();
    }
}
