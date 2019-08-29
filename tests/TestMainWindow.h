#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <QObject>

class TestMainWindow : public QObject
{
    Q_OBJECT
public:
    TestMainWindow();
    ~TestMainWindow();

private:
private Q_SLOTS:
    void updaterTest();
};
