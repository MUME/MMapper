#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

#include <memory>

#include <QByteArray>
#include <QString>
#include <QVariantMap>

class CGroup;

class NODISCARD GroupAction
{
protected:
    CGroup *m_group = nullptr;

public:
    GroupAction();
    virtual ~GroupAction();

private:
    virtual void virt_exec() = 0;

public:
    void exec() { virt_exec(); }
    void setGroup(CGroup *in) { this->m_group = in; }
    void schedule(CGroup *in) { setGroup(in); }
};

class NODISCARD AddCharacter final : public GroupAction
{
private:
    QVariantMap m_map;

public:
    explicit AddCharacter(const QVariantMap &map);

private:
    void virt_exec() final;
};

class NODISCARD RemoveCharacter final : public GroupAction
{
private:
    QString m_name;

public:
    explicit RemoveCharacter(const QVariantMap &variant);
    explicit RemoveCharacter(QByteArray) = delete;
    explicit RemoveCharacter(QString);

private:
    void virt_exec() final;
};

class NODISCARD UpdateCharacter final : public GroupAction
{
private:
    QVariantMap m_map;

public:
    explicit UpdateCharacter(QVariantMap variant);

private:
    void virt_exec() final;
};

class NODISCARD RenameCharacter final : public GroupAction
{
private:
    QVariantMap m_map;

public:
    explicit RenameCharacter(QVariantMap variant);

private:
    void virt_exec() final;
};

class NODISCARD ResetCharacters final : public GroupAction
{
public:
    ResetCharacters();

private:
    void virt_exec() final;
};
