QT       += core gui

greaterThan(QT_MAJOR_VERSION, 5):QT += core5compat
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
# 启用静态编译
#CONFIG += static

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    codeeditor.cpp \
    configreader.cpp \
    finddialog.cpp \
    hexviewer.cpp \
    main.cpp \
    mainwindow.cpp \
    replacedialog.cpp \
    settingsdialog.cpp \
    syntaxhighlighter.cpp

HEADERS += \
    codeeditor.h \
    configreader.h \
    finddialog.h \
    hexviewer.h \
    mainwindow.h \
    replacedialog.h \
    settingsdialog.h \
    syntaxhighlighter.h

FORMS +=

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    logo.ico \
    logo.rc

RESOURCES +=

RC_FILE += logo.rc

VERSION = 0.2025.3.22
QMAKE_TARGET_COMPANY = szy
QMAKE_TARGET_DESCRIPTION = File_Creater
QMAKE_TARGET_COPYRIGHT = Copyright(C) 2025
QMAKE_TARGET_PRODUCT = Apple_Cat
RC_LANG = 0x0800
