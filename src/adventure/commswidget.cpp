
#include <QtCore>
#include <QtWidgets>

#include "commswidget.h"

CommsWidget::CommsWidget(AdventureJournal &aj, QWidget *parent)
    : m_adventureJournal{aj}
    , QWidget{parent}
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_commsTextEdit = new QTextEdit(this);
    m_commsTextEdit->setPlainText("Narrates and Tells will appear here...");

    layout->addWidget(m_commsTextEdit);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_receivedComm,
            this,
            &CommsWidget::slot_onCommReceived,
            Qt::QueuedConnection); // REVISIT does using QueuedConnection matter? any benefit?
}

void CommsWidget::slot_onCommReceived(const QString &data)
{
    //qDebug() << "CommsWidget::slot_onCommReceived" << data;
    m_commsTextEdit->append(data);
}
