#include "xpstatuswidget.h"
#include "adventuresession.h"
#include "adventuretracker.h"
#include "adventurewidget.h"

XPStatusWidget::XPStatusWidget(AdventureTracker &at, QStatusBar *sb, QWidget *parent)
    : QPushButton(parent)
    , m_statusBar{sb}
    , m_tracker{at}
    , m_session{}
{
    setFlat(true);
    setStyleSheet("QPushButton { border: none; outline: none; }");

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
        auto xpSession = m_session->xp().gainedSession();
        auto tpSession = m_session->tp().gainedSession();
        auto xpf = AdventureWidget::formatPointsGained(xpSession);
        auto tpf = AdventureWidget::formatPointsGained(tpSession);
        auto msg = QString("%1 Session: %2 XP %3 TP").arg(m_session->name()).arg(xpf).arg(tpf);
        setText(msg);
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
        auto tpHourly = m_session->calculateHourlyRateTP();
        auto xpf = AdventureWidget::formatPointsGained(xpHourly);
        auto tpf = AdventureWidget::formatPointsGained(tpHourly);
        auto msg = QString("Hourly rate: %1 XP %2 TP").arg(xpf).arg(tpf);
        m_statusBar->showMessage(msg);
    }

    QWidget::enterEvent(event);
}

void XPStatusWidget::leaveEvent(QEvent *event)
{
    m_statusBar->clearMessage();

    QWidget::leaveEvent(event);
}

void XPStatusWidget::slot_updatedSession(const AdventureSession &session)
{
    m_session = session;
    updateContent();
}
