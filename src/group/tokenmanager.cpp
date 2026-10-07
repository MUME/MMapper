#include "tokenmanager.h"

#include "../configuration/configuration.h"
#include "../display/Filenames.h"
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
#include <QSet>

namespace {
TokenManager *g_tokenManager = nullptr;
}

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
    const QImage img = px.toImage().mirrored().convertToFormat(QImage::Format_RGBA8888);

    auto mmtex = MMTexture::alloc(
        QOpenGLTexture::Target2DArray,
        [&img](QOpenGLTexture &tex) {
            tex.setFormat(QOpenGLTexture::TextureFormat::RGBA8_UNorm);
            tex.setSize(img.width(), img.height());
            tex.setLayers(1);
            tex.setMipLevels(1);
            tex.setWrapMode(QOpenGLTexture::WrapMode::MirroredRepeat);
            tex.setMinMagFilters(QOpenGLTexture::Filter::Linear, QOpenGLTexture::Filter::Linear);
            tex.allocateStorage(QOpenGLTexture::PixelFormat::RGBA,
                                QOpenGLTexture::PixelType::UInt8);

            tex.setData(0,
                        0,
                        QOpenGLTexture::PixelFormat::RGBA,
                        QOpenGLTexture::PixelType::UInt8,
                        img.constBits());
        },
        /*forbidUpdates = */ true);

    const MMTextureId id = allocateTextureId();
    mmtex->setId(id);
    mmtex->setArrayPosition(MMTexArrayPosition{id, 0});

    return mmtex;
}

TokenManager::TokenManager(QObject *parent)
    : QObject(parent)
{
    g_tokenManager = this;
    scanDirectories();

    m_rescanTimer.setSingleShot(true);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        scheduleDirectoryScan();
    });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this]() {
        scheduleDirectoryScan();
    });
    connect(&m_rescanTimer, &QTimer::timeout, this, &TokenManager::scanDirectories);
}

TokenManager::~TokenManager()
{
    if (g_tokenManager == this) {
        g_tokenManager = nullptr;
    }
}

void TokenManager::scheduleDirectoryScan()
{
    m_rescanTimer.start(100);
}

void TokenManager::slot_resourcesDirectoryChanged()
{
    scheduleDirectoryScan();
}

void TokenManager::scanDirectories()
{
    for (const QString &path : m_availableFiles) {
        QPixmapCache::remove(path);
    }
    for (const QString &path : m_tokenPathCache) {
        QPixmapCache::remove(path);
    }
    m_tokenPathCache.clear();
    m_availableFiles.clear();
    const QStringList watchedFiles = m_watcher.files();
    const QStringList watchedDirectories = m_watcher.directories();
    if (!watchedFiles.isEmpty()) {
        m_watcher.removePaths(watchedFiles);
    }
    if (!watchedDirectories.isEmpty()) {
        m_watcher.removePaths(watchedDirectories);
    }

    const QString userTokensDir
        = QDir(getConfig().canvas.resourcesDirectory).filePath(QStringLiteral("tokens"));
    const QString assetTokensDir = QDir(getAssetsPath()).filePath(QStringLiteral("tokens"));

    const QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    const QSet<QByteArray> formats(supportedFormats.begin(), supportedFormats.end());

    // Scan from highest to lowest priority. The user's resource directory overrides
    // sideloaded assets, which override built-in Qt resources.
    const auto scanDirectory = [this, &formats](const QString &path, const bool watch) {
        if (path.isEmpty()) {
            return;
        }

        const QDir dir(path);
        if (!dir.exists()) {
            return;
        }

        if (watch) {
            m_watcher.addPath(path);
            QDirIterator directories(path,
                                     QDir::Dirs | QDir::NoDotAndDotDot,
                                     QDirIterator::Subdirectories);
            while (directories.hasNext()) {
                m_watcher.addPath(directories.next());
            }
        }

        QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString filePath = it.next();
            const QFileInfo info(filePath);
            if (!formats.contains(info.suffix().toLower().toUtf8())) {
                continue;
            }

            const QString key = normalizeKey(info.baseName());
            if (!key.isEmpty() && !m_availableFiles.contains(key)) {
                m_availableFiles.insert(key, filePath);
                if (watch) {
                    m_watcher.addPath(filePath);
                }
            }
        }
    };

    // Preserve the existing mod-first lookup, then fall back to packaged assets
    // and finally to any token images compiled into the Qt resource system.
    scanDirectory(userTokensDir, true);
    scanDirectory(assetTokensDir, false);
    scanDirectory(QStringLiteral(":/tokens"), false);

    if (!QDir(userTokensDir).exists()) {
        m_watcher.addPath(QFileInfo(userTokensDir).absolutePath());
    }

    m_pendingTextureChanges = true;
    emit sig_tokensChanged();
}

QPixmap TokenManager::getToken(const QString &key)
{
    if (m_fallbackPixmap.isNull())
        m_fallbackPixmap.load(":/pixmaps/char-room-sel.png");

    if (key == kForceFallback) {
        return m_fallbackPixmap;
    }

    const QString ov = overrideFor(key);
    if (ov == kForceFallback) {
        return m_fallbackPixmap;
    }

    QString lookup = key;
    if (!ov.isEmpty()) {
        lookup = ov; // use the user-chosen icon basename
    }

    QString resolvedKey = normalizeKey(lookup);
    if (resolvedKey.isEmpty()) {
        resolvedKey = QStringLiteral("blank_character");
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

    const auto availableIt = m_availableFiles.constFind(resolvedKey);
    if (availableIt != m_availableFiles.cend()) {
        const QString &path = availableIt.value();

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

    // Final fallback: built-in resource image
    QString finalFallback = ":/pixmaps/char-room-sel.png";
    m_tokenPathCache[resolvedKey] = finalFallback; // ✅ Cache fallback
    return QPixmap(finalFallback);
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

    return INVALID_MM_TEXTURE_ID;
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

void TokenManager::clearOpenGLTextures(OpenGL &gl)
{
    for (const SharedMMTexture &texture : m_ownedTextures) {
        gl.setTextureLookup(texture->getId(), {});
    }
    m_textureCache.clear();
    m_ownedTextures.clear();
    m_pendingTextureChanges = false;
}

void TokenManager::processPendingTextureChanges(OpenGL &gl)
{
    if (m_pendingTextureChanges && QOpenGLContext::currentContext()) {
        clearOpenGLTextures(gl);
    }
}

void TokenManager::cleanupOpenGLTexturesIfCreated(OpenGL &gl)
{
    if (g_tokenManager != nullptr && QOpenGLContext::currentContext() != nullptr) {
        g_tokenManager->clearOpenGLTextures(gl);
    }
}

// retrieve pointer later
SharedMMTexture TokenManager::textureById(MMTextureId id) const
{
    for (const auto &ptr : m_ownedTextures)
        if (ptr->getId() == id)
            return ptr;
    return {};
}
