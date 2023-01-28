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

    // Try to order these by frequency to minimize unneccessary parsing

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

    if (m_gainedLevelParser.parse(line)) {
        emit sig_gainedLevel();
        return;
    }

    if (m_diedParser.parse(line)) {
        double xpLost = checkpointXP();
        emit sig_died(xpLost);
        return;
    }

    if (m_lostLevelParser.parse(line)) {
        double xpLost = checkpointXP();
        emit sig_lostLevel(xpLost);
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

    // REVISIT handle Goodbye() specially due to malloc bug in GmcpMessage
    // when trying to extract the string via toQString() on that message
    // type, needs more investigation.
    if (msg.isCoreGoodbye() and m_Progress.has_value()) {
        qDebug().noquote() << QString("Adventure: ending session for %1").arg(m_Progress->name());
        emit sig_endedSession(m_Progress->name());
        m_Progress.reset();
        return;
    }

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

    parseIfReceivedComm(msg, doc);

    parseIfUpdatedChar(msg, doc);

    parseIfUpdatedXP(msg, doc);
}

void AdventureTracker::parseIfReceivedComm(GmcpMessage msg, QJsonDocument doc)
{
    if (!msg.isCommChannelText())
        return;

    QJsonObject obj = doc.object();

    if (!(obj.contains("channel") and obj.contains("text")))
        return;

    if (obj["channel"].toString() == "tells") {
        emit sig_receivedTell(obj["text"].toString());

    } else if (obj["channel"].toString() == "tales") {
        emit sig_receivedNarrate(obj["text"].toString());
    }
}

void AdventureTracker::parseIfUpdatedChar(GmcpMessage msg, QJsonDocument doc)
{
    if (!(msg.isCharName() or msg.isCharStatusVars()))
        return;

    QJsonObject obj = doc.object();

    if (!obj.contains("name"))
        return;

    auto charName = obj["name"].toString();

    if (!m_Progress.has_value()) {
        qDebug().noquote() << QString("Adventure: new adventure for char %1").arg(charName);

        m_Progress.emplace(charName);
        emit sig_updatedChar(m_Progress->name());
    }

    if (m_Progress.has_value() and m_Progress->name() != charName) {
        qDebug().noquote() << QString("Adventure: adventure ending for %1, new adventure for %2")
                                  .arg(m_Progress->name())
                                  .arg(charName);

        emit sig_endedSession(m_Progress->name());
        m_Progress.emplace(charName);
        emit sig_updatedChar(m_Progress->name());
    }
}

void AdventureTracker::parseIfUpdatedXP(GmcpMessage msg, QJsonDocument doc)
{
    if (!msg.isCharVitals())
        return;

    QJsonObject obj = doc.object();

    if (!obj.contains("xp"))
        return;

    auto xpCurrent = obj["xp"].toDouble();

    if (m_Progress.has_value()) {
        m_Progress->updateXP(xpCurrent);
        emit sig_updatedXP(m_Progress->xpInitial(), m_Progress->xpCurrent());
    }
}

double AdventureTracker::checkpointXP()
{
    if (m_Progress.has_value()) {
        return m_Progress->checkpointXP();
    } else {
        qDebug().noquote() << "Adventure: attempting to checkpointXP() without valid state.";
        return 0;
    }
}
