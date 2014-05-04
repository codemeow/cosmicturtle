#-------------------------------------------------
#
# Project created by QtCreator 2014-04-26T16:48:32
#
#-------------------------------------------------

QT       += core
QT       += serialport
QT       -= gui

TARGET = cosmicturtle
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    com/csmturtle.cpp

HEADERS += \
    com/csmturtle.hpp \
    log/csmlogtest.hpp
