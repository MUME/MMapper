/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

#ifndef COMPONENT
#define COMPONENT

#include <qobject.h>
#include <qthread.h>
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
  void setOption(const QString & key, const QVariant & value) {options[key] = value;}


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
