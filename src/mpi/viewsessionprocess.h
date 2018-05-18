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

#ifndef _VIEWSESSIONPROCESS_H_
#define _VIEWSESSIONPROCESS_H_

#include <QProcess>
#include <QTemporaryFile>

class ViewSessionProcess: public QProcess
{
    Q_OBJECT

public:
    ViewSessionProcess(int key, QString title, QString body, QObject *parent = 0);
    virtual ~ViewSessionProcess();

protected slots:
    virtual void onError(QProcess::ProcessError);
    virtual void onFinished(int, QProcess::ExitStatus);

protected:
    QStringList splitCommandLine(const QString &cmdLine);

    int m_key;
    QString m_title;
    QString m_body;

    QTemporaryFile m_file;

};

#endif /* _VIEWSESSIONPROCESS_H_ */
