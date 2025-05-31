// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "DescriptionWidget.h"

#include "../client/displaywidget.h"
#include "../configuration/configuration.h"
#include "../map/mmapper2room.h"
#include "../preferences/ansicombo.h"
#include "../src/global/Charset.h"

#include <optional>
#include <set>

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QPixmap>
#include <QRegularExpression>
#include <QResizeEvent>

DescriptionWidget::DescriptionWidget(QWidget *const parent)
    : QWidget(parent)
    , m_label(new QLabel(this))
    , m_textEdit(new QTextEdit(this))
{
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
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
    setFont(font);

    scanDirectories();

    const auto &resourcesDir = getConfig().canvas.resourcesDirectory;
    m_watcher.addPath(resourcesDir + "/rooms");
    m_watcher.addPath(resourcesDir + "/areas");
    connect(&m_watcher,
            &QFileSystemWatcher::directoryChanged,
            this,
            [this](const QString & /*path*/) {
                scanDirectories();
                m_pixmapCache.clear();
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
                int dotIndex = filePath.lastIndexOf('.');
                if (dotIndex == -1) {
                    assert(false);
                    continue;
                }

                QString baseName;
                if (filePath.startsWith(":/")) {
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

                qInfo() << "Found file:" << fileInfo.filePath() << "Base name:" << baseName
                        << "Suffix:" << suffix;
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
    m_textEdit->setGeometry(0, 0, newSize.width(), newSize.height());
    updateBackground();
}

void DescriptionWidget::updateBackground()
{
    auto loadImage = [&](const QString &fileName) -> QPixmap {
        QPixmap pixmap;
        if (fileName.startsWith(":/")) {
            pixmap.load(fileName);
        } else if (fileName.startsWith("/")) {
            QString custom = getConfig().canvas.resourcesDirectory + fileName;
            pixmap.load(custom);
        }
        return pixmap;
    };

    if (!m_fileName.isEmpty() && !m_pixmapCache.contains(m_fileName)) {
        QPixmap newPixmap = loadImage(m_fileName);
        if (!newPixmap.isNull()) {
            m_pixmapCache.insert(m_fileName, new QPixmap(newPixmap));
        }
    }

    QPixmap scaled;
    if (QPixmap *pixmap = m_pixmapCache.object(m_fileName)) {
        scaled = pixmap->scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }
    m_label->setPixmap(scaled);
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
