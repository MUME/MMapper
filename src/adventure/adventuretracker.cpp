#include <QJsonDocument>
#include <QJsonObject>

#include "adventuretracker.h"
#include "parser/parserutils.h"

#include <QDebug>

AdventureTracker::AdventureTracker(GameObserver &observer, QObject *const parent)
    : QObject{parent}
    , m_observer{observer}
    , m_session{nullptr}
{
    connect(&m_observer,
            &GameObserver::sig_sentToUserText,
            this,
            &AdventureTracker::slot_onUserText);

    connect(&m_observer,
            &GameObserver::sig_sentToUserGmcp,
            this,
            &AdventureTracker::slot_onUserGmcp);
}

AdventureTracker::~AdventureTracker() {}

void AdventureTracker::slot_onUserText(const QByteArray &ba)
{
    QString line = QString::fromLatin1(ba).trimmed();
    ParserUtils::removeAnsiMarksInPlace(line);

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
        emit sig_died(xpLost);
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
        parseIfUpdatedChar(msg);
    }

    if (msg.isCharStatusVars()) {
        parseIfUpdatedChar(msg);
    }

    if (msg.isCharVitals()) {
        parseIfUpdatedXP(msg);
    }

    if (msg.isCommChannelText()) {
        parseIfReceivedComm(msg);
    }

    if (msg.isCoreGoodbye()) {
        parseIfGoodbye(msg);
    }
}

void AdventureTracker::parseIfGoodbye([[maybe_unused]] GmcpMessage msg)
{
    // REVISIT If you try to call msg.getJson()->toQString().toUtf8() on a
    // CoreGoodbye message, the GmcpMessage code crashes, trying to malloc a
    // huge 0xffffffff chunk of memory. Needs investigation.

    if (m_session != nullptr) {
        qDebug().noquote() << QString("Adventure: ending session for %1").arg(m_session->name());
        m_session->endSession();
        emit sig_endedSession(*m_session);
        m_session.reset();
    }
}

void AdventureTracker::parseIfReceivedComm(GmcpMessage msg)
{
    auto s = msg.getJson()->toQString().toUtf8();
    QJsonObject obj = QJsonDocument::fromJson(s).object();

    if (!(obj.contains("channel") and obj.contains("text")))
        return;

    if (obj["channel"].toString() == "tells") {
        emit sig_receivedTell(obj["text"].toString());

    } else if (obj["channel"].toString() == "tales") {
        emit sig_receivedNarrate(obj["text"].toString());
    }
}

void AdventureTracker::parseIfUpdatedChar(GmcpMessage msg)
{
    auto s = msg.getJson()->toQString().toUtf8();
    QJsonObject obj = QJsonDocument::fromJson(s).object();

    if (!obj.contains("name"))
        return;

    auto charName = obj["name"].toString();

    if (m_session == nullptr) {
        qDebug().noquote() << QString("Adventure: new adventure for %1").arg(charName);

        m_session = std::make_unique<AdventureSession>(charName);
        emit sig_updatedSession(*m_session);
        return;
    }

    if (m_session != nullptr and m_session->name() != charName) {
        qDebug().noquote() << QString("Adventure: new adventure for %1 replacing %2")
                                  .arg(charName)
                                  .arg(m_session->name());

        m_session->endSession();
        emit sig_endedSession(*m_session);
        m_session = std::make_unique<AdventureSession>(charName);
        emit sig_updatedSession(*m_session);
    }
}

void AdventureTracker::parseIfUpdatedXP(GmcpMessage msg)
{
    auto s = msg.getJson()->toQString().toUtf8();
    QJsonObject obj = QJsonDocument::fromJson(s).object();

    if (!obj.contains("xp"))
        return;

    auto xpCurrent = obj["xp"].toDouble();

    if (m_session != nullptr) {
        m_session->updateXP(xpCurrent);
        emit sig_updatedSession(*m_session);
    }
}

double AdventureTracker::checkpointXP()
{
    if (m_session != nullptr) {
        return m_session->checkpointXPGained();
    } else {
        qDebug().noquote() << "Adventure: attempting to checkpointXP() without valid session.";
        return 0;
    }
}
