#ifndef TOKENMANAGER_H
#define TOKENMANAGER_H

#include "../opengl/OpenGLTypes.h" // MMTextureId forward-declared here

#include <memory>

#include <QFileSystemWatcher>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QTimer>
#include <QVector>

class MMTexture; // forward
class OpenGL;
using SharedMMTexture = std::shared_ptr<MMTexture>; // …

enum class NODISCARD TokenImportResultEnum { IMPORTED, ALREADY_EXISTS, FAILED };

class NODISCARD_QOBJECT TokenManager final : public QObject
{
    Q_OBJECT

public:
    explicit TokenManager(QObject *parent = nullptr);
    ~TokenManager() final;

    QPixmap getToken(const QString &key, bool bearForm = false);
    const QMap<QString, QString> &availableFiles() const { return m_availableFiles; }
    TokenImportResultEnum importTokenImage(const QString &sourcePath,
                                           bool replaceExisting,
                                           QString &tokenName,
                                           QString &errorMessage);
    static QString overrideFor(const QString &displayName);
    static QString bearOverrideFor(const QString &displayName);
    MMTextureId textureIdFor(const QString &key, bool bearForm = false); // ← per-token cache
    MMTextureId uploadNow(const QString &key, bool bearForm, const QPixmap &px);
    SharedMMTexture textureById(MMTextureId id) const;
    void invalidateTextures();
    void processPendingTextureChanges(OpenGL &gl);

    static void cleanupOpenGLTexturesIfCreated(OpenGL &gl);

public slots:
    void slot_resourcesDirectoryChanged();

signals:
    void sig_tokensChanged();

private:
    void scanDirectories();
    void scheduleDirectoryScan();
    void clearOpenGLTextures(OpenGL &gl);
    QPixmap loadToken(const QString &lookup);

    QMap<QString, QString> m_availableFiles;
    QFileSystemWatcher m_watcher;
    QTimer m_rescanTimer;
    mutable QMap<QString, QString> m_tokenPathCache;
    QPixmap m_fallbackPixmap;

    QHash<QString, MMTextureId> m_textureCache; // key → GL id
    QVector<SharedMMTexture> m_ownedTextures;   // keep textures alive
    bool m_pendingTextureChanges = false;
};

// sentinel
extern const QString kForceFallback;
TokenManager &tokenManager();

#endif
