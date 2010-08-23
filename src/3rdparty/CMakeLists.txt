set(qtiocompressor_SRCS
    qtiocompressor.cpp
)

set(qtiocompressor_MOC_HDRS
    qtiocompressor.h
)

QT4_WRAP_CPP(qtiocompressor_MOC_SRCS ${qtiocompressor_MOC_HDRS})

ADD_LIBRARY(qtiocompressor STATIC
    ${qtiocompressor_SRCS}
    ${qtiocompressor_MOC_SRCS}
)
