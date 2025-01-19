#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/macros.h"
#include "ui_parserpage.h"

#include <QString>
#include <QWidget>
#include <QtCore>

class QObject;

enum class NODISCARD UiCharsetEnum { AsciiOrLatin1, UTF8 };

class NODISCARD_QOBJECT ParserPage : public QWidget, private Ui::ParserPage
{
    Q_OBJECT

public:
    explicit ParserPage(QWidget *parent);

public slots:
    void slot_loadConfig();
    void slot_roomNameColorClicked();
    void slot_roomDescColorClicked();
    void slot_removeEndDescPatternClicked();
    void slot_addEndDescPatternClicked();
    void slot_testPatternClicked();
    void slot_validPatternClicked();
    void slot_endDescPatternsListActivated(const QString &);
    void slot_suppressXmlTagsCheckBoxStateChanged(int);

private:
    void savePatterns();
};
