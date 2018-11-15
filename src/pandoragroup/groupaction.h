#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef GROUPACTION_H
#define GROUPACTION_H

#include <QByteArray>
#include <QByteArrayDataPtr>
#include <QString>
#include <QVariantMap>

class CGroup;

class GroupAction
{
public:
    explicit GroupAction();
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
    QVariantMap map{};
};

class RemoveCharacter final : public GroupAction
{
public:
    explicit RemoveCharacter(const QVariantMap &variant);
    explicit RemoveCharacter(QByteArray);

protected:
    void exec() override;

private:
    QByteArray name{};
};

class UpdateCharacter final : public GroupAction
{
public:
    explicit UpdateCharacter(const QVariantMap &variant);

protected:
    void exec() override;

private:
    QVariantMap map{};
};

class RenameCharacter final : public GroupAction
{
public:
    explicit RenameCharacter(const QVariantMap &variant);

protected:
    void exec() override;

private:
    QVariantMap map{};
};

class ResetCharacters final : public GroupAction
{
public:
    explicit ResetCharacters();

protected:
    void exec() override;
};

#endif // GROUPACTION_H
