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

rtpdump_OBJS	= hpt.o host2ip.o                     rd.o rtpdump.o
rtpplay_OBJS	= hpt.o host2ip.o notify.o multimer.o rd.o rtpplay.o
rtpsend_OBJS	= hpt.o host2ip.o notify.o multimer.o      rtpsend.o
rtptrans_OBJS	= hpt.o host2ip.o notify.o multimer.o      rtptrans.o

HAVE_SRCS = \
	have-err.c		\
	have-gethostbyname.c	\
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

# Allegedly, windows need these _empty_ include files.
# Please say that isn't so and get rid of this.
EMPTY = \
	win/include/arpa/inet.h		\
	win/include/netdb.h		\
	win/include/netinet/in.h	\
	win/include/sys/select.h	\
	win/include/sys/socket.h	\
	win/include/sys/time.h		\
	win/include/sys/uio.h		\
	win/include/unistd.h

WINDOWS = \
	win/rtptools.sln				\
	win/rtpdump.vcxproj win/rtpplay.vcxproj		\
	win/rtpsend.vcxproj win/rtptrans.vcxproj	\
	win/gettimeofday.c win/gettimeofday.h		\
	win/winsocklib.c win/winsocklib.h		\
	win/getopt.c win/getopt.h			\
	$(EMPTY)

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
	$(HAVE_SRCS)

# FIXME INSTALL
# FIXME rtptools.spec
# FIXME hsearch.h hsearch.c: have-hsearch.c, compat-hsearch.c

include Makefile.local

all: $(PROG) Makefile.local
html: $(HTML)
install: all

.PHONY: install clean distclean depend

include Makefile.depend

distclean: clean
	rm -f Makefile.local config.h config.h.old config.log config.log.old

clean:
	rm -f $(TARBALL) $(BINS) $(OBJS) $(HTML)
	rm -rf rtptools-$(VERSION) .rpmbuild win/include
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
	$(CC) $(CFLAGS) -o rtpdump $(rtpdump_OBJS) $(LDADD)

rtpplay: $(rtpplay_OBJS)
	$(CC) $(CFLAGS) -o rtpplay $(rtpplay_OBJS) $(LDADD)

rtpsend: $(rtpsend_OBJS)
	$(CC) $(CFLAGS) -o rtpsend $(rtpsend_OBJS) $(LDADD)

rtptrans: $(rtptrans_OBJS)
	$(CC) $(CFLAGS) -o rtptrans $(rtptrans_OBJS) $(LDADD)

$(EMPTY):
	mkdir -p win/include/{arpa,netinet,sys}
	touch $@

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
	mkdir -p .rpmbuild/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
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
	{ which mandoc > /dev/null && mandoc -Thtml -Wstyle $< > $@ ; } || \
	{ which groff  > /dev/null && groff  -Thtml -mdoc   $< > $@ ; }
