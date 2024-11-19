#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "../global/macros.h"

#include <QObject>

// Added this one-off helper file since Qt MOC couldn't handle all the MACROs in Configuration.h
class ConfigObserver final : public QObject
{
    Q_OBJECT

public:
    NODISCARD static ConfigObserver &get()
    {
        static ConfigObserver singleton;
        return singleton;
    }

signals:
    void sig_configChanged(const std::type_info &configGroup);

private:
    ConfigObserver() = default;
};
