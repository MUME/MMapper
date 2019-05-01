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

#ifndef DISPLAYWIDGET_H
#define DISPLAYWIDGET_H

#include <QColor>
#include <QFont>
#include <QString>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextFormat>
#include <QtCore>
#include <QtGui>

class QObject;
class QResizeEvent;
class QTextDocument;
class QWidget;

class DisplayWidget : public QTextEdit
{
private:
    using base = QTextEdit;

private:
    Q_OBJECT

public:
    explicit DisplayWidget(QWidget *parent = nullptr);
    ~DisplayWidget() override;

public slots:
    void displayText(const QString &str);

protected:
    QTextCursor m_cursor{};
    QTextCharFormat m_format{};

    void resizeEvent(QResizeEvent *event) override;

private:
    QColor m_foregroundColor{}, m_backgroundColor{};
    QFont m_serverOutputFont{};
    // TODO: Create state machine
    bool m_ansi256Foreground{false};
    bool m_ansi256Background{false};
    bool m_backspace{false};

    void setDefaultFormat(QTextCharFormat &format);
    void updateFormat(QTextCharFormat &format, int ansiCode);
    void updateFormatBoldColor(QTextCharFormat &format);

signals:
    void showMessage(const QString &, int);
    void windowSizeChanged(int, int);
};

#endif /* DISPLAYWIDGET_H */
