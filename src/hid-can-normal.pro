#-------------------------------------------------
#
# Project created by QtCreator 2015-12-03T11:35:35
#
#-------------------------------------------------

QT       += core gui

TARGET = hid
TEMPLATE = app


SOURCES += \
    window.cpp \
    main.cpp \
    hid.cpp \
    calcbitrate.cpp

HEADERS  += \
    windows.h\
    window.h \
    hid.h \
    calcbitrate.h

LIBS += C:/MinGW/lib/libkernel32.a \
C:/MinGW/lib/libsetupapi.a \
C:/MinGW/lib/libuser32.a \
C:/MinGW/lib/libhid.a \
C:/MinGW/lib/libhidparse.a
