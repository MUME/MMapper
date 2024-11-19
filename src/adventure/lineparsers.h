#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "../global/macros.h"

#include <optional>
#include <utility>

#include <QString>

struct NODISCARD LineParserResult final : private std::optional<QString>
{
    using base = std::optional<QString>;
    using base::base;
    using base::operator bool;
    using base::has_value;
    using base::value;

    NODISCARD bool operator==(const LineParserResult &rhs) const
    {
        const bool ok = has_value();
        if (ok != rhs.has_value()) {
            return false;
        }
        return !ok || value() == rhs.value();
    }
    NODISCARD bool operator!=(const LineParserResult &rhs) const { return !(*this == rhs); }
};

namespace AccomplishedTaskParser {
NODISCARD extern bool parse(const QString &line);
}

namespace AchievementParser {
NODISCARD extern LineParserResult parse(const QString &prev, const QString &line);
}

namespace DiedParser {
NODISCARD extern bool parse(const QString &line);
}

namespace GainedLevelParser {
NODISCARD extern bool parse(const QString &line);
}

namespace HintParser {
NODISCARD extern LineParserResult parse(const QString &prev, const QString &line);
}

class NODISCARD KillAndXPParser final
{
private:
    QString m_lastSuccessVal;
    int m_linesSinceShareExp = 0;
    bool m_pending = false;

public:
    NODISCARD LineParserResult parse(const QString &line);
};
