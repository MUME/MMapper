#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QByteArray>
#include <QByteArrayDataPtr>
#include <QString>
#include <QVariantMap>

class CGroup;

class GroupAction
{
public:
    GroupAction();
    virtual ~GroupAction();
    virtual void exec() = 0;
    void setGroup(CGroup *in) { this->group = in; }
    void schedule(CGroup *in) { setGroup(in); }

protected:
    CGroup *group = nullptr;
};

class AddCharacter final : public GroupAction
{
public:
    explicit AddCharacter(const QVariantMap &map);

protected:
    void exec() override;

private:
    QVariantMap map;
};

class RemoveCharacter final : public GroupAction
{
public:
    explicit RemoveCharacter(const QVariantMap &variant);
    explicit RemoveCharacter(QByteArray);

protected:
    void exec() override;

private:
    QByteArray name;
};

class UpdateCharacter final : public GroupAction
{
public:
    explicit UpdateCharacter(const QVariantMap &variant);

protected:
    void exec() override;

private:
    QVariantMap map;
};

class RenameCharacter final : public GroupAction
{
public:
    explicit RenameCharacter(const QVariantMap &variant);

protected:
    void exec() override;

private:
    QVariantMap map;
};

class ResetCharacters final : public GroupAction
{
public:
    ResetCharacters();

protected:
    void exec() override;
};
