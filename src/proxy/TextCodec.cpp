// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "TextCodec.h"

#include "../configuration/configuration.h"
#include "../global/TextUtils.h"

#include <optional>
#include <QDebug>

struct NODISCARD TextCodec::Pimpl
{
    Pimpl() = default;
    virtual ~Pimpl();

private:
    virtual void virt_setEncodingForName(const std::string_view &encodingName) = 0;

private:
    NODISCARD virtual CharacterEncodingEnum virt_getEncoding() const = 0;
    NODISCARD virtual bool virt_supports(const std::string_view &encodingName) const = 0;
    NODISCARD virtual std::list<std::string_view> virt_supportedEncodings() const = 0;
    NODISCARD virtual std::string_view virt_getName() const = 0;

public:
    void setEncodingForName(const std::string_view &sv) { virt_setEncodingForName(sv); }

public:
    NODISCARD CharacterEncodingEnum getEncoding() const { return virt_getEncoding(); }
    NODISCARD bool supports(const std::string_view &sv) const { return virt_supports(sv); }
    NODISCARD std::list<std::string_view> supportedEncodings() const
    {
        return virt_supportedEncodings();
    }
    NODISCARD std::string_view getName() const { return virt_getName(); }
};

TextCodec::Pimpl::~Pimpl() = default;

class AutoSelectTextCodec final : public TextCodec::Pimpl
{
private:
    std::optional<CharacterEncodingEnum> opt;

public:
    AutoSelectTextCodec() = default;
    ~AutoSelectTextCodec() final;

private:
    void virt_setEncodingForName(const std::string_view &sv) final
    {
        const auto upper = ::toUpperLatin1(sv);
        if (ENCODING_LATIN_1 == upper)
            opt = CharacterEncodingEnum::LATIN1;
        else if (ENCODING_UTF_8 == upper)
            opt = CharacterEncodingEnum::UTF8;
        else if (ENCODING_US_ASCII == upper)
            opt = CharacterEncodingEnum::ASCII;
        else
            qWarning() << "Refusing to autoselect to an unsupported codec"
                       << ::toQByteArrayLatin1(upper);
    }

private:
    NODISCARD CharacterEncodingEnum virt_getEncoding() const final
    {
        return opt.value_or(getConfig().general.characterEncoding);
    }
    NODISCARD bool virt_supports(const std::string_view &sv) const final
    {
        const auto upper = ::toUpperLatin1(sv);
        for (const auto &supported : supportedEncodings()) {
            if (supported == upper) {
                return true;
            }
        }
        return false;
    }
    NODISCARD std::list<std::string_view> virt_supportedEncodings() const final
    {
        // Prefer Latin-1 over UTF-8 since MUME only understands Latin-1
        return {ENCODING_LATIN_1, ENCODING_UTF_8, ENCODING_US_ASCII};
    }
    NODISCARD std::string_view virt_getName() const final
    {
        switch (opt.value_or(getConfig().general.characterEncoding)) {
        case CharacterEncodingEnum::UTF8:
            return ENCODING_UTF_8;
        case CharacterEncodingEnum::ASCII:
            return ENCODING_US_ASCII;
        case CharacterEncodingEnum::LATIN1:
            return ENCODING_LATIN_1;
        }
        abort();
    }
};

AutoSelectTextCodec::~AutoSelectTextCodec() = default;

class ForcedTextCodec final : public TextCodec::Pimpl
{
private:
    const CharacterEncodingEnum encoding;
    const std::string_view name;

public:
    ForcedTextCodec(const CharacterEncodingEnum encoding, const std::string_view name)
        : encoding(encoding)
        , name(name)
    {}
    ~ForcedTextCodec() final;

private:
    void virt_setEncodingForName(const std::string_view &sv) final
    {
        const auto upper = ::toUpperLatin1(sv);
        if (getName() != upper)
            qWarning() << "Refusing to switch to an unforced codec" << ::toQByteArrayLatin1(upper);
    }

private:
    NODISCARD CharacterEncodingEnum virt_getEncoding() const final { return encoding; }
    NODISCARD bool virt_supports(const std::string_view &sv) const final
    {
        return name == ::toUpperLatin1(sv);
    }
    NODISCARD std::list<std::string_view> virt_supportedEncodings() const final { return {name}; }
    NODISCARD std::string_view virt_getName() const final { return name; }
};

ForcedTextCodec::~ForcedTextCodec() = default;

TextCodec::TextCodec(const TextCodecStrategyEnum strategy)
{
    switch (strategy) {
    case TextCodecStrategyEnum::AUTO_SELECT_CODEC:
        m_pimpl = std::make_unique<AutoSelectTextCodec>();
        break;
    case TextCodecStrategyEnum::FORCE_LATIN_1:
        m_pimpl = std::make_unique<ForcedTextCodec>(CharacterEncodingEnum::LATIN1, ENCODING_LATIN_1);
        break;
    case TextCodecStrategyEnum::FORCE_UTF_8:
        m_pimpl = std::make_unique<ForcedTextCodec>(CharacterEncodingEnum::UTF8, ENCODING_UTF_8);
        break;
    case TextCodecStrategyEnum::FORCE_US_ASCII:
        m_pimpl = std::make_unique<ForcedTextCodec>(CharacterEncodingEnum::ASCII, ENCODING_US_ASCII);
        break;
    default:
        abort();
    }
}

TextCodec::~TextCodec() = default;

void TextCodec::setEncodingForName(const std::string_view &sv)
{
    m_pimpl->setEncodingForName(sv);
}

CharacterEncodingEnum TextCodec::getEncoding() const
{
    return m_pimpl->getEncoding();
}

bool TextCodec::supports(const std::string_view &sv) const
{
    return m_pimpl->supports(sv);
}

std::list<std::string_view> TextCodec::supportedEncodings() const
{
    return m_pimpl->supportedEncodings();
}

std::string_view TextCodec::getName() const
{
    return m_pimpl->getName();
}
