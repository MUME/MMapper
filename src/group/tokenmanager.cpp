#include "tokenmanager.h"
#include "../configuration/configuration.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QStandardPaths>
#include <QDebug>

const QString kForceFallback(QStringLiteral("__force_fallback__"));

static QString normalizeKey(QString key)
{
    static const QRegularExpression nonWordReg(
        QStringLiteral("[^a-z0-9_]+"));

    key = key.toLower();
    key.replace(nonWordReg, QStringLiteral("_"));
    return key;
}

QString TokenManager::overrideFor(const QString &displayName)
{
    const auto &over = getConfig().groupManager.tokenOverrides;
    auto it = over.constFind(displayName.trimmed());
    return (it != over.constEnd()) ? it.value() : QString();
}

TokenManager::TokenManager()
{
    scanDirectories();
    m_fallbackPixmap.load(":/pixmaps/char-room-sel.png");
}

void TokenManager::scanDirectories()
{
    m_availableFiles.clear();
    m_watcher.removePaths(m_watcher.files());
    m_watcher.removePaths(m_watcher.directories());

    const QString tokensDir = getConfig().canvas.resourcesDirectory + "/tokens";

    QDir dir(tokensDir);
    if (!dir.exists()) {
        qWarning() << "TokenManager: 'tokens' directory not found at:" << tokensDir;
        return;
    }

    m_watcher.addPath(tokensDir);

    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    QSet<QByteArray> formats(supportedFormats.begin(), supportedFormats.end());

    QDirIterator it(tokensDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString path = it.next();
        QFileInfo info(path);
        QString suffix = info.suffix().toLower();

        if (formats.contains(suffix.toUtf8()))
        {
            QString key = normalizeKey(info.baseName());
            if (!m_availableFiles.contains(key))
            {
                m_availableFiles.insert(key, path);
                qDebug() << "TokenManager: Found token image" << key << "at" << path;
                m_watcher.addPath(path);
            }
        }
    }
}

QPixmap TokenManager::getToken(const QString &key)
{
    if (key == kForceFallback) {
        return m_fallbackPixmap;
    }

    QString lookup = key;
    const QString ov = overrideFor(key);
    if (!ov.isEmpty())
        lookup = ov;                       // use the user-chosen icon basename

    QString resolvedKey = normalizeKey(lookup);

    //qDebug() << "TokenManager: Normalized key:" << resolvedKey;

    if (resolvedKey.isEmpty()) {
        qWarning() << "TokenManager: Received empty key — defaulting to 'blank_character'";
        resolvedKey = "blank_character";
    }

    //qDebug() << "TokenManager: Requested key:" << resolvedKey;

    if (m_tokenPathCache.contains(resolvedKey)) {
        QString path = m_tokenPathCache[resolvedKey];
        QPixmap cached;
        if (QPixmapCache::find(path, &cached)) {
            qDebug() << "TokenManager: Using cached pixmap (via cache) for" << path;
            return cached;
        }
        QPixmap pix;
        if (pix.load(path)) {
            QPixmapCache::insert(path, pix);
            return pix;
        }
        qWarning() << "TokenManager: Cached path was invalid:" << path;
    }

    QString matchedKey;
    for (const QString &k : m_availableFiles.keys()) {
        if (k.compare(resolvedKey, Qt::CaseInsensitive) == 0) {
            matchedKey = k;
            break;
        }
    }

    qDebug() << "TokenManager: Available token keys:";
    for (const auto &availableKey : m_availableFiles.keys()) {
        qDebug() << " - " << availableKey;
    }

    if (!matchedKey.isEmpty())
    {
        const QString &path = m_availableFiles.value(matchedKey);
        qDebug() << "TokenManager: Found path for key:" << matchedKey << "->" << path;

        m_tokenPathCache[resolvedKey] = path;

        QPixmap cached;
        if (QPixmapCache::find(path, &cached)) {
            qDebug() << "TokenManager: Using cached pixmap for" << path;
            return cached;
        }

        QPixmap pix;
        if (pix.load(path))
        {
            qDebug() << "TokenManager: Loaded and caching image for" << matchedKey;
            QPixmapCache::insert(path, pix);
            return pix;
        }
        else
        {
            qWarning() << "TokenManager: Failed to load image from path:" << path;
        }
    }
    else
    {
        qWarning() << "TokenManager: No match found for key:" << resolvedKey;
    }

    // Fallback: user-defined blank_character.png in tokens folder
    QString userFallback = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tokens/blank_character.png";
    if (QFile::exists(userFallback)) {
        qDebug() << "TokenManager: Using fallback image from modding folder:" << userFallback;
        m_tokenPathCache[resolvedKey] = userFallback;  // ✅ Cache fallback
        return QPixmap(userFallback);
    }

    // Final fallback: built-in resource image
    QString finalFallback = ":/pixmaps/char-room-sel.png";
    qDebug() << "TokenManager: Using final fallback image";
    m_tokenPathCache[resolvedKey] = finalFallback;  // ✅ Cache fallback
    return QPixmap(finalFallback);
}

const QMap<QString, QString> &TokenManager::availableFiles() const
{
    return m_availableFiles;
}
