#-------------------------------------------------
#
# Project created by QtCreator 2019-07-29T09:59:27
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = avtest
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    r2d.cpp

HEADERS  += mainwindow.h \
    r2d.h

FORMS    += mainwindow.ui

INCLUDEPATH += $$PWD/../../ffmpeg-4.1.4-win32-dev/include

LIBS     += $$PWD/../../ffmpeg-4.1.4-win32-shared/bin/avcodec-58.dll \
            $$PWD/../../ffmpeg-4.1.4-win32-shared/bin/avformat-58.dll \
            $$PWD/../../ffmpeg-4.1.4-win32-shared/bin/avutil-56.dll \
            $$PWD/../../ffmpeg-4.1.4-win32-shared/bin/swscale-5.dll \
            $$PWD/../../ffmpeg-4.1.4-win32-shared/bin/swresample-3.dll
