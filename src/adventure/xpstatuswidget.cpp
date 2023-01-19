#include "xpstatuswidget.h"
#include "QtCore/qdebug.h"

XPStatusWidget::XPStatusWidget(AdventureTracker &at)
    : m_adventureTracker{at}
{
    connect(&m_adventureTracker,
            &AdventureTracker::sig_updatedXP,
            this,
            &XPStatusWidget::slot_updatedXP);
}

void XPStatusWidget::slot_updatedXP(const double currentXP)
{
    qDebug() << "XPStatusWidget::slot_updatedXP called";

    setText(QString("Gained XP: %1").arg(currentXP));
    repaint();
}
