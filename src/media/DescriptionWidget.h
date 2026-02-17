#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../map/RoomHandle.h"

#include <QCache>
#include <QImage>
#include <QLabel>
#include <QTextEdit>
#include <QWidget>

class MediaLibrary;

class NODISCARD_QOBJECT DescriptionWidget final : public QWidget
{
    Q_OBJECT

private:
    MediaLibrary &m_library;

    QLabel *m_label;
    QTextEdit *m_textEdit;

private:
    QCache<QString, QImage> m_imageCache;

private:
    QString m_fileName;

public:
    explicit DescriptionWidget(MediaLibrary &library, QWidget *parent = nullptr);
    ~DescriptionWidget() final = default;

public:
    void updateRoom(const RoomHandle &r);

protected:
    void resizeEvent(QResizeEvent *event) override;

protected:
    NODISCARD QSize minimumSizeHint() const override;
    NODISCARD QSize sizeHint() const override;

private:
    void updateBackground();
};
