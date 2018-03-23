# $Id: Makefile,v 1.518 2018/02/27 11:16:23 schwarze Exp $
#
# Copyright (c) 2010, 2011, 2012 Kristaps Dzonsons <kristaps@bsd.lv>
# Copyright (c) 2011, 2013-2017 Ingo Schwarze <schwarze@openbsd.org>
# Copyright (c) 2018 Jan Stary <hans@stare.cz>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

VERSION = 1.23

SRCS = \
	ansi.h		\
	host2ip.c	\
	hpt.c		\
	hsearch.c	\
	hsearch.h	\
	multimer.c	\
	multimer.h	\
	notify.c	\
	notify.h	\
	rd.c		\
	rtp.h		\
	rtpdump.c	\
	rtpdump.h	\
	rtpplay.c	\
	rtpsend.c	\
	rtptrans.c	\
	sysdep.h	\
	vat.h

BINS =	rtpdump rtpplay rtpsend rtptrans
PROG =	multidump multiplay $(BINS)
MAN1 =	multidump.1 multiplay.1 rtpdump.1 rtpplay.1 rtpsend.1 rtptrans.1

rtpdump_OBJS	= hpt.o host2ip.o                     rd.o rtpdump.o
rtpplay_OBJS	= hpt.o host2ip.o notify.o multimer.o rd.o rtpplay.o
rtpsend_OBJS	= hpt.o host2ip.o notify.o multimer.o      rtpsend.o
rtptrans_OBJS	= hpt.o host2ip.o notify.o multimer.o      rtptrans.o

DISTFILES = \
	INSTALL			\
	LICENSE			\
	Makefile		\
	Makefile.depend		\
	bark.rtp		\
	configure		\
	configure.local.example	\
	ChangeLog.html		\
	rtptools.html.in	\
	rtptools.spec		\
	$(MAN1)			\
	$(SRCS)			\
	$(TESTSRCS)

HAVE_SRCS = \
	have-err.c		\
	have-strtonum.c		\
	have-msgcontrol.c

COMPAT_SRCS = \
	compat_err.c		\
	compat_strtonum.c

COMPAT_OBJS = \
	compat_err.o		\
	compat_strtonum.o

OBJS =	$(rtpdump_OBJS) $(rtpplay_OBJS) $(rtpsend_OBJS) $(rtptrans_OBJS)
OBJS +=	$(COMPAT_OBJS)

include Makefile.local

all: $(PROG) Makefile.local

install: all

.PHONY: install clean distclean depend

include Makefile.depend

distclean: clean
	rm -f Makefile.local config.h config.h.old config.log config.log.old

clean:
	rm -f $(BINS) $(OBJS)
	rm -rf *.dSYM *.core *~ .*~

install: $(PROG) $(MAN1)
	install -d $(BINDIR)      && install -m 0755 $(PROG) $(BINDIR)
	install -d $(MANDIR)/man1 && install -m 0444 $(MAN1) $(MANDIR)/man1

uninstall:
	cd $(BINDIR)      && rm $(PROG)
	cd $(MANDIR)/man1 && rm $(MAN1)

Makefile.local config.h: configure $(HAVESRCS)
	@echo "$@ is out of date; please run ./configure"
	@exit 1

rtpdump: $(rtpdump_OBJS)
	$(CC) $(CFLAGS) -o rtpdump $(rtpdump_OBJS)

rtpplay: $(rtpplay_OBJS)
	$(CC) $(CFLAGS) -o rtpplay $(rtpplay_OBJS)

rtpsend: $(rtpsend_OBJS)
	$(CC) $(CFLAGS) -o rtpsend $(rtpsend_OBJS)

rtptrans: $(rtptrans_OBJS)
	$(CC) $(CFLAGS) -o rtptrans $(rtptrans_OBJS)

# --- maintainer targets ---

depend: config.h
	mkdep -f depend $(CFLAGS) $(SRCS)
	perl -e 'undef $$/; $$_ = <>; s|/usr/include/\S+||g; \
		s|\\\n||g; s|  +| |g; s| $$||mg; print;' \
		depend > _depend
	mv _depend depend

dist: rtptools.sha256

rtptools.sha256: rtptools.tar.gz
	sha256 rtptools.tar.gz > $@

rtptools.tar.gz: $(DISTFILES)
	mkdir -p .dist/rtptools-$(VERSION)/
	$(INSTALL) -m 0644 $(DISTFILES) .dist/mandoc-$(VERSION)
	chmod 755 .dist/mandoc-$(VERSION)/configure
	( cd .dist/ && tar czf ../$@ mandoc-$(VERSION) )
	rm -rf .dist/

.SUFFIXES: .c .o
.SUFFIXES: .1 .1.html

.c.o:
	$(CC) $(CFLAGS) -c $<

.h.h.html:
	highlight -I $< > $@

.1.1.html:
	mandoc -Thtml $< > $@
	#mandoc -Thtml -Wall,stop \
		#-Ostyle=mandoc.css,man=%N.%S.html,includes=%I.html $< > $@


# The rest of this file is the relevant portions of old Makefile.am
# that we need go through to make sure nothing is left behind
#
#bin_PROGRAMS = rtpdump rtpplay rtpsend rtptrans
#dist_bin_SCRIPTS = multidump multiplay
#man_MANS     = rtpdump.1 rtpplay.1 rtpsend.1 rtptrans.1 multidump.1 multiplay.1
#man_HTML     = rtpdump.html rtpplay.html rtpsend.html rtptrans.html \
#               multidump.html multiplay.html

#COMMON = \ ansi.h \ host2ip.c \ hpt.c \ multimer.c \ multimer.h \ notify.c \ notify.h \ rtp.h \ sysdep.h \ vat.h
#rtpdump_SOURCES = $(COMMON) rd.c rtpdump.h rtpdump.c
#if DARWIN
#rtpplay_SOURCES = $(COMMON) rd.c hsearch.c rtpplay.c
#else
#rtpplay_SOURCES = $(COMMON) rd.c rtpplay.c
#endif
#rtpsend_SOURCES = $(COMMON) rtpsend.c
#rtptrans_SOURCES= $(COMMON) rtptrans.c

#if HAVE_GROFF
#GEN_HTML = groff -Thtml -mdoc
#endif
#if HAVE_MANDOC
#GEN_HTML = mandoc -Thtml
#endif

#html: $(man_MANS)
#if !FOUND_GEN_HTML
#@echo "No mandoc or groff to generate html, skipping."
#@exit 1
#endif
#for MAN_FILE in $(man_MANS) ; do \
#FILE=$${MAN_FILE%%.*} ; \
#$(GEN_HTML) $${MAN_FILE} > $${FILE}.html ;\
#done
#sed s/VERSION/$(VERSION)/g rtptools.html.in > rtptools.html
#
#rpm: $(bin_PROGRAMS) rtptools.spec dist
#mkdir -p ./rpmbuild/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
#cp rtptools-$(VERSION).tar.gz ./rpmbuild/SOURCES/.
#sed s/VERSION/$(VERSION)/g  rtptools.spec > ./rpmbuild/SPECS/rtptools-$(VERSION).spec
#rpmbuild --define "_topdir `pwd`/rpmbuild" -ba rpmbuild/SPECS/rtptools-$(VERSION).spec
#
#clean-local:
#rm -f $(man_HTML) rtptools.html

#EXTRA_DIST = ChangeLog.html bark.rtp \
#hsearch.h hsearch.c multidump multiplay \
#$(man_MANS) \
#win/*.c win/*.h win/include/*.h \
#win/include/arpa/*.h win/include/netinet/*.h \
#win/include/sys/*.h \
#win/rtptools.sln win/rtptools.suo \
#win/rtpdump.vcxproj* win/rtpplay.vcxproj* win/rtpsend.vcxproj* \
#win/rtptrans.vcxproj* \
#rtptools.spec rtptools.html.in \
#LICENSE README.md
