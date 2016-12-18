include(3rdparty/zlib/zlib.pri)
include(3rdparty/qtiocompressor/qtiocompressor.pri)

DEPENDPATH += .

INCLUDEPATH += \
    src \
    src/global \
    src/mapstorage \
    src/mapdata \
    src/proxy \
    src/parser \
    src/preferences \
    src/configuration \
    src/display \
    src/mainwindow \
    src/expandoracommon \
    src/pathmachine \
    src/mapfrontend \
    src/pandoragroup

FORMS += \
    src/preferences/generalpage.ui \
    src/preferences/parserpage.ui \
    src/preferences/pathmachinepage.ui \
    src/mainwindow/roomeditattrdlg.ui \
    src/mainwindow/infomarkseditdlg.ui \
    src/mainwindow/findroomsdlg.ui \
    src/preferences/groupmanagerpage.ui \
    src/mainwindow/aboutdialog.ui

HEADERS += \
    src/global/defs.h \
    src/mapdata/roomselection.h \
    src/mapdata/mapdata.h \
    src/mapdata/mmapper2room.h \
    src/mapdata/mmapper2exit.h \
    src/mapdata/infomark.h \
    src/mapdata/customaction.h \
    src/mapdata/roomfactory.h \
    src/proxy/telnetfilter.h \
    src/proxy/proxy.h \
    src/proxy/connectionlistener.h \
    src/parser/patterns.h \
    src/parser/parser.h \
    src/parser/mumexmlparser.h \
    src/parser/abstractparser.h \
    src/preferences/configdialog.h \
    src/preferences/generalpage.h \
    src/preferences/parserpage.h \
    src/preferences/pathmachinepage.h \
    src/preferences/ansicombo.h \
    src/mainwindow/mainwindow.h \
    src/mainwindow/roomeditattrdlg.h \
    src/mainwindow/infomarkseditdlg.h \
    src/mainwindow/findroomsdlg.h \
    src/mainwindow/aboutdialog.h \
    src/configuration/configuration.h \
    src/display/mapcanvas.h \
    src/display/mapwindow.h \
    src/display/connectionselection.h \
    src/display/prespammedpath.h \
    src/expandoracommon/room.h \
    src/expandoracommon/frustum.h \
    src/expandoracommon/component.h \
    src/expandoracommon/coordinate.h \
    src/expandoracommon/listcycler.h \
    src/expandoracommon/abstractroomfactory.h \
    src/expandoracommon/mmapper2event.h \
    src/expandoracommon/parseevent.h \
    src/expandoracommon/property.h \
    src/expandoracommon/roomadmin.h \
    src/expandoracommon/roomrecipient.h \
    src/expandoracommon/exit.h \
    src/pathmachine/approved.h \
    src/pathmachine/experimenting.h \
    src/pathmachine/pathmachine.h \
    src/pathmachine/path.h \
    src/pathmachine/pathparameters.h \
    src/pathmachine/mmapper2pathmachine.h \
    src/pathmachine/roomsignalhandler.h \
    src/pathmachine/onebyone.h \
    src/pathmachine/crossover.h \
    src/pathmachine/syncing.h \
    src/mapfrontend/intermediatenode.h \
    src/mapfrontend/map.h \
    src/mapfrontend/mapaction.h \
    src/mapfrontend/mapfrontend.h \
    src/mapfrontend/roomcollection.h \
    src/mapfrontend/roomlocker.h \
    src/mapfrontend/roomoutstream.h \
    src/mapfrontend/searchtreenode.h \
    src/mapfrontend/tinylist.h \
    src/mapstorage/mapstorage.h \
    src/mapstorage/abstractmapstorage.h \
    src/mapstorage/basemapsavefilter.h \
    src/mapstorage/filesaver.h \
    src/mapstorage/oldroom.h \
    src/mapstorage/olddoor.h \
    src/mapstorage/roomsaver.h \
    src/mapstorage/oldconnection.h \
    src/mapstorage/progresscounter.h \
    src/parser/roompropertysetter.h \
    src/pandoragroup/CGroup.h \
    src/pandoragroup/CGroupChar.h \
    src/pandoragroup/CGroupClient.h \
    src/pandoragroup/CGroupCommunicator.h \
    src/pandoragroup/CGroupServer.h \
    src/pandoragroup/CGroupStatus.h \
    src/preferences/groupmanagerpage.h

SOURCES += \
    src/main.cpp \
    src/mapdata/mapdata.cpp \
    src/mapdata/mmapper2room.cpp \
    src/mapdata/mmapper2exit.cpp \
    src/mapdata/roomfactory.cpp \
    src/mapdata/roomselection.cpp \
    src/mapdata/customaction.cpp \
    src/mainwindow/mainwindow.cpp \
    src/mainwindow/roomeditattrdlg.cpp \
    src/mainwindow/infomarkseditdlg.cpp \
    src/mainwindow/findroomsdlg.cpp \
    src/mainwindow/aboutdialog.cpp \
    src/display/mapwindow.cpp \
    src/display/mapcanvas.cpp \
    src/display/connectionselection.cpp \
    src/display/prespammedpath.cpp \
    src/preferences/configdialog.cpp \
    src/preferences/generalpage.cpp \
    src/preferences/parserpage.cpp \
    src/preferences/pathmachinepage.cpp \
    src/preferences/ansicombo.cpp \
    src/configuration/configuration.cpp \
    src/proxy/connectionlistener.cpp \
    src/proxy/telnetfilter.cpp \
    src/proxy/proxy.cpp \
    src/parser/parser.cpp \
    src/parser/mumexmlparser.cpp \
    src/parser/abstractparser.cpp \
    src/parser/patterns.cpp \
    src/parser/roompropertysetter.cpp \
    src/expandoracommon/component.cpp \
    src/expandoracommon/coordinate.cpp \
    src/expandoracommon/frustum.cpp \
    src/expandoracommon/property.cpp \
    src/expandoracommon/mmapper2event.cpp \
    src/expandoracommon/parseevent.cpp \
    src/mapfrontend/intermediatenode.cpp \
    src/mapfrontend/map.cpp \
    src/mapfrontend/mapaction.cpp \
    src/mapfrontend/mapfrontend.cpp \
    src/mapfrontend/roomcollection.cpp \
    src/mapfrontend/roomlocker.cpp \
    src/mapfrontend/searchtreenode.cpp \
    src/pathmachine/approved.cpp \
    src/pathmachine/experimenting.cpp \
    src/pathmachine/pathmachine.cpp \
    src/pathmachine/mmapper2pathmachine.cpp \
    src/pathmachine/roomsignalhandler.cpp \
    src/pathmachine/path.cpp \
    src/pathmachine/pathparameters.cpp \
    src/pathmachine/syncing.cpp \
    src/pathmachine/onebyone.cpp \
    src/pathmachine/crossover.cpp \
    src/mapstorage/roomsaver.cpp \
    src/mapstorage/abstractmapstorage.cpp \
    src/mapstorage/basemapsavefilter.cpp \
    src/mapstorage/filesaver.cpp \
    src/mapstorage/mapstorage.cpp \
    src/mapstorage/oldconnection.cpp \
    src/mapstorage/progresscounter.cpp \
    src/pandoragroup/CGroup.cpp \
    src/pandoragroup/CGroupChar.cpp \
    src/pandoragroup/CGroupClient.cpp \
    src/pandoragroup/CGroupCommunicator.cpp \
    src/pandoragroup/CGroupServer.cpp \
    src/pandoragroup/CGroupStatus.cpp \
    src/preferences/groupmanagerpage.cpp

TARGET = mmapper

DEFINES += MMAPPER_VERSION=\\\"2.3.6\\\"
GIT_COMMIT_HASH = $$system("git log -1 --format=%h")
!isEmpty(GIT_COMMIT_HASH){
    DEFINES += GIT_COMMIT_HASH=\\\"$$GIT_COMMIT_HASH\\\"
}
GIT_BRANCH = $$system("git rev-parse --abbrev-ref HEAD")
!isEmpty(GIT_BRANCH){
    DEFINES += GIT_BRANCH=\\\"$$GIT_BRANCH\\\"
}
windows {
    contains(QMAKE_CC, gcc){
        # MingW
        LIBS += -lopengl32
    }
    contains(QMAKE_CC, cl){
        # Visual Studio
        LIBS += opengl32.lib
    }
}
linux {
    LIBS += -lopengl32
}
RESOURCES += src/resources/mmapper2.qrc
RC_ICONS = src/resources/win32/m.ico
ICON = src/resources/macosx/m.icns
TEMPLATE = app

QT += opengl network xml gui
CONFIG += release opengl network xml gui
CONFIG -= debug

RCC_DIR = build/resources
UI_DIR = build/ui
MOC_DIR = build/moc
debug {
    DESTDIR = build/bin/debug
    OBJECTS_DIR = build/debug-obj
}
release {
    DESTDIR = build/bin/release
    OBJECTS_DIR = build/release-obj
}
