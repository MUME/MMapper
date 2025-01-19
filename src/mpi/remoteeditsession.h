#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/TaggedString.h"
#include "../global/macros.h"

#include <algorithm>

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QtCore>
#include <QtGlobal>

class RemoteEdit;
class RemoteEditProcess;
class RemoteEditWidget;

namespace tags {
struct NODISCARD RemoteSessionTag final
{};
} // namespace tags

using RemoteSession = TaggedStringLatin1<tags::RemoteSessionTag>;

// Internally shared across all view sessions
static const RemoteSession REMOTE_VIEW_SESSION_ID = RemoteSession("-1");

class NODISCARD_QOBJECT RemoteEditSession : public QObject
{
    Q_OBJECT

private:
    friend class RemoteEditExternalSession;
    friend class RemoteEditInternalSession;

private:
    bool m_connected = true;
    const uint32_t m_internalId = 0;
    const RemoteSession m_sessionId = REMOTE_VIEW_SESSION_ID;
    RemoteEdit *m_manager = nullptr;
    QString m_content;

public:
    explicit RemoteEditSession(uint32_t internalId,
                               const RemoteSession &sessionId,
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

public:
    explicit RemoteEditInternalSession(uint32_t internalId,
                                       const RemoteSession &sessionId,
                                       const QString &title,
                                       const QString &body,
                                       RemoteEdit *remoteEdit);
    ~RemoteEditInternalSession() final;

private:
    QScopedPointer<RemoteEditWidget> m_widget;
};

class NODISCARD_QOBJECT RemoteEditExternalSession final : public RemoteEditSession
{
    Q_OBJECT

public:
    explicit RemoteEditExternalSession(uint32_t internalId,
                                       const RemoteSession &sessionId,
                                       const QString &title,
                                       const QString &body,
                                       RemoteEdit *remoteEdit);
    ~RemoteEditExternalSession() final;

private:
    QScopedPointer<RemoteEditProcess> m_process;
};
