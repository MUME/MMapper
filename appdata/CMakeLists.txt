# Linux Install Settings
if(UNIX AND NOT APPLE)
    install(FILES org.mume.MMapper.appdata.xml
            DESTINATION share/metainfo
            COMPONENT desktopintegration
    )
endif(UNIX AND NOT APPLE)
