QT += core xml
QT -= gui network

#QMAKE_CFLAGS_RELEASE += /MT
#QMAKE_CXXFLAGS_RELEASE += /MT
#QMAKE_CFLAGS_RELEASE -= -MD
#QMAKE_CXXFLAGS_RELEASE -= -MD

CONFIG += c++14

TARGET = SziFileEraseLib

CONFIG += staticlib
TEMPLATE = lib

HEADERS += \
    SziFileEraseDriver.h \
    SziFileSearch.h \
    DriverFileErase.h

SOURCES += \
    SziFileEraseDriver.cpp \
    SziFileSearch.cpp
