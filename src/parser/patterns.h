#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <QtCore>

class QByteArray;
class QString;

namespace Patterns {
extern bool matchScore(const QString &str);
extern bool matchNoDescriptionPatterns(const QString &);
extern bool matchPasswordPatterns(const QByteArray &);
extern bool matchLoginPatterns(const QByteArray &);
extern bool matchMenuPromptPatterns(const QByteArray &);
extern bool matchPattern(const QString &pattern, const QString &str);
} // namespace Patterns
