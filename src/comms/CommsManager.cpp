// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors
// Author: originally MuMeM for Mudlet Client (adapted for mMapper: Shimrod with Claude)

#include "CommsManager.h"

#include "../configuration/configuration.h"
#include "../global/AnsiTextUtils.h"
#include "../proxy/GmcpMessage.h"

#include <QDateTime>
#include <QRegularExpression>

CommsManager::CommsManager(QObject *parent)
    : QObject(parent)
{}

CommsManager::~CommsManager() = default;

void CommsManager::slot_parseGmcpInput(const GmcpMessage &msg)
{
    if (msg.isCommChannelText()) {
        parseCommChannelText(msg);
    }
}

void CommsManager::parseCommChannelText(const GmcpMessage &msg)
{
    const auto optDoc = msg.getJsonDocument();
    if (!optDoc.has_value()) {
        return;
    }

    const auto optObj = optDoc->getObject();
    if (!optObj.has_value()) {
        return;
    }

    const auto &obj = optObj.value();

    // Extract fields from GMCP message
    // Structure: { "channel": "tells", "talker": "Name", "talker-type": "npc", "text": "..." }
    const auto channelOpt = obj.getString("channel");
    const auto talkerOpt = obj.getString("talker");
    const auto textOpt = obj.getString("text");
    const auto talkerTypeOpt = obj.getString("talker-type");

    if (!channelOpt.has_value() || !talkerOpt.has_value() || !textOpt.has_value()) {
        return;
    }

    const QString channel = channelOpt.value();
    const QString talker = talkerOpt.value();
    const QString text = textOpt.value();

    // Determine talker type based on talker name and talker-type field
    TalkerType talkerType = TalkerType::PLAYER; // Default

    if (talker == "you") {
        talkerType = TalkerType::YOU;
    } else if (talkerTypeOpt.has_value()) {
        const QString talkerTypeStr = talkerTypeOpt.value();
        if (talkerTypeStr == "npc") {
            talkerType = TalkerType::NPC;
        } else if (talkerTypeStr == "ally") {
            talkerType = TalkerType::ALLY;
        } else if (talkerTypeStr == "neutral") {
            talkerType = TalkerType::NEUTRAL;
        } else if (talkerTypeStr == "enemy") {
            talkerType = TalkerType::ENEMY;
        }
    }

    // Note: Text may contain ANSI codes, but we'll display it as-is for now
    // ANSI stripping can be added later if needed

    // Map channel name to CommType
    CommType type = getCommTypeFromChannel(channel);
    CommCategory category = getCategoryFromType(type);

    // Track yells from GMCP to avoid fallback duplicates
    if (type == CommType::YELL) {
        trackYellMessage(talker, text);
    }

    // Create and emit the message
    CommMessage commMsg;
    commMsg.type = type;
    commMsg.category = category;
    commMsg.sender = talker;
    commMsg.message = text;
    commMsg.timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    commMsg.talkerType = talkerType;

    emit sig_newMessage(commMsg);
}

CommType CommsManager::getCommTypeFromChannel(const QString &channel)
{
    // Map GMCP channel names to CommType (support both singular and plural forms)
    if (channel == "tells" || channel == "tell") {
        return CommType::TELL;
    } else if (channel == "whispers" || channel == "whisper") {
        return CommType::WHISPER;
    } else if (channel == "groups" || channel == "group") {
        return CommType::GROUP;
    } else if (channel == "says" || channel == "say") {
        return CommType::SAY;
    } else if (channel == "emotes" || channel == "emote") {
        return CommType::EMOTE;
    } else if (channel == "tales" || channel == "narrates" || channel == "narrate") {
        return CommType::NARRATE;
    } else if (channel == "yells" || channel == "yell") {
        return CommType::YELL;
    } else if (channel == "prayers" || channel == "prayer" || channel == "pray") {
        return CommType::PRAY;
    } else if (channel == "shouts" || channel == "shout") {
        return CommType::SHOUT;
    } else if (channel == "songs" || channel == "song" || channel == "sing") {
        return CommType::SING;
    } else if (channel == "questions" || channel == "question" || channel == "ask") {
        return CommType::ASK;
    } else if (channel == "socials" || channel == "social") {
        return CommType::SOCIAL;
    }

    // Default to SAY for unknown channels
    return CommType::SAY;
}

CommCategory CommsManager::getCategoryFromType(CommType type)
{
    switch (type) {
    case CommType::TELL:
    case CommType::WHISPER:
    case CommType::GROUP:
        return CommCategory::DIRECT;

    case CommType::SAY:
    case CommType::EMOTE:
    case CommType::SOCIAL:
        return CommCategory::LOCAL;

    case CommType::NARRATE:
    case CommType::YELL:
    case CommType::PRAY:
    case CommType::SHOUT:
    case CommType::SING:
    case CommType::ASK:
        return CommCategory::GLOBAL;

    default:
        return CommCategory::LOCAL;
    }
}

void CommsManager::slot_parseRawGameText(const QString &rawText)
{
    // Check if fallback parsing is enabled
    if (!getConfig().parser.enableYellFallbackParsing) {
        return;
    }

    parseFallbackYell(rawText);
}

void CommsManager::parseFallbackYell(const QString &rawText)
{
    // Pattern to match yell messages:
    // "Name yells [from direction] 'message'"
    // Examples:
    //   Círdan the Shipwright yells from below 'Come here if you want to speak with me!'
    //   A thief yells 'HELP! *Shimrod the Elf* is trying to kill me in the Robbers Haven!'
    //   You yell 'Hello!'

    // Strip ANSI codes from the text before pattern matching
    QString cleanText = rawText.trimmed();

    // Remove ANSI codes using regex
    static const QRegularExpression ansiPattern(R"(\x1B\[[0-9;]*[a-zA-Z])");
    cleanText.remove(ansiPattern);

    // Pattern: Name yells [anything] 'message' [optional text after quote]
    // Captures everything between "yells" and the opening quote as the qualifier
    // Examples: "Name yells 'msg'", "Name yells loudly 'msg'",
    //          "Name yells faintly from below 'msg'", "Name yells loudly from far to the east 'msg'"
    static const QRegularExpression yellPattern(R"(^(.+?) yells?(?: (.+?))? '(.+?)')",
                                                QRegularExpression::CaseInsensitiveOption);

    auto match = yellPattern.match(cleanText);
    if (!match.hasMatch()) {
        return;
    }

    QString sender = match.captured(1).trimmed();
    QString qualifier = match.captured(2).trimmed(); // Everything between "yells" and "'" (optional)
    QString message = match.captured(3);

    // Check if this is a duplicate from GMCP (within last 2 seconds)
    if (isRecentYellDuplicate(sender, message)) {
        return; // Skip this fallback yell, already got it from GMCP
    }

    // Determine talker type based on sender name
    TalkerType talkerType = TalkerType::NPC; // Default to NPC

    if (sender.startsWith("You", Qt::CaseInsensitive)) {
        talkerType = TalkerType::YOU;
    } else if (!sender.startsWith("A ", Qt::CaseInsensitive)
               && !sender.startsWith("An ", Qt::CaseInsensitive)
               && !sender.startsWith("The ", Qt::CaseInsensitive) && !sender.contains(" the ")) {
        // If name doesn't start with article, likely a player
        talkerType = TalkerType::PLAYER;
    }

    // Add qualifier (direction/volume info) to message if present
    QString fullMessage = message;
    if (!qualifier.isEmpty()) {
        fullMessage = QString("[%1] %2").arg(qualifier, message);
    }

    // Create and emit the comm message
    CommMessage commMsg;
    commMsg.type = CommType::YELL;
    commMsg.category = CommCategory::GLOBAL;
    commMsg.sender = sender;
    commMsg.message = fullMessage;
    commMsg.timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    commMsg.talkerType = talkerType;

    // Track this yell to avoid future duplicates
    trackYellMessage(sender, message);

    emit sig_newMessage(commMsg);
}

void CommsManager::trackYellMessage(const QString &sender, const QString &message)
{
    // Create a unique key for this yell
    const QString key = QString("%1|%2").arg(sender, message);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    // Store the timestamp
    m_recentYells[key] = now;

    // Clean up old entries (older than 5 seconds)
    const qint64 cutoff = now - 5000;
    auto it = m_recentYells.begin();
    while (it != m_recentYells.end()) {
        if (it.value() < cutoff) {
            it = m_recentYells.erase(it);
        } else {
            ++it;
        }
    }
}

bool CommsManager::isRecentYellDuplicate(const QString &sender, const QString &message) const
{
    const QString key = QString("%1|%2").arg(sender, message);

    if (!m_recentYells.contains(key)) {
        return false;
    }

    // Check if it's recent (within last 2 seconds)
    const qint64 timestamp = m_recentYells[key];
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 age = now - timestamp;

    return age < 2000; // 2 second window
}
