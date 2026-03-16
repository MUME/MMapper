#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../display/MapCanvasConfig.h"

#include <functional>
#include <memory>
#include <vector>

#include <QGroupBox>

class NODISCARD SliderSpinboxButton
{
public:
    virtual ~SliderSpinboxButton();
    virtual void setEnabled(bool enabled) = 0;
    virtual void forcedUpdate() = 0;
};

class NODISCARD_QOBJECT AdvancedGraphicsGroupBox final : public QObject
{
    Q_OBJECT

    template<int D>
    friend class SliderSpinboxButtonImpl;

private:
    QGroupBox *const m_groupBox;
    using UniqueSsb = std::unique_ptr<SliderSpinboxButton>;
    std::vector<UniqueSsb> m_globalSsbs;
    std::vector<UniqueSsb> m_3dSsbs;
    // purposely unused; this variable exists as an RAII for the change monitors.
    Signal2Lifetime m_lifetime;

public:
    explicit AdvancedGraphicsGroupBox(QGroupBox &groupBox);
    ~AdvancedGraphicsGroupBox() final;

public:
    explicit operator QGroupBox &() { return deref(m_groupBox); }
    NODISCARD QGroupBox *getGroupBox() { return m_groupBox; }

private:
    void graphicsSettingsChanged() { emit sig_graphicsSettingsChanged(); }
    void enableSsbs(bool enabled);

signals:
    void sig_graphicsSettingsChanged();
};
