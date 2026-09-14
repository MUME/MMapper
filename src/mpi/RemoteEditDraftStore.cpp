// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "RemoteEditDraftStore.h"

#include "../configuration/configuration.h"
#include "../global/io.h"
#include "../global/random.h"

#include <sstream>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageLogContext>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QUrl>

namespace {

constexpr const std::string_view VALID_RANDOM_CHARS
    = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

NODISCARD QString randomSuffix(const int length)
{
    std::ostringstream os;
    for (int i = 0; i < length; ++i) {
        os << VALID_RANDOM_CHARS[getRandom(VALID_RANDOM_CHARS.length())];
    }
    return mmqt::toQStringUtf8(os.str());
}

// MUME reuses small session ids, so the timestamp -- not just the id --
// keeps concurrent/successive drafts from colliding.
NODISCARD QString makeKeyStem(const RemoteSessionId sessionId)
{
    return QString("%1_%2").arg(sessionId.asInt32()).arg(QDateTime::currentMSecsSinceEpoch());
}

NODISCARD QString encodeFileName(const RemoteSessionId sessionId, const QString &title)
{
    const QByteArray encoded = QUrl::toPercentEncoding(title);
    QByteArray safeTitle = encoded.left(50);
    if (safeTitle.size() < encoded.size()) {
        // Don't split a "%XX" escape at the truncation boundary.
        const qsizetype lastPercent = safeTitle.lastIndexOf('%');
        if (lastPercent >= 0 && safeTitle.size() - lastPercent < 3) {
            safeTitle.truncate(lastPercent);
        }
    }
    return QString("draft_%1_%2.txt").arg(makeKeyStem(sessionId), QString::fromLatin1(safeTitle));
}

NODISCARD bool decodeFileName(const QString &fileName, RemoteSessionId &sessionId, QString &title)
{
    static const QRegularExpression re("^draft_(-?\\d+)_(\\d+)_(.*)\\.txt$");
    const QRegularExpressionMatch match = re.match(fileName);
    if (!match.hasMatch()) {
        return false;
    }
    sessionId = RemoteSessionId(match.captured(1).toInt());
    title = QUrl::fromPercentEncoding(match.captured(3).toUtf8());
    return true;
}

constexpr const char *const SETTINGS_GROUP = "RemoteEditDrafts";

} // namespace

RemoteEditDraftStore::~RemoteEditDraftStore() = default;

std::unique_ptr<RemoteEditDraftStore> RemoteEditDraftStore::makeDefault()
{
#ifdef Q_OS_WASM
    return std::make_unique<RemoteEditSettingsDraftStore>();
#else
    return std::make_unique<RemoteEditFileDraftStore>();
#endif
}

// ---------------------------------------------------------------------------

QString RemoteEditFileDraftStore::getDirectory()
{
    const QString dir = getConfig().mumeClientProtocol.editorDirectory;
    QDir().mkpath(dir);
    return dir;
}

QString RemoteEditFileDraftStore::filePath(const QString &key) const
{
    if (key.isEmpty()) {
        return QString();
    }
    return QDir(getDirectory()).absoluteFilePath(key);
}

QString RemoteEditFileDraftStore::create(const RemoteSessionId sessionId,
                                         const QString &title,
                                         const QString &content)
{
    QString fileName = encodeFileName(sessionId, title);
    if (QFile::exists(filePath(fileName))) {
        fileName = fileName.chopped(4) + "_" + randomSuffix(5) + ".txt";
    }

    QFile file(filePath(fileName));
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Unable to create draft" << file.fileName();
        return QString();
    }
    file.write(mmqt::toQByteArrayLatin1(content)); // MPI is always Latin1
    file.flush();
    std::ignore = io::fsyncNoexcept(file);
    return fileName;
}

bool RemoteEditFileDraftStore::save(const QString &key, const QString &content)
{
    QSaveFile file(filePath(key));
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        return false;
    }
    file.write(mmqt::toQByteArrayLatin1(content));
    return file.commit();
}

QString RemoteEditFileDraftStore::read(const QString &key) const
{
    if (QFile file(filePath(key)); file.open(QFile::ReadOnly)) {
        return QString::fromLatin1(file.readAll());
    }
    qWarning() << "Unable to read draft" << filePath(key);
    return QString();
}

void RemoteEditFileDraftStore::remove(const QString &key)
{
    if (key.isEmpty()) {
        return;
    }
    if (!QFile::remove(filePath(key))) {
        qWarning() << "Unable to remove draft" << filePath(key);
    }
}

QList<RemoteEditDraftInfo> RemoteEditFileDraftStore::list() const
{
    QList<RemoteEditDraftInfo> drafts;
    const QDir dir(getDirectory());
    for (const QString &fileName : dir.entryList({"draft_*.txt"}, QDir::Files)) {
        RemoteSessionId sessionId;
        QString title;
        if (decodeFileName(fileName, sessionId, title)) {
            drafts.append({fileName, title, sessionId, QFileInfo(dir, fileName).lastModified()});
        }
    }
    return drafts;
}

// ---------------------------------------------------------------------------

QString RemoteEditSettingsDraftStore::create(const RemoteSessionId sessionId,
                                             const QString &title,
                                             const QString &content)
{
    const QString key = makeKeyStem(sessionId) + "_" + randomSuffix(5);
    auto settings = makeAppSettings();
    settings->beginGroup(SETTINGS_GROUP);
    settings->beginGroup(key);
    settings->setValue("sessionId", sessionId.asInt32());
    settings->setValue("title", title);
    settings->setValue("content", content);
    settings->setValue("lastModified", QDateTime::currentDateTime());
    settings->endGroup();
    settings->endGroup();
    settings->sync();
    return key;
}

bool RemoteEditSettingsDraftStore::save(const QString &key, const QString &content)
{
    auto settings = makeAppSettings();
    settings->beginGroup(SETTINGS_GROUP);
    settings->beginGroup(key);
    if (!settings->contains("title")) {
        return false;
    }
    settings->setValue("content", content);
    settings->setValue("lastModified", QDateTime::currentDateTime());
    settings->endGroup();
    settings->endGroup();
    settings->sync();
    return settings->status() == QSettings::NoError;
}

QString RemoteEditSettingsDraftStore::read(const QString &key) const
{
    auto settings = makeAppSettings();
    return settings->value(QString("%1/%2/content").arg(SETTINGS_GROUP, key)).toString();
}

void RemoteEditSettingsDraftStore::remove(const QString &key)
{
    if (key.isEmpty()) {
        return;
    }
    auto settings = makeAppSettings();
    settings->beginGroup(SETTINGS_GROUP);
    settings->remove(key);
    settings->endGroup();
    settings->sync();
}

QList<RemoteEditDraftInfo> RemoteEditSettingsDraftStore::list() const
{
    QList<RemoteEditDraftInfo> drafts;
    auto settings = makeAppSettings();
    settings->beginGroup(SETTINGS_GROUP);
    for (const QString &key : settings->childGroups()) {
        settings->beginGroup(key);
        drafts.append({key,
                       settings->value("title").toString(),
                       RemoteSessionId(settings->value("sessionId").toInt()),
                       settings->value("lastModified").toDateTime()});
        settings->endGroup();
    }
    settings->endGroup();
    return drafts;
}
