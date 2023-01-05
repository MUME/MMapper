#pragma once
//
// Created by Azazello on 16.12.22.
//

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>

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

class Spells : public QObject
{
    Q_OBJECT

    std::vector<TSpell> spells;
signals:
    void sig_sendSpellsUpdateToUser(/* changes */);

public:
    explicit Spells(QObject *parent);
    virtual ~Spells();

    // spells
    void addSpell(
        QByteArray spellName, QByteArray up, QByteArray refresh, QByteArray down, bool addon);
    QString spellUpFor(unsigned int p);
    void reset();

    void updateSpellsState(QByteArray line);
    QByteArray checkAffectedByLine(QByteArray line);
};
