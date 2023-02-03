#include "xpstatuswidget.h"
#include "adventuretracker.h"
#include "adventurewidget.h"

XPStatusWidget::XPStatusWidget(AdventureTracker &at, QStatusBar *sb, QWidget *parent)
    : QPushButton(parent)
    , m_statusBar{sb}
    , m_tracker{at}
    , m_session{}
{
    setFlat(true);
    setMaximumHeight(22);
    setToolTip("Click to open the Adventure Journal");

    setMouseTracking(true);

    update();

    connect(&m_tracker,
            &AdventureTracker::sig_updatedSession,
            this,
            &XPStatusWidget::slot_updatedSession);

    connect(&m_tracker,
            &AdventureTracker::sig_endedSession,
            this,
            &XPStatusWidget::slot_updatedSession);
}

void XPStatusWidget::update()
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
    if (m_statusBar != nullptr) {
        auto xpHourly = calculateHourlyRateXP();
        auto msg = QString("Hourly rate: %1 XP").arg(AdventureWidget::formatXPGained(xpHourly));
        m_statusBar->showMessage(msg);
    }
    QWidget::enterEvent(event);
}

void XPStatusWidget::leaveEvent(QEvent *event)
{
    if (m_statusBar != nullptr) {
        m_statusBar->clearMessage();
    }
    QWidget::leaveEvent(event);
}

void XPStatusWidget::slot_updatedSession(const AdventureSession &session)
{
    qDebug() << "adventuremem: function param " << &session;
    m_session = session;
    qDebug() << "adventuremem: member var " << &m_session;
    update();
}

double XPStatusWidget::calculateHourlyRateXP()
{
    if (m_session.has_value()) {
        auto start = m_session->startTime();
        auto end = std::chrono::steady_clock::now();
        if (m_session->isEnded())
            end = m_session->endTime();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);

        double xpAllSession = m_session->xpCurrent() - m_session->xpInitial();
        double xpAllSessionPerSecond = xpAllSession / static_cast<double>(elapsed.count());

        qDebug().noquote() << QString("seconds %1 xp %2").arg(elapsed.count()).arg(xpAllSession);

        return xpAllSessionPerSecond * 3600;

    } else {
        return 0.0;
    }
}
