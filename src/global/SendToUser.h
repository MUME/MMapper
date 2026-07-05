#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "AnsiOstream.h"
#include "Signal2.h"
#include "mm_source_location.h"

#include <exception>
#include <functional>

#include <QString>

class AnsiOstream;
namespace global {

void registerSendToUser(Signal2Lifetime &, Signal2<QString>::Function callback);

void sendToUser(const QString &str);
void sendToUser(const std::function<void(AnsiOstream &)> &f);
void sendExceptionToUser(const std::exception_ptr &, std::string_view when);

enum class NODISCARD LogDestEnum : uint8_t { Debug, Info, Warning };
void logAndSendToUser(LogDestEnum dest,
                      const std::function<void(AnsiOstream &)> &f,
                      mm::source_location loc = MM_SOURCE_LOCATION());

// logging only
void logOnly(LogDestEnum dest,
             const std::function<void(AnsiOstream &)> &f,
             mm::source_location loc = MM_SOURCE_LOCATION());
void logException(LogDestEnum dest,
                  const std::exception_ptr &,
                  mm::source_location loc = MM_SOURCE_LOCATION());

} // namespace global
