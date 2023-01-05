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
}

AdventureJournal::~AdventureJournal()
{}

void AdventureJournal::slot_onUserText(const QByteArray &ba)
{
    //qDebug() << "AdventureJournal::slot_updateJournal called";

    // Remove ANSI and convert to UTF-8
    QString str = QString::fromLatin1(ba).trimmed();
    ParserUtils::removeAnsiMarksInPlace(str);
    //QString line = ::toStdStringUtf8(str);

    if (str.contains("narrates '")) {
        emit sig_receivedNarrate(str);
    }

    if (str.contains("tells you '")) {
        emit sig_receivedTell(str);
    }

    auto idx_isdead = str.indexOf(" is dead! R.I.P.");
    if (idx_isdead > 0) {
        qDebug().noquote() << "AdventureJournal: player killed mob: " << str.left(idx_isdead);
    }

    if (str.contains("You gain a level!")) {
        qDebug().noquote() << "AdventureJournal: player gained a level!";
    }
}
