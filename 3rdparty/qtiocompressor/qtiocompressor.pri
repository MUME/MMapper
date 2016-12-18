INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

qtiocompressor-uselib:!qtiocompressor-buildlib {
    LIBS += -L$$QTIOCOMPRESSOR_LIBDIR -l$$QTIOCOMPRESSOR_LIBNAME
} else {
    SOURCES += $$PWD/qtiocompressor.cpp
    HEADERS += $$PWD/qtiocompressor.h
}

win32 {
    contains(TEMPLATE, lib):contains(CONFIG, shared):DEFINES += QT_QTIOCOMPRESSOR_EXPORT
    else:qtiocompressor-uselib:DEFINES += QT_QTIOCOMPRESSOR_IMPORT
}
