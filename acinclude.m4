dnl Function to link an architecture specific file
dnl LINK_ARCH_SRC(source_dir, arch, source_file)
AC_DEFUN([COPY_ARCH_SRC],
[
  echo "copying $1/$2/$3 -> $1/$3"
  old="$srcdir/$1/$2/$3"
  new="$srcdir/$1/$3"
  cat >$new <<__EOF__
/* WARNING:  This file was automatically generated!
 * Original: $old
 */
__EOF__
  cat >>$new <$old
])

AC_DEFUN([TARGET_SHIZZLE],
[
  ARCH=""

  AC_MSG_CHECKING([target operating system])

  case $target in
    *-*-linux*)
      ARCH=linux
      ;;
    *)
      AC_ERROR([You are attempting to compile for an unsupported platform])
      ;;
  esac

  AC_MSG_RESULT([$ARCH])

  # this doesn't actually do anything yet.. but it will someday when we port
  # libburn

  #COPY_ARCH_SRC(libburn, $ARCH, transport.c)
])
