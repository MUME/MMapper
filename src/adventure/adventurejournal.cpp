#include "adventurejournal.h"
#include "global/TextUtils.h"
#include "parser/parserutils.h"

#include <QDebug>

AdventureJournal::AdventureJournal(QObject *const parent)
    : QObject{parent}
{}

AdventureJournal::~AdventureJournal()
{
    //m_debugFile.flush();
    //m_debugFile.close();
}

void AdventureJournal::slot_updateJournal(const QByteArray &ba)
{
    //qDebug() << "AdventureJournal::slot_updateJournal called";

    // Remove ANSI and convert to UTF-8
    QString str = QString::fromLatin1(ba).trimmed();
    ParserUtils::removeAnsiMarksInPlace(str);
    //QString line = ::toStdStringUtf8(str);

    auto idx_isdead = str.indexOf(" is dead! R.I.P.");
    if (idx_isdead > 0) {
        qDebug().noquote() << "AdventureJournal: player killed: " << str.left(idx_isdead);
    }

    if(str.contains("You gain a level!")) {
        qDebug().noquote() << "AdventureJournal: player gained a level!";
    }

    if(str.contains("tells you") or str.contains("narrates")) {
        qDebug().noquote() << "Adventure Journal: comm received: " << str;
        m_commsList.append(str);
    }
}
