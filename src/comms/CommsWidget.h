#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMap>
#include <QResizeEvent>

#include "../global/utils.h"
#include "CommsManager.h"

class AutoLogger;

enum class NODISCARD CharMobFilterEnum : uint8_t { BOTH, CHAR_ONLY, MOB_ONLY };

class CommsWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit CommsWidget(CommsManager &commsManager, AutoLogger *autoLogger, QWidget *parent);
    ~CommsWidget() override;

    DELETE_CTORS_AND_ASSIGN_OPS(CommsWidget);

public slots:
    void slot_onNewMessage(const CommMessage &msg);
    void slot_loadSettings();
    void slot_saveLog();
    void slot_saveLogOnExit();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void slot_onFilterToggled(CommType type, bool enabled);
    void slot_onCharMobToggle();

private:
    void setupUI();
    void connectSignals();
    void appendFormattedMessage(const CommMessage &msg);
    void formatAndInsertMessage(QTextCursor &cursor, const CommMessage &msg, const QString &sender, const QString &message);
    QString stripAnsiCodes(const QString &text);
    QString cleanSenderName(const QString &sender);
    QColor getColorForType(CommType type);
    QColor getColorForTalker(TalkerType talkerType);
    bool isMessageFiltered(const CommMessage &msg) const;
    void updateFilterButtonAppearance(QPushButton *button, bool enabled);
    void updateCharMobButtonAppearance();
    void rebuildDisplay();
    void updateButtonLabels();

    // Reference to data source
    CommsManager &m_commsManager;
    AutoLogger *m_autoLogger = nullptr;

    // UI components
    QTextEdit *m_textDisplay = nullptr;
    QToolButton *m_charMobToggle = nullptr;

    // Filter buttons by type
    QMap<CommType, QPushButton*> m_filterButtons;
    QMap<CommType, bool> m_filterStates; // true = show, false = muted/filtered

    CharMobFilterEnum m_charMobFilter = CharMobFilterEnum::BOTH;

    // Message cache for re-filtering
    struct CachedMessage {
        CommMessage msg;
    };
    QList<CachedMessage> m_messageCache;

    // Maximum messages to keep in cache
    static constexpr int MAX_MESSAGES = 1024;
};
