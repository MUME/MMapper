#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

#include <memory>

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

class NODISCARD_QOBJECT ClientWidget final : public QWidget
{
    Q_OBJECT

private:
    const std::unique_ptr<Ui::ClientWidget> m_ui;
    QPointer<ClientTelnet> m_telnet;

public:
    explicit ClientWidget(QWidget *parent);
    ~ClientWidget() final;

public:
    NODISCARD bool isUsingClient() const;

signals:
    void sig_relayMessage(const QString &);

public slots:
    void slot_onVisibilityChanged(bool);
    void slot_onShowMessage(const QString &);
    void slot_saveLog();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};
