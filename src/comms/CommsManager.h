#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include <QString>
#include <QObject>
#include <QSet>
#include <QPair>

#include "../global/utils.h"

class GmcpMessage;

enum class NODISCARD CommType {
    TELL,
    WHISPER,
    GROUP,
    SAY,
    EMOTE,
    NARRATE,
    YELL,
    PRAY,
    SHOUT,
    SING,
    ASK,
    SOCIAL
};

enum class NODISCARD CommCategory {
    DIRECT,  // tells, whispers
    LOCAL,   // say, emote, social
    GLOBAL   // narrate, yell, pray, shout, sing, ask (questions)
};

enum class NODISCARD TalkerType {
    YOU,      // Messages sent by the player (talker: "you")
    PLAYER,   // Regular player (no talker-type specified)
    NPC,      // NPC (talker-type: "npc")
    ALLY,     // Ally (talker-type: "ally")
    NEUTRAL,  // Neutral (talker-type: "neutral")
    ENEMY     // Enemy (talker-type: "enemy")
};

struct NODISCARD CommMessage final
{
    CommType type = CommType::SAY;
    CommCategory category = CommCategory::LOCAL;
    QString sender;
    QString message;
    QString timestamp;
    TalkerType talkerType = TalkerType::PLAYER;
};

class CommsManager final : public QObject
{
    Q_OBJECT

public:
    explicit CommsManager(QObject *parent);
    ~CommsManager() override;

    DELETE_CTORS_AND_ASSIGN_OPS(CommsManager);

public slots:
    void slot_parseGmcpInput(const GmcpMessage &msg);
    void slot_parseRawGameText(const QString &rawText);

signals:
    void sig_newMessage(const CommMessage &msg);
    void sig_log(const QString &module, const QString &message);

private:
    void parseCommChannelText(const GmcpMessage &msg);
    void parseFallbackYell(const QString &rawText);
    CommType getCommTypeFromChannel(const QString &channel);
    CommCategory getCategoryFromType(CommType type);
    void trackYellMessage(const QString &sender, const QString &message);
    bool isRecentYellDuplicate(const QString &sender, const QString &message) const;

    // Track recent GMCP yells to avoid duplicates from fallback parsing
    // Format: "sender|message" -> timestamp (in msecs since epoch)
    QHash<QString, qint64> m_recentYells;
};
