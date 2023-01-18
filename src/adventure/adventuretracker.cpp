#include <QJsonDocument>
#include <QJsonObject>

#include "adventuretracker.h"
#include "parser/parserutils.h"

#include <QDebug>

AdventureTracker::AdventureTracker(GameObserver &observer, QObject *const parent)
    : QObject{parent}
    , m_observer{observer}
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

    if (m_killParser.parse(line)) {
        auto killName = m_killParser.getLastSuccessVal();
        double xpGained = checkpointXP();
        emit sig_killedMob(killName, xpGained);
        return;
    }

    if (m_achievementParser.parse(line)) {
        auto achievement = m_achievementParser.getLastSuccessVal();
        double xpGained = checkpointXP();
        emit sig_achievedSomething(achievement, xpGained);
        return;
    }

    if (m_hintParser.parse(line)) {
        auto hint = m_hintParser.getLastSuccessVal();
        emit sig_receivedHint(hint);
        return;
    }

    if (m_gainedLevelParser.parse(line)) {
        emit sig_gainedLevel();
        return;
    }
}

void AdventureTracker::slot_onUserGmcp(const GmcpMessage &msg)
{
    // https://mume.org/help/generic_mud_communication_protocol

    if (!(msg.isCharName() or msg.isCharStatusVars() or msg.isCharVitals()
          or msg.isCommChannelText()))
        return;

    auto s = msg.getJson()->toQString().toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(s);

    if (!doc.isObject()) {
        qInfo() << "Received GMCP: " << msg.getName().toQString()
                << "containing invalid Json: expecting object, got: " << s;
        return;
    }

    parseIfUpdatedChar(doc);

    parseIfUpdatedXP(doc);

    parseIfReceivedComm(msg, doc);
}

void AdventureTracker::parseIfReceivedComm(GmcpMessage msg, QJsonDocument doc)
{
    QJsonObject obj = doc.object();

    if (!msg.isCommChannelText() or !obj.contains("channel") or !obj.contains("text"))
        return;

    if (obj["channel"].toString() == "tells") {
        emit sig_receivedTell(obj["text"].toString());

    } else if (obj["channel"].toString() == "tales") {
        emit sig_receivedNarrate(obj["text"].toString());
    }
}

void AdventureTracker::parseIfUpdatedChar(QJsonDocument doc)
{
    QJsonObject obj = doc.object();

    if (obj.contains("name")) {
        updateCharfromMud(obj["name"].toString());
    }
}

void AdventureTracker::parseIfUpdatedXP(QJsonDocument doc)
{
    QJsonObject obj = doc.object();

    if (obj.contains("xp")) {
        updateXPfromMud(obj["xp"].toDouble());
    }
}

void AdventureTracker::updateCharfromMud(QString charName)
{
    if (m_currentCharName.isEmpty()) {
        qDebug().noquote() << QString("Adventure: new session for char %1").arg(charName);
        m_currentCharName = charName;
        return;
    }

    if (m_currentCharName == charName) {
        // nothing to do here, same character
        return;
    }

    // So a new character has logged in, need to wipe the old state
    qDebug().noquote() << QString("Adventure: char change, %1 replacing %2")
                              .arg(charName)
                              .arg(m_currentCharName);

    m_currentCharName = charName;
    m_xpInitial.reset();
    m_xpCheckpoint.reset();
    m_xpCurrent.reset();
}

void AdventureTracker::updateXPfromMud(double currentXP)
{
    if (!m_xpInitial.has_value()) {
        qDebug().noquote() << QString("Adventure: initial XP: %1")
                                  .arg(QString::number(currentXP, 'f', 0));
        m_xpInitial = currentXP;
    }

    if (!m_xpCheckpoint.has_value()) {
        m_xpCheckpoint = currentXP;
    }

    m_xpCurrent = currentXP;
    emit sig_updatedXP(m_xpCurrent.value());
}

double AdventureTracker::checkpointXP()
{
    if (!m_xpCurrent.has_value() or !m_xpCheckpoint.has_value()) {
        qDebug().noquote() << "Adventure: attempting to checkpointXP() without valid state.";
        return 0;
    }

    double xpGained = m_xpCurrent.value() - m_xpCheckpoint.value();
    m_xpCheckpoint.emplace(m_xpCurrent.value());

    return xpGained;
}
