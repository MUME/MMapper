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
    QString str = QString::fromLatin1(ba).trimmed();
    ParserUtils::removeAnsiMarksInPlace(str);

    // We keep track of the N most recent lines so that we can parse with the
    // context of a sliding window. Array indices are interpeted backwards,
    // so [0] is the last line received, [1] is the second-to-last, etc.
    // Must create our own QString copy for this, since the `ba` passed above
    // is reused. So shfit the array and add a copy.
    for (size_t i = m_lastLines.size(); i > 0; i--) {
        m_lastLines[i] = m_lastLines[i - 1];
    }
    m_lastLines[0] = new QString(str);

    parseIfKillAndXP();

    parseIfAchievedSomething();

    if (str.contains("You gain a level!")) {
        qDebug().noquote() << "AdventureJournal: player gained a level!";
    }
}

void AdventureTracker::slot_onUserGmcp(const GmcpMessage &gmcpMessage)
{
    // https://mume.org/help/generic_mud_communication_protocol

    if (!(gmcpMessage.isCharName() or gmcpMessage.isCharVitals() or gmcpMessage.isCommChannelText()))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(gmcpMessage.getJson()->toQString().toUtf8());

    if (!doc.isObject()) {
        qInfo() << "Received GMCP: " << gmcpMessage.getName().toQString()
                << "containing invalid Json: expecting object, got: "
                << gmcpMessage.getJson()->toQString();
        return;
    }

    QJsonObject obj = doc.object();

    if (gmcpMessage.isCommChannelText() && obj.contains("channel") && obj.contains("text")) {
        if (obj["channel"].toString() == "tells") {
            emit sig_receivedTell(obj["text"].toString());

        } else if (obj["channel"].toString() == "tales") {
            emit sig_receivedNarrate(obj["text"].toString());
        }
    }

    if (obj.contains("xp")) {
        updateXPfromMud(obj["xp"].toDouble());
    }
}

void AdventureTracker::parseIfKillAndXP()
{
    // We define a "kill with XP earned" event as:
    //   - The last line received contains "is dead! R.I.P." or "disappears into nothing."
    //   - AND among the K most recent lines previously received, at least one contains:
    //     -  "You receive your share of experience."
    //     -  "You gain a level!
    //     -  "You feel revitalized as the dark power within" (Dark Oath)

    auto lastLine = m_lastLines[0];

    if (lastLine == nullptr)
        return;

    auto idx_dead = lastLine->indexOf(" is dead! R.I.P.");
    if (idx_dead == -1)
        lastLine->indexOf(" disappears into nothing.");
    if (idx_dead == -1)
        return;

    // A kill has occurred, but did the player earn xp?
    auto mobName = lastLine->left(idx_dead);
    bool earnedXP = std::any_of(m_lastLines.begin(), m_lastLines.end(), [](QString *l) {
        return l != nullptr
               and (l->contains("You receive your share of experience.")
                    or l->contains("You gain a level!")
                    or l->contains("You feel revitalized as the dark power within"));
    });

    if (!earnedXP)
        return;

    double xpGained = checkpointXP();
    emit sig_killedMob(mobName, xpGained);
}

void AdventureTracker::parseIfAchievedSomething()
{
    // If second-to-last line starts with "You achieved something new!" then
    // we interpet the last line as an achivement. Hopefully this will be
    // replaced with GMCP at some point.
    auto line2 = m_lastLines[1];
    if (line2 == nullptr)
        return;

    auto idx_achieved = line2->indexOf("You achieved something new!");

    if (idx_achieved != 0)
        return;

    // Line starts with You achieved...
    auto achievement = m_lastLines[0];
    double xpGained = checkpointXP();
    emit sig_achievedSomething(*achievement, xpGained);
}

void AdventureTracker::updateXPfromMud(double currentXP)
{
    if (!m_xpInitial.has_value()) {
        qDebug().noquote() << "Initial XP checkpoint: " + QString::number(currentXP, 'f', 0);
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
        qDebug() << "Attempting to checkpointXP() without valid state.";
        return 0;
    }

    double xpGained = m_xpCurrent.value() - m_xpCheckpoint.value();
    m_xpCheckpoint.emplace(m_xpCurrent.value());

    return xpGained;
}
