#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "../global/utils.h"

#include <memory>

#include <QPointer>
#include <QString>
#include <QWidget>
#include <QtCore>

class ClientTelnet;
class DisplayWidget;
class QEvent;
class QObject;
class StackedInputWidget;

struct ClientTelnetOutputs;
struct DisplayWidgetOutputs;
struct StackedInputWidgetOutputs;

namespace Ui {
class ClientWidget;
}

class NODISCARD_QOBJECT ClientWidget final : public QWidget
{
    Q_OBJECT

private:
    struct NODISCARD Pipeline final
    {
        struct NODISCARD Outputs final
        {
            std::unique_ptr<StackedInputWidgetOutputs> stackedInputWidgetOutputs;
            std::unique_ptr<DisplayWidgetOutputs> displayWidgetOutputs;
            std::unique_ptr<ClientTelnetOutputs> clientTelnetOutputs;
        };
        struct NODISCARD Objects final
        {
            std::unique_ptr<ClientTelnet> clientTelnet;
            std::unique_ptr<Ui::ClientWidget> ui;
        };

        Outputs outputs;
        Objects objs;

        ~Pipeline();
    };

    Pipeline m_pipeline;

public:
    explicit ClientWidget(QWidget *parent);
    ~ClientWidget() final;

private:
    void initPipeline();
    void initStackedInputWidget();
    void initDisplayWidget();
    void initClientTelnet();

private:
    NODISCARD Ui::ClientWidget &getUi() // NOLINT (no, it should not be const)
    {
        return deref(m_pipeline.objs.ui);
    }
    NODISCARD const Ui::ClientWidget &getUi() const { return deref(m_pipeline.objs.ui); }
    NODISCARD DisplayWidget &getDisplay();
    NODISCARD StackedInputWidget &getInput();
    NODISCARD ClientTelnet &getTelnet();

public:
    NODISCARD bool isUsingClient() const;

private:
    void relayMessage(const QString &msg) { emit sig_relayMessage(msg); }

protected:
    NODISCARD bool focusNextPrevChild(bool next) override;

signals:
    void sig_relayMessage(const QString &);

public slots:
    void slot_onVisibilityChanged(bool);
    void slot_onShowMessage(const QString &);
    void slot_saveLog();
};
