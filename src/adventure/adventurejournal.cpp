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

    if (str.contains("narrates '")) {
        emit sig_receivedNarrate(str);
    }

    if (str.contains("tells you '")) {
        emit sig_receivedTell(str);
    }

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

    if (!(gmcpMessage.isCharName() or gmcpMessage.isCharStatusVars() or gmcpMessage.isCharVitals()))
        return;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(gmcpMessage.getJson()->toQString().toUtf8());

    if (!jsonDoc.isObject()) {
        qInfo() << "Received GMCP: " << gmcpMessage.getName().toQString()
                << "containing invalid Json: expecting object, got: "
                << gmcpMessage.getJson()->toQString();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    if (jsonObj.contains("xp")) {
        qInfo() << "GMCP xp: " << jsonObj["xp"].toDouble();
        emit sig_updatedXP(jsonObj["xp"].toDouble());
    }

    if (jsonObj.contains("next-level-xp")) {
        qInfo() << "GMCP next-level-xp" << jsonObj["next-level-xp"].toDouble();
    }
}
