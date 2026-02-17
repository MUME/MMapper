// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "DescriptionWidget.h"

#include "../client/displaywidget.h"
#include "../configuration/configuration.h"
#include "../global/Charset.h"
#include "../global/RAII.h"
#include "../map/mmapper2room.h"
#include "../preferences/ansicombo.h"
#include "MediaLibrary.h"

#include <memory>
#include <optional>
#include <vector>

#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QPixmap>
#include <QRegularExpression>
#include <QResizeEvent>

static constexpr int MAX_DESCRIPTION_WIDTH = 80;
static constexpr int TOP_PADDING_LINES = 5; // Padding for the room title and description
static constexpr int BASE_BLUR_RADIUS = 16;
static constexpr int DOWNSCALE_FACTOR = 10;

DescriptionWidget::DescriptionWidget(MediaLibrary &library, QWidget *const parent)
    : QWidget(parent)
    , m_library(library)
    , m_label(new QLabel(this))
    , m_textEdit(new QTextEdit(this))
{
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_label->setGeometry(rect());

    m_textEdit->setGeometry(rect());
    m_textEdit->setReadOnly(true);
    m_textEdit->setFrameStyle(QFrame::NoFrame);
    m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    m_textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);

    m_textEdit->setAutoFillBackground(false);
    QPalette palette = m_textEdit->viewport()->palette();
    palette.setColor(QPalette::Base, QColor(0, 0, 0, 0));
    m_textEdit->viewport()->setPalette(palette);
    m_textEdit->raise();

    QFont font;
    font.fromString(getConfig().integratedClient.font);
    m_textEdit->setFont(font);

    connect(&m_library, &MediaLibrary::sig_mediaChanged, this, [this]() {
        m_imageCache.clear();
        updateBackground();
    });

    updateBackground();
}

void DescriptionWidget::resizeEvent(QResizeEvent *event)
{
    const QSize &newSize = event->size();
    m_label->setGeometry(0, 0, newSize.width(), newSize.height());
    m_textEdit->setGeometry(0,
                            0,
                            std::min(newSize.width(),
                                     QFontMetrics(m_textEdit->font()).averageCharWidth()
                                             * MAX_DESCRIPTION_WIDTH
                                         + 2 * m_textEdit->frameWidth()),
                            newSize.height());
    updateBackground();
}

QSize DescriptionWidget::minimumSizeHint() const
{
    return QSize{sizeHint().width() / 3, sizeHint().height() / 3};
}

QSize DescriptionWidget::sizeHint() const
{
    return QSize{384, 576};
}

void DescriptionWidget::updateBackground()
{
    const auto loadAndCacheImage = [&](const QString &imagePath) -> QImage * {
        if (imagePath.isEmpty()) {
            return nullptr;
        }

        if (QImage *cachedImage = m_imageCache.object(imagePath)) {
            return cachedImage;
        }

        std::unique_ptr<QImage> temp = std::make_unique<QImage>();
        if (!temp->load(imagePath) || temp->isNull()) {
            qWarning() << "Failed to load image:" << imagePath;
            return nullptr;
        }

        m_imageCache.insert(imagePath, temp.release());
        return m_imageCache.object(imagePath);
    };

    QImage *const baseImage = loadAndCacheImage(m_fileName);
    if (!baseImage || baseImage->isNull()) {
        m_label->clear();
        return;
    }

    // Decide if widget is padded
    const QSize widgetSize = size();
    const auto hasRoomRightOfTextEdit = std::invoke([&]() -> bool {
        const QRect textEditGeometry = m_textEdit->geometry();
        const int spaceRightOfTextEdit = widgetSize.width()
                                         - (textEditGeometry.x() + textEditGeometry.width());
        return baseImage->width() <= spaceRightOfTextEdit;
    });
    const int topPadding = hasRoomRightOfTextEdit
                               ? 0
                               : TOP_PADDING_LINES * QFontMetrics(m_textEdit->font()).lineSpacing();

    // Scale image and paint last
    QImage resultImage(widgetSize, QImage::Format_ARGB32_Premultiplied);
    resultImage.fill(Qt::transparent);
    QPainter painter(&resultImage);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const RAIICallback painterEndRAII{[&]() {
        const QSize imageFitSize(widgetSize.width(), widgetSize.height() - topPadding);
        const QImage scaledImageForCenter = baseImage->scaled(imageFitSize,
                                                              Qt::KeepAspectRatio,
                                                              Qt::SmoothTransformation);
        const QPoint center((widgetSize.width() - scaledImageForCenter.width()) / 2,
                            (widgetSize.height() - scaledImageForCenter.height()) / 2
                                + (hasRoomRightOfTextEdit ? 0 : topPadding / 2));
        painter.drawImage(center, scaledImageForCenter);
        painter.end();
        m_label->setPixmap(QPixmap::fromImage(resultImage));
    }};

    // Blur background
    const QSize downscaledBlurSourceSize = QSize(std::max(1, widgetSize.width() / DOWNSCALE_FACTOR),
                                                 std::max(1,
                                                          widgetSize.height() / DOWNSCALE_FACTOR));
    QImage blurSource = baseImage
                            ->scaled(downscaledBlurSourceSize,
                                     Qt::IgnoreAspectRatio,
                                     Qt::SmoothTransformation)
                            .convertToFormat(QImage::Format_ARGB32_Premultiplied);

    if (blurSource.isNull() || blurSource.width() == 0 || blurSource.height() == 0) {
        qWarning() << "Blur source image is invalid or empty. Skipping blur.";
        return;
    }

    const int blurRadius = std::max(0,
                                    std::min(std::min(BASE_BLUR_RADIUS / DOWNSCALE_FACTOR,
                                                      (blurSource.width() - 1) / 2),
                                             (blurSource.height() - 1) / 2));
    if (blurRadius == 0) {
        qDebug() << "Actual blur radius is 0. Skipping blur.";
        return;
    }

    const auto stackBlur = [&](QImage &image, int radius) {
        const int w = image.width();
        const int h = image.height();
        const int div = 2 * radius + 1;
        std::vector<QRgb> linePixels;

        // Horizontal blur
        linePixels.resize(static_cast<size_t>(w));
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                linePixels[static_cast<size_t>(x)] = image.pixel(x, y);
            }
            for (int x = 0; x < w; ++x) {
                int rSum = 0, gSum = 0, bSum = 0, aSum = 0;
                for (int i = -radius; i <= radius; ++i) {
                    size_t idx = static_cast<size_t>(std::clamp(x + i, 0, w - 1));
                    QRgb pixel = linePixels[idx];
                    rSum += qRed(pixel);
                    gSum += qGreen(pixel);
                    bSum += qBlue(pixel);
                    aSum += qAlpha(pixel);
                }
                image.setPixel(x, y, qRgba(rSum / div, gSum / div, bSum / div, aSum / div));
            }
        }

        // Vertical blur
        linePixels.resize(static_cast<size_t>(h));
        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                linePixels[static_cast<size_t>(y)] = image.pixel(x, y);
            }
            for (int y = 0; y < h; ++y) {
                int rSum = 0, gSum = 0, bSum = 0, aSum = 0;
                for (int i = -radius; i <= radius; ++i) {
                    size_t idy = static_cast<size_t>(std::clamp(y + i, 0, h - 1));
                    QRgb pixel = linePixels[idy];
                    rSum += qRed(pixel);
                    gSum += qGreen(pixel);
                    bSum += qBlue(pixel);
                    aSum += qAlpha(pixel);
                }
                image.setPixel(x, y, qRgba(rSum / div, gSum / div, bSum / div, aSum / div));
            }
        }
    };

    stackBlur(blurSource, blurRadius);

    const QImage fullBlurredBgImage = blurSource.scaled(widgetSize,
                                                        Qt::IgnoreAspectRatio,
                                                        Qt::SmoothTransformation);
    painter.drawImage(0, 0, fullBlurredBgImage);
}

void DescriptionWidget::updateRoom(const RoomHandle &r)
{
    m_textEdit->clear();

    if (!r) {
        m_fileName = "";
        updateBackground();
        return;
    }

    // Track if a new filename is being set to avoid redundant updates
    QString newFileName;

    const ServerRoomId id = r.getServerId();
    if (id != INVALID_SERVER_ROOMID) {
        newFileName = m_library.findImage("rooms", QString::number(id.asUint32()));
    }

    if (newFileName.isEmpty()) {
        const RoomArea &area = r.getArea();
        static const QRegularExpression regex("^the\\s+");
        QString areaName = area.toQString().toLower().remove(regex).replace(' ', '-');
        mmqt::toAsciiInPlace(areaName);
        newFileName = m_library.findImage("areas", areaName);
    }

    // Only update background if the filename has changed
    if (newFileName != m_fileName) {
        m_fileName = newFileName;
        updateBackground();
    }

    // Set partial background for all text
    QTextBlockFormat blockFormat;
    QColor backgroundColor = getConfig().integratedClient.backgroundColor;
    blockFormat.setBackground(backgroundColor);

    QTextCursor cursor = m_textEdit->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(blockFormat);

    auto toColor = [](const QString &str) -> QColor {
        AnsiCombo::AnsiColor color = AnsiCombo::colorFromString(str);
        if (color.fg.hasColor()) {
            return color.getFgColor();
        } else {
            return getConfig().integratedClient.foregroundColor;
        }
    };
    {
        QTextCharFormat charFormat;
        charFormat.setForeground(toColor(getConfig().parser.roomNameColor));
        cursor.insertText(r.getName().toQString() + "\n", charFormat);
    }
    {
        QTextCharFormat charFormat;
        charFormat.setForeground(toColor(getConfig().parser.roomDescColor));
        cursor.insertText(r.getDescription().toQString().simplified(), charFormat);
    }
}
