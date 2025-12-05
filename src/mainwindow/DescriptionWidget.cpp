// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "DescriptionWidget.h"

#include "../client/displaywidget.h"
#include "../configuration/configuration.h"
#include "../global/Charset.h"
#include "../global/RAII.h"
#include "../map/mmapper2room.h"
#include "../preferences/ansicombo.h"

#include <memory>
#include <optional>
#include <set>
#include <vector>

#include <QDirIterator>
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

DescriptionWidget::DescriptionWidget(QWidget *const parent)
    : QWidget(parent)
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

    scanDirectories();

    const auto &resourcesDir = getConfig().canvas.resourcesDirectory;
    m_watcher.addPath(resourcesDir + "/rooms");
    m_watcher.addPath(resourcesDir + "/areas");
    connect(&m_watcher,
            &QFileSystemWatcher::directoryChanged,
            this,
            [this](const QString & /*path*/) {
                scanDirectories();
                m_imageCache.clear();
                updateBackground();
            });

    updateBackground();
}

void DescriptionWidget::scanDirectories()
{
    m_availableFiles.clear();

    // Get supported image formats
    const QByteArrayList supportedFormatsList = QImageReader::supportedImageFormats();
    qInfo() << "Supported Image Formats:" << supportedFormatsList;
    std::set<QString> supportedFormats;
    for (const QByteArray &format : supportedFormatsList) {
        supportedFormats.insert(QString::fromUtf8(format).toLower());
    }

    auto scanPath = [&](const QString &path) {
        QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QFileInfo fileInfo(it.next());
            QString suffix = fileInfo.suffix();

            // Check if the file has a supported image extension
            if (supportedFormats.find(suffix.toLower()) != supportedFormats.end()) {
                QString filePath = fileInfo.filePath();
                auto dotIndex = filePath.lastIndexOf('.');
                if (dotIndex == -1) {
                    assert(false);
                    continue;
                }

                const bool isQrc = filePath.startsWith(":/");
                QString baseName;
                if (isQrc) {
                    baseName = filePath.left(dotIndex);
                } else {
                    QString resourcesPath = getConfig().canvas.resourcesDirectory;
                    if (!filePath.startsWith(resourcesPath)) {
                        assert(false);
                        continue;
                    }
                    baseName = filePath.mid(resourcesPath.length())
                                   .left(dotIndex - resourcesPath.length());
                }

                if (baseName.isEmpty()) {
                    assert(false);
                    continue;
                }

                if (!isQrc) {
                    qDebug() << "Found file:" << fileInfo.filePath() << "Base name:" << baseName
                             << "Suffix:" << suffix;
                }
                m_availableFiles.insert({baseName, suffix});
            }
        }
    };

    // Scan file system directories and QRC resources
    const auto &resourcesDir = getConfig().canvas.resourcesDirectory;
    scanPath(resourcesDir + "/rooms");
    scanPath(resourcesDir + "/areas");
    scanPath(":/rooms");
    scanPath(":/areas");

    qInfo() << "Scanned background directories. Found" << m_availableFiles.size() << "files.";
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
    const auto loadAndCacheImage = [&](const QString &fileName) -> QImage * {
        if (fileName.isEmpty()) {
            return nullptr;
        }

        if (QImage *cachedImage = m_imageCache.object(fileName)) {
            return cachedImage;
        }

        QString imagePath = fileName;
        if (!fileName.startsWith(":/")) {
            imagePath = getConfig().canvas.resourcesDirectory + fileName;
        }

        std::unique_ptr<QImage> temp = std::make_unique<QImage>();
        if (!temp->load(imagePath) || temp->isNull()) {
            qWarning() << "Failed to load image:" << imagePath;
            return nullptr;
        }

        m_imageCache.insert(fileName, temp.release());
        return m_imageCache.object(fileName);
    };

    QImage *const baseImage = loadAndCacheImage(m_fileName);
    if (!baseImage || baseImage->isNull()) {
        m_label->clear();
        return;
    }

    // Decide if widget is padded
    const QSize widgetSize = size();
    const auto hasRoomRightOfTextEdit = [&]() -> bool {
        const QRect textEditGeometry = m_textEdit->geometry();
        const int spaceRightOfTextEdit = widgetSize.width()
                                         - (textEditGeometry.x() + textEditGeometry.width());
        return baseImage->width() <= spaceRightOfTextEdit;
    }();
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
    bool fileNameChanged = false;

    auto findPrioritizedImage =
        [&](const QString &baseNameWithoutPrefix) -> std::optional<std::pair<QString, QString>> {
        // Check file system path first
        QString fileSystemBaseName = "/" + baseNameWithoutPrefix;
        auto it = m_availableFiles.find(fileSystemBaseName);
        if (it != m_availableFiles.end()) {
            return std::make_optional(std::pair<QString, QString>(it->first, it->second));
        }

        // If not found, check QRC path
        QString qrcBaseName = ":/" + baseNameWithoutPrefix;
        it = m_availableFiles.find(qrcBaseName);
        if (it != m_availableFiles.end()) {
            return std::make_optional<std::pair<QString, QString>>(it->first, it->second);
        }

        return std::nullopt;
    };

    auto trySetFileName = [&](const QString &baseNameWithoutPrefix) {
        if (auto optFound = findPrioritizedImage(baseNameWithoutPrefix)) {
            const auto &found = optFound.value();
            newFileName = found.first + "." + found.second;
            fileNameChanged = true;
        }
    };

    const ServerRoomId id = r.getServerId();
    if (id != INVALID_SERVER_ROOMID) {
        QString baseNameWithoutPrefix = QString("rooms/%1").arg(id.asUint32());
        trySetFileName(baseNameWithoutPrefix);
    }

    if (!fileNameChanged || newFileName.isEmpty()) {
        const RoomArea &area = r.getArea();
        static const QRegularExpression regex("^the\\s+");
        QString baseNameWithoutPrefix = area.toQString().toLower().remove(regex).replace(' ', '-');
        mmqt::toAsciiInPlace(baseNameWithoutPrefix);
        baseNameWithoutPrefix = "areas/" + baseNameWithoutPrefix;
        trySetFileName(baseNameWithoutPrefix);
    }

    // Only update background if the filename has changed
    if (fileNameChanged && newFileName != m_fileName) {
        m_fileName = newFileName;
        updateBackground();
    } else if (!fileNameChanged && !m_fileName.isEmpty()) {
        // If no new file was found and there was a previous file, clear it.
        m_fileName = "";
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
