HEADERS += window.h \
    sqlite-3.8.8.2\sqlite3.h
SOURCES += window.cpp \
    main.cpp \
    sqlite-3.8.8.2\sqlite3.c
QT += sql widgets
CONFIG += x86 ppc x86_64 ppc64

FORMS += \
    mainwindow.ui
