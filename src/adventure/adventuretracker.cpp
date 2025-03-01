// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "adventuretracker.h"

#include "../global/JsonObj.h"
#include "../global/RAII.h"
#include "../proxy/GmcpMessage.h"

#include <memory>
#include <optional>

#include <QDebug>

AdventureTracker::AdventureTracker(GameObserver &observer, QObject *const parent)
    : QObject{parent}
    , m_observer{observer}
    , m_session{nullptr}
{
    m_observer.sig2_sentToUserString.connect(m_lifetime,
                                             [this](const QString &str) { onUserText(str); });

    m_observer.sig2_sentToUserGmcp.connect(m_lifetime,
                                           [this](const GmcpMessage &gmcp) { onUserGmcp(gmcp); });
}

void AdventureTracker::onUserText(const QString &line)
{
    // These are sorted by order of frequency, which could create a problem for stateful
    // parsers that miss out on state because another parser returned before the parser
    // was able to learn the current state.
    //
    // Currently the KillAndXPParser is the only stateful parser; the rest of the parsers
    // use a common previous line that's safely remembered by this RAII callback.
    const RAIICallback uponReturn{[this, &line]() { m_prevLine = line; }};

    if (auto tmp = m_killParser.parse(line)) {
        auto killName = tmp.value();
        double xpGained = checkpointXP();
        emit sig_killedMob(killName, xpGained);
        return;
    }

    if (GainedLevelParser::parse(line)) {
        emit sig_gainedLevel();
        return;
    }

    if (auto tmp = AchievementParser::parse(m_prevLine, line)) {
        auto achievement = tmp.value();
        auto xpGained = checkpointXP();
        emit sig_achievedSomething(achievement, xpGained);
        return;
    }

    if (AccomplishedTaskParser::parse(line)) {
        auto xpGained = checkpointXP();
        emit sig_accomplishedTask(xpGained);
        return;
    }

    if (DiedParser::parse(line)) {
        auto xpLost = checkpointXP();
        emit sig_diedInGame(xpLost);
        return;
    }

    if (auto tmp = HintParser::parse(m_prevLine, line)) {
        auto hint = tmp.value();
        emit sig_receivedHint(hint);
        return;
    }
}

void AdventureTracker::onUserGmcp(const GmcpMessage &msg)
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
    if (!msg.getJsonDocument().has_value()) {
        return;
    }

    auto pObj = msg.getJsonDocument()->getObject();
    if (!pObj) {
        return;
    }
    const auto &obj = *pObj;

    auto optCharName = obj.getString("name");
    if (!optCharName) {
        return;
    }
    auto charName = optCharName.value();

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

    if (!msg.getJsonDocument().has_value()) {
        return;
    }

    auto pObj = msg.getJsonDocument()->getObject();
    if (!pObj) {
        return;
    }
    const auto &obj = *pObj;

    bool updated = false;
    if (auto optXp = obj.getDouble("xp")) {
        m_session->updateXP(optXp.value());
        updated = true;
    }

    if (auto optTp = obj.getDouble("tp")) {
        m_session->updateTP(optTp.value());
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
