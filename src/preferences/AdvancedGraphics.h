#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../display/MapCanvasConfig.h"

#include <functional>
#include <memory>
#include <vector>

#include <QGroupBox>

class SliderSpinboxButton;
class AdvancedGraphicsGroupBox final : public QObject
{
    Q_OBJECT

private:
    friend SliderSpinboxButton;
    QGroupBox *m_groupBox;
    using UniqueSsb = std::unique_ptr<SliderSpinboxButton>;
    std::vector<UniqueSsb> m_ssbs;
    // purposely unused; this variable exists as an RAII for the change monitors.
    ConnectionSet m_connections;

public:
    explicit AdvancedGraphicsGroupBox(QGroupBox &groupBox);
    ~AdvancedGraphicsGroupBox() final;

public:
    explicit operator QGroupBox &() { return deref(m_groupBox); }
    NODISCARD QGroupBox *getGroupBox() { return m_groupBox; }

signals:
    void sig_graphicsSettingsChanged();

private:
    void graphicsSettingsChanged() { emit sig_graphicsSettingsChanged(); }
    void enableSsbs(bool enabled);
};
