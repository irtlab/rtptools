win32 {
    !contains(QMAKE_TARGET.arch, x86_64) {
        message("x86 build")
        LIBS += -lws2_32
    } else {
        message("x86_64 build")
        LIBS += -lhttpapi
    }

    ROOT_PATH = $$PWD/../..

    HEADERS += \
        $$ROOT_PATH/payload.h \
        $$ROOT_PATH/sysdep.h

    SOURCES += \
        $$ROOT_PATH/utils.c \
        $$ROOT_PATH/compat-err.c \
        $$ROOT_PATH/compat-getopt.c \
        $$ROOT_PATH/compat-progname.c \
        $$ROOT_PATH/compat-gettimeofday.c \
        $$ROOT_PATH/multimer.c \
        $$ROOT_PATH/notify.c \
        $$ROOT_PATH/payload.c \
        $$ROOT_PATH/rd.c \
        $$ROOT_PATH/winsocklib.c
}
else {
    error(Only for win32)
}
