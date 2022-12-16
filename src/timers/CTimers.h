//
// Created by Azazello on 16.12.22.
//

#ifndef MMAPPER_CTIMERS_H
#define MMAPPER_CTIMERS_H

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QMutex>
#include <QObject>

struct TTimer
{
    int id;
    QByteArray name;
    QByteArray desc;
    int duration;
    QElapsedTimer timer;
};

typedef struct
{
    QByteArray name;        /* spells name */
    QByteArray up_mes;      /* up message/pattern */
    QByteArray down_mes;    /* down message */
    QByteArray refresh_mes; /* refresh message */
    QElapsedTimer timer;    /* timer */
    bool addon;             /* if this spell has to be added after the "Affected by:" line */
    bool up;                /* is this spell currently up ? */
    bool silently_up;       /* this spell is up, but time wasn't set for ome reason (reconnect) */
                            /* this option is required for better GroupManager functioning */
} TSpell;

class CTimers : public QObject {
    Q_OBJECT

    QMutex	m_lock;

    int 	m_nextId;
    QList<TTimer *> m_timers;
    QList<TTimer *> m_countdowns;

    std::vector<TSpell>  spells;


    QByteArray getTimers();
    QByteArray getCountdowns();

signals:
    void sig_sendTimersUpdateToUser(const QString str);

public:
    CTimers(QObject *parent);
    virtual ~CTimers();

    static QString msToMinSec(int ms)
    {
        QString s;
        int min;
        int sec;

        sec = ms / 1000;
        min = sec / 60;
        sec = sec % 60;

        s = QString("%1%2:%3%4")
                .arg( min / 10 )
                .arg( min % 10 )
                .arg( sec / 10 )
                .arg( sec % 10 );

        return s;
    }


    void addTimer(QByteArray name, QByteArray desc);
    void addCountdown(QByteArray name, QByteArray desc, int time);

    bool removeTimer(QByteArray name);
    bool removeCountdown(QByteArray name);

    // for stat command representation
    QByteArray getStatCommandEntry();

    void clear();

    // spells
    void addSpell(QByteArray spellname, QByteArray up, QByteArray down, QByteArray refresh, bool addon);
    void addSpell(const TSpell &s);
    QString spellUpFor(unsigned int p);
    void resetSpells();

    void updateSpellsState(QByteArray line);
    QByteArray checkAffectedByLine(QByteArray line);

public slots:
    void finishCountdownTimer();

};


#endif //MMAPPER_CTIMERS_H
