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

#ifndef _EDITSESSIONPROCESS_H_
#define _EDITSESSIONPROCESS_H_

#include "viewsessionprocess.h"

#include <QDateTime>

class EditSessionProcess : public ViewSessionProcess
{
    Q_OBJECT

public:
    EditSessionProcess(int key, const QString &title, const QString &body, QObject *parent = 0);
    ~EditSessionProcess();

protected slots:
    void onError(QProcess::ProcessError);
    void onFinished(int, QProcess::ExitStatus);

protected:
    void cancelEdit();
    void finishEdit();

private:
    QDateTime m_previousTime;

signals:
    void cancel(const int);
    void save(const QString &, const int);
};

#endif /* _EDITSESSIONPROCESS_H_ */
