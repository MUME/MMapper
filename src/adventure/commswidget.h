#pragma once

#include "adventure/adventurejournal.h"
#include <QtWidgets>
#include <QWidget>

class CommsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CommsWidget(AdventureJournal &aj, QWidget *parent = nullptr);

public slots:
    void slot_onCommReceived(const QString &data);

private:
    AdventureJournal &m_adventureJournal;
    QTextEdit *m_commsTextEdit = nullptr;
};
