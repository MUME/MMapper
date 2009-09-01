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

#include <QApplication>
#include <QtGui>
#include <QtCore>

#define WITH_SPLASH

#include "mainwindow.h"
#include "coordinate.h"
#include "pathmachine.h"
#include "mapfrontend.h"
#include "configuration.h"
int main(int argc, char **argv)
{
#if QT_VERSION >= 0x0040500
  /* Experimental:
   * This might improve rendering speed... but its problematic
   */
   //QApplication::setGraphicsSystem("opengl");
#endif
  QApplication    app(argc, argv);

  Config().read();
#ifdef WITH_SPLASH
  QPixmap pixmap(":/pixmaps/splash20.png");
  QSplashScreen *splash = new QSplashScreen(pixmap);
  splash->show();
#endif
  MainWindow     mw;
  if (Config().m_autoLoadWorld && Config().m_autoLoadFileName!="")
  {
	mw.loadFile(Config().m_autoLoadFileName);
  }
  mw.show();
#ifdef WITH_SPLASH
  splash->finish(&mw);
  delete splash;
#endif
  int ret = app.exec();
  Config().write();
  return ret;
}

