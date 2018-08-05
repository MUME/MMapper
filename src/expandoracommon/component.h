#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef COMPONENT
#define COMPONENT

#include <iostream>
#include <map>
#include <QObject>
#include <QString>
#include <QThread>
#include <QVariant>
#include <QtCore>

class Component;

#ifdef Q_OS_WIN
#define MY_EXPORT __declspec(dllexport)
#else
#define MY_EXPORT
#endif

class ComponentThreader : public QThread
{
private:
    Q_OBJECT;
    Component *owner = nullptr;

public:
    explicit ComponentThreader(Component *c)
        : owner(c)
    {}
    void run() override;
};

class Component : public QObject
{
    friend class ComponentThreader;

private:
    Q_OBJECT
    void runInit() { init(); }

protected:
    ComponentThreader *thread = nullptr;
    std::map<QString, QVariant> options{};
    virtual void init() {}

public:
    /* set the component to a running state
     * after passing any arguments via options the configuration
     * will call start.
     * INIT can be overloaded to carry one-time functionality
     * init will be called by start and will run in the correct thread
     * while start is used to figure out
     * which thread that is.
     */
    void start();

    virtual ~Component();
    explicit Component(bool threaded = false);
    void setOption(const QString &key, const QVariant &value);
};

#endif
