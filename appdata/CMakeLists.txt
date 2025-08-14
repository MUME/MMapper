# Linux Install Settings
if(UNIX AND NOT APPLE)
    configure_file(org.mume.MMapper.appdata.xml.in ${CMAKE_CURRENT_BINARY_DIR}/org.mume.MMapper.appdata.xml)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.mume.MMapper.appdata.xml
            DESTINATION share/metainfo
            COMPONENT desktopintegration
    )
endif(UNIX AND NOT APPLE)
