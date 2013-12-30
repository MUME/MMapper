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

#include <QThread>
#include <QVariant>
#include <iostream>
#include <map>

#ifdef Q_WS_WIN
# define MY_EXPORT __declspec(dllexport)
#else
# define MY_EXPORT
#endif

class Component;

class ComponentThreader : public QThread
{
private:
  Q_OBJECT;
  Component * owner;

public:
  ComponentThreader(Component * c) : owner(c) {}
  void run();
};

class Component : public QObject
{
  friend class ComponentThreader;
private:
  Q_OBJECT
  void runInit() {init();}

protected:
  ComponentThreader * thread;
  std::map<QString, QVariant> options;
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
  virtual Qt::ConnectionType requiredConnectionType(const QString &) {return Qt::AutoCompatConnection;}
  Component(bool threaded = false);
  void setOption(const QString & key, const QVariant & value);
};

/**
 * every component that should be available from a library should inherit Component
 * and implement a componentCreator which is available via "extern "C" MY_EXPORT ..."
 */
typedef Component * (*componentCreator)();

class ComponentCreator
{
public:
  virtual Component * create() = 0;
  virtual ~ComponentCreator() {}
  static std::map<QString, ComponentCreator *> & creators();
};

template <class T>
class Initializer : public ComponentCreator
{

public:
  Initializer(QString name)
  {
    creators()[name] = this;
  }
  T * create()
  {
    return new T;
  }
};



#ifdef DMALLOC
#include <mpatrol.h>
#endif

#endif
