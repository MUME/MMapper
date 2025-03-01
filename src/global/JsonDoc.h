#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/TaggedString.h"
#include "../global/macros.h"
#include "JsonArray.h"
#include "JsonObj.h"
#include "JsonValue.h"

#include <optional>
#include <string>

#include <QJsonDocument>

template<typename Tag_>
class NODISCARD JsonDoc final
{
    using Tag = Tag_;

private:
    const QJsonDocument m_doc;
    const OptJsonInt m_int;

public:
    explicit JsonDoc(const TaggedStringUtf8<Tag> &json)
        : m_doc{QJsonDocument::fromJson(json.toQByteArray())}
        , m_int{[this, &json]() -> OptJsonInt {
            if (m_doc.isArray() || m_doc.isObject() || json.isEmpty())
                return std::nullopt;

            // Qt's Json parser doesn't support integers at the top level of a document
            size_t pos = 0;
            unsigned long num = 0;
            try {
                num = std::stoul(json.getStdStringUtf8(), &pos, 10);
            } catch (const std::exception &e) {
                qWarning() << "Exception in number parsing: " << e.what();
                return std::nullopt;
            }
            return OptJsonInt(static_cast<JsonInt>(num));
        }()}
    {}

public:
    NODISCARD OptJsonObj getObject() const
    {
        if (m_doc.isObject()) {
            return OptJsonObj(m_doc.object());
        }
        return std::nullopt;
    }

    NODISCARD OptJsonArray getArray() const
    {
        if (m_doc.isArray()) {
            return OptJsonArray(m_doc.array());
        }
        return std::nullopt;
    }

    NODISCARD OptJsonInt getInt() const { return m_int; }
};
