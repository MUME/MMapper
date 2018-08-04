#pragma once
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
#include <QSize>
#include <QString>
#include <QtCore>

class DisplayWidget;
class QCloseEvent;
class QEvent;
class QObject;
class QSplitter;
class QStatusBar;
class QWidget;
class StackedInputWidget;
class cTelnet;

class ClientWidget final : public QDialog
{
private:
    Q_OBJECT

public:
    explicit ClientWidget(QWidget *parent = nullptr);
    ~ClientWidget();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

signals:
    void sendToUser(const QString &);

public slots:
    void closeEvent(QCloseEvent *event) override;
    void connectToHost();
    void disconnectFromHost();
    void saveLog();
    void onConnected();
    void onDisconnected();
    void onSocketError(const QString &);
    void sendToMud(const QString &);
    void copy();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void readSettings();
    void writeSettings();

    bool m_connected = false;
    bool m_displayCopyAvailable = false;

    QSplitter *m_splitter = nullptr;
    DisplayWidget *m_display = nullptr;
    StackedInputWidget *m_input = nullptr;
    cTelnet *m_telnet = nullptr;
    QStatusBar *m_statusBar = nullptr;
};

#endif /* CLIENTWIDGET_H */
