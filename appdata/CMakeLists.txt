# Linux Install Settings
IF(UNIX AND NOT APPLE)
    INSTALL(FILES mmapper.appdata.xml
            DESTINATION share/metainfo
            COMPONENT desktopintegration
    )
ENDIF(UNIX AND NOT APPLE)
