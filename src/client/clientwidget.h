#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <QMainWindow>
#include <QSize>
#include <QString>
#include <QtCore>

class DisplayWidget;
class QCloseEvent;
class QEvent;
class QObject;
class QSplitter;
class QWidget;
class StackedInputWidget;
class ClientTelnet;

class ClientWidget final : public QMainWindow
{
private:
    Q_OBJECT

public:
    explicit ClientWidget(QWidget *parent = nullptr);
    ~ClientWidget() override;

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
    void writeSettings() const;

    bool m_connected = false;
    bool m_displayCopyAvailable = false;

    QSplitter *m_splitter = nullptr;
    DisplayWidget *m_display = nullptr;
    StackedInputWidget *m_input = nullptr;
    std::unique_ptr<ClientTelnet> m_telnet;
};
