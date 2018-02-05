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

#include <QTextEdit>
#include <QRegExp>

class QTextDocument;

class DisplayWidget : public QTextEdit
{
    Q_OBJECT

public:
    DisplayWidget(QWidget *parent = 0);
    ~DisplayWidget();

public slots:
    void displayText(const QString &str);

protected:
    QTextCursor m_cursor;
    QTextCharFormat m_format;

    void resizeEvent(QResizeEvent *event);

private:
    static const QRegExp s_ansiRx;

    QColor m_blackColor, m_redColor, m_greenColor, m_yellowColor, m_blueColor, m_magentaColor;
    QColor m_cyanColor, m_grayColor, m_darkGrayColor, m_brightRedColor, m_brightGreenColor;
    QColor m_brightYellowColor, m_brightBlueColor, m_brightMagentaColor, m_brightCyanColor;
    QColor m_whiteColor, m_foregroundColor, m_backgroundColor;
    QFont m_serverOutputFont;

    bool m_backspace;

    void setDefaultFormat(QTextCharFormat &format);
    void updateFormat(QTextCharFormat &format, int ansiCode);
    void updateFormatBoldColor(QTextCharFormat &format);

signals:
    void dimensionsChanged(int, int);
};

#endif /* DISPLAYWIDGET_H */
