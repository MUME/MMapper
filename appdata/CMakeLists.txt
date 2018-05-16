# Linux Install Settings
if(UNIX AND NOT APPLE)
    install(FILES mmapper.appdata.xml
            DESTINATION share/metainfo
            COMPONENT desktopintegration
    )
endif(UNIX AND NOT APPLE)
