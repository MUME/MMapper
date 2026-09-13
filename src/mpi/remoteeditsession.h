#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#if 1
// https://qt-project.atlassian.net/browse/QTBUG-31496
#include <QtCore/qglobal.h>
#endif

#include "../global/TaggedInt.h"
#include "../global/TaggedString.h"
#include "../global/macros.h"
#include "../proxy/TaggedBytes.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QtCore>
#include <QtGlobal>

class RemoteEdit;
class RemoteEditProcess;
class RemoteEditWidget;

namespace tags {
struct NODISCARD RemoteInternalIdTag final
{};
struct NODISCARD RemoteSessionIdTag final
{};
} // namespace tags

struct NODISCARD RemoteInternalId final
    : public TaggedInt<RemoteInternalId, tags::RemoteInternalIdTag, uint32_t>
{
    using TaggedInt::TaggedInt;
    constexpr RemoteInternalId()
        : RemoteInternalId{0}
    {}
    NODISCARD constexpr uint32_t asUint32() const { return value(); }
    friend std::ostream &operator<<(std::ostream &os, RemoteInternalId id);
};

// REVISIT: The successor edit format will need to be base64 with "content-type" metadata,
// so it can correctly transfer utf8 and latin1 files, as well as utf8 string data to mudlle,
// and possibly also various image formats; this would allow sharing of pictures.
struct NODISCARD RemoteSessionId final
    : public TaggedInt<RemoteSessionId, tags::RemoteSessionIdTag, int32_t>
{
    using TaggedInt::TaggedInt;
    constexpr RemoteSessionId()
        : RemoteSessionId{-1}
    {}
    NODISCARD constexpr int32_t asInt32() const { return value(); }
    friend std::ostream &operator<<(std::ostream &os, RemoteSessionId id);
};

// Internally shared across all view sessions
static inline const RemoteSessionId REMOTE_VIEW_SESSION_ID = RemoteSessionId(-1);

class NODISCARD_QOBJECT RemoteEditSession : public QObject
{
    Q_OBJECT

protected:
    RemoteEdit *m_manager = nullptr;
    QString m_content;
    QString m_title;
    const RemoteInternalId m_internalId{};
    const RemoteSessionId m_sessionId = REMOTE_VIEW_SESSION_ID;
    const bool m_draftRecovery = false;
    bool m_connected = true;
    QString m_draftFileName;

private:
#ifndef Q_OS_WASM
    friend class RemoteEditExternalSession;
#endif
    friend class RemoteEditInternalSession;

public:
    explicit RemoteEditSession(RemoteInternalId internalId,
                               RemoteSessionId sessionId,
                               QString title,
                               QString draftFileName,
                               bool draftRecovery,
                               RemoteEdit *remoteEdit);

public:
    NODISCARD auto getInternalId() const { return m_internalId; }
    NODISCARD auto getSessionId() const { return m_sessionId; }
    NODISCARD bool isEditSession() const { return m_sessionId != REMOTE_VIEW_SESSION_ID; }
    /// True for a window reopened from a persisted draft (recovery), as opposed
    /// to a live session originating from a MUME GMCP edit/view request.
    NODISCARD bool isDraftRecovery() const { return m_draftRecovery; }
    NODISCARD const QString &getContent() const { return m_content; }
    NODISCARD const QString &getTitle() const { return m_title; }
    void setContent(QString content) { m_content = std::move(content); }
    void cancel();
    void save();
    void discard();
    /// Raises/activates this session's window, if it has one (no-op otherwise).
    virtual void focus() {}
    /// Flushes any pending debounced auto-save immediately (no-op unless overridden).
    virtual void flushDraft() {}
    /// Short label for UI, e.g. "Internal" / "External".
    NODISCARD virtual const char *getEditorTypeName() const { return "Internal"; }

public:
    NODISCARD bool isConnected() const { return m_connected; }
    void setDisconnected() { m_connected = false; }
    NODISCARD const QString &getDraftFileName() const { return m_draftFileName; }
    NODISCARD QString getFullDraftPath() const;

protected slots:
    void slot_onCancel() { cancel(); }
    void slot_onDiscard() { discard(); }
    void slot_onSave(const QString &content)
    {
        setContent(content);
        save();
    }
};

class NODISCARD_QOBJECT RemoteEditInternalSession final : public RemoteEditSession
{
    Q_OBJECT

private:
    QPointer<RemoteEditWidget> m_widget;
    QTimer *m_debounceTimer = nullptr;
    QTimer *m_throttleTimer = nullptr;

public:
    explicit RemoteEditInternalSession(RemoteInternalId internalId,
                                       RemoteSessionId sessionId,
                                       const QString &title,
                                       const QString &body,
                                       const QString &draftFileName,
                                       bool draftRecovery,
                                       RemoteEdit *remoteEdit);
    ~RemoteEditInternalSession() final;

public:
    void focus() override;
    void flushDraft() override;

private slots:
    void slot_onTextModified(const QString &content);
    void slot_performAutoSave();
};

#ifndef Q_OS_WASM
class NODISCARD_QOBJECT RemoteEditExternalSession final : public RemoteEditSession
{
    Q_OBJECT

private:
    QPointer<RemoteEditProcess> m_process;

public:
    explicit RemoteEditExternalSession(RemoteInternalId internalId,
                                       RemoteSessionId sessionId,
                                       const QString &title,
                                       const QString &body,
                                       const QString &draftFileName,
                                       RemoteEdit *remoteEdit);
    ~RemoteEditExternalSession() final;

public:
    NODISCARD const char *getEditorTypeName() const override { return "External"; }
};
#endif
