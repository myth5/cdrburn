#!/bin/sh

# compile_cdrskin.sh  
# Copyright 2005 - 2006 Thomas Schmitt, scdbackup@gmx.net, GPL
# to be executed within  ./libburn-0.2.1  resp ./cdrskin-0.1.4

debug_opts=
def_opts=
libvers="-DCdrskin_libburn_0_2_1"
do_strip=0
static_opts=
warn_opts="-Wall"
fifo_source="cdrskin/cdrfifo.c"
compile_cdrskin=1
compile_cdrfifo=0

for i in "$@"
do
  if test "$i" = "-compile_cdrfifo"
  then
    compile_cdrfifo=1
  elif test "$i" = "-tarball_0_2"
  then
    libvers=
  elif test "$i" = "-cvs_A51208"
  then
    libvers="-DCdrskin_libburn_cvs_A51208_tS"
  elif test "$i" = "-cvs_A60220"
  then
    libvers="-DCdrskin_libburn_cvs_A60220_tS"
  elif test "$i" = "-newapi" -o "$i" = "-experimental"
  then
    def_opts="$def_opts -DCdrskin_new_api_tesT"
  elif test "$i" = "-do_not_compile_cdrskin"
  then
    compile_cdrskin=0
  elif test "$i" = "-do_diet"
  then
    fifo_source=
    def_opts="$def_opts -DCdrskin_extra_leaN"
    warn_opts=
  elif test "$i" = "-do_strip"
  then
    do_strip=1
  elif test "$i" = "-g"
  then
    debug_opts="$debug_opts -g"
  elif test "$i" = "-O2"
  then
    debug_opts="$debug_opts -O2"
  elif test "$i" = "-help" -o "$i" = "--help" -o "$i" = "-h"
  then
    echo "cdrskin/compile_cdrskin.sh : to be executed within  ./cdrskin-0.1.3.0.2.ts"
    echo "Options:"
    echo "  -compile_cdrfifo  compile program cdrskin/cdrfifo."
    echo "  -tarball_0_2      set macro to match libburn-0.2.ts.tar.gz"
    echo "  -cvs_A51208       set macro to match libburn-CVS of 8 Dec 2005."
    echo "  -cvs_A60220       set macro to match libburn-CVS of 20 Feb 2006."
    echo "  -do_not_compile_cdrskin  omit compilation of cdrskin/cdrskin."
    echo "  -experimental     use newly introduced libburn features."
    echo "  -do_diet          produce capability reduced lean version."
    echo "  -do_strip         apply program strip to compiled programs."
    echo "  -g                compile with cc option -g."
    echo "  -O2               compile with cc option -O2."
    echo "  -static           compile with cc option -static."
    exit 0
  elif test "$i" = "-static"
  then
    static_opts="-static"
  fi
done


timestamp="$(date -u '+%Y.%m.%d.%H%M%S')"
echo "Version timestamp :  $(sed -e 's/#define Cdrskin_timestamP "//' -e 's/"$//' cdrskin/cdrskin_timestamp.h)"
echo "Build timestamp   :  $timestamp"

if test "$compile_cdrskin"
then
  echo "compiling program cdrskin/cdrskin.c $static_opts $debug_opts $libvers $def_opts"
  cc $warn_opts -I. $static_opts $debug_opts $libvers $def_opts \
    -DCdrskin_build_timestamP='"'"$timestamp"'"' \
    \
    -o cdrskin/cdrskin \
    \
    cdrskin/cdrskin.c \
    $fifo_source \
    cdrskin/cleanup.c \
    \
    libburn/async.o \
    libburn/debug.o \
    libburn/drive.o \
    libburn/file.o \
    libburn/init.o \
    libburn/options.o \
    libburn/source.o \
    libburn/structure.o \
    \
    libburn/message.o \
    libburn/sg.o \
    libburn/write.o \
    \
    libburn/mmc.o \
    libburn/sbc.o \
    libburn/spc.o \
    libburn/util.o \
    \
    libburn/sector.o \
    libburn/toc.o \
    \
    libburn/crc.o \
    libburn/lec.o \
    \
    -lpthread
fi

if test "$compile_cdrfifo" = 1
then
  echo "compiling program cdrskin/cdrfifo.c $static_opts $debug_opts"
  cc $static_opts $debug_opts \
     -DCdrfifo_standalonE \
     -o cdrskin/cdrfifo \
     cdrskin/cdrfifo.c
fi

if test "$do_strip" = 1
then
  echo "stripping result cdrskin/cdrskin"
  strip cdrskin/cdrskin
  if test "$compile_cdrfifo" = 1
  then
    echo "stripping result cdrskin/cdrfifo"
    strip cdrskin/cdrfifo
  fi
fi

echo 'done.'
