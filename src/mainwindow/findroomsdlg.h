#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include "ui_findroomsdlg.h"
#include "roomrecipient.h"
#include "roomselection.h"

class MapData;
class MapCanvas;

typedef quint32 ExitsFlagsType;
typedef quint16 PromptFlagsType;

class FindRoomsDlg : public QDialog, public RoomRecipient, private Ui::FindRoomsDlg
{
    Q_OBJECT

signals:
  //void lookingForRooms(RoomRecipient *,ParseEvent *);
  void center(qint32 x, qint32 y);
  void log( const QString&, const QString& );
  //void newRoomSelection(const RoomSelection*);

public slots:
  void closeEvent( QCloseEvent * event );

public:
  FindRoomsDlg(MapData*, QWidget *parent = 0);
  void receiveRoom(RoomAdmin* sender, const Room* room);

private:
  MapData* m_mapData;
  QTreeWidgetItem* item;

  void adjustResultTable();
  QString& latinToAscii(QString& str);

  static const QString nullString;
  RoomAdmin* m_admin;
  const RoomSelection* m_roomSelection;

private slots:
  void on_lineEdit_textChanged();
  void findClicked();
  void enableFindButton(const QString &text);
  void itemDoubleClicked(QTreeWidgetItem *item);
};

#endif

