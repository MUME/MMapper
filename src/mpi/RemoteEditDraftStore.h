#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"
#include "remoteeditsession.h"

#include <memory>

#include <QList>
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

    /// Files under the configured editor directory on native platforms;
    /// QSettings (which Qt persists to browser storage) on WebAssembly.
    NODISCARD static std::unique_ptr<RemoteEditDraftStore> makeDefault();
};

class NODISCARD RemoteEditFileDraftStore final : public RemoteEditDraftStore
{
public:
    NODISCARD QString create(RemoteSessionId sessionId,
                             const QString &title,
                             const QString &content) override;
    NODISCARD bool save(const QString &key, const QString &content) override;
    NODISCARD QString read(const QString &key) const override;
    void remove(const QString &key) override;
    NODISCARD QList<RemoteEditDraftInfo> list() const override;
    NODISCARD QString filePath(const QString &key) const override;

private:
    NODISCARD static QString getDirectory();
};

class NODISCARD RemoteEditSettingsDraftStore final : public RemoteEditDraftStore
{
public:
    NODISCARD QString create(RemoteSessionId sessionId,
                             const QString &title,
                             const QString &content) override;
    NODISCARD bool save(const QString &key, const QString &content) override;
    NODISCARD QString read(const QString &key) const override;
    void remove(const QString &key) override;
    NODISCARD QList<RemoteEditDraftInfo> list() const override;
};
