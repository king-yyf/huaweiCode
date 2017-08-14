TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
DEFINES += _DEBUG
SOURCES += \
    cdn.cpp \
    deploy.cpp \
    io.cpp \
    graph.cpp

DISTFILES +=

HEADERS += \
    deploy.h \
    graph.h

DESTDIR = $$OUT_PWD/../bin
INCLUDEPATH += $$PWD/lib


