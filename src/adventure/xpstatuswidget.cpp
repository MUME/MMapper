#include "xpstatuswidget.h"
#include "adventuretracker.h"
#include "adventurewidget.h"

XPStatusWidget::XPStatusWidget(AdventureTracker &at, QStatusBar *sb, QWidget *parent)
    : QPushButton(parent)
    , m_statusBar{sb}
    , m_adventureTracker{at}
{
    setFlat(true);
    setMaximumHeight(22);
    setToolTip("Click to open the Adventure Journal");

    setMouseTracking(true);

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
    m_startTimePoint = std::chrono::steady_clock::now();
    m_xpInitial = 0.0;
    m_xpCurrent = 0.0;
}

void XPStatusWidget::update()
{
    if (m_charName.isEmpty()) {
        // newly initialized state
        setText("");
        hide();
        return;
    }

    double xpGained = m_xpCurrent - m_xpInitial;
    auto s = AdventureWidget::formatXPGained(xpGained);
    setText(QString("%1 Session: %2 XP").arg(m_charName).arg(s));
    show();
    repaint();
}

void XPStatusWidget::enterEvent(QEvent *event)
{
    if (m_statusBar != nullptr) {
        auto xpHourly = calculateXPHourlyRate();
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

void XPStatusWidget::slot_endedSession(const QString charName)
{
    resetSession();
    // We do NOT call update() after resetSession() here, so that info stays
    // visible for player to check later. Will be update()'d when new session
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

double XPStatusWidget::calculateXPHourlyRate()
{
    auto n = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(n - m_startTimePoint);

    double xpAllSessionPerSecond = (m_xpCurrent - m_xpInitial)
                                   / static_cast<double>(elapsed.count());

    return xpAllSessionPerSecond * 3600;
}
