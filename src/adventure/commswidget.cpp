
#include <QtCore>
#include <QtWidgets>

#include "commswidget.h"

CommsWidget::CommsWidget(QWidget *parent)
    : QWidget{parent}
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *textEdit = new QTextEdit(this);
    textEdit->setPlainText("comms will go here");

    layout->addWidget(textEdit);
}
