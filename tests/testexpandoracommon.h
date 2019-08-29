#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <QObject>

class TestExpandoraCommon : public QObject
{
    Q_OBJECT
public:
    TestExpandoraCommon();
    ~TestExpandoraCommon() override;

private:
private Q_SLOTS:
    void skippablePropertyTest();
    void stringPropertyTest();
};
