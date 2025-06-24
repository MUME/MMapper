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
    static QString overrideFor(const QString &displayName);
    const QMap<QString, QString> &availableFiles() const;

private:
    void scanDirectories();

    QMap<QString, QString> m_availableFiles;
    QFileSystemWatcher m_watcher;
    mutable QMap<QString, QString> m_tokenPathCache;
    QPixmap m_fallbackPixmap;
};

// Global sentinel that forces the built-in placeholder
extern const QString kForceFallback;

#endif
