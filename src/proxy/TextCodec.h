#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../configuration/configuration.h"
#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <cstdint>
#include <list>
#include <memory>
#include <string_view>

// supported IANA character sets
static constexpr const char *const ENCODING_LATIN_1 = "ISO-8859-1";
static constexpr const char *const ENCODING_UTF_8 = "UTF-8";
static constexpr const char *const ENCODING_US_ASCII = "US-ASCII";

enum class NODISCARD TextCodecStrategyEnum : uint8_t {
    FORCE_US_ASCII,
    FORCE_LATIN_1,
    FORCE_UTF_8,
    AUTO_SELECT_CODEC
};

class NODISCARD TextCodec final
{
public:
    struct Pimpl;

private:
    std::unique_ptr<Pimpl> m_pimpl;

public:
    explicit TextCodec(TextCodecStrategyEnum textCodecStrategy);
    ~TextCodec();
    DELETE_CTORS_AND_ASSIGN_OPS(TextCodec);

public:
    void setEncodingForName(const std::string_view &encodingName);

public:
    NODISCARD CharacterEncodingEnum getEncoding() const;
    NODISCARD bool supports(const std::string_view &encodingName) const;
    NODISCARD std::list<std::string_view> supportedEncodings() const;
    NODISCARD std::string_view getName() const;
};
