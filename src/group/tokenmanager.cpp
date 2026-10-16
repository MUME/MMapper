#include "tokenmanager.h"

#include "../configuration/configuration.h"
#include "../display/Filenames.h"
#include "../display/Textures.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLTypes.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QOpenGLContext>
#include <QPixmapCache>
#include <QSaveFile>
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

static QString findOverride(const QMap<QString, QString> &overrides, const QString &displayName)
{
    const QString name = displayName.trimmed();
    const auto exactMatch = overrides.constFind(name);
    if (exactMatch != overrides.constEnd()) {
        return exactMatch.value();
    }
    for (auto it = overrides.cbegin(); it != overrides.cend(); ++it) {
        if (it.key().trimmed().compare(name, Qt::CaseInsensitive) == 0) {
            return it.value();
        }
    }
    return {};
}

QString TokenManager::overrideFor(const QString &displayName)
{
    const auto &over = getConfig().groupManager.tokenOverrides;
    const auto it = over.constFind(displayName.trimmed());
    return (it != over.constEnd()) ? it.value() : QString();
}

QString TokenManager::bearOverrideFor(const QString &displayName)
{
    return findOverride(getConfig().groupManager.bearTokenOverrides, displayName);
}

static QString textureCacheKey(const QString &key, const bool bearForm)
{
    return QStringLiteral("%1:%2").arg(bearForm ? QStringLiteral("bear") : QStringLiteral("normal"),
                                       key);
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
            tex.allocateStorage(QOpenGLTexture::PixelFormat::RGBA, QOpenGLTexture::PixelType::UInt8);

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

TokenImportResultEnum TokenManager::importTokenImage(const QString &sourcePath,
                                                     const bool replaceExisting,
                                                     QString &tokenName,
                                                     QString &errorMessage)
{
    const QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.isFile()) {
        errorMessage = tr("The selected image file could not be found.");
        return TokenImportResultEnum::FAILED;
    }

    const QString resourcesDirectory = getConfig().canvas.resourcesDirectory;
    if (resourcesDirectory.isEmpty()) {
        errorMessage = tr("No resource directory has been configured.");
        return TokenImportResultEnum::FAILED;
    }

    const QString tokensDirectory = QDir(resourcesDirectory).filePath(QStringLiteral("tokens"));
    if (!QDir().mkpath(tokensDirectory)) {
        errorMessage = tr("The tokens folder could not be created:\n%1").arg(tokensDirectory);
        return TokenImportResultEnum::FAILED;
    }

    const QString destinationPath = QDir(tokensDirectory).filePath(sourceInfo.fileName());
    const QFileInfo destinationInfo(destinationPath);
    tokenName = sourceInfo.completeBaseName();

    if (sourceInfo.canonicalFilePath() == destinationInfo.canonicalFilePath()) {
        scheduleDirectoryScan();
        return TokenImportResultEnum::IMPORTED;
    }

    if (destinationInfo.exists() && !replaceExisting) {
        return TokenImportResultEnum::ALREADY_EXISTS;
    }

    const auto existingToken = m_availableFiles.constFind(normalizeKey(tokenName));
    if (existingToken != m_availableFiles.cend()) {
        const QFileInfo existingInfo(existingToken.value());
        const QString existingPath = existingInfo.absoluteFilePath();
        const QString relativePath = QDir(tokensDirectory).relativeFilePath(existingPath);
        const bool existingIsUserToken = relativePath != QStringLiteral("..")
                                         && !relativePath.startsWith(QStringLiteral("../"))
                                         && !QDir::isAbsolutePath(relativePath);
        if (existingIsUserToken
            && existingInfo.fileName().compare(sourceInfo.fileName(), Qt::CaseInsensitive) != 0) {
            errorMessage
                = tr("A token with the same selectable name already exists as %1. Rename the new "
                     "image or remove the existing file before importing it.")
                      .arg(existingInfo.fileName());
            return TokenImportResultEnum::FAILED;
        }
    }

    QFile sourceFile(sourcePath);
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        errorMessage = tr("The selected image could not be read:\n%1").arg(sourceFile.errorString());
        return TokenImportResultEnum::FAILED;
    }

    QSaveFile destinationFile(destinationPath);
    if (!destinationFile.open(QIODevice::WriteOnly)) {
        errorMessage = tr("The image could not be saved to:\n%1\n%2")
                           .arg(destinationPath, destinationFile.errorString());
        return TokenImportResultEnum::FAILED;
    }

    while (!sourceFile.atEnd()) {
        const QByteArray chunk = sourceFile.read(64 * 1024);
        if (chunk.isEmpty()) {
            if (sourceFile.error() != QFileDevice::NoError) {
                errorMessage = tr("The selected image could not be read:\n%1")
                                   .arg(sourceFile.errorString());
                destinationFile.cancelWriting();
                return TokenImportResultEnum::FAILED;
            }
            break;
        }
        if (destinationFile.write(chunk) != chunk.size()) {
            errorMessage = tr("The image could not be saved to:\n%1\n%2")
                               .arg(destinationPath, destinationFile.errorString());
            destinationFile.cancelWriting();
            return TokenImportResultEnum::FAILED;
        }
    }

    if (!destinationFile.commit()) {
        errorMessage = tr("The image could not be saved to:\n%1\n%2")
                           .arg(destinationPath, destinationFile.errorString());
        return TokenImportResultEnum::FAILED;
    }

    scheduleDirectoryScan();
    return TokenImportResultEnum::IMPORTED;
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

    const QString userTokensDir = QDir(getConfig().canvas.resourcesDirectory)
                                      .filePath(QStringLiteral("tokens"));
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

            const QString key = normalizeKey(info.completeBaseName());
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

QPixmap TokenManager::loadToken(const QString &lookup)
{
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
        return {};
    }

    return {};
}

QPixmap TokenManager::getToken(const QString &key, const bool bearForm)
{
    if (m_fallbackPixmap.isNull()) {
        m_fallbackPixmap.load(":/pixmaps/char-room-sel.png");
    }

    if (key == kForceFallback) {
        return m_fallbackPixmap;
    }

    if (bearForm) {
        const QString bearOverride = bearOverrideFor(key);
        if (!bearOverride.isEmpty() && bearOverride != kForceFallback) {
            if (QPixmap selected = loadToken(bearOverride); !selected.isNull()) {
                return selected;
            }
        }

        const QString defaultBearName = getConfig().groupManager.defaultBearToken;
        if (!defaultBearName.isEmpty()) {
            if (QPixmap defaultBear = loadToken(defaultBearName); !defaultBear.isNull()) {
                return defaultBear;
            }
        }
        if (QPixmap builtInBear = loadToken(QStringLiteral("Bear-M")); !builtInBear.isNull()) {
            return builtInBear;
        }

        // Keep self visible even if neither the selected image nor Bear-M.png is available.
        return getToken(key, false);
    }

    const QString tokenOverride = overrideFor(key);
    if (tokenOverride == kForceFallback) {
        return m_fallbackPixmap;
    }

    const QString lookup = tokenOverride.isEmpty() ? key : tokenOverride;
    if (QPixmap token = loadToken(lookup); !token.isNull()) {
        return token;
    }

    qWarning() << "TokenManager: No match found for key:" << normalizeKey(lookup);
    return m_fallbackPixmap;
}

TokenManager &tokenManager()
{
    static TokenManager instance; // created on first call (post-QGuiApp)
    return instance;
}

MMTextureId TokenManager::textureIdFor(const QString &key, const bool bearForm)
{
    const QString cacheKey = textureCacheKey(key, bearForm);
    if (m_textureCache.contains(cacheKey))
        return m_textureCache.value(cacheKey);

    return INVALID_MM_TEXTURE_ID;
}

MMTextureId TokenManager::uploadNow(const QString &key, const bool bearForm, const QPixmap &px)
{
    SharedMMTexture tex = makeTextureFromPixmap(px);
    MMTextureId id = tex->getId();

    if (id == INVALID_MM_TEXTURE_ID)
        return id;

    m_ownedTextures.push_back(std::move(tex));
    m_textureCache.insert(textureCacheKey(key, bearForm), id);
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

void TokenManager::invalidateTextures()
{
    m_pendingTextureChanges = true;
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
