// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "adventuretracker.h"

#include "../proxy/GmcpMessage.h"

#include <memory>
#include <optional>

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

AdventureTracker::AdventureTracker(GameObserver &observer, QObject *const parent)
    : QObject{parent}
    , m_observer{observer}
    , m_session{nullptr}
{
    connect(&m_observer,
            &GameObserver::sig_sentToUserString,
            this,
            &AdventureTracker::slot_onUserText);

    connect(&m_observer,
            &GameObserver::sig_sentToUserGmcp,
            this,
            &AdventureTracker::slot_onUserGmcp);
}

void AdventureTracker::slot_onUserText(const QString &line)
{
    // Try to order these by frequency to minimize unnecessary parsing

    if (m_killParser.parse(line)) {
        auto killName = m_killParser.getLastSuccessVal();
        double xpGained = checkpointXP();
        emit sig_killedMob(killName, xpGained);
        return;
    }

    if (m_gainedLevelParser.parse(line)) {
        emit sig_gainedLevel();
        return;
    }

    if (m_achievementParser.parse(line)) {
        auto achievement = m_achievementParser.getLastSuccessVal();
        auto xpGained = checkpointXP();
        emit sig_achievedSomething(achievement, xpGained);
        return;
    }

    if (m_accomplishedTaskParser.parse(line)) {
        auto xpGained = checkpointXP();
        emit sig_accomplishedTask(xpGained);
        return;
    }

    if (m_diedParser.parse(line)) {
        auto xpLost = checkpointXP();
        emit sig_diedInGame(xpLost);
        return;
    }

    if (m_hintParser.parse(line)) {
        auto hint = m_hintParser.getLastSuccessVal();
        emit sig_receivedHint(hint);
        return;
    }
}

void AdventureTracker::slot_onUserGmcp(const GmcpMessage &msg)
{
    // https://mume.org/help/generic_mud_communication_protocol

    if (msg.isCharName()) {
        parseIfUpdatedCharName(msg);
        return;
    }

    if (msg.isCharStatusVars()) {
        parseIfUpdatedCharName(msg);
        return;
    }

    if (msg.isCharVitals()) {
        parseIfUpdatedVitals(msg);
        return;
    }

    if (msg.isCoreGoodbye()) {
        parseIfGoodbye(msg);
        return;
    }
}

void AdventureTracker::parseIfGoodbye([[maybe_unused]] const GmcpMessage &msg)
{
    if (!m_session) {
        return;
    }

    qDebug().noquote() << QString("Adventure: ending session for %1").arg(m_session->name());
    m_session->endSession();
    emit sig_endedSession(m_session);
    m_session.reset();
}

void AdventureTracker::parseIfUpdatedCharName(const GmcpMessage &msg)
{
    std::optional<QJsonDocument> doc = msg.getJsonDocument();
    if (!doc || !doc->isObject()) {
        return;
    }

    QJsonObject obj = doc->object();
    if (!obj.contains("name")) {
        return;
    }

    auto charName = obj["name"].toString();

    if (m_session == nullptr) {
        qDebug().noquote() << QString("Adventure: new adventure for %1").arg(charName);

        m_session = std::make_shared<AdventureSession>(charName);
        emit sig_updatedSession(m_session);
        return;
    }

    if (m_session != nullptr && m_session->name() != charName) {
        qDebug().noquote() << QString("Adventure: new adventure for %1 replacing %2")
                                  .arg(charName, m_session->name());

        m_session->endSession();
        emit sig_endedSession(m_session);
        m_session = std::make_shared<AdventureSession>(charName);
        emit sig_updatedSession(m_session);
    }
}

void AdventureTracker::parseIfUpdatedVitals(const GmcpMessage &msg)
{
    if (m_session == nullptr) {
        qDebug().noquote() << "Adventure: can't update vitals without session.";
        return;
    }

    std::optional<QJsonDocument> doc = msg.getJsonDocument();
    if (!doc || !doc->isObject()) {
        return;
    }

    QJsonObject obj = doc->object();
    bool updated = false;

    if (obj.contains("xp")) {
        m_session->updateXP(obj["xp"].toDouble());
        updated = true;
    }

    if (obj.contains("tp")) {
        m_session->updateTP(obj["tp"].toDouble());
        updated = true;
    }

    if (updated) {
        emit sig_updatedSession(m_session);
    }
}

double AdventureTracker::checkpointXP()
{
    if (m_session == nullptr) {
        qDebug().noquote() << "Adventure: attempting to checkpointXP() without valid session.";
        return 0;
    }
    return m_session->checkpointXPGained();
}
