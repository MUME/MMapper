#pragma once

#include "adventure/adventurejournal.h"
#include <QtWidgets>
#include <QWidget>

class GameConsoleWidget : public QWidget
{
    Q_OBJECT

public:
    static const constexpr int MAX_LINES = 512;
    static const constexpr auto DEFAULT_CONTENT = "Narrates and Tells will appear here...";

    explicit GameConsoleWidget(AdventureJournal &aj, QWidget *parent = nullptr);

public slots:
    void slot_onNarrateReceived(const QString &data);
    void slot_onTellReceived(const QString &data);

private:
    void processMessage(const QString &data);

    AdventureJournal &m_adventureJournal;

    QTextDocument *m_commsTextDocument = nullptr;
    QTextCursor *m_commsCursor = nullptr;
    QTextEdit *m_commsTextEdit = nullptr;

    int m_numMessagesReceived = 0;
};
