// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "RemoteEditDraftStore.h"

#include "../global/TextUtils.h"
#include "../global/io.h"
#include "../global/random.h"

#include <utility>

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

// MUME reuses small session ids, so the timestamp -- not just the id --
// keeps concurrent/successive drafts from colliding.
NODISCARD QString makeKeyStem(const RemoteSessionId sessionId)
{
    return QString("%1_%2").arg(sessionId.asInt32()).arg(QDateTime::currentMSecsSinceEpoch());
}

NODISCARD QString encodeFileName(const RemoteSessionId sessionId, const QString &title)
{
    // Keep file names bounded by shortening the title itself, so neither a
    // %XX escape nor a multi-byte character is ever cut in half.
    constexpr qsizetype MAX_ENCODED_TITLE = 50;
    QString shortened = title;
    QByteArray encoded = QUrl::toPercentEncoding(shortened);
    while (encoded.size() > MAX_ENCODED_TITLE && !shortened.isEmpty()) {
        shortened.chop(1);
        if (!shortened.isEmpty() && shortened.back().isHighSurrogate()) {
            shortened.chop(1);
        }
        encoded = QUrl::toPercentEncoding(shortened);
    }
    return QString("draft_%1_%2.txt").arg(makeKeyStem(sessionId), QString::fromLatin1(encoded));
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

// ---------------------------------------------------------------------------

RemoteEditFileDraftStore::RemoteEditFileDraftStore(QString directory)
    : m_directory(std::move(directory))
{
    QDir().mkpath(m_directory);
}

QString RemoteEditFileDraftStore::filePath(const QString &key) const
{
    if (key.isEmpty()) {
        return QString();
    }
    return QDir(m_directory).absoluteFilePath(key);
}

QString RemoteEditFileDraftStore::create(const RemoteSessionId sessionId,
                                         const QString &title,
                                         const QString &content)
{
    QString fileName = encodeFileName(sessionId, title);
    if (QFile::exists(filePath(fileName))) {
        fileName = fileName.chopped(4) + "_" + mmqt::toQStringLatin1(getRandomString(5)) + ".txt";
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
    if (key.isEmpty() || !QFile::exists(filePath(key))) {
        return false;
    }
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
    const QDir dir(m_directory);
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

RemoteEditSettingsDraftStore::RemoteEditSettingsDraftStore(SettingsFactory makeSettings)
    : m_makeSettings(std::move(makeSettings))
{}

QString RemoteEditSettingsDraftStore::create(const RemoteSessionId sessionId,
                                             const QString &title,
                                             const QString &content)
{
    const QString key = makeKeyStem(sessionId) + "_" + mmqt::toQStringLatin1(getRandomString(5));
    auto settings = m_makeSettings();
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
    auto settings = m_makeSettings();
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
    auto settings = m_makeSettings();
    return settings->value(QString("%1/%2/content").arg(SETTINGS_GROUP, key)).toString();
}

void RemoteEditSettingsDraftStore::remove(const QString &key)
{
    if (key.isEmpty()) {
        return;
    }
    auto settings = m_makeSettings();
    settings->beginGroup(SETTINGS_GROUP);
    settings->remove(key);
    settings->endGroup();
    settings->sync();
}

QList<RemoteEditDraftInfo> RemoteEditSettingsDraftStore::list() const
{
    QList<RemoteEditDraftInfo> drafts;
    auto settings = m_makeSettings();
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
