#ifndef TOKENMANAGER_H
#define TOKENMANAGER_H

#include "../opengl/OpenGLTypes.h" // MMTextureId forward-declared here

#include <memory>

#include <QFileSystemWatcher>
#include <QHash>
#include <QMap>
#include <QPixmap>
#include <QString>
#include <QVector>

class MMTexture;                                    // forward
using SharedMMTexture = std::shared_ptr<MMTexture>; // …
QString canonicalTokenKey(const QString &name);

class TokenManager
{
public:
    TokenManager();

    QPixmap getToken(const QString &key);
    static QString overrideFor(const QString &displayName);
    const QMap<QString, QString> &availableFiles() const;

    MMTextureId textureIdFor(const QString &key); // ← per-token cache
    MMTextureId uploadNow(const QString &key, const QPixmap &px);
    QList<QString> m_pendingUploads;

    void rememberUpload(const QString &key, MMTextureId id, SharedMMTexture tex);

    SharedMMTexture textureById(MMTextureId id) const;

private:
    void scanDirectories();

    QMap<QString, QString> m_availableFiles;
    QFileSystemWatcher m_watcher;
    mutable QMap<QString, QString> m_tokenPathCache;
    QPixmap m_fallbackPixmap;

    QHash<QString, MMTextureId> m_textureCache; // key → GL id
    QVector<SharedMMTexture> m_ownedTextures;   // keep textures alive
};

// sentinel
extern const QString kForceFallback;
TokenManager &tokenManager();

#endif
