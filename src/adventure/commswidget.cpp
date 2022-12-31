
#include <QtCore>
#include <QtWidgets>

#include "commswidget.h"

CommsWidget::CommsWidget(AdventureJournal &aj, QWidget *parent)
    : QWidget{parent},
      m_adventureJournal{aj}
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_commsTextEdit = new QTextEdit(this);
    m_commsTextEdit->setPlainText("comms will go here");

    layout->addWidget(m_commsTextEdit);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_receivedComm,
            this,
            &CommsWidget::slot_onCommReceived,
            Qt::QueuedConnection);
}

void CommsWidget::slot_onCommReceived(const QString &data)
{
    m_commsTextEdit->append(data);

}
