#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <QString>
#include <QWidget>
#include <QtCore>

#include "../global/macros.h"
#include "ui_parserpage.h"

class QObject;

enum class NODISCARD UiCharsetEnum { AsciiOrLatin1, UTF8 };

class ParserPage : public QWidget, private Ui::ParserPage
{
    Q_OBJECT

public:
    explicit ParserPage(QWidget *parent = nullptr);

public slots:
    void loadConfig();
    void roomNameColorClicked();
    void roomDescColorClicked();
    void removeEndDescPatternClicked();
    void addEndDescPatternClicked();
    void testPatternClicked();
    void validPatternClicked();
    void endDescPatternsListActivated(const QString &);
    void suppressXmlTagsCheckBoxStateChanged(int);

private:
    void savePatterns();
};
