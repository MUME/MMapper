#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"
#include "remoteeditsession.h"

#include <functional>
#include <memory>

#include <QList>
#include <QSettings>
#include <QString>

/// Where unsent drafts live between (and across) sessions. A draft is
/// addressed by an opaque key handed out by create().
class NODISCARD RemoteEditDraftStore
{
public:
    virtual ~RemoteEditDraftStore();

    NODISCARD virtual QString create(RemoteSessionId sessionId,
                                     const QString &title,
                                     const QString &content)
        = 0;
    NODISCARD virtual bool save(const QString &key, const QString &content) = 0;
    NODISCARD virtual QString read(const QString &key) const = 0;
    virtual void remove(const QString &key) = 0;
    NODISCARD virtual QList<RemoteEditDraftInfo> list() const = 0;
    /// Absolute path of the draft, if this store keeps drafts as files that
    /// an external editor can open; empty otherwise.
    NODISCARD virtual QString filePath(const QString & /*key*/) const { return QString(); }
};

/// One file per draft under `directory`; the key is the file name.
class NODISCARD RemoteEditFileDraftStore final : public RemoteEditDraftStore
{
private:
    const QString m_directory;

public:
    explicit RemoteEditFileDraftStore(QString directory);

    NODISCARD QString create(RemoteSessionId sessionId,
                             const QString &title,
                             const QString &content) override;
    NODISCARD bool save(const QString &key, const QString &content) override;
    NODISCARD QString read(const QString &key) const override;
    void remove(const QString &key) override;
    NODISCARD QList<RemoteEditDraftInfo> list() const override;
    NODISCARD QString filePath(const QString &key) const override;
};

/// One QSettings group per draft. On WebAssembly Qt persists QSettings to
/// browser storage, which is what makes drafts survive a page reload there.
class NODISCARD RemoteEditSettingsDraftStore final : public RemoteEditDraftStore
{
public:
    using SettingsFactory = std::function<std::unique_ptr<QSettings>()>;

private:
    const SettingsFactory m_makeSettings;

public:
    explicit RemoteEditSettingsDraftStore(SettingsFactory makeSettings);

    NODISCARD QString create(RemoteSessionId sessionId,
                             const QString &title,
                             const QString &content) override;
    NODISCARD bool save(const QString &key, const QString &content) override;
    NODISCARD QString read(const QString &key) const override;
    void remove(const QString &key) override;
    NODISCARD QList<RemoteEditDraftInfo> list() const override;
};
