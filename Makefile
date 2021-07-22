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
TARBALL = rtptools-$(VERSION).tar.gz

SRCS = \
	multimer.c	\
	multimer.h	\
	notify.c	\
	notify.h	\
	payload.c	\
	payload.h	\
	rd.c		\
	rtp.h		\
	rtpdump.c	\
	rtpdump.h	\
	rtpplay.c	\
	rtpsend.c	\
	rtptrans.c	\
	sysdep.h	\
	utils.c		\
	vat.h

BINS =	rtpdump rtpplay rtpsend rtptrans
MULT =	multidump multiplay
PROG =	$(BINS) $(MULT)

MAN1 =	multidump.1		\
	multiplay.1		\
	rtpdump.1		\
	rtpplay.1		\
	rtpsend.1		\
	rtptrans.1

HTML =	multidump.1.html	\
	multiplay.1.html	\
	rtpdump.1.html		\
	rtpplay.1.html		\
	rtpsend.1.html		\
	rtptrans.1.html

rtpdump_OBJS	= utils.o                     payload.o rd.o rtpdump.o
rtpplay_OBJS	= utils.o notify.o multimer.o payload.o rd.o rtpplay.o
rtpsend_OBJS	= utils.o notify.o multimer.o                rtpsend.o
rtptrans_OBJS	= utils.o notify.o multimer.o                rtptrans.o

HAVE_SRCS = \
	have-err.c		\
	have-getopt.c		\
	have-gethostbyname.c	\
	have-gettimeofday.c	\
	have-progname.c		\
	have-strtonum.c		\
	have-msgcontrol.c

COMPAT_SRCS = \
	compat-err.c		\
	compat-getopt.c		\
	compat-gettimeofday.c	\
	compat-progname.c	\
	compat-strtonum.c \
	winsocklib.c

COMPAT_OBJS = \
	compat-err.o		\
	compat-getopt.o		\
	compat-gettimeofday.o	\
	compat-progname.o	\
	compat-strtonum.o \
	winsocklib.o

OBJS =	$(rtpdump_OBJS) $(rtpplay_OBJS) $(rtpsend_OBJS) $(rtptrans_OBJS)
OBJS +=	$(COMPAT_OBJS)

WINDOWS = \
	win/rtptools.sln				\
	win/rtpdump.vcxproj win/rtpplay.vcxproj		\
	win/rtpsend.vcxproj win/rtptrans.vcxproj	\
	win/winsocklib.c

DISTFILES = \
	LICENSE			\
	Makefile		\
	Makefile.depend		\
	bark.rtp		\
	configure		\
	configure.local.example	\
	ChangeLog.html		\
	$(MAN1)			\
	$(MULT)			\
	$(SRCS)			\
	$(HAVE_SRCS)		\
	$(COMPAT_SRCS)

# FIXME INSTALL

include Makefile.local

all: $(PROG) Makefile.local
html: $(HTML)
install: all

.PHONY: install clean distclean depend

include Makefile.depend

clean:
	rm -f $(TARBALL) $(BINS) $(OBJS) $(HTML)
	rm -rf *.dSYM *.core *~ .*~ win/*~
	rm -rf rtptools-$(VERSION) .rpmbuild

distclean: clean
	rm -f Makefile.local config.h config.h.old config.log config.log.old

check: $(PROG) bark.rtp
	./rtpdump < bark.rtp > /dev/null
	./rtpdump -F dump < bark.rtp > dump.rtp
	./rtpdump -F dump < dump.rtp > cast.rtp
	diff dump.rtp cast.rtp
	./rtpdump -F payload < bark.rtp > bark.raw
	./rtpdump -F payload < dump.rtp > dump.raw
	diff bark.raw dump.raw
	which play > /dev/null && play -c 1 -r 8000 -e u-law bark.raw || true
	rm -f dump.rtp cast.rtp dump.raw bark.raw

install: $(PROG) $(MAN1)
	install -d $(BINDIR)      && install -m 0755 $(PROG) $(BINDIR)
	install -d $(MANDIR)/man1 && install -m 0444 $(MAN1) $(MANDIR)/man1

uninstall:
	cd $(BINDIR)      && rm $(PROG)
	cd $(MANDIR)/man1 && rm $(MAN1)

lint: $(MAN1)
	mandoc -Tlint -Wstyle $(MAN1)

Makefile.local config.h: configure $(HAVESRCS)
	@echo "$@ is out of date; please run ./configure"
	@exit 1

rtpdump: $(rtpdump_OBJS) $(COMPAT_OBJS)
	$(CC) $(CFLAGS) -o rtpdump $(rtpdump_OBJS) $(COMPAT_OBJS) $(LDADD)

rtpplay: $(rtpplay_OBJS) $(COMPAT_OBJS)
	$(CC) $(CFLAGS) -o rtpplay $(rtpplay_OBJS) $(COMPAT_OBJS) $(LDADD)

rtpsend: $(rtpsend_OBJS) $(COMPAT_OBJS)
	$(CC) $(CFLAGS) -o rtpsend $(rtpsend_OBJS) $(COMPAT_OBJS) $(LDADD)

rtptrans: $(rtptrans_OBJS) $(COMPAT_OBJS)
	$(CC) $(CFLAGS) -o rtptrans $(rtptrans_OBJS) $(COMPAT_OBJS) $(LDADD)

# --- maintainer targets ---

depend: config.h
	mkdep -f depend $(CFLAGS) $(SRCS)
	perl -e 'undef $$/; $$_ = <>; s|/usr/include/\S+||g; \
		s|\\\n||g; s|  +| |g; s| $$||mg; print;' \
		depend > _depend
	mv _depend depend

dist: $(TARBALL)

$(TARBALL): $(DISTFILES) $(WINDOWS)
	rm -rf .dist
	mkdir -p .dist/rtptools-$(VERSION)/
	$(INSTALL) -m 0644 $(DISTFILES) .dist/rtptools-$(VERSION)/
	cp -r win .dist/rtptools-$(VERSION)/
	( cd .dist/rtptools-$(VERSION) && chmod 755 configure $(MULT) )
	( cd .dist && tar czf ../$@ rtptools-$(VERSION) )
	rm -rf .dist/

distcheck: dist
	rm -rf rtptools-$(VERSION) && tar xzf $(TARBALL)
	( cd rtptools-$(VERSION) && ./configure && make all )

rpm: $(TARBALL) rtptools.spec
	rm -rf .rpmbuild
	mkdir -p .rpmbuild/RPMS
	mkdir -p .rpmbuild/SOURCES
	mkdir -p .rpmbuild/SPECS
	mkdir -p .rpmbuild/SRPMS
	cp $(TARBALL) .rpmbuild/SOURCES/
	sed s/VERSION/$(VERSION)/g  rtptools.spec > rtptools-$(VERSION).spec
	mv rtptools-$(VERSION).spec .rpmbuild/SPECS/
	rpmbuild --define "_topdir `pwd`/.rpmbuild" \
		-ba .rpmbuild/SPECS/rtptools-$(VERSION).spec

.SUFFIXES: .c .o
.SUFFIXES: .1 .1.html

.c.o:
	$(CC) $(CFLAGS) -c $<

.1.1.html:
	mandoc -Thtml -O style=style.css,man=%N.%S.html -Wstyle $< > $@ || \
	groff  -Thtml -mdoc   $< > $@
