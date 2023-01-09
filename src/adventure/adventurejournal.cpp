#include <QJsonDocument>
#include <QJsonObject>

#include "adventurejournal.h"
#include "global/TextUtils.h"
#include "parser/parserutils.h"

#include <QDebug>

AdventureJournal::AdventureJournal(GameObserver &observer, QObject *const parent)
    : QObject{parent}
    , m_observer{observer}
{
    connect(&m_observer,
            &GameObserver::sig_sentToUserText,
            this,
            &AdventureJournal::slot_onUserText);

    connect(&m_observer,
            &GameObserver::sig_sentToUserGmcp,
            this,
            &AdventureJournal::slot_onUserGmcp);
}

AdventureJournal::~AdventureJournal() {}

void AdventureJournal::slot_onUserText(const QByteArray &ba)
{
    // Remove ANSI
    QString str = QString::fromLatin1(ba).trimmed();
    ParserUtils::removeAnsiMarksInPlace(str);
    // QString line = ::toStdStringUtf8(str);

    auto idx_isdead = str.indexOf(" is dead! R.I.P.");
    if (idx_isdead > 0) {
        emit sig_killedMob(str.left(idx_isdead));
    }

    if (str.contains("You gain a level!")) {
        qDebug().noquote() << "AdventureJournal: player gained a level!";
    }
}

void AdventureJournal::slot_onUserGmcp(const GmcpMessage &gmcpMessage)
{
    // https://mume.org/help/generic_mud_communication_protocol

    qDebug() << "GMCP received: " << gmcpMessage.getName().toQString();

    if (!(gmcpMessage.isCharName() or gmcpMessage.isCharStatusVars() or gmcpMessage.isCharVitals()
          or gmcpMessage.isCommChannelText()))
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
        if (obj["channel"].toString() == "tells")
            emit sig_receivedTell(obj["text"].toString());
        else if (obj["channel"].toString() == "narrates")
            emit sig_receivedNarrate(obj["text"].toString());
    }

    if (obj.contains("xp")) {
        qInfo() << "GMCP xp: " << obj["xp"].toDouble();
        emit sig_updatedXP(obj["xp"].toDouble());
    }

    if (obj.contains("next-level-xp")) {
        qInfo() << "GMCP next-level-xp" << obj["next-level-xp"].toDouble();
    }
}
