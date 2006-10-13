FORMS += ./preferences/generalpage.ui \
        ./preferences/parserpage.ui \
        ./preferences/pathmachinepage.ui \
        ./mainwindow/roomeditattrdlg.ui \
        ./mainwindow/infomarkseditdlg.ui
HEADERS += ./global/defs.h \
          ./mapdata/roomselection.h \
          ./mapdata/mapdata.h \
          ./mapdata/mmapper2room.h \
          ./mapdata/mmapper2exit.h \
          ./mapdata/infomark.h \
          ./mapdata/customaction.h \
          ./mapdata/roomfactory.h \
          ./mapdata/drawstream.h \
          ./proxy/telnetfilter.h \
          ./proxy/proxy.h \
          ./proxy/connectionlistener.h \
          ./parser/patterns.h \
          ./parser/parser.h \
          ./parser/mumexmlparser.h \
          ./parser/abstractparser.h \
          ./preferences/configdialog.h \
          ./preferences/generalpage.h \
          ./preferences/parserpage.h \
          ./preferences/pathmachinepage.h \
          ./preferences/ansicombo.h \
          ./mainwindow/mainwindow.h \
          ./mainwindow/roomeditattrdlg.h \
          ./mainwindow/infomarkseditdlg.h \
          ./configuration/configuration.h \
          ./display/mapcanvas.h \
          ./display/mapwindow.h \
          ./display/connectionselection.h \
          ./display/prespammedpath.h \
          ./expandoracommon/room.h \
          ./expandoracommon/frustum.h \
          ./expandoracommon/component.h \
          ./expandoracommon/coordinate.h \
          ./expandoracommon/listcycler.h \
          ./expandoracommon/abstractroomfactory.h \
          ./expandoracommon/mmapper2event.h \
          ./expandoracommon/parseevent.h \
          ./expandoracommon/property.h \
          ./expandoracommon/roomadmin.h \
          ./expandoracommon/roomrecipient.h \
          ./expandoracommon/exit.h \
          ./pathmachine/approved.h \
          ./pathmachine/experimenting.h \
          ./pathmachine/pathmachine.h \
          ./pathmachine/path.h \
          ./pathmachine/pathparameters.h \
          ./pathmachine/mmapper2pathmachine.h \
          ./pathmachine/roomsignalhandler.h \
          ./pathmachine/onebyone.h \
          ./pathmachine/crossover.h \
          ./pathmachine/syncing.h \
          ./mapfrontend/intermediatenode.h \
          ./mapfrontend/map.h \
          ./mapfrontend/mapaction.h \
          ./mapfrontend/mapfrontend.h \
          ./mapfrontend/roomcollection.h \
          ./mapfrontend/roomlocker.h \
          ./mapfrontend/roomoutstream.h \
          ./mapfrontend/searchtreenode.h \
          ./mapstorage/abstractmapstorage.h \
          ./mapstorage/mapstorage.h \
          ./mapstorage/oldroom.h \
          ./mapstorage/olddoor.h \
          ./mapstorage/roomsaver.h \
          ./mapstorage/oldconnection.h \
	  ./mapfrontend/tinylist.h
SOURCES += main.cpp \
          ./mapdata/mapdata.cpp \
          ./mapdata/mmapper2room.cpp \
          ./mapdata/mmapper2exit.cpp \
          ./mapdata/roomfactory.cpp \
          ./mapdata/drawstream.cpp \
          ./mapdata/roomselection.cpp \
          ./mapdata/customaction.cpp \
          ./mainwindow/mainwindow.cpp \
          ./mainwindow/roomeditattrdlg.cpp \
          ./mainwindow/infomarkseditdlg.cpp \
          ./display/mapwindow.cpp \
          ./display/mapcanvas.cpp \
          ./display/connectionselection.cpp \
          ./display/prespammedpath.cpp \
          ./preferences/configdialog.cpp \
          ./preferences/generalpage.cpp \
          ./preferences/parserpage.cpp \
          ./preferences/pathmachinepage.cpp \
          ./preferences/ansicombo.cpp \
          ./configuration/configuration.cpp \
          ./proxy/connectionlistener.cpp \
          ./proxy/telnetfilter.cpp \
          ./proxy/proxy.cpp \
          ./parser/parser.cpp \
          ./parser/mumexmlparser.cpp \
          ./parser/abstractparser.cpp \
          ./parser/patterns.cpp \
          ./expandoracommon/component.cpp \
          ./expandoracommon/coordinate.cpp \
          ./expandoracommon/frustum.cpp \
          ./expandoracommon/abstractroomfactory.cpp \
          ./expandoracommon/init.cpp \
          ./expandoracommon/property.cpp \
          ./expandoracommon/mmapper2event.cpp \
          ./expandoracommon/parseevent.cpp \
          ./mapfrontend/intermediatenode.cpp \
          ./mapfrontend/map.cpp \
          ./mapfrontend/mapaction.cpp \
          ./mapfrontend/mapfrontend.cpp \
          ./mapfrontend/roomcollection.cpp \
          ./mapfrontend/roomlocker.cpp \
          ./mapfrontend/searchtreenode.cpp \
          ./pathmachine/approved.cpp \
          ./pathmachine/experimenting.cpp \
          ./pathmachine/pathmachine.cpp \
          ./pathmachine/mmapper2pathmachine.cpp \
          ./pathmachine/roomsignalhandler.cpp \
          ./pathmachine/path.cpp \
          ./pathmachine/pathparameters.cpp \
          ./pathmachine/syncing.cpp \
          ./pathmachine/onebyone.cpp \
          ./pathmachine/crossover.cpp \
          ./mapstorage/roomsaver.cpp \
          ./mapstorage/abstractmapstorage.cpp \
          ./mapstorage/mapstorage.cpp \
          ./mapstorage/oldconnection.cpp

TARGET = mmapper2

SVN_REVISION=$$system("svn info | grep Revision | sed s/Revision:\ //")
!isEmpty(SVN_REVISION) {
	DEFINES += SVN_REVISION=$$SVN_REVISION
}
RESOURCES += resources/mmapper2.qrc
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += . ./global ./mapstorage ./mapdata ./proxy ./parser ./preferences ./configuration ./display ./mainwindow ./expandoracommon ./pathmachine ./mapfrontend
LIBS +=
QT += opengl network
CONFIG -= release
CONFIG += debug
RCC_DIR = ../build/resources
UI_DIR = ../build/ui
MOC_DIR = ../build/moc
debug{
 DESTDIR = ../bin/debug
 OBJECTS_DIR = ../build/debug-obj
}
release{
 DESTDIR = ../bin/release
 OBJECTS_DIR = ../build/release-obj
}
