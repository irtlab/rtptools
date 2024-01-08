TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG -= debug_and_release debug_and_release_target
CONFIG += silent

include($$PWD/../common.pri)

SOURCES += \
    $$PWD/../../../rtptrans.c
