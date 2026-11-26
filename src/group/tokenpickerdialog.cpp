// SPDX-License-Identifier: GPL-2.0-or-later

#include "tokenpickerdialog.h"

#include "tokenmanager.h"

#include <QAbstractItemView>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSet>
#include <QSize>
#include <QStringList>
#include <QVBoxLayout>

TokenPickerDialog::~TokenPickerDialog() = default;

TokenPickerDialog::TokenPickerDialog(const QMap<QString, QString> &availableFiles, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Choose Character Token"));
    resize(1040, 700);

    auto *layout = new QVBoxLayout(this);
    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("Filter tokens…"));
    m_filter->setClearButtonEnabled(true);
    layout->addWidget(m_filter);

    auto *contentLayout = new QHBoxLayout;
    m_tokenList = new QListWidget(this);
    m_tokenList->setViewMode(QListView::IconMode);
    m_tokenList->setFlow(QListView::LeftToRight);
    m_tokenList->setWrapping(true);
    m_tokenList->setResizeMode(QListView::Adjust);
    m_tokenList->setMovement(QListView::Static);
    m_tokenList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tokenList->setIconSize(QSize(96, 96));
    m_tokenList->setGridSize(QSize(128, 140));
    m_tokenList->setWordWrap(true);
    contentLayout->addWidget(m_tokenList, 1);

    auto *previewLayout = new QVBoxLayout;
    m_preview = new QLabel(this);
    m_preview->setMinimumSize(220, 220);
    m_preview->setMaximumSize(220, 220);
    m_preview->setAlignment(Qt::AlignCenter);
    previewLayout->addWidget(m_preview, 0, Qt::AlignLeft);

    m_selectedName = new QLabel(this);
    m_selectedName->setWordWrap(true);
    previewLayout->addWidget(m_selectedName);

    auto *importButton = new QPushButton(tr("Import New Image"), this);
    auto *cancelButton = new QPushButton(tr("Cancel"), this);
    auto *selectButton = new QPushButton(tr("Select"), this);
    importButton->setMinimumHeight(44);
    cancelButton->setMinimumHeight(44);
    selectButton->setMinimumHeight(44);
    m_selectButton = selectButton;
    m_selectButton->setEnabled(false);
    previewLayout->addWidget(importButton);
    previewLayout->addWidget(cancelButton);
    previewLayout->addWidget(selectButton);
    previewLayout->addStretch(1);
    contentLayout->addLayout(previewLayout);
    layout->addLayout(contentLayout, 1);

    auto *bottomLayout = new QHBoxLayout;
    auto *defaultButton = new QPushButton(tr("Use Default"), this);
    bottomLayout->addWidget(defaultButton);
    bottomLayout->addStretch(1);
    layout->addLayout(bottomLayout);

    connect(m_filter, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int i = 0; i < m_tokenList->count(); ++i) {
            auto *item = m_tokenList->item(i);
            const bool hidden = !item->text().contains(text, Qt::CaseInsensitive);
            item->setHidden(hidden);
            if (hidden && item->isSelected()) {
                m_tokenList->clearSelection();
            }
        }
    });
    connect(m_tokenList,
            &QListWidget::itemSelectionChanged,
            this,
            &TokenPickerDialog::updatePreview);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(selectButton, &QPushButton::clicked, this, [this]() {
        const auto selectedItems = m_tokenList->selectedItems();
        if (!selectedItems.isEmpty()) {
            m_selectedToken = selectedItems.first()->data(Qt::UserRole).toString();
            accept();
        }
    });
    connect(defaultButton, &QPushButton::clicked, this, [this]() {
        m_useDefault = true;
        accept();
    });

    connect(importButton, &QPushButton::clicked, this, [this]() {
        QSet<QString> uniquePatterns;
        for (const QByteArray &format : QImageReader::supportedImageFormats()) {
            uniquePatterns.insert(QStringLiteral("*.%1").arg(QString::fromLatin1(format)));
        }
        QStringList patterns = uniquePatterns.values();
        patterns.sort(Qt::CaseInsensitive);
        const QString fileFilter = patterns.isEmpty()
                                       ? tr("Image files (*)")
                                       : tr("Image files (%1)").arg(patterns.join(QLatin1Char(' ')));
        const QString sourcePath = QFileDialog::getOpenFileName(this,
                                                                tr("Import New Token Image"),
                                                                QString(),
                                                                fileFilter);
        if (sourcePath.isEmpty()) {
            return;
        }

        QImageReader reader(sourcePath);
        if (!reader.canRead()) {
            QMessageBox::warning(this,
                                 tr("Cannot Read Image"),
                                 tr("MMapper cannot read this image format, or the file is damaged."));
            return;
        }

        QSize imageSize = reader.size();
        if (!imageSize.isValid()) {
            const QImage image = reader.read();
            if (image.isNull()) {
                QMessageBox::warning(
                    this,
                    tr("Cannot Read Image"),
                    tr("MMapper cannot read this image format, or the file is damaged."));
                return;
            }
            imageSize = image.size();
        }

        if (imageSize.width() != imageSize.height()) {
            const QString message
                = tr("This image is %1 × %2 pixels. Map tokens are displayed in a square, so "
                     "the image may appear stretched. Import it anyway?")
                      .arg(imageSize.width())
                      .arg(imageSize.height());
            const auto answer = QMessageBox::warning(this,
                                                     tr("Image Is Not Square"),
                                                     message,
                                                     QMessageBox::Yes | QMessageBox::Cancel,
                                                     QMessageBox::Cancel);
            if (answer != QMessageBox::Yes) {
                return;
            }
        }

        QString tokenName;
        QString errorMessage;
        auto &manager = tokenManager();
        auto result = manager.importTokenImage(sourcePath, false, tokenName, errorMessage);
        if (result == TokenImportResultEnum::ALREADY_EXISTS) {
            const QString message
                = tr("An image named %1 already exists in your tokens folder. Replace it?\n\n"
                     "Characters using this token will immediately display the updated image.")
                      .arg(QFileInfo(sourcePath).fileName());
            const auto answer = QMessageBox::question(this,
                                                      tr("Replace Token Image?"),
                                                      message,
                                                      QMessageBox::Yes | QMessageBox::No,
                                                      QMessageBox::No);
            if (answer != QMessageBox::Yes) {
                return;
            }
            result = manager.importTokenImage(sourcePath, true, tokenName, errorMessage);
        }
        if (result == TokenImportResultEnum::FAILED) {
            QMessageBox::warning(this, tr("Image Import Failed"), errorMessage);
            return;
        }

        m_filter->clear();
        m_tokenToSelectAfterImport = tokenName;
    });

    connect(&tokenManager(), &TokenManager::sig_tokensChanged, this, [this]() {
        populateTokens(tokenManager().availableFiles(), m_tokenToSelectAfterImport);
        m_tokenToSelectAfterImport.clear();
    });

    populateTokens(availableFiles);
}

void TokenPickerDialog::populateTokens(const QMap<QString, QString> &availableFiles,
                                       const QString &selectToken)
{
    m_tokenList->clear();
    QListWidgetItem *itemToSelect = nullptr;
    for (auto it = availableFiles.cbegin(); it != availableFiles.cend(); ++it) {
        const QFileInfo info(it.value());
        auto *item = new QListWidgetItem(QIcon(it.value()), info.fileName());
        item->setData(Qt::UserRole, info.completeBaseName());
        item->setTextAlignment(Qt::AlignHCenter);
        m_tokenList->addItem(item);
        if (info.completeBaseName() == selectToken) {
            itemToSelect = item;
        }
    }

    if (itemToSelect != nullptr) {
        m_tokenList->setCurrentItem(itemToSelect);
        itemToSelect->setSelected(true);
        m_tokenList->scrollToItem(itemToSelect);
    } else if (m_tokenList->count() == 0) {
        m_selectedName->setText(tr("No token images found."));
    } else {
        updatePreview();
    }
}

void TokenPickerDialog::updatePreview()
{
    const auto selectedItems = m_tokenList->selectedItems();
    const bool hasSelection = !selectedItems.isEmpty();
    m_selectButton->setEnabled(hasSelection);
    if (!hasSelection) {
        m_preview->clear();
        m_selectedName->clear();
        m_selectedToken.clear();
        return;
    }

    const auto *item = selectedItems.first();
    const QString fileName = item->text();
    const QString tokenName = item->data(Qt::UserRole).toString();
    const QPixmap pixmap = item->icon().pixmap(m_preview->size());
    m_preview->setPixmap(
        pixmap.scaled(m_preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_selectedName->setText(fileName);
    m_selectedToken = tokenName;
}
