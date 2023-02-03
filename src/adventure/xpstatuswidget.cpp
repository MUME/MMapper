#include "xpstatuswidget.h"
#include "adventuresession.h"
#include "adventuretracker.h"
#include "adventurewidget.h"

XPStatusWidget::XPStatusWidget(AdventureTracker &at, QStatusBar &sb, QWidget *parent)
    : QPushButton(parent)
    , m_statusBar{sb}
    , m_tracker{at}
    , m_session{}
{
    setFlat(true);
    setMaximumHeight(22);
    setToolTip("Click to open the Adventure Journal");

    setMouseTracking(true);

    updateContent();

    connect(&m_tracker,
            &AdventureTracker::sig_updatedSession,
            this,
            &XPStatusWidget::slot_updatedSession);

    connect(&m_tracker,
            &AdventureTracker::sig_endedSession,
            this,
            &XPStatusWidget::slot_updatedSession);
}

void XPStatusWidget::updateContent()
{
    if (m_session.has_value()) {
        double xpGained = m_session->xpCurrent() - m_session->xpInitial();
        auto s = AdventureWidget::formatXPGained(xpGained);
        setText(QString("%1 Session: %2 XP").arg(m_session->name()).arg(s));
        show();
        repaint();
    } else {
        setText("");
        hide();
    }
}

void XPStatusWidget::enterEvent(QEvent *event)
{
    if (m_session.has_value()) {
        auto xpHourly = m_session->calculateHourlyRateXP();
        auto msg = QString("Hourly rate: %1 XP").arg(AdventureWidget::formatXPGained(xpHourly));
        m_statusBar.showMessage(msg);
    }

    QWidget::enterEvent(event);
}

void XPStatusWidget::leaveEvent(QEvent *event)
{
    m_statusBar.clearMessage();

    QWidget::leaveEvent(event);
}

void XPStatusWidget::slot_updatedSession(const AdventureSession &session)
{
    m_session = session;
    updateContent();
}
