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

MAN1 =	multidump.1 multiplay.1 rtpdump.1 rtpplay.1 rtpsend.1 rtptrans.1
BINS =	rtpdump rtpplay rtpsend rtptrans
MULT =	multidump multiplay
PROG =	$(BINS) $(MULT)

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
# FIXME rtptools.html(.in)
# FIXME hsearch.h hsearch.c - create have-hsearch.c

# FIXME Windows allegedly needs these _empty_ includes(!)
#win/*.c win/*.h win/include/*.h \
#win/include/arpa/*.h win/include/netinet/*.h \
#win/include/sys/*.h \

# FIXME other windows cruft
#win/rtptools.sln win/rtptools.suo \
#win/rtpdump.vcxproj* win/rtpplay.vcxproj* win/rtpsend.vcxproj* \
#win/rtptrans.vcxproj* \

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
	$(CC) $(CFLAGS) -o rtpdump $(rtpdump_OBJS) $(LDADD)

rtpplay: $(rtpplay_OBJS)
	$(CC) $(CFLAGS) -o rtpplay $(rtpplay_OBJS) $(LDADD)

rtpsend: $(rtpsend_OBJS)
	$(CC) $(CFLAGS) -o rtpsend $(rtpsend_OBJS) $(LDADD)

rtptrans: $(rtptrans_OBJS)
	$(CC) $(CFLAGS) -o rtptrans $(rtptrans_OBJS) $(LDADD)

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
	which groff  > /dev/null && groff  -Thtml -mdoc   $< > $@
	which mandoc > /dev/null && mandoc -Thtml -Wstyle $< > $@


# The rest of this file is the relevant portions of old Makefile.am
# that we need go through to make sure nothing is left behind

#if DARWIN #rtpplay_SOURCES = $(COMMON) rd.c hsearch.c rtpplay.c


#rpm: $(bin_PROGRAMS) rtptools.spec dist
#mkdir -p ./rpmbuild/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
#cp rtptools-$(VERSION).tar.gz ./rpmbuild/SOURCES/.
#sed s/VERSION/$(VERSION)/g  rtptools.spec > ./rpmbuild/SPECS/rtptools-$(VERSION).spec
#rpmbuild --define "_topdir `pwd`/rpmbuild" -ba rpmbuild/SPECS/rtptools-$(VERSION).spec

#sed s/VERSION/$(VERSION)/g rtptools.html.in > rtptools.html
#clean-local:
#rm -f $(man_HTML) rtptools.html
