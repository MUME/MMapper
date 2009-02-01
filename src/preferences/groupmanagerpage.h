/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

#ifndef GROUPMANAGERPAGE_H
#define GROUPMANAGERPAGE_H

#include <QWidget>
#include "ui_groupmanagerpage.h"
#include "CGroup.h"
#include "configdialog.h"

class GroupManagerPage : public QWidget, private Ui::GroupManagerPage
{
  Q_OBJECT

  public slots:

    void colorNameTextChanged(const QString&);
    void changeColorClicked();
    void charNameTextChanged(const QString&);
    void acceptCharClicked();

    void remoteHostTextChanged(const QString&);
    void remotePortValueChanged(int);
    void localPortValueChanged(int);
    void acceptClientClicked();
    void acceptServerClicked();

    void rulesWarningChanged(int);

  public:
    GroupManagerPage(CGroup*, QWidget *parent = 0);

  private:
    CGroup* m_groupManager;

};


#endif

