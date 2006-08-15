pkgconfigdir=$(libdir)/pkgconfig
libincludedir=$(includedir)/libburn/@BURN_MAJOR_VERSION@/libburn

lib_LTLIBRARIES = libburn/libburn.la libisofs/libisofs.la

## ========================================================================= ##

# Build libraries
libburn_libburn_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libburn_libburn_la_SOURCES = \
	libburn/async.c \
	libburn/async.h \
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
        libisofs/errors.h \
        libisofs/errors.c \
        libisofs/tree.h \
        libisofs/tree.c \
        libisofs/volume.h \
        libisofs/volume.c \
        libisofs/util.h \
        libisofs/util.c \
        libisofs/ecma119.c \
        libisofs/ecma119.h \
        libisofs/struct.h \
        libisofs/struct.c \
        libisofs/susp.h \
        libisofs/susp.c \
        libisofs/rockridge.h \
        libisofs/rockridge.c \
	libisofs/libisofs.h

libinclude_HEADERS = \
	libburn/libburn.h \
	libisofs/libisofs.h

## ========================================================================= ##

## Build test applications
noinst_PROGRAMS = \
	test/blank \
	test/burn \
	test/burniso \
	test/devices \
	test/iso \
	test/master \
	test/poll \
	test/rip \
	test/toc \
	test/structest

test_devices_CPPFLAGS = -Ilibburn
test_devices_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_devices_SOURCES = test/devices.c
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
test_burniso_CPPFLAGS = -Ilibburn
test_burniso_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_burniso_SOURCES = test/burniso.c
test_master_CPPFLAGS = -Ilibburn
test_master_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_master_SOURCES = test/master.c
test_structest_CPPFLAGS = -Ilibburn
test_structest_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_structest_SOURCES = test/structest.c
test_blank_CPPFLAGS = -Ilibburn
test_blank_LDADD = $(libburn_libburn_la_OBJECTS) $(THREAD_LIBS)
test_blank_SOURCES = test/blank.c
test_iso_CPPFLAGS = -Ilibisofs
test_iso_LDADD = $(libisofs_libisofs_la_OBJECTS) $(THREAD_LIBS)
test_iso_SOURCES = test/iso.c

## ========================================================================= ##

## Build documentation (You need Doxygen for this to work)
webhost = http://libburn-api.pykix.org
webpath = /

docdeps = \
	doc/doxygen.conf \
	doc/comments \
	libburn/libburn.h \
	libisofs/libisofs.h \
	test/burniso.c

doc: doc/html

doc/html: $(docdeps)
	$(RM) -r $@
	doxygen doc/doxygen.conf

doc-upload: doc/html
	scp -r $</* $(webhost):$(webpath)

.PHONY: doc

## ========================================================================= ##

# Indent source files
indent_files = \
	$(libisofs_libisofs_la_SOURCES) \
	$(libburn_libburn_la_SOURCES) \
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
	COPYING
