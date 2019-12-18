#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QPointer>
#include <QString>
#include <QWidget>
#include <QtCore>

class QObject;
class QEvent;
class ClientTelnet;

namespace Ui {
class ClientWidget;
}

class ClientWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit ClientWidget(QWidget *parent = nullptr);
    ~ClientWidget() override;

public:
    bool isUsingClient() const;

signals:
    void relayMessage(const QString &);

public slots:
    void onVisibilityChanged(bool);
    void onShowMessage(const QString &);
    void saveLog();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::ClientWidget *ui = nullptr;

    QPointer<ClientTelnet> m_telnet;
};
