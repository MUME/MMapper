/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "configdialog.h"
#include "generalpage.h"
#include "parserpage.h"
#include "pathmachinepage.h"
#include "groupmanagerpage.h"

#include <QListWidget>
#include <QStackedWidget>

ConfigDialog::ConfigDialog(CGroup* gm)
{
  m_groupManager = gm;
  contentsWidget = new QListWidget;
  contentsWidget->setViewMode(QListView::IconMode);
  contentsWidget->setIconSize(QSize(70, 70));
  contentsWidget->setMovement(QListView::Static);
  contentsWidget->setMaximumWidth(90);
  contentsWidget->setMinimumWidth(90);    
  contentsWidget->setSpacing(9);

  pagesWidget = new QStackedWidget;
  pagesWidget->setMinimumWidth(400);    
  pagesWidget->setMinimumHeight(540);    

  QPushButton *closeButton = new QPushButton(tr("Close"));

  createIcons();

  connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

  QHBoxLayout *horizontalLayout = new QHBoxLayout;
  horizontalLayout->addWidget(contentsWidget);
  horizontalLayout->addWidget(pagesWidget, 1);

  QHBoxLayout *buttonsLayout = new QHBoxLayout;
  buttonsLayout->addStretch(1);
  buttonsLayout->addWidget(closeButton);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addLayout(horizontalLayout);
  mainLayout->addStretch(1);
  mainLayout->addSpacing(12);
  mainLayout->addLayout(buttonsLayout);
  setLayout(mainLayout);

  setWindowTitle(tr("Config Dialog"));

  pagesWidget->addWidget(new GeneralPage);
    //contentsWidget->setCurrentRow(0);
  pagesWidget->setCurrentIndex(0);
}

void ConfigDialog::createIcons()
{
  QListWidgetItem *configButton = new QListWidgetItem(contentsWidget);
  configButton->setIcon(QIcon(":/icons/generalcfg.png"));
  configButton->setText(tr("General"));
  configButton->setTextAlignment(Qt::AlignHCenter);
  configButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

  QListWidgetItem *updateButton = new QListWidgetItem(contentsWidget);
  updateButton->setIcon(QIcon(":/icons/parsercfg.png"));
  updateButton->setText(tr("Parser"));
  updateButton->setTextAlignment(Qt::AlignHCenter);
  updateButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

  QListWidgetItem *groupButton = new QListWidgetItem(contentsWidget);
  groupButton->setIcon(QIcon(":/icons/groupcfg.png"));
  groupButton->setText(tr("Group\nManager"));
  groupButton->setTextAlignment(Qt::AlignHCenter);
  groupButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

/*
  QListWidgetItem *queryButton = new QListWidgetItem(contentsWidget);
  queryButton->setIcon(QIcon(":/icons/pathmachinecfg.png"));
  queryButton->setText(tr("Pathmachine"));
  queryButton->setTextAlignment(Qt::AlignHCenter);
  queryButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
*/

  connect(contentsWidget,
          SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
                 this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
}

void ConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
  if (!current)
    current = previous;

  if (pagesWidget->count()<=1)
  {
    pagesWidget->addWidget(new ParserPage);
    pagesWidget->addWidget(new GroupManagerPage(m_groupManager));
  }

  pagesWidget->setCurrentIndex(contentsWidget->row(current));
}

