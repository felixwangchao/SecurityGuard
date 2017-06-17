#-------------------------------------------------
#
# Project created by QtCreator 2017-06-03T14:11:07
#
#-------------------------------------------------

QT       += core gui
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SecurityGuard
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp \
    processthreadmodule_dialog.cpp \
    cpu_memory_dialog.cpp \
    cleaner_dialog.cpp \
    service_dialog.cpp \
    pe_dialog.cpp \
    virus_dialog.cpp \
    dialog.cpp

HEADERS  += mainwindow.h \
    processthreadmodule_dialog.h \
    cpu_memory_dialog.h \
    cleaner_dialog.h \
    service_dialog.h \
    pe_dialog.h \
    virus_dialog.h \
    dialog.h

FORMS    += mainwindow.ui \
    processthreadmodule_dialog.ui \
    cpu_memory_dialog.ui \
    cleaner_dialog.ui \
    service_dialog.ui \
    pe_dialog.ui \
    virus_dialog.ui \
    dialog.ui

LIBS += -lpsapi
LIBS += -lws2_32
LIBS += -lpowrprof
LIBS += -lUser32
LIBS += -lshell32

QT += network
include(qxtglobalshortcut5/qxt.pri)

RESOURCES += \
    resource.qrc
