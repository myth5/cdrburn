pkgconfigdir=$(libdir)/pkgconfig
libincludedir=$(includedir)/libburn

lib_LTLIBRARIES = libburn/libburn.la libisofs/libisofs.la

## ========================================================================= ##

# Build libraries
libburn_libburn_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libburn_libburn_la_SOURCES = \
	libburn/async.c \
	libburn/async.h \
	libburn/back_hacks.h \
	libburn/crc.c \
	libburn/crc.h \
	libburn/debug.c \
	libburn/debug.h \
	libburn/drive.c \
	libburn/drive.h \
	libburn/error.h \
	libburn/file.c \
	libburn/file.h \
	libburn/init.c \
	libburn/init.h \
	libburn/lec.c \
	libburn/lec.h \
	libburn/libburn.h \
	libburn/message.c \
	libburn/message.h \
	libburn/mmc.c \
	libburn/mmc.h \
	libburn/null.c \
	libburn/null.h \
	libburn/options.c \
	libburn/options.h \
	libburn/read.c \
	libburn/read.h \
	libburn/sbc.c \
	libburn/sbc.h \
	libburn/sector.c \
	libburn/sector.h \
	libburn/sg.c \
	libburn/sg.h \
	libburn/source.h \
	libburn/source.c \
	libburn/spc.c \
	libburn/spc.h \
	libburn/structure.c \
	libburn/structure.h \
	libburn/toc.c \
	libburn/toc.h \
	libburn/transport.h \
	libburn/util.c \
	libburn/util.h \
	libburn/write.c \
	libburn/write.h \
	version.h

libisofs_libisofs_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libisofs_libisofs_la_SOURCES = \
	libisofs/tree.h \
	libisofs/tree.c \
	libisofs/volume.h \
	libisofs/volume.c \
	libisofs/util.h \
	libisofs/util.c \
	libisofs/ecma119.c \
	libisofs/ecma119.h \
	libisofs/ecma119_tree.c \
	libisofs/ecma119_tree.h \
	libisofs/susp.h \
	libisofs/susp.c \
	libisofs/rockridge.h \
	libisofs/rockridge.c \
	libisofs/joliet.c \
	libisofs/joliet.h \
	libisofs/exclude.c \
	libisofs/exclude.h \
	libisofs/hash.h \
	libisofs/hash.c 

libinclude_HEADERS = \
	libburn/libburn.h \
	libisofs/libisofs.h

## ========================================================================= ##

## Build test applications
noinst_PROGRAMS = \
	test/libburner \
	test/burn \
	test/iso \
	test/master \
	test/poll \
	test/rip \
	test/toc \
	test/structest

bin_PROGRAMS = \
	cdrskin/cdrskin

test_libburner_CPPFLAGS = -Ilibburn
test_libburner_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_libburner_SOURCES = test/libburner.c
test_poll_CPPFLAGS = -Ilibburn
test_poll_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_poll_SOURCES = test/poll.c
test_toc_CPPFLAGS = -Ilibburn
test_toc_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_toc_SOURCES = test/toc.c
test_rip_CPPFLAGS = -Ilibburn
test_rip_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_rip_SOURCES = test/rip.c
test_burn_CPPFLAGS = -Ilibburn
test_burn_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_burn_SOURCES = test/burn.c
test_master_CPPFLAGS = -Ilibburn
test_master_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_master_SOURCES = test/master.c
test_structest_CPPFLAGS = -Ilibburn
test_structest_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_structest_SOURCES = test/structest.c
test_iso_CPPFLAGS = -Ilibisofs
test_iso_LDADD = $(libisofs_libisofs_la_OBJECTS) $(THREAD_LIBS)
test_iso_SOURCES = test/iso.c

##  cdrskin construction site - ts A60816
cdrskin_cdrskin_CPPFLAGS = -Ilibburn
cdrskin_cdrskin_CFLAGS = -DCdrskin_libburn_0_2_1
cdrskin_cdrskin_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
cdrskin_cdrskin_SOURCES = cdrskin/cdrskin.c cdrskin/cdrfifo.c cdrskin/cdrfifo.h cdrskin/cleanup.c cdrskin/cleanup.h cdrskin/cdrskin_timestamp.h
##
## Open questions: how to compute $timestamp and express -DX="$timestamp"
##                 



## ========================================================================= ##

## Build documentation (You need Doxygen for this to work)
webhost = http://libburn-api.pykix.org
webpath = /
docdir = $(DESTDIR)$(prefix)/share/doc/$(PACKAGE)-$(VERSION)

docdeps = \
	doc/doxygen.conf \
	doc/comments \
	libburn/libburn.h \
	libisofs/libisofs.h \
	test/burniso.c

doc:	doc/html

doc/html: $(docdeps)
	if [ -f ./doc/doc.lock ]; then \
		$(RM) -r doc/html; \
		doxygen doc/doxygen.conf; \
	fi

doc-upload: doc/html
	scp -r $</* $(webhost):$(webpath)

all: doc

install-data-local: 
	if [ -f ./doc/doc.lock ]; then \
		$(mkinstalldirs) $(docdir)/html; \
		$(INSTALL_DATA) doc/html/* $(docdir)/html; \
	fi

uninstall-local:
	rm -rf $(docdir)

## ========================================================================= ##

# Indent source files
indent_files = \
	$(libisofs_libisofs_la_SOURCES) \
	$(libburn_libburn_la_SOURCES) \
	$(test_libburner_SOURCES) \
	$(test_devices_SOURCES) \
	$(test_poll_SOURCES) \
	$(test_toc_SOURCES) \
	$(test_rip_SOURCES) \
	$(test_burn_SOURCES) \
	$(test_burniso_SOURCES) \
	$(test_master_SOURCES) \
	$(test_structest_SOURCES) \
	$(test_blank_SOURCES)

indent: $(indent_files)
	indent -bad -bap -nbbb -nbbo -nbc -bli0 -br -bls   \
		-cdw -ce -cli0 -ncs -nbfda -i8 -l79 -lc79   \
		-lp -saf -sai -nprs -npsl -saw -sob -ss -ut \
		-sbi0 -nsc -ts8 -npcs -ncdb -fca            \
		$^

.PHONY: indent

## ========================================================================= ##

# Extra things
nodist_pkgconfig_DATA = \
	libburn-1.pc \
	libisofs-1.pc

EXTRA_DIST = \
	libburn-1.pc.in \
	libisofs-1.pc.in \
	version.h.in \
	doc/doxygen.conf.in \
	README \
	cdrskin/README \
	cdrskin/cdrecord_spy.sh \
	cdrskin/compile_cdrskin.sh \
	cdrskin/changelog.txt \
	cdrskin/cdrskin_eng.html \
	cdrskin/wiki_plain.txt \
	COPYING

