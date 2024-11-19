#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/AnsiTextUtils.h"

#include <vector>

#include <QColor>
#include <QComboBox>
#include <QIcon>
#include <QString>
#include <QVector>
#include <QtCore>

class QObject;
class QWidget;

class AnsiCombo : public QComboBox
{
    Q_OBJECT

private:
    using super = QComboBox;

public:
    struct NODISCARD AnsiItem final
    {
        QString description;
        QIcon picture;

        size_t ui_index = 0;
        AnsiColor16 color;
        AnsiColor16LocationEnum loc = AnsiColor16LocationEnum::Foreground;
    };

private:
    struct NODISCARD FlatMap final
    {
    private:
        std::vector<AnsiItem> values;

    private:
        NODISCARD size_t getInternalIndex(const AnsiColor16 color) const
        {
            const auto &opt = color.color;
            if (opt.has_value()) {
                return static_cast<size_t>(opt.value());
            }
            return 16; // default
        }

    public:
        NODISCARD const AnsiItem &getItemAtUiIndex(const size_t idx) const
        {
            const auto &item = values.at(idx);
            assert(item.ui_index == idx);
            return item;
        }

        NODISCARD const AnsiItem &getItem(AnsiColor16 color) const
        {
            for (auto &item : values) {
                if (item.color == color) {
                    return item;
                }
            }

            assert(false);
            std::abort();
        }

        void insert(AnsiItem item)
        {
            assert(values.size() < 17);
            item.ui_index = values.size();
            values.emplace_back(item);
        }
        void clear() { values.clear(); }
    };

private:
    // There's not really a good default value for this.
    AnsiColor16LocationEnum m_mode = AnsiColor16LocationEnum::Foreground;
    FlatMap m_map;

public:
    static void makeWidgetColoured(QWidget *, const QString &ansiColor, bool changeText = true);

    explicit AnsiCombo(AnsiColor16LocationEnum mode, QWidget *parent);
    explicit AnsiCombo(QWidget *parent);

    void initColours(AnsiColor16LocationEnum mode);

    NODISCARD AnsiColor16LocationEnum getMode() const { return m_mode; }

    NODISCARD AnsiColor16 getAnsiCode() const;
    void setAnsiCode(AnsiColor16);

    struct NODISCARD AnsiColor final
    {
        AnsiColor16 bg;
        AnsiColor16 fg;
        bool bold = false;
        bool italic = false;
        bool underline = false;

    public:
        NODISCARD AnsiColor16Enum getBg() const
        {
            return bg.color.value_or(AnsiColor16Enum::white);
        }
        NODISCARD AnsiColor16Enum getFg() const
        {
            return fg.color.value_or(AnsiColor16Enum::black);
        }
        NODISCARD QColor getBgColor() const { return mmqt::toQColor(getBg()); }
        NODISCARD QColor getFgColor() const { return mmqt::toQColor(getFg()); }

        NODISCARD QString getBgName() const { return mmqt::toQStringLatin1(bg.to_string_view()); }
        NODISCARD QString getFgName() const { return mmqt::toQStringLatin1(fg.to_string_view()); }
    };

    /// \return true if string is valid ANSI color code
    NODISCARD static AnsiColor colorFromString(const QString &ansiString);
};
