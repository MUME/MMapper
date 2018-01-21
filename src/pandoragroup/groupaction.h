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

#include <QString>
#include <QDomNode>

class CGroup;

class GroupAction
{
public:
    GroupAction() {}
    virtual ~GroupAction() {}
    virtual void exec() = 0;
    void setGroup(CGroup *in)
    {
        this->group = in;
    }
    void schedule(CGroup *in)
    {
        setGroup(in);
    }
protected:
    CGroup *group;
};

class AddCharacter : public GroupAction
{
public:
    AddCharacter(QDomNode blob);
protected:
    void exec();
private:
    QDomNode blob;
};

class RemoveCharacter : public GroupAction
{
public:
    RemoveCharacter(QDomNode blob);
    RemoveCharacter(const QByteArray &);
protected:
    void exec();
private:
    QByteArray name;
};

class UpdateCharacter : public GroupAction
{
public:
    UpdateCharacter(QDomNode blob);
protected:
    void exec();
private:
    QDomNode blob;
};

class RenameCharacter : public GroupAction
{
public:
    RenameCharacter(QDomNode blob);
protected:
    void exec();
private:
    QDomNode blob;
};

class ResetCharacters : public GroupAction
{
public:
    ResetCharacters();
protected:
    void exec();
};

#endif // GROUPACTION_H
