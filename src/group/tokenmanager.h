#ifndef TOKENMANAGER_H
#define TOKENMANAGER_H

#include <QString>
#include <QPixmap>
#include <QPixmapCache>
#include <QMap>
#include <QFileSystemWatcher>

class TokenManager
{
public:
    TokenManager();

    QPixmap getToken(const QString &key);
    const QMap<QString, QString> &availableFiles() const;

private:
    void scanDirectories();

    QMap<QString, QString> m_availableFiles;
    QFileSystemWatcher m_watcher;
    static QString normalizeKey(const QString &rawKey);
mutable QMap<QString, QString> m_tokenPathCache;
};
#endif
