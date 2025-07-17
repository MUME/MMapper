#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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

private:
    RemoteEdit *m_manager = nullptr;
    QString m_content;
    const RemoteInternalId m_internalId{};
    const RemoteSessionId m_sessionId = REMOTE_VIEW_SESSION_ID;
    bool m_connected = true;

private:
    friend class RemoteEditExternalSession;
    friend class RemoteEditInternalSession;

public:
    explicit RemoteEditSession(const RemoteInternalId internalId,
                               const RemoteSessionId sessionId,
                               RemoteEdit *remoteEdit);

public:
    NODISCARD auto getInternalId() const { return m_internalId; }
    NODISCARD auto getSessionId() const { return m_sessionId; }
    NODISCARD bool isEditSession() const { return m_sessionId != REMOTE_VIEW_SESSION_ID; }
    NODISCARD const QString &getContent() const { return m_content; }
    void setContent(QString content) { m_content = std::move(content); }
    void cancel();
    void save();

public:
    NODISCARD bool isConnected() const { return m_connected; }
    void setDisconnected() { m_connected = false; }

protected slots:
    void slot_onCancel() { cancel(); }
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

public:
    explicit RemoteEditInternalSession(const RemoteInternalId internalId,
                                       const RemoteSessionId sessionId,
                                       const QString &title,
                                       const QString &body,
                                       RemoteEdit *remoteEdit);
    ~RemoteEditInternalSession() final;
};

class NODISCARD_QOBJECT RemoteEditExternalSession final : public RemoteEditSession
{
    Q_OBJECT

private:
    QPointer<RemoteEditProcess> m_process;

public:
    explicit RemoteEditExternalSession(const RemoteInternalId internalId,
                                       const RemoteSessionId sessionId,
                                       const QString &title,
                                       const QString &body,
                                       RemoteEdit *remoteEdit);
    ~RemoteEditExternalSession() final;
};
