#ifndef FLAGSETTER
#define FLAGSETTER

#include <QMap>
#include <QByteArray>
#include "coordinate.h"
#include "component.h"
#include "roomrecipient.h"
#include "roomadmin.h"
#include "mapaction.h"

class RoomPropertySetterSlave : public RoomRecipient
{
private:
	AbstractAction * action;
public:
	RoomPropertySetterSlave(AbstractAction * in_action) : action(in_action) {}
	void receiveRoom(RoomAdmin *, const Room *);
	bool getResult() {return !action;}
};

class RoomPropertySetter : public Component
{
private:
	Q_OBJECT
	QMap<QByteArray, uint> propPositions;
	QMap<QByteArray, uint> fieldValues;

public:
	RoomPropertySetter();
	virtual Qt::ConnectionType requiredConnectionType(const QString &) {return Qt::DirectConnection;}
	
public slots:
	void parseProperty(const QByteArray &, const Coordinate &);
	
signals:
	void sendToUser(const QByteArray&);
	void lookingForRooms(RoomRecipient *,const Coordinate &);
};

#endif
