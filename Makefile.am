## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = rtpdump rtpplay rtpsend rtptrans

COMMON = \
        ansi.h \
        host2ip.c \
        hpt.c \
        multimer.c \
        multimer.h \
        notify.c \
        notify.h \
        rtp.h \
        sysdep.h \
        types.h \
        vat.h

rtpdump_SOURCES = $(COMMON) rd.c rtpdump.h rtpdump.c
rtpplay_SOURCES = $(COMMON) rd.c rtpplay.c
rtpsend_SOURCES = $(COMMON) rtpsend.c
rtptrans_SOURCES= $(COMMON) rtptrans.c

EXTRA_DIST = ChangeLog rtptools.html bark.rtp \
        nt/*.c nt/include/*.h nt/include/*/*.h \
        hsearch.h multidump multiplay \
        rtptools.dsw rtpdump.dsp rtpplay.dsp rtpsend.dsp rtptrans.dsp
