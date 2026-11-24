// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "MumeEditOptionsJson.h"

#include <utility>

#include <QString>

MumeEditOptions parseMumeEditOptions(const JsonObj &obj)
{
    MumeEditOptions options;

    auto optMaxSize = obj.getInt("max-size");
    if (!optMaxSize) {
        optMaxSize = obj.getInt("max_size");
    }
    if (optMaxSize) {
        options.trySetMaxSize(optMaxSize.value());
    }

    if (auto optAllowed = obj.getArray("allowed-ranges")) {
        MumeEditOptions::AllowedRanges allowed;
        for (const auto &elem : optAllowed.value()) {
            if (auto optArr = elem.getArray()) {
                const auto &arr = optArr.value();
                if (arr.size() >= 2) {
                    if (auto startOpt = arr[0].getInt()) {
                        if (auto endOpt = arr[1].getInt()) {
                            allowed.tryAddRange(startOpt.value(), endOpt.value());
                        }
                    }
                }
            } else if (auto optStr = elem.getString()) {
                const QString str = optStr.value();
                if (str == QStringLiteral("ansi-sgr")) {
                    allowed.ansiSgr = true;
                } else if (str == QStringLiteral("emoji")) {
                    allowed.emoji = true;
                }
            }
        }
        options.allowed = std::move(allowed);
    }
    return options;
}
