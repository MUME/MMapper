/************************************************************************
**
** Authors:   Thomas Equeter <waba@waba.be>
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
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

#ifndef INCLUDED_STEPCOUNTER_H
#define INCLUDED_STEPCOUNTER_H

#include <QObject>

class ProgressCounter : public QObject
{
  Q_OBJECT

  quint32 m_totalSteps, m_steps, m_percentage;

public:
  ProgressCounter();
  ProgressCounter( QObject *parent );
  virtual ~ProgressCounter();

  void step( quint32 steps = 1 );
  void increaseTotalStepsBy( quint32 steps );
  void reset();

signals:
  void onPercentageChanged( quint32 );
};

#endif /* INCLUDED_STEPCOUNTER_H */
