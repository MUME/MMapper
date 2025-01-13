#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Consts.h"
#include "NullPointerException.h"
#include "RuleOf5.h"
#include "TextUtils.h"

#include <cassert>
#include <cstring>
#include <memory>
#include <optional>
#include <string>

#include <QByteArray>
#include <QString>

enum class NODISCARD EncodingEnum { Latin1, Utf8 };

template<EncodingEnum Encoding_>
struct NODISCARD EncodedStdStringView final
{
    std::string_view sv;
    static inline constexpr EncodingEnum getEncoding() { return Encoding_; }
    explicit EncodedStdStringView(std::string_view sv_)
        : sv{sv_}
    {}
};

template<EncodingEnum Encoding_, typename Tag_>
class NODISCARD TaggedString
{
public:
    using Tag = Tag_;
    static inline constexpr const EncodingEnum Encoding = Encoding_;

private:
    std::string m_str;

public:
    TaggedString() = default;
    explicit TaggedString(std::nullptr_t) = delete;
    explicit TaggedString(const char *const s)
        : m_str((s == nullptr) ? "" : s)
    {
        assert(s != nullptr);
    }
    template<size_t N>
    explicit TaggedString(const char (&s)[N])
        : m_str{std::string{s}}
    {
        assert(s != nullptr);
    }
    explicit TaggedString(std::string s)
        : m_str(std::move(s))
    {}
    explicit TaggedString(const QString &s) = delete;
    DEFAULT_RULE_OF_5(TaggedString);

protected:
    NODISCARD const std::string &getRawStdString() const & { return m_str; }
    NODISCARD std::string getRawStdString() && { return std::exchange(m_str, {}); }

public:
    NODISCARD bool empty() const { return m_str.empty(); }
    NODISCARD bool isEmpty() const { return empty(); }

public:
    NODISCARD bool operator==(const TaggedString &rhs) const { return m_str == rhs.m_str; }
    NODISCARD bool operator!=(const TaggedString &rhs) const { return !(rhs == *this); }

public:
    NODISCARD
    QByteArray toQByteArray() const
    {
        if constexpr (Encoding == EncodingEnum::Latin1)
            return mmqt::toQByteArrayLatin1(m_str);
        else if constexpr (Encoding == EncodingEnum::Utf8)
            return mmqt::toQByteArrayUtf8(m_str);
        else
            std::abort();
    }
    NODISCARD
    QString toQString() const
    {
        if constexpr (Encoding == EncodingEnum::Latin1)
            return mmqt::toQStringLatin1(m_str);
        else if constexpr (Encoding == EncodingEnum::Utf8)
            return mmqt::toQStringUtf8(m_str);
        else
            std::abort();
    }

public:
    NODISCARD
    friend auto operator+(const TaggedString &taggedString, const QString &qs)
    {
        return taggedString.toQString() + qs;
    }
    NODISCARD
    friend auto operator+(const QString &qs, const TaggedString &taggedString)
    {
        return qs + taggedString.toQString();
    }
};

// Latin1
template<typename Tag_>
class NODISCARD TaggedStringLatin1 : public TaggedString<EncodingEnum::Latin1, Tag_>
{
private:
    using Base = TaggedString<EncodingEnum::Latin1, Tag_>;

public:
    using Base::Base;
    explicit TaggedStringLatin1(const QString &s)
        : Base{mmqt::toStdStringLatin1(s)}
    {}
    DEFAULT_RULE_OF_5(TaggedStringLatin1);

public:
    NODISCARD const std::string &getStdStringLatin1() const & { return Base::getRawStdString(); }
    NODISCARD std::string getStdStringLatin1() && { return std::move(*this).getRawStdString(); }
};

// Utf8
template<typename Tag_>
class NODISCARD TaggedStringUtf8 : public TaggedString<EncodingEnum::Utf8, Tag_>
{
private:
    using Base = TaggedString<EncodingEnum::Utf8, Tag_>;

public:
    using Base::Base;
    TaggedStringUtf8() = default;
    explicit TaggedStringUtf8(const QString &s)
        : Base{mmqt::toStdStringUtf8(s)}
    {}
    DEFAULT_RULE_OF_5(TaggedStringUtf8);

public:
    NODISCARD const std::string &getStdStringUtf8() const & { return Base::getRawStdString(); }
    NODISCARD std::string getStdStringUtf8() && { return std::move(*this).getRawStdString(); }
};

// Latin1
template<EncodingEnum Encoding_, typename Tag_>
class NODISCARD TaggedBoxedString
{
public:
    using Tag = Tag_;
    static inline constexpr const EncodingEnum Encoding = Encoding_;

private:
    std::shared_ptr<const char[]> m_ptr;
    std::string_view m_view;

public:
    TaggedBoxedString() = default;
    explicit TaggedBoxedString(std::nullptr_t) = delete;
    explicit TaggedBoxedString(const char *const s)
        : TaggedBoxedString{std::string_view{(s == nullptr) ? "" : s}}
    {
        if (s == nullptr)
            throw NullPointerException();
    }
    explicit TaggedBoxedString(const std::string &s)
        : TaggedBoxedString{std::string_view{s}}
    {}

    explicit TaggedBoxedString(const std::string_view sv)
    {
        const size_t len = sv.size();
        char *const buf = new char[len + 1];
        std::memcpy(buf, sv.data(), len);
        buf[len] = char_consts::C_NUL;

        /* give ownership of the pointer to shared_ptr */
        *this = TaggedBoxedString{std::shared_ptr<const char[]>(buf), len};

        assert(m_view == sv);
    }
    explicit TaggedBoxedString(std::shared_ptr<const char[]> ptr, const size_t len)
        : m_ptr{std::move(ptr)}
        , m_view{m_ptr.get(), len}
    {
        if (m_ptr == nullptr)
            throw NullPointerException();

        if (m_view.data()[len] != char_consts::C_NUL)
            throw std::runtime_error("not null terminated");
    }

    // NOTE: call mmqt::makeXXX() or use mmqt::toStdStringLatin1()
    explicit TaggedBoxedString(const QString &s) = delete;
    DEFAULT_RULE_OF_5(TaggedBoxedString);

protected:
    NODISCARD std::string_view getRawStdStringView() const { return m_view; }
    NODISCARD std::string toRawStdString() const { return std::string{m_view}; }

public:
    NODISCARD bool operator==(const TaggedBoxedString &rhs) const
    {
        if (m_view.size() != rhs.m_view.size())
            return false;
        // string_view doesn't have this short circuit!
        if (reinterpret_cast<uintptr_t>(m_view.data())
            == reinterpret_cast<uintptr_t>(rhs.m_view.data()))
            return true;
        return m_view == rhs.m_view;
    }
    NODISCARD bool operator!=(const TaggedBoxedString &rhs) const { return !(rhs == *this); }

public:
    NODISCARD bool empty() const { return m_view.empty(); }
    NODISCARD bool isEmpty() const { return empty(); }

public:
    NODISCARD bool operator<(const TaggedBoxedString &rhs) const { return m_view < rhs.m_view; }
    NODISCARD bool operator>(const TaggedBoxedString &rhs) const { return rhs < *this; }
    NODISCARD bool operator<=(const TaggedBoxedString &rhs) const { return !(rhs < *this); }
    NODISCARD bool operator>=(const TaggedBoxedString &rhs) const { return !(*this < rhs); }
};

// Latin1
template<typename Tag_>
class NODISCARD TaggedBoxedStringLatin1 : public TaggedBoxedString<EncodingEnum::Latin1, Tag_>
{
private:
    using Base = TaggedBoxedString<EncodingEnum::Latin1, Tag_>;

public:
    using Base::Base;
    explicit TaggedBoxedStringLatin1(const QString &s)
        : Base{mmqt::toStdStringLatin1(s)}
    {}
    DEFAULT_RULE_OF_5(TaggedBoxedStringLatin1);

public:
    template<typename U>
    explicit TaggedBoxedStringLatin1(TaggedStringLatin1<U> s)
        : TaggedBoxedStringLatin1{std::move(s).getStdStringLatin1()}
    {}

public:
    NODISCARD std::string_view getStdStringViewLatin1() const &
    {
        return Base::getRawStdStringView();
    }
    NODISCARD std::string toStdStringLatin1() const & { return Base::toRawStdString(); }

public:
    NODISCARD QByteArray toQByteArray() const
    {
        return mmqt::toQByteArrayLatin1(getStdStringViewLatin1());
    }
    NODISCARD QString toQString() const { return mmqt::toQStringLatin1(getStdStringViewLatin1()); }

public:
    NODISCARD
    friend auto operator+(const TaggedBoxedStringLatin1 &taggedString, const QString &qs)
    {
        return taggedString.toQString() + qs;
    }
    NODISCARD
    friend auto operator+(const QString &qs, const TaggedBoxedStringLatin1 &taggedString)
    {
        return qs + taggedString.toQString();
    }
};

// Utf8
template<typename Tag_>
class NODISCARD TaggedBoxedStringUtf8 : public TaggedBoxedString<EncodingEnum::Utf8, Tag_>
{
private:
    using Base = TaggedBoxedString<EncodingEnum::Utf8, Tag_>;

public:
    using Base::Base;
    explicit TaggedBoxedStringUtf8(const QString &s)
        : Base{mmqt::toStdStringUtf8(s)}
    {}
    DEFAULT_RULE_OF_5(TaggedBoxedStringUtf8);

public:
    template<typename U>
    explicit TaggedBoxedStringUtf8(TaggedStringUtf8<U> s)
        : TaggedBoxedStringUtf8{std::move(s).getStdStringUtf8()}
    {}

public:
    NODISCARD std::string_view getStdStringViewUtf8() const &
    {
        return Base::getRawStdStringView();
    }
    NODISCARD std::string toStdStringUtf8() && { return std::move(*this).toRawStdString(); }

public:
    NODISCARD QByteArray toQByteArray() const
    {
        return mmqt::toQByteArrayUtf8(getStdStringViewUtf8());
    }
    NODISCARD QString toQString() const { return mmqt::toQStringUtf8(getStdStringViewUtf8()); }
};
