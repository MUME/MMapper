/************************************************************************
**
** Authors:   Thomas Equeter <waba@waba.be>
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
