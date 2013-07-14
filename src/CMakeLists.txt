set(mmapper_SRCS
    main.cpp
    configuration/configuration.cpp
    display/connectionselection.cpp
    display/mapcanvas.cpp
    display/mapwindow.cpp
    display/prespammedpath.cpp
    expandoracommon/component.cpp
    expandoracommon/coordinate.cpp
    expandoracommon/frustum.cpp
    expandoracommon/mmapper2event.cpp
    expandoracommon/parseevent.cpp
    expandoracommon/property.cpp
    mainwindow/aboutdialog.cpp
    mainwindow/findroomsdlg.cpp
    mainwindow/infomarkseditdlg.cpp
    mainwindow/mainwindow.cpp
    mainwindow/roomeditattrdlg.cpp
    mapdata/customaction.cpp
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
    mapstorage/basemapsavefilter.cpp
    mapstorage/filesaver.cpp
    mapstorage/mapstorage.cpp
    mapstorage/oldconnection.cpp
    mapstorage/progresscounter.cpp
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

# Application Icon
if (WIN32)
    set(mmapper_WINRC resources/mmapper2.rc)
endif (WIN32)
if (APPLE)
    set(mmapper_SRCS ${mmapper_SRCS} resources/macosx/m.icns)
    set_source_files_properties(resources/macosx/m.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE m.icns)
 endif (APPLE)

set(mmapper_MOC_HDRS
    display/connectionselection.h
    display/mapcanvas.h
    display/mapwindow.h
    display/prespammedpath.h
    expandoracommon/component.h
    mainwindow/aboutdialog.h
    mainwindow/findroomsdlg.h
    mainwindow/infomarkseditdlg.h
    mainwindow/mainwindow.h
    mainwindow/roomeditattrdlg.h
    mapdata/mapdata.h
    mapfrontend/mapfrontend.h
    mapstorage/abstractmapstorage.h
    mapstorage/mapstorage.h
    mapstorage/progresscounter.h
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
    mainwindow/aboutdialog.ui
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
    ${OPENGL_INCLUDE_DIR}
    ${QTIOCOMPRESSOR_INCLUDE_DIRS}
)

QT4_WRAP_CPP(mmapper_MOC_SRCS ${mmapper_MOC_HDRS} ${qtiocompressor_MOC_HDRS})
QT4_WRAP_UI(mmapper_UIS_H ${mmapper_UIS})
QT4_ADD_RESOURCES(mmapper_RCC_SRCS ${mmapper_RCS})

# Build the executable
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(EXECUTABLE_FLAGS WIN32 MACOSX_BUNDLE)
endif (CMAKE_BUILD_TYPE STREQUAL "Release")

add_executable(mmapper ${EXECUTABLE_FLAGS}
    ${mmapper_SRCS}
    ${mmapper_MOC_SRCS}
    ${mmapper_UIS_H}
    ${mmapper_RCC_SRCS}
    ${mmapper_WINRC}
    ${qtiocompressor_SRCS}
)

target_link_libraries(mmapper
    ${QT_QTMAIN_LIBRARY}
    ${QT_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
    ${ZLIB_LIBRARIES}
    ${QTIOCOMPRESSOR_LIBRARIES}
)

# Begin CPack Settings
#include(InstallRequiredSystemLibraries)

# Linux Install Settings
if (UNIX AND NOT APPLE)
    # Install Executables
    install(
        TARGETS mmapper
	RUNTIME DESTINATION bin
	COMPONENT applications
    )

    # Install Map
    install(
        FILES ${PROJECT_SOURCE_DIR}/map/arda.mm2
	DESTINATION share/games/mmapper
	COMPONENT map
    )

    # Linux Desktop Integration

    install(FILES mmapper.desktop
            DESTINATION share/applications
            COMPONENT desktopintegration
    )
  
    # Install icons
    foreach(RES 16 32 48)
        install(
	    FILES resources/icons/hi${RES}-app-mmapper.png
	    DESTINATION share/icons/hicolor/${RES}x${RES}/apps/
	    COMPONENT desktopintegration
	    RENAME mmapper.png
	)
    endforeach(RES)
  
    install(
        FILES resources/icons/m.png
	DESTINATION share/icons/hicolor/128x128/apps/
	COMPONENT desktopintegration
	RENAME mmapper.png
    )

    set(CPACK_GENERATOR "DEB;TGZ") # Others: RPM, STGZ
    set(CPACK_SOURCE_GENERATOR "TGZ")

    # Debian
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "nschimme@gmail.com")
    set(CPACK_DEBIAN_PACKAGE_NAME "mmapper")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqtcore4 (>= 4.1.0), libqtgui4 (>= 4.1.0), libqt4-opengl (>= 4.1.0), libqt4-network (>= 4.1.0), libqt4-xml (>= 4.1.0)")

endif (UNIX AND NOT APPLE)

# Windows Install Settings
if (WIN32)
    # Install Executables
    install(
        TARGETS mmapper
	RUNTIME DESTINATION .
	COMPONENT applications
    )

    # Install Map
    install(
        FILES ${PROJECT_SOURCE_DIR}/map/arda.mm2
	DESTINATION .
	COMPONENT map
    )

    # Install Library Files
    # Note: this is not necessary if Qt is compiled statically
    message(STATUS "Qt libraries are at ${QT_LIBRARY_DIR}")
    install(FILES
        "${QT_LIBRARY_DIR}/QtCore${QT_VERSION_MAJOR}.dll"
        "${QT_LIBRARY_DIR}/QtGui${QT_VERSION_MAJOR}.dll"
        "${QT_LIBRARY_DIR}/QtNetwork${QT_VERSION_MAJOR}.dll"
        "${QT_LIBRARY_DIR}/QtXml${QT_VERSION_MAJOR}.dll"
        "${QT_LIBRARY_DIR}/QtOpenGL${QT_VERSION_MAJOR}.dll"
        DESTINATION .
	COMPONENT libraries
    )
    if (MINGW)
	string(REGEX REPLACE "[\\|/][^\\/]+exe$" ""
	       MINGW_LIBRARY_DIR
	       ${CMAKE_CXX_COMPILER}
        )
        message(STATUS "MingW library is at ${MINGW_LIBRARY_DIR}")

        install(FILES 
            "${MINGW_LIBRARY_DIR}/mingwm10.dll"
	    DESTINATION .
	    COMPONENT libraries
	)
    else (MINGW)
	# VC++ Redistributable
	install(FILES
		${CMAKE_CURRENT_SOURCE_DIR}/resources/win32/vcredist_x86.exe
	        DESTINATION .
	        COMPONENT libraries
	)
	set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\vcredist_x86.exe\\\" /q:a'")
    endif (MINGW)

    # Install Help Documentation
    install(FILES
        "${PROJECT_SOURCE_DIR}/doc/Installation Guide.URL"
	"${PROJECT_SOURCE_DIR}/doc/mapping_advice.txt"
	DESTINATION .
	COMPONENT help
    )

    set(CPACK_GENERATOR "NSIS;ZIP") # ZIP requires 7-zip, NSIS requires NSIS
    set(CPACK_SOURCE_GENERATOR "ZIP")

    # Windows (NSIS) Settings
    set(CPACK_PACKAGE_EXECUTABLES "" "") # Broken in CMake (prefixes bin/)
    set(CPACK_NSIS_MENU_LINKS "mmapper.exe;MMapper;Installation Guide.URL;Installation Guide")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "mmapper.exe")
    set(CPACK_NSIS_DISPLAY_NAME "MMapper")
    set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\sourceforge.net\\\\projects\\\\mmapper")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/win32\\\\m.ico")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons\\\\m.png")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${CMAKE_CURRENT_SOURCE_DIR}/resources/win32\\\\m.ico")
    set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
    set(CPACK_NSIS_MODIFY_PATH "ON")

endif (WIN32)

# Apple Install Settings
if (APPLE)
    # Bundle the libraries with the binary (requires Qt 4.5)
    add_custom_command(
        TARGET mmapper
	POST_BUILD
        COMMAND macdeployqt ${CMAKE_CURRENT_BINARY_DIR}/mmapper.app
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Deploying the Qt Framework onto the bundle"
    )
   
    # Install Executables
    install(
        TARGETS mmapper
	BUNDLE DESTINATION MMapper
	RUNTIME DESTINATION .
	COMPONENT applications
    )

    # Install Map
    install(
        FILES ${PROJECT_SOURCE_DIR}/map/arda.mm2
	DESTINATION MMapper
	COMPONENT map
    )

    # Install Help Documentation
    install(FILES
        "${PROJECT_SOURCE_DIR}/doc/Installation Guide.URL"
	"${PROJECT_SOURCE_DIR}/doc/mapping_advice.txt"
	DESTINATION MMapper
	COMPONENT help
    )
    
    # If we are supporting older machines, use the following
    set(CPACK_OSX_PACKAGE_VERSION 10.5)

    # Package Settings
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_SOURCE_GENERATOR "TGZ")

    # Libraries are bundled directly
    set(CPACK_COMPONENT_LIBRARIES_HIDDEN TRUE)

    # Bundle Properties
   SET(MACOSX_BUNDLE_BUNDLE_NAME MMapper)
    SET(MACOSX_BUNDLE_BUNDLE_VERSION ${VERSION})
    SET(MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION})
    SET(MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${VERSION}")
endif (APPLE)

# More General CPack Settings
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.txt")
#set(CPACK_RESOURCE_FILE_README  "${PROJECT_SOURCE_DIR}/INSTALL.txt")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "Nils Schimmelmann")
set(CPACK_PACKAGE_CONTACT "nschimme@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A mud mapper especially written for the mud MUME")
if (SVN_BUILD)
    set(CPACK_PACKAGE_VERSION "svn-build-${SVN_BUILD}")
else (SVN_BUILD)
    set(CPACK_PACKAGE_VERSION ${VERSION})
endif (SVN_BUILD)
set(CPACK_PACKAGE_VERSION_MAJOR "2")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MMapper")

# Components:
set(CPACK_STRIP_FILES TRUE)
# Applications Component
set(CPACK_COMPONENTS_ALL applications map libraries help)
set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "MMapper")
set(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION "A mud mapper especially written for the mud MUME")
set(CPACK_COMPONENT_APPLICATIONS_REQUIRED TRUE)

# Map Component
set(CPACK_COMPONENT_MAP_DISPLAY_NAME "Included Map")
set(CPACK_COMPONENT_MAP_DESCRIPTION "A map of Arda (without secret exits)")

# Libraries Component
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Runtime Libraries")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "Qt/MingW libraries for running MMapper")

# Help Component
set(CPACK_COMPONENT_HELP_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_HELP_DESCRIPTION "Guide on how to set up your client")
set(CPACK_COMPONENT_HELP_HIDDEN TRUE)

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};applications;ALL;/")

# Source Package
set(CPACK_SOURCE_IGNORE_FILES
    "~$"
    "/\\\\.svn/"
    "/build/"
    "/winbuild/"
)
set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-source"
)
set(CPACK_PACKAGE_FILE_NAME    "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}"
)
include(CPack)
