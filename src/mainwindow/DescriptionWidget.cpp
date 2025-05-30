// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "DescriptionWidget.h"

#include "../client/displaywidget.h"
#include "../configuration/configuration.h"
#include "../map/mmapper2room.h"
#include "../preferences/ansicombo.h"
#include "../src/global/Charset.h"

#include <QFile>
#include <QPixmap>
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
                if (!m_fileName.isEmpty()) {
                    updateBackground();
                }
            });

    updateBackground();
}

void DescriptionWidget::scanDirectories()
{
    m_availableFiles.clear();
    const auto &resourcesDir = getConfig().canvas.resourcesDirectory;

    auto scanSubdir = [&](const QString &subdir) {
        QString dirPath = resourcesDir + "/" + subdir;
        QDirIterator it(dirPath, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString relativePath = it.next().mid(dirPath.length() + 1);
            m_availableFiles.insert(subdir + "/" + relativePath);
        }
    };

    scanSubdir("rooms");
    scanSubdir("areas");

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
    if (!m_fileName.isEmpty() && !m_pixmapCache.contains(m_fileName)) {
        QPixmap newPixmap;
        QString custom = getConfig().canvas.resourcesDirectory + "/" + m_fileName;
        if (newPixmap.load(custom)) {
            qInfo() << "Loaded" << m_fileName << "into cache";
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

    const ServerRoomId id = r.getServerId();
    if (id != INVALID_SERVER_ROOMID) {
        auto fileName = QString("rooms/%1.jpg").arg(id.asUint32());
        if (m_availableFiles.find(fileName) != m_availableFiles.end()) {
            newFileName = fileName;
            fileNameChanged = true;
        }
    }

    const RoomArea &area = r.getArea();
    if (!fileNameChanged || newFileName.isEmpty()) {
        static const QRegularExpression regex("^the\\s+");
        QString fileName = area.toQString().toLower().remove(regex).replace(' ', '-');
        mmqt::toAsciiInPlace(fileName);
        fileName = "areas/" + fileName + ".jpg";
        if (m_availableFiles.find(fileName) != m_availableFiles.end()) {
            newFileName = fileName;
            fileNameChanged = true;
        }
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
