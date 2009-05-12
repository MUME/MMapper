set(mmapper_SRCS
    main.cpp
    configuration/configuration.cpp
    display/connectionselection.cpp
    display/mapcanvas.cpp
    display/mapwindow.cpp
    display/prespammedpath.cpp
    expandoracommon/abstractroomfactory.cpp
    expandoracommon/component.cpp
    expandoracommon/coordinate.cpp
    expandoracommon/frustum.cpp
    expandoracommon/init.cpp
    expandoracommon/mmapper2event.cpp
    expandoracommon/parseevent.cpp
    expandoracommon/property.cpp
    mainwindow/findroomsdlg.cpp
    mainwindow/infomarkseditdlg.cpp
    mainwindow/mainwindow.cpp
    mainwindow/roomeditattrdlg.cpp
    mapdata/customaction.cpp
    mapdata/drawstream.cpp
    mapdata/mapdata.cpp
    mapdata/mmapper2exit.cpp
    mapdata/mmapper2room.cpp
    mapdata/roomfactory.cpp
    mapdata/roomselection.cpp
    mapfrontend/intermediatenode.cpp
    mapfrontend/mapaction.cpp
    mapfrontend/map.cpp
    mapfrontend/mapfrontend.cpp
    mapfrontend/roomcollection.cpp
    mapfrontend/roomlocker.cpp
    mapfrontend/searchtreenode.cpp
    mapstorage/abstractmapstorage.cpp
    mapstorage/mapstorage.cpp
    mapstorage/oldconnection.cpp
    mapstorage/roomsaver.cpp
    pandoragroup/CGroupChar.cpp
    pandoragroup/CGroupClient.cpp
    pandoragroup/CGroupCommunicator.cpp
    pandoragroup/CGroup.cpp
    pandoragroup/CGroupServer.cpp
    pandoragroup/CGroupStatus.cpp
    parser/abstractparser.cpp
    parser/mumexmlparser.cpp
    parser/parser.cpp
    parser/patterns.cpp
    parser/roompropertysetter.cpp
    pathmachine/approved.cpp
    pathmachine/crossover.cpp
    pathmachine/experimenting.cpp
    pathmachine/mmapper2pathmachine.cpp
    pathmachine/onebyone.cpp
    pathmachine/path.cpp
    pathmachine/pathmachine.cpp
    pathmachine/pathparameters.cpp
    pathmachine/roomsignalhandler.cpp
    pathmachine/syncing.cpp
    preferences/ansicombo.cpp
    preferences/configdialog.cpp
    preferences/generalpage.cpp
    preferences/groupmanagerpage.cpp
    preferences/parserpage.cpp
    preferences/pathmachinepage.cpp
    proxy/connectionlistener.cpp
    proxy/proxy.cpp
    proxy/telnetfilter.cpp
)

set(mmapper_RCS resources/mmapper2.qrc)

if (MINGW)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mmapper2_rc.o
                       COMMAND windres.exe -I ${CMAKE_CURRENT_SOURCE_DIR}
                                           -i ${CMAKE_CURRENT_SOURCE_DIR}/resources/mmapper2.rc
                                           -o ${CMAKE_CURRENT_BINARY_DIR}/mmapper2_rc.o)
    set(mmapper_WINRC ${CMAKE_CURRENT_BINARY_DIR}/mmapper2_rc.o)
else (MINGW)
    set(mmapper_WINRC resources/mmapper2.rc)
endif (MINGW)

set(mmapper_MOC_HDRS
    display/connectionselection.h
    display/mapcanvas.h
    display/mapwindow.h
    display/prespammedpath.h
    expandoracommon/component.h
    mainwindow/findroomsdlg.h
    mainwindow/infomarkseditdlg.h
    mainwindow/mainwindow.h
    mainwindow/roomeditattrdlg.h
    mapdata/mapdata.h
    mapfrontend/mapfrontend.h
    mapstorage/abstractmapstorage.h
    mapstorage/mapstorage.h
    pandoragroup/CGroupClient.h
    pandoragroup/CGroupCommunicator.h
    pandoragroup/CGroup.h
    pandoragroup/CGroupServer.h
    parser/abstractparser.h
    parser/mumexmlparser.h
    parser/parser.h
    parser/roompropertysetter.h
    pathmachine/mmapper2pathmachine.h
    pathmachine/pathmachine.h
    pathmachine/roomsignalhandler.h
    preferences/ansicombo.h
    preferences/configdialog.h
    preferences/generalpage.h
    preferences/groupmanagerpage.h
    preferences/parserpage.h
    preferences/pathmachinepage.h
    proxy/connectionlistener.h
    proxy/proxy.h
    proxy/telnetfilter.h
)

set(mmapper_UIS
    mainwindow/findroomsdlg.ui
    mainwindow/infomarkseditdlg.ui
    mainwindow/roomeditattrdlg.ui
    preferences/generalpage.ui
    preferences/groupmanagerpage.ui
    preferences/parserpage.ui
    preferences/pathmachinepage.ui
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/configuration
    ${CMAKE_CURRENT_SOURCE_DIR}/display
    ${CMAKE_CURRENT_SOURCE_DIR}/expandoracommon
    ${CMAKE_CURRENT_SOURCE_DIR}/global
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow
    ${CMAKE_CURRENT_SOURCE_DIR}/mapdata
    ${CMAKE_CURRENT_SOURCE_DIR}/mapfrontend
    ${CMAKE_CURRENT_SOURCE_DIR}/mapstorage
    ${CMAKE_CURRENT_SOURCE_DIR}/pandoragroup
    ${CMAKE_CURRENT_SOURCE_DIR}/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/pathmachine
    ${CMAKE_CURRENT_SOURCE_DIR}/preferences
    ${CMAKE_CURRENT_SOURCE_DIR}/proxy
)

QT4_WRAP_CPP(mmapper_MOC_SRCS ${mmapper_MOC_HDRS})
QT4_WRAP_UI(mmapper_UIS_H ${mmapper_UIS})
QT4_ADD_RESOURCES(mmapper_RCC_SRCS ${mmapper_RCS})

add_executable(mmapper WIN32
    ${mmapper_SRCS}
    ${mmapper_MOC_SRCS}
    ${mmapper_UIS_H}
    ${mmapper_RCC_SRCS}
    ${mmapper_WINRC}
)

target_link_libraries(mmapper
    ${QT_QTMAIN_LIBRARY}
    ${QT_LIBRARIES}
)

install(TARGETS mmapper DESTINATION bin)

if (UNIX)
    install(FILES mmapper.desktop DESTINATION ${SHARE_INSTALL_PREFIX}/applications)
  
    # Install icons
    foreach(RES 16 32 48)
        install(
	    FILES resources/icons/hi${RES}-app-mmapper.png
	    DESTINATION ${SHARE_INSTALL_PREFIX}/icons/hicolor/${RES}x${RES}/apps/
	    RENAME mmapper.png
	)
    endforeach(RES)
  
    install(
        FILES resources/icons/m.png
	DESTINATION ${SHARE_INSTALL_PREFIX}/icons/hicolor/128x128/apps/
	RENAME mmapper.png
    )
endif (UNIX)
