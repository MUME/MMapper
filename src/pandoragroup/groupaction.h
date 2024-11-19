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
public:
    GroupAction();
    virtual ~GroupAction();

private:
    virtual void virt_exec() = 0;

public:
    void exec() { virt_exec(); }
    void setGroup(CGroup *in) { this->group = in; }
    void schedule(CGroup *in) { setGroup(in); }

protected:
    CGroup *group = nullptr;
};

class NODISCARD AddCharacter final : public GroupAction
{
public:
    explicit AddCharacter(const QVariantMap &map);

private:
    void virt_exec() final;

private:
    QVariantMap map;
};

class NODISCARD RemoveCharacter final : public GroupAction
{
public:
    explicit RemoveCharacter(const QVariantMap &variant);
    explicit RemoveCharacter(QByteArray);

private:
    void virt_exec() final;

private:
    QByteArray name;
};

class NODISCARD UpdateCharacter final : public GroupAction
{
public:
    explicit UpdateCharacter(const QVariantMap &variant);

private:
    void virt_exec() final;

private:
    QVariantMap map;
};

class NODISCARD RenameCharacter final : public GroupAction
{
public:
    explicit RenameCharacter(const QVariantMap &variant);

private:
    void virt_exec() final;

private:
    QVariantMap map;
};

class NODISCARD ResetCharacters final : public GroupAction
{
public:
    ResetCharacters();

private:
    void virt_exec() final;
};
