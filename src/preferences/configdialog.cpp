// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "configdialog.h"

#include "../configuration/configuration.h"
#include "audiopage.h"
#include "autologpage.h"
#include "clientpage.h"
#include "generalpage.h"
#include "graphicspage.h"
#include "grouppage.h"
#include "mumeprotocolpage.h"
#include "parserpage.h"
#include "pathmachinepage.h"
#include "ui_configdialog.h"

#include <QIcon>
#include <QListWidget>
#include <QTimer>
#include <QtWidgets>

namespace {
QString getSearchableText(const QWidget *widget)
{
    if (auto *const cb = qobject_cast<const QCheckBox *>(widget))
        return cb->text();
    if (auto *const rb = qobject_cast<const QRadioButton *>(widget))
        return rb->text();
    if (auto *const lbl = qobject_cast<const QLabel *>(widget))
        return lbl->text();
    if (auto *const pb = qobject_cast<const QPushButton *>(widget))
        return pb->text();
    if (auto *const gb = qobject_cast<const QGroupBox *>(widget))
        return gb->title();
    return QString();
}

enum StackPage { Preferences = 0, SearchResults = 1, NoResults = 2 };
} // namespace

ConfigDialog::ConfigDialog(QWidget *const parent)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Preferences"));

    auto generalPage = new GeneralPage(this);
    auto graphicsPage = new GraphicsPage(this);
    auto parserPage = new ParserPage(this);
    auto clientPage = new ClientPage(this);
    auto groupPage = new GroupPage(this);
    auto autoLogPage = new AutoLogPage(this);
    auto audioPage = new AudioPage(this);
    auto mumeProtocolPage = new MumeProtocolPage(this);
    auto pathmachinePage = new PathmachinePage(this);

    const auto makeSectionHeader = [](const QString &name,
                                      QWidget *const headerParent) -> QWidget * {
        auto *container = new QWidget(headerParent);
        auto *layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 4);

        auto *label = new QLabel(name, container);
        QFont font = label->font();
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
        label->setFont(font);
        layout->addWidget(label);

        auto *line = new QFrame(container);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line, 1);

        return container;
    };

    auto addPage = [this, &makeSectionHeader](QWidget *widget,
                                              const QString &name,
                                              const QString &iconPath) {
        auto *item = new QListWidgetItem(QIcon(iconPath), name, ui->contentsWidget);

        auto *container = new QWidget(this);
        auto *containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->addWidget(makeSectionHeader(name, container));
        containerLayout->addWidget(widget);

        ui->scrollLayout->insertWidget(ui->scrollLayout->indexOf(ui->footerWidget) - 1, container);
        m_pages.append({name, widget, item, container});
    };

    addPage(generalPage, tr("General"), ":/icons/generalcfg.png");
    addPage(graphicsPage, tr("Graphics"), ":/icons/graphicscfg.png");
    addPage(parserPage, tr("Parser"), ":/icons/parsercfg.png");
    addPage(clientPage, tr("Integrated Client"), ":/icons/terminal.png");
    addPage(groupPage, tr("Group Panel"), ":/icons/group-recolor.png");
    addPage(autoLogPage, tr("Auto Logger"), ":/icons/autologgercfg.png");
    addPage(audioPage, tr("Audio"), ":/icons/audiocfg.png");
    addPage(mumeProtocolPage, tr("Mume Protocol"), ":/icons/mumeprotocolcfg.png");
    addPage(pathmachinePage, tr("Path Machine"), ":/icons/pathmachinecfg.png");

    connect(ui->backToTopBtn, &QPushButton::clicked, this, [this]() {
        if (!m_pages.isEmpty()) {
            ui->contentsWidget->setCurrentItem(m_pages.first().item);
        }
    });

    ui->mainSplitter->setStretchFactor(0, 0);
    ui->mainSplitter->setStretchFactor(1, 1);

    connect(ui->contentsWidget,
            &QListWidget::currentItemChanged,
            this,
            &ConfigDialog::slot_changePage);
    connect(ui->pagesScrollArea->verticalScrollBar(),
            &QScrollBar::valueChanged,
            this,
            &ConfigDialog::slot_onScroll);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ConfigDialog::slot_ok);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ConfigDialog::slot_cancel);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(50);
    connect(m_searchTimer, &QTimer::timeout, this, [this]() { slot_search(ui->searchBar->text()); });
    connect(ui->searchBar, &QLineEdit::textChanged, m_searchTimer, qOverload<>(&QTimer::start));

    connect(ui->searchResultsList,
            &QListWidget::itemActivated,
            this,
            &ConfigDialog::slot_onResultSelected);
    connect(ui->searchResultsList,
            &QListWidget::itemClicked,
            this,
            &ConfigDialog::slot_onResultSelected);

    connect(generalPage, &GeneralPage::sig_reloadConfig, this, [this]() { emit sig_loadConfig(); });

    connect(this, &ConfigDialog::sig_loadConfig, generalPage, &GeneralPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, graphicsPage, &GraphicsPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, parserPage, &ParserPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, clientPage, &ClientPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, autoLogPage, &AutoLogPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, audioPage, &AudioPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, groupPage, &GroupPage::slot_loadConfig);
    connect(groupPage,
            &GroupPage::sig_groupSettingsChanged,
            this,
            &ConfigDialog::sig_groupSettingsChanged);
    connect(this,
            &ConfigDialog::sig_loadConfig,
            mumeProtocolPage,
            &MumeProtocolPage::slot_loadConfig);
    connect(this, &ConfigDialog::sig_loadConfig, pathmachinePage, &PathmachinePage::slot_loadConfig);
    connect(graphicsPage,
            &GraphicsPage::sig_graphicsSettingsChanged,
            this,
            &ConfigDialog::sig_graphicsSettingsChanged);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::closeEvent(QCloseEvent *const event)
{
    getConfig().write();
    event->accept();
}

void ConfigDialog::showEvent(QShowEvent *const event)
{
    // Populate the preference pages from config each time the widget is shown
    emit sig_loadConfig();
    event->accept();
}

void ConfigDialog::scrollToWidget(QWidget *target, bool focus)
{
    const int targetY = target->mapTo(ui->scrollAreaWidgetContents, QPoint(0, 0)).y();

    // We need to delay the scrolling slightly to ensure the scroll area has
    // updated its layout after the stack switch.
    QTimer::singleShot(0, this, [this, targetY, target, focus]() {
        ui->pagesScrollArea->verticalScrollBar()->setValue(targetY);
        if (focus) {
            target->setFocus();
        }
    });
}

void ConfigDialog::slot_changePage(QListWidgetItem *current, QListWidgetItem *const /*previous*/)
{
    if (current == nullptr) {
        return;
    }

    if (!ui->searchBar->text().isEmpty()) {
        ui->searchBar->clear();
        // search bar clear will restore Stack index 0
    }

    for (const auto &page : m_pages) {
        if (page.item == current) {
            scrollToWidget(page.container);
            break;
        }
    }
}

void ConfigDialog::slot_onScroll(int value)
{
    if (!ui->searchBar->text().isEmpty()) {
        return;
    }

    QListWidgetItem *activeItem = nullptr;
    for (const auto &page : m_pages) {
        const int pageY = page.container->mapTo(ui->scrollAreaWidgetContents, QPoint(0, 0)).y();
        if (pageY <= value + 8) {
            activeItem = page.item;
        }
    }

    // If we reached the bottom, ensure the last item is selected
    if (value >= ui->pagesScrollArea->verticalScrollBar()->maximum() && !m_pages.isEmpty()) {
        activeItem = m_pages.last().item;
    }

    if (activeItem && ui->contentsWidget->currentItem() != activeItem) {
        const QSignalBlocker blocker{ui->contentsWidget};
        ui->contentsWidget->setCurrentItem(activeItem);
    }
}

void ConfigDialog::slot_ok()
{
    getConfig().write();
    accept();
}

void ConfigDialog::slot_cancel()
{
    setConfig().read();
    emit sig_loadConfig();
    reject();
}

void ConfigDialog::slot_search(const QString &text)
{
    ui->searchResultsList->clear();

    if (text.isEmpty()) {
        ui->rightStack->setCurrentIndex(StackPage::Preferences);
        for (const auto &page : m_pages) {
            page.item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        }
        ui->searchBar->setFocus();
        return;
    }

    for (const auto &page : m_pages) {
        bool anyChildMatches = false;
        const auto children = page.widget->findChildren<QWidget *>();

        QList<QListWidgetItem *> pageResults;

        for (auto *const child : children) {
            QString matchText = getSearchableText(child);
            if (!matchText.isEmpty() && matchText.contains(text, Qt::CaseInsensitive)) {
                matchText.remove('&');
                auto *const item = new QListWidgetItem(QString("  \xE2\x80\xA2 %1").arg(matchText));
                item->setData(Qt::UserRole, QVariant::fromValue(static_cast<QObject *>(child)));
                pageResults.append(item);
                anyChildMatches = true;
            }
        }

        const bool pageMatches = page.name.contains(text, Qt::CaseInsensitive);
        if (pageMatches || anyChildMatches) {
            page.item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            auto *const headerItem = new QListWidgetItem(page.item->icon(), page.name);
            headerItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            headerItem->setData(Qt::UserRole,
                                QVariant::fromValue(static_cast<QObject *>(page.widget)));
            QFont font = headerItem->font();
            font.setBold(true);
            headerItem->setFont(font);
            ui->searchResultsList->addItem(headerItem);

            for (auto *res : pageResults) {
                ui->searchResultsList->addItem(res);
            }
        } else {
            page.item->setFlags(Qt::NoItemFlags);

            // If the current sidebar selection is being disabled, clear it
            if (ui->contentsWidget->currentItem() == page.item) {
                const QSignalBlocker blocker{ui->contentsWidget};
                ui->contentsWidget->setCurrentItem(nullptr);
            }
        }
    }

    if (ui->searchResultsList->count() > 0) {
        ui->rightStack->setCurrentIndex(StackPage::SearchResults);
    } else {
        ui->rightStack->setCurrentIndex(StackPage::NoResults);
    }

    ui->searchBar->setFocus();
}

void ConfigDialog::slot_onResultSelected(QListWidgetItem *const item)
{
    if (item == nullptr) {
        return;
    }

    auto *const widget = qobject_cast<QWidget *>(item->data(Qt::UserRole).value<QObject *>());
    if (widget == nullptr) {
        return;
    }

    ui->searchBar->clear();
    ui->rightStack->setCurrentIndex(StackPage::Preferences);

    // Find which page this widget belongs to
    for (const auto &page : m_pages) {
        if (page.widget == widget || page.widget->isAncestorOf(widget)) {
            const QSignalBlocker blocker{ui->contentsWidget};
            ui->contentsWidget->setCurrentItem(page.item);

            scrollToWidget(widget, true);
            break;
        }
    }
}
