#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QDebug>
#include <QString>

#include "../global/macros.h"

class NODISCARD QuotedQString final
{
private:
    QString str;

public:
    explicit QuotedQString(QString input)
        : str{std::move(input)}
    {}

    friend QDebug &operator<<(QDebug &debug, const QuotedQString &q)
    {
        debug.quote();
        debug << q.str;
        debug.noquote();
        return debug;
    }
};
