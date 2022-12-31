#pragma once

#include <fstream>
#include <QObject>

class AdventureJournal final : public QObject
{
    Q_OBJECT
public:
    explicit AdventureJournal(QObject * const parent = nullptr);
    ~AdventureJournal() final;

public slots:
    void slot_updateJournal(const QByteArray &ba);

private:
    std::fstream m_debugFile;
    QList<QString> m_commsList;
};


