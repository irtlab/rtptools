# rtptools Makefile
# Copyright 1996-1999 by Henning Schulzrinne

# Configuration:
# Uses the following environment variables:
#   CC    compiler (gcc, etc.)
#   ARCH  architecture (sun4, sun5, sgi, hp, ...)
#         Important note:  Use sun4 for SunOS 4.1.x, sun5 for SunOS 5.x
#         (Solaris)

# Where to put executables.
BIN_DIR = $(HOME)/$(ARCH)/bin
#BIN_DIR = $(HOME)/Misc/Nevot/bin/$(OSTYPE)

# Where to temporary object files; by default below this directory
# so that several platforms can be handled.
OBJ_DIR = $(ARCH)

# Where dynamically loadable versions of libtcl*, etc. are to be found.
# LIB_DIR = -L$(HOME)/$(ARCH)/lib -R$(HOME)/$(ARCH)/lib
LIB_DIR = -L/usr/local/lib -R/usr/local/lib

# Where statically linked libraries are to be found.
# LIB_DIR_STATIC = -L/net/step/home/$(ARCH)/lib/static
LIB_DIR_STATIC = -L/usr/local/lib

# Compiler optimization (typically, -g or -O)
OPT = -g

# Warning flags (-Wall for gcc)
WFLAGS = -Wall

# System-specific definitions:
include Makefile.$(ARCH)

# You shouldn't have to modify anything below this line.
# -----------------------------------------------------
SHELL  = /bin/sh
TCL2C  = ./tcl2c
V      = 1.11
CO     = /bin/true
LDLIBS = $(SYSLIBS)
CFLAGS = $(OPT) $(WFLAGS) $(SYSFLAGS) -D$(ARCH) -I. -I$(ARCH)
FTP    = /import/ftp/pub/schulzrinne/rtptools/
WWW    = $(HOME)/html/rtptools

all: dir $(BIN_DIR)/rtpdump \
     $(BIN_DIR)/rtpplay \
     $(BIN_DIR)/rtpsend \
     $(BIN_DIR)/rtptrans

OBJ = $(OBJ_DIR)/host2ip.o \
  $(OBJ_DIR)/hpt.o \
  $(OBJ_DIR)/multimer.o \
  $(OBJ_DIR)/notify.o

$(BIN_DIR)/rtpdump: $(OBJ_DIR)/rtpdump.o $(OBJ_DIR)/rd.o $(OBJ)
	$(CC) $(CFLAGS) $(OBJ_DIR)/rtpdump.o $(OBJ_DIR)/rd.o $(OBJ) \
    $(LIB_DIR) $(LDFLAGS) $(LDLIBS) -o $@ 

$(BIN_DIR)/rtpplay: $(OBJ_DIR)/rtpplay.o $(OBJ_DIR)/rd.o $(OBJ) $(HSEARCH_LIB)
	$(CC) $(CFLAGS) $(OBJ_DIR)/rtpplay.o $(OBJ_DIR)/rd.o $(OBJ) \
    $(LIB_DIR) $(LDFLAGS) $(LDLIBS) $(HSEARCH_LIB) -o $@

$(BIN_DIR)/rtpsend: $(OBJ_DIR)/rtpsend.o $(OBJ)
	$(CC) $(CFLAGS) $(OBJ_DIR)/rtpsend.o $(OBJ) \
    $(LIB_DIR) $(LDFLAGS) $(LDLIBS) -o $@

$(BIN_DIR)/rtptrans: $(OBJ_DIR)/rtptrans.o $(OBJ)
	$(CC) $(CFLAGS) $(OBJ_DIR)/rtptrans.o $(OBJ) \
    $(LIB_DIR) $(LDFLAGS) $(LDLIBS) -o $@ 

dir:
	-test -d $(ARCH) || mkdir $(ARCH)

$(OBJ_DIR)/host2ip.o: host2ip.c
	$(CC) -c $(CFLAGS) $? -o $@

$(OBJ_DIR)/hpt.o: hpt.c
	$(CC) -c $(CFLAGS) $? -o $@

$(OBJ_DIR)/multimer.o: multimer.c
	$(CC) -c $(CFLAGS) $? -o $@

$(OBJ_DIR)/notify.o: notify.c
	$(CC) -c $(CFLAGS) $? -o $@

$(OBJ_DIR)/rd.o: rd.c
	$(CC) -c $(CFLAGS) $? -o $@

$(OBJ_DIR)/rtpdump.o: rtpdump.c
	$(CC) -c $(CFLAGS) $? -o $@

$(OBJ_DIR)/rtpplay.o: rtpplay.c
	$(CC) -c $(CFLAGS) $? -o $@

$(OBJ_DIR)/rtpsend.o: rtpsend.c
	$(CC) -c $(CFLAGS) $? -o $@

$(OBJ_DIR)/rtptrans.o: rtptrans.c
	$(CC) -c $(CFLAGS) $? -o $@


F = README Makefile* *.[ch] *.html sun4/netinet/in.h
dist: clean rtptools-$V.tar.gz
rtptools-$V.tar.gz: $F
	cd ..; tar cvhf rtptools-$V/rtptools-$V.tar \
	  rtptools-$V/README \
	  rtptools-$V/Makefile* \
	  rtptools-$V/multi* \
	  rtptools-$V/*.[ch] \
	  rtptools-$V/*.html \
	  rtptools-$V/nt \
	  rtptools-$V/sun4/netinet/in.h \
	  rtptools-$V/freebsd/*search*.[ch]
	gzip rtptools-$V.tar
	cp -p $@ $(FTP)
	cp -p *.html $(WWW)

bindist: clean rtptools-$V.sun5.tar.gz
rtptools-$V.sun5.tar.gz: $F
	tar cvhf rtptools-$V.sun5.tar sun5/rtp*
	gzip rtptools-$V.sun5.tar
	cp -p $@ $(FTP)

clean:
	rm -f *.o *.tar.gz core* sun4/*.o sun5/*.o freebsd/*.[oa] *~
