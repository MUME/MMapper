#include "tokenmanager.h"

#include "../configuration/configuration.h"
#include "../display/Textures.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLTypes.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QOpenGLContext>
#include <QPixmapCache>
#include <QQueue>
#include <QStandardPaths>

const QString kForceFallback(QStringLiteral("__force_fallback__"));

static QString normalizeKey(QString key)
{
    static const QRegularExpression nonWordReg(QStringLiteral("[^a-z0-9_]+"));

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

static SharedMMTexture makeTextureFromPixmap(const QPixmap &px)
{
    using QT = QOpenGLTexture;

    auto mmtex = MMTexture::alloc(
        QT::Target2D,
        [&px](QT &tex) { tex.setData(px.toImage().mirrored()); },
        /*forbidUpdates = */ true);

    auto *tex = mmtex->get();
    tex->setWrapMode(QT::ClampToEdge);
    tex->setMinMagFilters(QT::Linear, QT::Linear);

    const MMTextureId internalId = allocateTextureId();
    mmtex->setId(internalId);

    return mmtex;
}

TokenManager::TokenManager()
{
    scanDirectories();
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
    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo info(path);
        QString suffix = info.suffix().toLower();

        if (formats.contains(suffix.toUtf8())) {
            QString key = normalizeKey(info.baseName());
            if (!m_availableFiles.contains(key)) {
                m_availableFiles.insert(key, path);
                m_watcher.addPath(path);
            }
        }
    }
}

QPixmap TokenManager::getToken(const QString &key)
{
    if (m_fallbackPixmap.isNull())
        m_fallbackPixmap.load(":/pixmaps/char-room-sel.png");

    if (key == kForceFallback) {
        return m_fallbackPixmap;
    }

    QString lookup = key;
    const QString ov = overrideFor(key);
    if (!ov.isEmpty())
        lookup = ov; // use the user-chosen icon basename

    QString resolvedKey = normalizeKey(lookup);

    if (resolvedKey.isEmpty()) {
        qWarning() << "TokenManager: Received empty key — defaulting to 'blank_character'";
        resolvedKey = "blank_character";
    }

    if (m_tokenPathCache.contains(resolvedKey)) {
        QString path = m_tokenPathCache[resolvedKey];
        QPixmap cached;
        if (QPixmapCache::find(path, &cached)) {
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

    for (const auto &availableKey : m_availableFiles.keys()) {
    }

    if (!matchedKey.isEmpty()) {
        const QString &path = m_availableFiles.value(matchedKey);

        m_tokenPathCache[resolvedKey] = path;

        QPixmap cached;
        if (QPixmapCache::find(path, &cached)) {
            return cached;
        }

        QPixmap pix;
        if (pix.load(path)) {
            QPixmapCache::insert(path, pix);
            return pix;
        } else {
            qWarning() << "TokenManager: Failed to load image from path:" << path;
        }
    } else {
        qWarning() << "TokenManager: No match found for key:" << resolvedKey;
    }

    // Fallback: user-defined blank_character.png in tokens folder
    QString userFallback = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                           + "/tokens/blank_character.png";
    if (QFile::exists(userFallback)) {
        m_tokenPathCache[resolvedKey] = userFallback; // ✅ Cache fallback
        return QPixmap(userFallback);
    }

    // Final fallback: built-in resource image
    QString finalFallback = ":/pixmaps/char-room-sel.png";
    m_tokenPathCache[resolvedKey] = finalFallback; // ✅ Cache fallback
    return QPixmap(finalFallback);
}

const QMap<QString, QString> &TokenManager::availableFiles() const
{
    return m_availableFiles;
}

TokenManager &tokenManager()
{
    static TokenManager instance; // created on first call (post-QGuiApp)
    return instance;
}

MMTextureId TokenManager::textureIdFor(const QString &key)
{
    if (m_textureCache.contains(key))
        return m_textureCache.value(key);

    /* NOT current – do NOT try to upload, just remember we need to.     */
    if (!m_pendingUploads.contains(key))
        m_pendingUploads.append(key);
    return INVALID_MM_TEXTURE_ID;
}

QString canonicalTokenKey(const QString &name)
{
    return normalizeKey(name); // reuse the existing static helper
}

MMTextureId TokenManager::uploadNow(const QString &key, const QPixmap &px)
{
    SharedMMTexture tex = makeTextureFromPixmap(px);
    MMTextureId id = tex->getId();

    if (id == INVALID_MM_TEXTURE_ID)
        return id;

    m_ownedTextures.push_back(std::move(tex));
    m_textureCache.insert(key, id);
    return id;
}

// keep tex alive + cache the id
void TokenManager::rememberUpload(const QString &key, MMTextureId id, SharedMMTexture tex)
{
    if (id == INVALID_MM_TEXTURE_ID)
        return;

    m_ownedTextures.push_back(std::move(tex));
    m_textureCache.insert(key, id);
}

// retrieve pointer later
SharedMMTexture TokenManager::textureById(MMTextureId id) const
{
    for (const auto &ptr : m_ownedTextures)
        if (ptr->getId() == id)
            return ptr;
    return {};
}
