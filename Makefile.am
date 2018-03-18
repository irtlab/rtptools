## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = rtpdump rtpplay rtpsend rtptrans
dist_bin_SCRIPTS = multidump multiplay
man_MANS     = rtpdump.1 rtpplay.1 rtpsend.1 rtptrans.1 multidump.1 multiplay.1
man_HTML     = rtpdump.html rtpplay.html rtpsend.html rtptrans.html \
               multidump.html multiplay.html

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
        vat.h

rtpdump_SOURCES = $(COMMON) rd.c rtpdump.h rtpdump.c
if DARWIN
rtpplay_SOURCES = $(COMMON) rd.c hsearch.c rtpplay.c
else
rtpplay_SOURCES = $(COMMON) rd.c rtpplay.c
endif
rtpsend_SOURCES = $(COMMON) rtpsend.c
rtptrans_SOURCES= $(COMMON) rtptrans.c

if HAVE_GROFF
GEN_HTML = groff -Thtml -mdoc
endif
if HAVE_MANDOC
GEN_HTML = mandoc -Thtml
endif

html: $(man_MANS)
if !FOUND_GEN_HTML
	@echo "No mandoc or groff to generate html, skipping."
	@exit 1
endif
	for MAN_FILE in $(man_MANS) ; do \
	  FILE=$${MAN_FILE%%.*} ; \
	  $(GEN_HTML) $${MAN_FILE} > $${FILE}.html ;\
	done
	sed s/VERSION/$(VERSION)/g rtptools.html.in > rtptools.html

rpm: $(bin_PROGRAMS) rtptools.spec dist
	mkdir -p ./rpmbuild/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
	cp rtptools-$(VERSION).tar.gz ./rpmbuild/SOURCES/.
	sed s/VERSION/$(VERSION)/g  rtptools.spec > ./rpmbuild/SPECS/rtptools-$(VERSION).spec
	rpmbuild --define "_topdir `pwd`/rpmbuild" -ba rpmbuild/SPECS/rtptools-$(VERSION).spec

clean-local:
	rm -f $(man_HTML) rtptools.html

EXTRA_DIST = ChangeLog.html bark.rtp \
        hsearch.h hsearch.c multidump multiplay \
        $(man_MANS) \
        win/*.c win/*.h win/include/*.h \
        win/include/arpa/*.h win/include/netinet/*.h \
        win/include/sys/*.h \
        rtptools.sln rtptools.suo rtptools.sdf \
        rtpdump.vcxproj* rtpplay.vcxproj* rtpsend.vcxproj* rtptrans.vcxproj* \
        rtptools.spec rtptools.html.in \
        LICENSE README.md
