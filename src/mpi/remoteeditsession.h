#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <algorithm>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QtCore>
#include <QtGlobal>

class RemoteEdit;
class RemoteEditProcess;
class RemoteEditWidget;

static constexpr const int REMOTE_EDIT_VIEW_KEY = -1;

class RemoteEditSession : public QObject
{
    Q_OBJECT

    friend class RemoteEditExternalSession;
    friend class RemoteEditInternalSession;

public:
    explicit RemoteEditSession(uint sessionId, int key, RemoteEdit *remoteEdit);

    auto getId() const { return m_sessionId; }

    auto getKey() const { return m_key; }

    bool isEditSession() const { return m_key != REMOTE_EDIT_VIEW_KEY; }

    const QString &getContent() const { return m_content; }

    void setContent(QString content) { m_content = std::move(content); }

    void cancel();
    void save();

protected slots:
    void onCancel() { cancel(); }
    void onSave(const QString &content)
    {
        setContent(content);
        save();
    }

private:
    const uint m_sessionId = 0;
    const int m_key = REMOTE_EDIT_VIEW_KEY;
    RemoteEdit *m_manager = nullptr;
    QString m_content{};
};

class RemoteEditInternalSession : public RemoteEditSession
{
    Q_OBJECT
public:
    explicit RemoteEditInternalSession(
        uint sessionId, int key, const QString &title, const QString &body, RemoteEdit *remoteEdit);
    ~RemoteEditInternalSession();

private:
    QScopedPointer<RemoteEditWidget> m_widget;
};

class RemoteEditExternalSession : public RemoteEditSession
{
    Q_OBJECT
public:
    explicit RemoteEditExternalSession(
        uint sessionId, int key, const QString &title, const QString &body, RemoteEdit *remoteEdit);
    ~RemoteEditExternalSession();

private:
    QScopedPointer<RemoteEditProcess> m_process;
};
