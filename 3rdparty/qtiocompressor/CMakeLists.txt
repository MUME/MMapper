set(qtiocompressor_SRCS
    qtiocompressor.cpp
)

set(qtiocompressor_MOC_HDRS
    qtiocompressor.h
)

QT4_WRAP_CPP(qtiocompressor_MOC_SRCS ${qtiocompressor_MOC_HDRS})

include_directories(
    ${ZLIB_INCLUDE_DIR}
)

add_library(qtiocompressor OBJECT
    ${qtiocompressor_SRCS}
    ${qtiocompressor_MOC_SRCS}
)
