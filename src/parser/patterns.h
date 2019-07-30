#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <QtCore>

class QByteArray;
class QString;

class Patterns
{
public:
    static bool matchScore(const QString &str);
    static bool matchNoDescriptionPatterns(const QString &);
    static bool matchPasswordPatterns(const QByteArray &);
    static bool matchLoginPatterns(const QByteArray &);
    static bool matchMenuPromptPatterns(const QByteArray &);

    static bool matchPattern(QString pattern, const QString &str);
    static bool matchPattern(QByteArray pattern, const QByteArray &str);
};
