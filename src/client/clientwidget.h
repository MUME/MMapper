/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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

#ifndef CLIENTWIDGET_H
#define CLIENTWIDGET_H

#include <QDialog>

class StackedInputWidget;
class DisplayWidget;

class QCloseEvent;
class QSplitter;
class QStatusBar;
class cTelnet;

class ClientWidget : public QDialog
{
    Q_OBJECT

public:
    ClientWidget(QWidget *parent = 0);
    ~ClientWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

signals:
    void sendToUser(const QString &);

public slots:
    void closeEvent(QCloseEvent *event);
    void connectToHost();
    void disconnectFromHost();
    void onConnected();
    void onDisconnected();
    void onSocketError(const QString &errorStr);
    void sendToMud(const QByteArray &);

private:
    void readSettings();
    void writeSettings();

    bool m_connected;

    QSplitter *m_splitter;
    DisplayWidget *m_display;
    StackedInputWidget *m_input;
    cTelnet *m_telnet;
    QStatusBar *m_statusBar;
};

#endif /* CLIENTWIDGET_H */
