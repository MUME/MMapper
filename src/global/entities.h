#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include <functional>
#include <QByteArray>
#include <QString>

static constexpr const uint32_t MAX_UNICODE_CODEPOINT = 0x10FFFFu;

class OptQChar final
{
private:
    QChar qc{};
    bool valid = false;

public:
    explicit OptQChar() = default;
    explicit OptQChar(QChar _qc)
        : qc{_qc}
        , valid{true}
    {}
    explicit operator bool() const { return valid; }
    QChar value() const { return qc; }
};

namespace entities {
struct EncodedLatin1;
/// Without entities
struct DecodedUnicode : QString
{
    using QString::QString;
    explicit DecodedUnicode(const char *s)
        : QString{QString::fromLatin1(s)}
    {}
    explicit DecodedUnicode(QString s)
        : QString{std::move(s)}
    {}

    DecodedUnicode(QByteArray) = delete;
    DecodedUnicode(QByteArray &&) = delete;
    DecodedUnicode(const QByteArray &) = delete;
    DecodedUnicode(EncodedLatin1) = delete;
    DecodedUnicode(EncodedLatin1 &&) = delete;
    DecodedUnicode(const EncodedLatin1 &) = delete;
};
/// With entities
struct EncodedLatin1 : QByteArray
{
    using QByteArray::QByteArray;
    explicit EncodedLatin1(const char *s)
        : QByteArray{s}
    {}
    explicit EncodedLatin1(QByteArray s)
        : QByteArray{std::move(s)}
    {}
    EncodedLatin1(QString) = delete;
    EncodedLatin1(QString &&) = delete;
    EncodedLatin1(const QString &) = delete;
    EncodedLatin1(DecodedUnicode) = delete;
    EncodedLatin1(DecodedUnicode &&) = delete;
    EncodedLatin1(const DecodedUnicode &) = delete;
};

enum class EncodingType { Translit, Lossless };
EncodedLatin1 encode(const DecodedUnicode &name, EncodingType encodingType = EncodingType::Translit);
DecodedUnicode decode(const EncodedLatin1 &input);

struct EntityCallback
{
    virtual ~EntityCallback();
    virtual void decodedEntity(int start, int len, OptQChar decoded) = 0;
};
void foreachEntity(const QStringRef &line, EntityCallback &callback);

} // namespace entities
