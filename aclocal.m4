# aclocal.m4 generated automatically by aclocal 1.6.3 -*- Autoconf -*-

# Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002
# Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl
dnl define macros :
dnl AC_ENABLE_PRODUCTION AC_DISABLE_PRODUCTION
dnl and 
dnl AC_ENABLE_DEBUG AC_DISABLE_DEBUG
dnl
dnl version $Id$
dnl author Patrick GOLDBRONN
dnl
 
# AC_ENABLE_PRODUCTION
AC_DEFUN(AC_ENABLE_PRODUCTION, [dnl
define([AC_ENABLE_PRODUCTION_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(production,
changequote(<<, >>)dnl
<<  --enable-production[=PKGS]  build without debug information [default=>>AC_ENABLE_PRODUCTION_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case "$enableval" in
yes) enable_production=yes ;;
no) enable_production=no ;;
*)
  enable_production=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_production=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_production=AC_ENABLE_PRODUCTION_DEFAULT)dnl

if test "X$enable_production" = "Xyes"; then
  CFLAGS="$CFLAGS -O"
  CXXFLAGS="$CXXFLAGS -O -Wno-deprecated "
fi
])

# AC_DISABLE_PRODUCTION - set the default flag to --disable-production
AC_DEFUN(AC_DISABLE_PRODUCTION, [AC_ENABLE_PRODUCTION(no)])

# AC_ENABLE_DEBUG
AC_DEFUN(AC_ENABLE_DEBUG, [dnl
define([AC_ENABLE_DEBUG_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(debug,
changequote(<<, >>)dnl
<<  --enable-debug[=PKGS]  build without debug information [default=>>AC_ENABLE_DEBUG_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case "$enableval" in
yes) enable_debug=yes ;;
no) enable_debug=no ;;
*)
  enable_debug=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_debug=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_debug=AC_ENABLE_DEBUG_DEFAULT)dnl

if test "X$enable_debug" = "Xyes"; then
  CFLAGS="$CFLAGS -g -D_DEBUG_ "
  CXXFLAGS="$CXXFLAGS -g -D_DEBUG_ -Wno-deprecated "
fi
])

# AC_DISABLE_DEBUG - set the default flag to --disable-debug
AC_DEFUN(AC_DISABLE_DEBUG, [AC_ENABLE_DEBUG(no)])



# serial 40 AC_PROG_LIBTOOL
AC_DEFUN(AC_PROG_LIBTOOL,
[AC_REQUIRE([AC_LIBTOOL_SETUP])dnl

# Save cache, so that ltconfig can load it
AC_CACHE_SAVE

# Actually configure libtool.  ac_aux_dir is where install-sh is found.
CC="$CC" CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" \
LD="$LD" LDFLAGS="$LDFLAGS" LIBS="$LIBS" \
LN_S="$LN_S" NM="$NM" RANLIB="$RANLIB" \
DLLTOOL="$DLLTOOL" AS="$AS" OBJDUMP="$OBJDUMP" \
${CONFIG_SHELL-/bin/sh} $ac_aux_dir/ltconfig --no-reexec \
$libtool_flags $ac_aux_dir/ltmain.sh $lt_target \
|| AC_MSG_ERROR([libtool configure failed])

# Reload cache, that may have been modified by ltconfig
AC_CACHE_LOAD

# This can be used to rebuild libtool when needed
LIBTOOL_DEPS="$ac_aux_dir/ltconfig $ac_aux_dir/ltmain.sh"

# Always use our own libtool.
LIBTOOL='$(SHELL) $(top_builddir)/libtool'
AC_SUBST(LIBTOOL)dnl

# Redirect the config.log output again, so that the ltconfig log is not
# clobbered by the next message.
exec 5>>./config.log
])

AC_DEFUN(AC_LIBTOOL_SETUP,
[AC_PREREQ(2.13)dnl
AC_REQUIRE([AC_ENABLE_SHARED])dnl
AC_REQUIRE([AC_ENABLE_STATIC])dnl
AC_REQUIRE([AC_ENABLE_FAST_INSTALL])dnl
AC_REQUIRE([AC_CANONICAL_HOST])dnl
AC_REQUIRE([AC_CANONICAL_BUILD])dnl
AC_REQUIRE([AC_PROG_RANLIB])dnl
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_LD])dnl
AC_REQUIRE([AC_PROG_NM])dnl
AC_REQUIRE([AC_PROG_LN_S])dnl
dnl

case "$target" in
NONE) lt_target="$host" ;;
*) lt_target="$target" ;;
esac

# Check for any special flags to pass to ltconfig.
#
# the following will cause an existing older ltconfig to fail, so
# we ignore this at the expense of the cache file... Checking this 
# will just take longer ... bummer!
#libtool_flags="--cache-file=$cache_file"
#
test "$enable_shared" = no && libtool_flags="$libtool_flags --disable-shared"
test "$enable_static" = no && libtool_flags="$libtool_flags --disable-static"
test "$enable_fast_install" = no && libtool_flags="$libtool_flags --disable-fast-install"
test "$ac_cv_prog_gcc" = yes && libtool_flags="$libtool_flags --with-gcc"
test "$ac_cv_prog_gnu_ld" = yes && libtool_flags="$libtool_flags --with-gnu-ld"
ifdef([AC_PROVIDE_AC_LIBTOOL_DLOPEN],
[libtool_flags="$libtool_flags --enable-dlopen"])
ifdef([AC_PROVIDE_AC_LIBTOOL_WIN32_DLL],
[libtool_flags="$libtool_flags --enable-win32-dll"])
AC_ARG_ENABLE(libtool-lock,
  [  --disable-libtool-lock  avoid locking (might break parallel builds)])
test "x$enable_libtool_lock" = xno && libtool_flags="$libtool_flags --disable-lock"
test x"$silent" = xyes && libtool_flags="$libtool_flags --silent"

# Some flags need to be propagated to the compiler or linker for good
# libtool support.
case "$lt_target" in
*-*-irix6*)
  # Find out which ABI we are using.
  echo '[#]line __oline__ "configure"' > conftest.$ac_ext
  if AC_TRY_EVAL(ac_compile); then
    case "`/usr/bin/file conftest.o`" in
    *32-bit*)
      LD="${LD-ld} -32"
      ;;
    *N32*)
      LD="${LD-ld} -n32"
      ;;
    *64-bit*)
      LD="${LD-ld} -64"
      ;;
    esac
  fi
  rm -rf conftest*
  ;;

*-*-sco3.2v5*)
  # On SCO OpenServer 5, we need -belf to get full-featured binaries.
  SAVE_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -belf"
  AC_CACHE_CHECK([whether the C compiler needs -belf], lt_cv_cc_needs_belf,
    [AC_TRY_LINK([],[],[lt_cv_cc_needs_belf=yes],[lt_cv_cc_needs_belf=no])])
  if test x"$lt_cv_cc_needs_belf" != x"yes"; then
    # this is probably gcc 2.8.0, egcs 1.0 or newer; no need for -belf
    CFLAGS="$SAVE_CFLAGS"
  fi
  ;;

ifdef([AC_PROVIDE_AC_LIBTOOL_WIN32_DLL],
[*-*-cygwin* | *-*-mingw*)
  AC_CHECK_TOOL(DLLTOOL, dlltool, false)
  AC_CHECK_TOOL(AS, as, false)
  AC_CHECK_TOOL(OBJDUMP, objdump, false)
  ;;
])
esac
])

# AC_LIBTOOL_DLOPEN - enable checks for dlopen support
AC_DEFUN(AC_LIBTOOL_DLOPEN, [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])])

# AC_LIBTOOL_WIN32_DLL - declare package support for building win32 dll's
AC_DEFUN(AC_LIBTOOL_WIN32_DLL, [AC_BEFORE([$0], [AC_LIBTOOL_SETUP])])

# AC_ENABLE_SHARED - implement the --enable-shared flag
# Usage: AC_ENABLE_SHARED[(DEFAULT)]
#   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
#   `yes'.
AC_DEFUN(AC_ENABLE_SHARED, [dnl
define([AC_ENABLE_SHARED_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(shared,
changequote(<<, >>)dnl
<<  --enable-shared[=PKGS]  build shared libraries [default=>>AC_ENABLE_SHARED_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case "$enableval" in
yes) enable_shared=yes ;;
no) enable_shared=no ;;
*)
  enable_shared=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_shared=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_shared=AC_ENABLE_SHARED_DEFAULT)dnl
])

# AC_DISABLE_SHARED - set the default shared flag to --disable-shared
AC_DEFUN(AC_DISABLE_SHARED, [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
AC_ENABLE_SHARED(no)])

# AC_ENABLE_STATIC - implement the --enable-static flag
# Usage: AC_ENABLE_STATIC[(DEFAULT)]
#   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
#   `yes'.
AC_DEFUN(AC_ENABLE_STATIC, [dnl
define([AC_ENABLE_STATIC_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(static,
changequote(<<, >>)dnl
<<  --enable-static[=PKGS]  build static libraries [default=>>AC_ENABLE_STATIC_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case "$enableval" in
yes) enable_static=yes ;;
no) enable_static=no ;;
*)
  enable_static=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_static=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_static=AC_ENABLE_STATIC_DEFAULT)dnl
])

# AC_DISABLE_STATIC - set the default static flag to --disable-static
AC_DEFUN(AC_DISABLE_STATIC, [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
AC_ENABLE_STATIC(no)])


# AC_ENABLE_FAST_INSTALL - implement the --enable-fast-install flag
# Usage: AC_ENABLE_FAST_INSTALL[(DEFAULT)]
#   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
#   `yes'.
AC_DEFUN(AC_ENABLE_FAST_INSTALL, [dnl
define([AC_ENABLE_FAST_INSTALL_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(fast-install,
changequote(<<, >>)dnl
<<  --enable-fast-install[=PKGS]  optimize for fast installation [default=>>AC_ENABLE_FAST_INSTALL_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case "$enableval" in
yes) enable_fast_install=yes ;;
no) enable_fast_install=no ;;
*)
  enable_fast_install=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_fast_install=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_fast_install=AC_ENABLE_FAST_INSTALL_DEFAULT)dnl
])

# AC_ENABLE_FAST_INSTALL - set the default to --disable-fast-install
AC_DEFUN(AC_DISABLE_FAST_INSTALL, [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
AC_ENABLE_FAST_INSTALL(no)])

# AC_PROG_LD - find the path to the GNU or non-GNU linker
AC_DEFUN(AC_PROG_LD,
[AC_ARG_WITH(gnu-ld,
[  --with-gnu-ld           assume the C compiler uses GNU ld [default=no]],
test "$withval" = no || with_gnu_ld=yes, with_gnu_ld=no)
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_CANONICAL_HOST])dnl
AC_REQUIRE([AC_CANONICAL_BUILD])dnl
ac_prog=ld
if test "$ac_cv_prog_gcc" = yes; then
  # Check if gcc -print-prog-name=ld gives a path.
  AC_MSG_CHECKING([for ld used by GCC])
  ac_prog=`($CC -print-prog-name=ld) 2>&5`
  case "$ac_prog" in
    # Accept absolute paths.
changequote(,)dnl
    [\\/]* | [A-Za-z]:[\\/]*)
      re_direlt='/[^/][^/]*/\.\./'
changequote([,])dnl
      # Canonicalize the path of ld
      ac_prog=`echo $ac_prog| sed 's%\\\\%/%g'`
      while echo $ac_prog | grep "$re_direlt" > /dev/null 2>&1; do
	ac_prog=`echo $ac_prog| sed "s%$re_direlt%/%"`
      done
      test -z "$LD" && LD="$ac_prog"
      ;;
  "")
    # If it fails, then pretend we aren't using GCC.
    ac_prog=ld
    ;;
  *)
    # If it is relative, then search for the first ld in PATH.
    with_gnu_ld=unknown
    ;;
  esac
elif test "$with_gnu_ld" = yes; then
  AC_MSG_CHECKING([for GNU ld])
else
  AC_MSG_CHECKING([for non-GNU ld])
fi
AC_CACHE_VAL(ac_cv_path_LD,
[if test -z "$LD"; then
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}${PATH_SEPARATOR-:}"
  for ac_dir in $PATH; do
    test -z "$ac_dir" && ac_dir=.
    if test -f "$ac_dir/$ac_prog" || test -f "$ac_dir/$ac_prog$ac_exeext"; then
      ac_cv_path_LD="$ac_dir/$ac_prog"
      # Check to see if the program is GNU ld.  I'd rather use --version,
      # but apparently some GNU ld's only accept -v.
      # Break only if it was the GNU/non-GNU ld that we prefer.
      if "$ac_cv_path_LD" -v 2>&1 < /dev/null | egrep '(GNU|with BFD)' > /dev/null; then
	test "$with_gnu_ld" != no && break
      else
	test "$with_gnu_ld" != yes && break
      fi
    fi
  done
  IFS="$ac_save_ifs"
else
  ac_cv_path_LD="$LD" # Let the user override the test with a path.
fi])
LD="$ac_cv_path_LD"
if test -n "$LD"; then
  AC_MSG_RESULT($LD)
else
  AC_MSG_RESULT(no)
fi
test -z "$LD" && AC_MSG_ERROR([no acceptable ld found in \$PATH])
AC_PROG_LD_GNU
])

AC_DEFUN(AC_PROG_LD_GNU,
[AC_CACHE_CHECK([if the linker ($LD) is GNU ld], ac_cv_prog_gnu_ld,
[# I'd rather use --version here, but apparently some GNU ld's only accept -v.
if $LD -v 2>&1 </dev/null | egrep '(GNU|with BFD)' 1>&5; then
  ac_cv_prog_gnu_ld=yes
else
  ac_cv_prog_gnu_ld=no
fi])
])

# AC_PROG_NM - find the path to a BSD-compatible name lister
AC_DEFUN(AC_PROG_NM,
[AC_MSG_CHECKING([for BSD-compatible nm])
AC_CACHE_VAL(ac_cv_path_NM,
[if test -n "$NM"; then
  # Let the user override the test.
  ac_cv_path_NM="$NM"
else
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}${PATH_SEPARATOR-:}"
  for ac_dir in $PATH /usr/ccs/bin /usr/ucb /bin; do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/nm || test -f $ac_dir/nm$ac_exeext ; then
      # Check to see if the nm accepts a BSD-compat flag.
      # Adding the `sed 1q' prevents false positives on HP-UX, which says:
      #   nm: unknown option "B" ignored
      if ($ac_dir/nm -B /dev/null 2>&1 | sed '1q'; exit 0) | egrep /dev/null >/dev/null; then
	ac_cv_path_NM="$ac_dir/nm -B"
	break
      elif ($ac_dir/nm -p /dev/null 2>&1 | sed '1q'; exit 0) | egrep /dev/null >/dev/null; then
	ac_cv_path_NM="$ac_dir/nm -p"
	break
      else
	ac_cv_path_NM=${ac_cv_path_NM="$ac_dir/nm"} # keep the first match, but
	continue # so that we can try to find one that supports BSD flags
      fi
    fi
  done
  IFS="$ac_save_ifs"
  test -z "$ac_cv_path_NM" && ac_cv_path_NM=nm
fi])
NM="$ac_cv_path_NM"
AC_MSG_RESULT([$NM])
])

# AC_CHECK_LIBM - check for math library
AC_DEFUN(AC_CHECK_LIBM,
[AC_REQUIRE([AC_CANONICAL_HOST])dnl
LIBM=
case "$lt_target" in
*-*-beos* | *-*-cygwin*)
  # These system don't have libm
  ;;
*-ncr-sysv4.3*)
  AC_CHECK_LIB(mw, _mwvalidcheckl, LIBM="-lmw")
  AC_CHECK_LIB(m, main, LIBM="$LIBM -lm")
  ;;
*)
  AC_CHECK_LIB(m, main, LIBM="-lm")
  ;;
esac
])

# AC_LIBLTDL_CONVENIENCE[(dir)] - sets LIBLTDL to the link flags for
# the libltdl convenience library and INCLTDL to the include flags for
# the libltdl header and adds --enable-ltdl-convenience to the
# configure arguments.  Note that LIBLTDL and INCLTDL are not
# AC_SUBSTed, nor is AC_CONFIG_SUBDIRS called.  If DIR is not
# provided, it is assumed to be `libltdl'.  LIBLTDL will be prefixed
# with '${top_builddir}/' and INCLTDL will be prefixed with
# '${top_srcdir}/' (note the single quotes!).  If your package is not
# flat and you're not using automake, define top_builddir and
# top_srcdir appropriately in the Makefiles.
AC_DEFUN(AC_LIBLTDL_CONVENIENCE, [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
  case "$enable_ltdl_convenience" in
  no) AC_MSG_ERROR([this package needs a convenience libltdl]) ;;
  "") enable_ltdl_convenience=yes
      ac_configure_args="$ac_configure_args --enable-ltdl-convenience" ;;
  esac
  LIBLTDL='${top_builddir}/'ifelse($#,1,[$1],['libltdl'])/libltdlc.la
  INCLTDL='-I${top_srcdir}/'ifelse($#,1,[$1],['libltdl'])
])

# AC_LIBLTDL_INSTALLABLE[(dir)] - sets LIBLTDL to the link flags for
# the libltdl installable library and INCLTDL to the include flags for
# the libltdl header and adds --enable-ltdl-install to the configure
# arguments.  Note that LIBLTDL and INCLTDL are not AC_SUBSTed, nor is
# AC_CONFIG_SUBDIRS called.  If DIR is not provided and an installed
# libltdl is not found, it is assumed to be `libltdl'.  LIBLTDL will
# be prefixed with '${top_builddir}/' and INCLTDL will be prefixed
# with '${top_srcdir}/' (note the single quotes!).  If your package is
# not flat and you're not using automake, define top_builddir and
# top_srcdir appropriately in the Makefiles.
# In the future, this macro may have to be called after AC_PROG_LIBTOOL.
AC_DEFUN(AC_LIBLTDL_INSTALLABLE, [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
  AC_CHECK_LIB(ltdl, main,
  [test x"$enable_ltdl_install" != xyes && enable_ltdl_install=no],
  [if test x"$enable_ltdl_install" = xno; then
     AC_MSG_WARN([libltdl not installed, but installation disabled])
   else
     enable_ltdl_install=yes
   fi
  ])
  if test x"$enable_ltdl_install" = x"yes"; then
    ac_configure_args="$ac_configure_args --enable-ltdl-install"
    LIBLTDL='${top_builddir}/'ifelse($#,1,[$1],['libltdl'])/libltdl.la
    INCLTDL='-I${top_srcdir}/'ifelse($#,1,[$1],['libltdl'])
  else
    ac_configure_args="$ac_configure_args --enable-ltdl-install=no"
    LIBLTDL="-lltdl"
    INCLTDL=
  fi
])

dnl old names
AC_DEFUN(AM_PROG_LIBTOOL, [indir([AC_PROG_LIBTOOL])])dnl
AC_DEFUN(AM_ENABLE_SHARED, [indir([AC_ENABLE_SHARED], $@)])dnl
AC_DEFUN(AM_ENABLE_STATIC, [indir([AC_ENABLE_STATIC], $@)])dnl
AC_DEFUN(AM_DISABLE_SHARED, [indir([AC_DISABLE_SHARED], $@)])dnl
AC_DEFUN(AM_DISABLE_STATIC, [indir([AC_DISABLE_STATIC], $@)])dnl
AC_DEFUN(AM_PROG_LD, [indir([AC_PROG_LD])])dnl
AC_DEFUN(AM_PROG_NM, [indir([AC_PROG_NM])])dnl

dnl This is just to silence aclocal about the macro not being used
ifelse([AC_DISABLE_FAST_INSTALL])dnl

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl
dnl @synopsis AC_C_DEPEND_FLAG
dnl
dnl define C_DEPEND_FLAG
dnl define CXX_DEPEND_FLAG
dnl
dnl @version $Id$
dnl @author Marc Tajchman
dnl
AC_DEFUN(AC_DEPEND_FLAG,
[AC_CACHE_CHECK(which flag for dependency information generation,
ac_cv_depend_flag,
[AC_LANG_SAVE
 AC_LANG_C
 echo "conftest.o: conftest.c" > conftest.verif
 echo "int  main() { return 0; }" > conftest.c

 C_DEPEND_FLAG=
 for ac_C_DEPEND_FLAG in -xM -MM -M ; do

    rm -f conftest.d conftest.err
    ${CC-cc} ${ac_C_DEPEND_FLAG} -c conftest.c 1> conftest.d 2> conftest.err
    if test -f conftest.u ; then
       mv conftest.u conftest.d
    fi

    rm -f conftest
    diff -b -B conftest.d conftest.verif > conftest
    if test ! -s conftest ; then
       C_DEPEND_FLAG=${ac_C_DEPEND_FLAG}
       break
    fi
 done

dnl use gcc option -MG : asume unknown file will be construct later
 rm -f conftest.d conftest.err
 ${CC-cc} ${C_DEPEND_FLAG} -MG -c conftest.c 1> conftest.d 2> conftest.err
 if test -f conftest.u ; then
    mv conftest.u conftest.d
 fi
 rm -f conftest
 diff -b -B conftest.d conftest.verif > conftest
 if test ! -s conftest ; then
    C_DEPEND_FLAG=${C_DEPEND_FLAG}" -MG"
 fi

 rm -f conftest*
 if test "x${C_DEPEND_FLAG}" = "x" ; then
    echo "cannot determine flag (C language)"
    exit
 fi

 echo -n " C : " ${C_DEPEND_FLAG}

 AC_LANG_CPLUSPLUS
 echo "conftest.o: conftest.cxx" > conftest.verif
 echo "int  main() { return 0; }" > conftest.cxx

 CXX_DEPEND_FLAG=
 for ac_CXX_DEPEND_FLAG in -xM -MM -M ; do

    rm -f conftest.d conftest.err
    ${CXX-c++} ${ac_CXX_DEPEND_FLAG} -c conftest.cxx 1> conftest.d 2> conftest.err
    if test -f conftest.u ; then
       mv conftest.u conftest.d
    fi

    rm -f conftest
    diff -b -B conftest.d conftest.verif > conftest
    if test ! -s conftest ; then
       CXX_DEPEND_FLAG=${ac_CXX_DEPEND_FLAG}
       break
    fi
 done

dnl use g++ option -MG : asume unknown file will be construct later
 rm -f conftest.d conftest.err
 ${CXX-c++} ${CXX_DEPEND_FLAG} -MG -c conftest.cxx 1> conftest.d 2> conftest.err
 if test -f conftest.u ; then
    mv conftest.u conftest.d
 fi
 rm -f conftest
 diff -b -B conftest.d conftest.verif > conftest
 if test ! -s conftest ; then
    CXX_DEPEND_FLAG=${CXX_DEPEND_FLAG}" -MG"
 fi


 rm -f conftest*
 if test "x${CXX_DEPEND_FLAG}" = "x" ; then
    echo "cannot determine flag (C++ language)"
    exit
 fi

 echo -n " C++ : " ${CXX_DEPEND_FLAG}
 AC_LANG_RESTORE

 AC_SUBST(C_DEPEND_FLAG)
 AC_SUBST(CXX_DEPEND_FLAG)
])
])

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl
dnl @synopsis AC_CXX_HAVE_SSTREAM
dnl
dnl If the C++ library has a working stringstream, define HAVE_SSTREAM.
dnl
dnl @author Ben Stanley
dnl @version $Id$
dnl
dnl modified by Marc Tajchman (CEA) - 10/10/2002
dnl
AC_DEFUN([AC_CXX_HAVE_SSTREAM],
[AC_CACHE_CHECK(whether the compiler has stringstream,
HAVE_SSTREAM,
[AC_REQUIRE([AC_CXX_NAMESPACES])
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([#include <sstream>
#ifdef HAVE_NAMESPACES
using namespace std;
#endif],[stringstream message; message << "Hello"; return 0;],
 HAVE_SSTREAM=yes, HAVE_SSTREAM=no)
 AC_LANG_RESTORE
])
AC_SUBST(HAVE_SSTREAM)
])

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl
dnl @synopsis AC_CXX_NAMESPACES
dnl
dnl If the compiler can prevent names clashes using namespaces, define
dnl HAVE_NAMESPACES.
dnl
dnl @version $Id$
dnl @author Luc Maisonobe
dnl
AC_DEFUN(AC_CXX_NAMESPACES,
[AC_CACHE_CHECK(whether the compiler implements namespaces,
ac_cv_cxx_namespaces,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([namespace Outer { namespace Inner { int i = 0; }}],
                [using namespace Outer::Inner; return i;],
 ac_cv_cxx_namespaces=yes, ac_cv_cxx_namespaces=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_namespaces" = yes; then
  AC_DEFINE(HAVE_NAMESPACES,,[define if the compiler implements namespaces])
fi
])

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl
AC_DEFUN([CHECK_BOOST],[

AC_CHECKING(for BOOST Library)

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

AC_SUBST(BOOST_CPPFLAGS)
BOOST_CPPFLAGS=""
boost_ok=no

if test -z ${BOOSTDIR}; then
  AC_MSG_WARN(You must provide BOOSTDIR variable)
else
  AC_MSG_RESULT(\$BOOSTDIR = ${BOOSTDIR})
  AC_CHECKING(for boost/shared_ptr.hpp header file)
  dnl BOOST headers
  CPPFLAGS_old="${CPPFLAGS}"
  BOOST_CPPFLAGS="-ftemplate-depth-32 -I${BOOSTDIR}"
  CPPFLAGS="${CPPFLAGS} ${BOOST_CPPFLAGS}"

  AC_CHECK_HEADER(boost/shared_ptr.hpp,boost_ok=yes,boost_ok=no)

  CPPFLAGS="${CPPFLAGS_old}"
  boost_ok=yes
fi

AC_LANG_RESTORE

])dnl



dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl

AC_DEFUN([CHECK_MPICH],[

AC_REQUIRE([AC_PROG_CC])dnl

AC_ARG_WITH(mpich,
	    --with-mpich=DIR root directory path of MPICH installation,
	    WITHMPICH="yes",WITHMPICH="no")

MPICH_INCLUDES=""
MPICH_LIBS=""
if test "$WITHMPICH" = yes; then

  echo
  echo ---------------------------------------------
  echo testing mpich
  echo ---------------------------------------------
  echo
  MPICH_HOME=$withval

  if test "$MPICH_HOME"; then
    MPICH_INCLUDES="-I$MPICH_HOME/include"
    MPICH_LIBS="-L$MPICH_HOME/lib"
  fi

  CPPFLAGS_old="$CPPFLAGS"
  CPPFLAGS="$MPICH_INCLUDES $CPPFLAGS"
  AC_CHECK_HEADER(mpi.h,WITHMPICH="yes",WITHMPICH="no")
  CPPFLAGS="$CPPFLAGS_old"

  if test "$WITHMPICH" = "yes";then
    LDFLAGS_old="$LDFLAGS"
    LDFLAGS="$MPICH_LIBS $LDFLAGS"
    AC_CHECK_LIB(mpich,MPI_Init,
               AC_CHECK_LIB(pmpich, PMPI_Init,WITHMPICH="yes",WITHMPICH="no"),
               WITHMPICH="no")
    LDFLAGS="$LDFLAGS_old"
  fi

  MPICH_LIBS="$MPICH_LIBS -lpmpich -lmpich"

fi
AC_SUBST(MPICH_INCLUDES)
AC_SUBST(MPICH_LIBS)
AC_SUBST(WITHMPICH)

])dnl

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl

dnl CHECK_PYTHON([module, classes])
dnl
dnl Adds support for distributing Python modules or classes.
dnl Python library files distributed as a `module' are installed
dnl under PYTHON_SITE_PACKAGE (eg, ./python1.5/site-package/package-name)
dnl while those distributed as `classes' are installed under PYTHON_SITE
dnl (eg, ./python1.5/site-packages).  The default is to install as
dnl a `module'.

AC_DEFUN(CHECK_PYTHON,
 [
  AC_ARG_WITH(python,
   [  --with-python=DIR root directory path of python installation ],
   [PYTHON="$withval/bin/python"
    AC_MSG_RESULT("select python distribution in $withval")
   ], [
    AC_PATH_PROG(PYTHON, python)
    ])
  
  AC_CHECKING([local Python configuration])
  PYTHON_PREFIX=`echo $PYTHON | sed -e "s,[[^/]]*$,,;s,/$,,;s,^$,.,"`
  PYTHON_PREFIX=`echo $PYTHON_PREFIX | sed -e "s,[[^/]]*$,,;s,/$,,;s,^$,.,"`
  PYTHONHOME=$PYTHON_PREFIX

  AC_SUBST(PYTHON_PREFIX)
  AC_SUBST(PYTHONHOME)

  changequote(<<, >>)dnl
  PYTHON_VERSION=`$PYTHON -c "import sys; print sys.version[:3]"`
  changequote([, ])dnl
  AC_SUBST(PYTHON_VERSION)

  PY_MAKEFILE=$PYTHON_PREFIX/lib/python$PYTHON_VERSION/config/Makefile
  if test ! -f "$PY_MAKEFILE"; then
     AC_MSG_ERROR([*** Couldn't find ${PY_MAKEFILE}.  Maybe you are
*** missing the development portion of the python installation])
  fi

  AC_SUBST(PYTHON_INCLUDES)
  AC_SUBST(PYTHON_LIBS)

  PYTHON_INCLUDES=-I$PYTHON_PREFIX/include/python$PYTHON_VERSION
  PYTHON_LIBS="-L${PYTHON_PREFIX}/lib/python${PYTHON_VERSION}/config -lpython${PYTHON_VERSION}"
  PYTHON_LIB=$PYTHON_LIBS
  PYTHON_LIBA=$PYTHON_PREFIX/lib/python$PYTHON_VERSION/config/libpython$PYTHON_VERSION.a

  dnl At times (like when building shared libraries) you may want
  dnl to know which OS Python thinks this is.

  AC_SUBST(PYTHON_PLATFORM)
  PYTHON_PLATFORM=`$PYTHON -c "import sys; print sys.platform"`

  AC_SUBST(PYTHON_SITE)
  AC_ARG_WITH(python-site,
[  --with-python-site=DIR          Use DIR for installing platform independent
                                  Python site-packages],

dnl modification : by default, we install python script in salome root tree

dnl [PYTHON_SITE="$withval"
dnl python_site_given=yes],
dnl [PYTHON_SITE=$PYTHON_PREFIX"/lib/python"$PYTHON_VERSION/site-packages
dnl python_site_given=no])

[PYTHON_SITE="$withval"
python_site_given=yes],
[PYTHON_SITE=$prefix"/lib/python"$PYTHON_VERSION/site-packages
python_site_given=no])

  AC_SUBST(PYTHON_SITE_PACKAGE)
  PYTHON_SITE_PACKAGE=$PYTHON_SITE/$PACKAGE


  dnl Get PYTHON_SITE from --with-python-site-exec or from
  dnl --with-python-site or from running Python

  AC_SUBST(PYTHON_SITE_EXEC)
  AC_ARG_WITH(python-site-exec,
[  --with-python-site-exec=DIR     Use DIR for installing platform dependent
                                  Python site-packages],
[PYTHON_SITE_EXEC="$withval"],
[if test "$python_site_given" = yes; then
  PYTHON_SITE_EXEC=$PYTHON_SITE
else
  PYTHON_SITE_EXEC=$PYTHON_EXEC_PREFIX"/lib/python"$PYTHON_VERSION/site-packages
fi])

  dnl Set up the install directory
  ifelse($1, classes,
[PYTHON_SITE_INSTALL=$PYTHON_SITE],
[PYTHON_SITE_INSTALL=$PYTHON_SITE_PACKAGE])
  AC_SUBST(PYTHON_SITE_INSTALL)

  dnl Also lets automake think PYTHON means something.

  pythondir=$PYTHON_PREFIX"/lib/python"$PYTHON_VERSION/
  AC_SUBST(pythondir)

 AC_MSG_CHECKING([if we need libdb])
 PY_NEEDOPENDB=`nm $PYTHON_LIBA | grep dbopen | grep U`
  if test "x$PY_NEEDOPENDB" != "x"; then
     PYTHON_LIBS="$PYTHON_LIBS -ldb"
     AC_MSG_RESULT(yes)
  else
     AC_MSG_RESULT(no)
  fi

 AC_MSG_CHECKING([if we need libdl])
  PY_NEEDOPENDL=`nm $PYTHON_LIBA | grep dlopen | grep U`
  if test "x$PY_NEEDOPENDL" != "x"; then
     PYTHON_LIBS="$PYTHON_LIBS -ldl"
     AC_MSG_RESULT(yes)
  else
     AC_MSG_RESULT(no)
  fi

 AC_MSG_CHECKING([if we need libutil])
  PY_NEEDOPENPTY=`nm $PYTHON_LIBA | grep openpty | grep U`
  if test "x$PY_NEEDOPENPTY" != "x"; then
     PYTHON_LIBS="$PYTHON_LIBS -lutil"
     AC_MSG_RESULT(yes)
  else
     AC_MSG_RESULT(no)
  fi

 AC_MSG_CHECKING([if we need tcltk])
  PY_NEEDTCLTK=`nm $PYTHON_LIBA | grep Tcl_Init | grep U`
  if test "x$PY_NEEDTCLTK" != "x"; then
     PYTHON_LIBS="$PYTHON_LIBS -ltcl -ltk"
     AC_MSG_RESULT(yes)
  else
     AC_MSG_RESULT(no)
  fi

  python_ok=yes
  AC_MSG_RESULT(looks good)])

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl

AC_DEFUN([CHECK_SWIG],[
AC_REQUIRE([CHECK_PYTHON])dnl

swig_ok=yes

AC_ARG_WITH(swig,
    [  --with-swig=EXEC swig executable ],
    [SWIG="$withval"
      AC_MSG_RESULT("select $withval as swig executable")
    ], [
      AC_PATH_PROG(SWIG, swig)
    ])

if test "x$SWIG" = "x"
then
    swig_ok=no
    AC_MSG_RESULT(swig not in PATH variable)
fi

if  test "x$swig_ok" = "xyes"
then
   AC_MSG_CHECKING(python wrapper generation with swig)
   cat > conftest.h << EOF
int f(double);
EOF

   $SWIG -module conftest -python conftest.h >/dev/null 2>&1
   if test -f conftest_wrap.c
   then
      SWIG_FLAGS="-c++ -python -shadow"
   else
      swig_ok=no  
   fi
   rm -f conftest*
   AC_MSG_RESULT($swig_ok) 
fi

AC_SUBST(SWIG_FLAGS)
AC_SUBST(SWIG)

AC_MSG_RESULT(for swig: $swig_ok)

])dnl
dnl

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl
dnl@synopsis ENABLE_PTHREADS
dnl
dnl modify CFLAGS, CXXFLAGS and LIBS for compiling pthread-based programs.
dnl
dnl@author  (C) Ruslan Shevchenko <Ruslan@Shevchenko.Kiev.UA>, 1998, 2000
dnl@id  $Id$
dnl
dnl
AC_DEFUN([ENABLE_PTHREADS],[
AC_REQUIRE([CHECK_PTHREADS])

if test -z "$enable_pthreads_done"
then
 CFLAGS="$CFLAGS $CFLAGS_PTHREADS"
 CXXFLAGS="$CXXFLAGS $CXXFLAGS_PTHREADS"
 LIBS="$LIBS $LIBS_PTHREADS"
fi
enable_pthreads_done=yes
])dnl
dnl

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl
#@synonpsis CHECK_PTHREADS
dnl  check for pthreads system interfaces.
dnl  set CFLAGS_PTHREADS,  CXXFLAGS_PTHREADS and LIBS_PTHREADS to
dnl  flags to compiler flags for multithread program compilation (if exists),
dnl  and library, if one required.
dnl
dnl@author   (C) Ruslan Shevchenko <Ruslan@Shevchenko.Kiev.UA>, 1998
dnl@id $Id$
dnl ----------------------------------------------------------------
dnl CHECK_PTHREADS
AC_DEFUN(CHECK_PTHREADS,[
AC_REQUIRE([AC_CANONICAL_SYSTEM])dnl
AC_CHECK_HEADER(pthread.h,AC_DEFINE(HAVE_PTHREAD_H))
AC_CHECK_LIB(posix4,nanosleep, LIBS_PTHREADS="-lposix4",LIBS_PTHREADS="")
AC_CHECK_LIB(pthread,pthread_mutex_lock, 
             LIBS_PTHREADS="-lpthread $LIBS_PTHREADS")
AC_MSG_CHECKING([parameters for using pthreads])
case $build_os in
  freebsd*)
    CFLAGS_PTHREADS="-pthread"
    CXXFLAGS_PTHREADS="-pthread"
    ;;
  *)
    ;;
esac
AC_MSG_RESULT(["flags: $CFLAGS_PTHREADS\;libs: $LIBS_PTHREADS"])
threads_ok=yes
])dnl
dnl
dnl


AC_DEFUN([CHECK_OMNIORB],[
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_CXX])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
AC_REQUIRE([AC_PROG_CXXCPP])dnl

AC_CHECKING(for omniORB)
omniORB_ok=yes

if test "x$PYTHON" = "x" 
then
  CHECK_PYTHON
fi

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

AC_PATH_PROG(OMNIORB_IDL, omniidl)
if test "xOMNIORB_IDL" = "x"
then
  omniORB_ok=no
  AC_MSG_RESULT(omniORB binaries not in PATH variable)
else
  omniORB_ok=yes
fi

if  test "x$omniORB_ok" = "xyes"
then
  AC_SUBST(OMNIORB_IDL)

  OMNIORB_BIN=`echo ${OMNIORB_IDL} | sed -e "s,[[^/]]*$,,;s,/$,,;s,^$,.,"`
  OMNIORB_LIB=`echo ${OMNIORB_BIN} | sed -e "s,bin,lib,"`

  OMNIORB_ROOT=`echo ${OMNIORB_BIN}  | sed -e "s,[[^/]]*$,,;s,/$,,;s,^$,.,"`
  OMNIORB_ROOT=`echo ${OMNIORB_ROOT} | sed -e "s,[[^/]]*$,,;s,/$,,;s,^$,.,"`
  AC_SUBST(OMNIORB_ROOT)

  OMNIORB_INCLUDES="-I$OMNIORB_ROOT/include -I$OMNIORB_ROOT/include/omniORB3 -I$OMNIORB_ROOT/include/COS"
dnl  OMNIORB_INCLUDES="-I$OMNIORB_ROOT/include -I$OMNIORB_ROOT/include/omniORB4 -I$OMNIORB_ROOT/include/COS"
  AC_SUBST(OMNIORB_INCLUDES)

  ENABLE_PTHREADS

  OMNIORB_CXXFLAGS=
  case $build_cpu in
    sparc*)
      AC_DEFINE(__sparc__)
      OMNIORB_CXXFLAGS="$OMNIORB_CXXFLAGS -D__sparc__"
      ;;
   *86*)
      AC_DEFINE(__x86__)
      OMNIORB_CXXFLAGS="$OMNIORB_CXXFLAGS -D__x86__"
      ;;
  esac
  case $build_os in
    solaris*)
      AC_DEFINE(__sunos__)
      __OSVERSION__=5
      AC_DEFINE(__OSVERSION__)
      OMNIORB_CXXFLAGS="$OMNIORB_CXXFLAGS -D__sunos__"
      ;;
   linux*)
      AC_DEFINE(__linux__)
      __OSVERSION__=2
      AC_DEFINE(__OSVERSION__)
      OMNIORB_CXXFLAGS="$OMNIORB_CXXFLAGS -D__linux__"
      ;;
  esac
  AC_SUBST(OMNIORB_CXXFLAGS)

  CPPFLAGS_old=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS $OMNIORB_CXXFLAGS $OMNIORB_INCLUDES"

  AC_LANG_CPLUSPLUS
  AC_CHECK_HEADER(CORBA.h,omniORB_ok="yes",omniORB_ok="no")

  CPPFLAGS=$CPPFLAGS_old

fi

dnl omniORB_ok=yes

if test "x$omniORB_ok" = "xyes" 
then
  OMNIORB_LDFLAGS="-L$OMNIORB_LIB"

  LIBS_old=$LIBS
  LIBS="$LIBS $OMNIORB_LDFLAGS -lomnithread"

  CXXFLAGS_old=$CXXFLAGS
  CXXFLAGS="$CXXFLAGS $OMNIORB_CXXFLAGS $OMNIORB_INCLUDES"

  AC_MSG_CHECKING(whether we can link with omnithreads)
  AC_CACHE_VAL(salome_cv_lib_omnithreads,[
    AC_TRY_LINK(
#include <omnithread.h>
,   omni_mutex my_mutex,
    eval "salome_cv_lib_omnithreads=yes",eval "salome_cv_lib_omnithreads=no")
  ])

  omniORB_ok="$salome_cv_lib_omnithreads"
  if  test "x$omniORB_ok" = "xno"
  then
    AC_MSG_RESULT(omnithreads not found)
  else
    AC_MSG_RESULT(yes)
  fi

  LIBS=$LIBS_old
  CXXFLAGS=$CXXFLAGS_old
fi


dnl omniORB_ok=yes
if test "x$omniORB_ok" = "xyes" 
then

  AC_CHECK_LIB(socket,socket, LIBS="-lsocket $LIBS",,)
  AC_CHECK_LIB(nsl,gethostbyname, LIBS="-lnsl $LIBS",,)

  LIBS_old=$LIBS
  OMNIORB_LIBS="$OMNIORB_LDFLAGS -lomniORB3 -ltcpwrapGK -lomniDynamic3 -lomnithread -lCOS3 -lCOSDynamic3"
dnl  OMNIORB_LIBS="$OMNIORB_LDFLAGS -lomniORB4 -lomniDynamic4 -lomnithread -lCOS4 -lCOSDynamic4"
  AC_SUBST(OMNIORB_LIBS)

  LIBS="$OMNIORB_LIBS $LIBS"
  CXXFLAGS_old=$CXXFLAGS
  CXXFLAGS="$CXXFLAGS $OMNIORB_CXXFLAGS $OMNIORB_INCLUDES"

  AC_MSG_CHECKING(whether we can link with omniORB3)
  AC_CACHE_VAL(salome_cv_lib_omniorb3,[
    AC_TRY_LINK(
#include <CORBA.h>
,   CORBA::ORB_var orb,
    eval "salome_cv_lib_omniorb3=yes",eval "salome_cv_lib_omniorb3=no")
  ])
  omniORB_ok="$salome_cv_lib_omniorb3"

  omniORB_ok=yes
  if test "x$omniORB_ok" = "xno" 
  then
    AC_MSG_RESULT(omniORB library linking failed)
    omniORB_ok=no
  else
    AC_MSG_RESULT(yes)
  fi
  LIBS="$LIBS_old"
  CXXFLAGS=$CXXFLAGS_old
fi


if test "x$omniORB_ok" = "xyes" 
then

  OMNIORB_IDLCXXFLAGS="-I$OMNIORB_ROOT/idl"
  OMNIORB_IDLPYFLAGS="-bpython -I$OMNIORB_ROOT/idl"
  AC_SUBST(OMNIORB_IDLCXXFLAGS)
  AC_SUBST(OMNIORB_IDLPYFLAGS)

  OMNIORB_IDL_CLN_H=.hh
  OMNIORB_IDL_CLN_CXX=SK.cc
  OMNIORB_IDL_CLN_OBJ=SK.o 
  AC_SUBST(OMNIORB_IDL_CLN_H)
  AC_SUBST(OMNIORB_IDL_CLN_CXX)
  AC_SUBST(OMNIORB_IDL_CLN_OBJ)

  OMNIORB_IDL_SRV_H=.hh
  OMNIORB_IDL_SRV_CXX=SK.cc
  OMNIORB_IDL_SRV_OBJ=SK.o
  AC_SUBST(OMNIORB_IDL_SRV_H)
  AC_SUBST(OMNIORB_IDL_SRV_CXX)
  AC_SUBST(OMNIORB_IDL_SRV_OBJ)

  OMNIORB_IDL_TIE_H=
  OMNIORB_IDL_TIE_CXX=
  AC_SUBST(OMNIORB_IDL_TIE_H)
  AC_SUBST(OMNIORB_IDL_TIE_CXX)
  
  AC_DEFINE(OMNIORB)

  CORBA_HAVE_POA=1
  AC_DEFINE(CORBA_HAVE_POA)

  CORBA_ORB_INIT_HAVE_3_ARGS=1
  AC_DEFINE(CORBA_ORB_INIT_HAVE_3_ARGS)
  CORBA_ORB_INIT_THIRD_ARG='"omniORB3"'
  AC_DEFINE(CORBA_ORB_INIT_THIRD_ARG, "omniORB3")

fi

omniORBpy_ok=no
if  test "x$omniORB_ok" = "xyes"
then
  AC_MSG_CHECKING(omniORBpy (CORBA.py file available))
  if test -f ${OMNIORB_ROOT}/lib/python/CORBA.py
  then
    omniORBpy_ok=yes
    PYTHONPATH=${OMNIORB_ROOT}/lib/python:${OMNIORB_LIB}:${PYTHON_PREFIX}/lib/python${PYTHON_VERSION}:${PYTHONPATH}
    AC_SUBST(PYTHONPATH)
    AC_MSG_RESULT(yes)
  fi
fi

AC_LANG_RESTORE

AC_MSG_RESULT(for omniORBpy: $omniORBpy_ok)
AC_MSG_RESULT(for omniORB: $omniORB_ok)

# Save cache
AC_CACHE_SAVE

])dnl
dnl

#  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
#  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
# 
#  This library is free software; you can redistribute it and/or 
#  modify it under the terms of the GNU Lesser General Public 
#  License as published by the Free Software Foundation; either 
#  version 2.1 of the License. 
# 
#  This library is distributed in the hope that it will be useful, 
#  but WITHOUT ANY WARRANTY; without even the implied warranty of 
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
#  Lesser General Public License for more details. 
# 
#  You should have received a copy of the GNU Lesser General Public 
#  License along with this library; if not, write to the Free Software 
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
# 
#  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
#
#
AC_DEFUN([CHECK_CORBA],[

if test x"$DEFAULT_ORB" = x"omniORB"
then

  #  Contient le nom de l'ORB
  ORB=omniorb

  AC_MSG_RESULT(default orb : omniORB)
  IDL=$OMNIORB_IDL
  AC_SUBST(IDL)

  CORBA_ROOT=$OMNIORB_ROOT
  CORBA_INCLUDES=$OMNIORB_INCLUDES
  CORBA_CXXFLAGS=$OMNIORB_CXXFLAGS
  CORBA_LIBS=$OMNIORB_LIBS
  IDLCXXFLAGS=$OMNIORB_IDLCXXFLAGS
  IDLPYFLAGS=$OMNIORB_IDLPYFLAGS

  AC_SUBST(CORBA_ROOT)
  AC_SUBST(CORBA_INCLUDES)
  AC_SUBST(CORBA_CXXFLAGS)
  AC_SUBST(CORBA_LIBS)
  AC_SUBST(IDLCXXFLAGS)
  AC_SUBST(IDLPYFLAGS)

  IDL_CLN_H=$OMNIORB_IDL_CLN_H
  IDL_CLN_CXX=$OMNIORB_IDL_CLN_CXX
  IDL_CLN_OBJ=$OMNIORB_IDL_CLN_OBJ

  AC_SUBST(IDL_CLN_H)
  AC_SUBST(IDL_CLN_CXX)
  AC_SUBST(IDL_CLN_OBJ)

  IDL_SRV_H=$OMNIORB_IDL_SRV_H
  IDL_SRV_CXX=$OMNIORB_IDL_SRV_CXX
  IDL_SRV_OBJ=$OMNIORB_IDL_SRV_OBJ

  AC_SUBST(IDL_SRV_H)
  AC_SUBST(IDL_SRV_CXX)
  AC_SUBST(IDL_SRV_OBJ)

else
    AC_MSG_RESULT($DEFAULT_ORB unknown orb)

fi

])dnl
dnl

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl
AC_DEFUN([CHECK_OPENGL],[
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
AC_ARG_WITH(opengl,
   [  --with-opengl=DIR root directory path of OpenGL installation ],
   [
      opengl_dir="$withval"
      local_opengl=yes
    ], [
      dirs="/usr/lib /usr/local/lib /opt/graphics/OpenGL/lib /usr/openwin/lib /usr/X11R6/lib"
    ])dnl

AC_CHECKING(for OpenGL)

AC_SUBST(OGL_INCLUDES)
AC_SUBST(OGL_LIBS)

OGL_INCLUDES=""
OGL_LIBS=""

OpenGL_ok=no

dnl openGL headers

# by default

if test "x$local_opengl" = "xyes" ; then
   if test -f "${opengl_dir}/include/GL/gl.h" ; then
      OpenGL_ok=yes
      OGL_INCLUDES="-I${opengl_dir}/include"
      OGL_LIBS="-L${opengl_dir}/lib"
      AC_MSG_RESULT(select OpenGL distribution in ${opengl_dir})
   else
      AC_MSG_RESULT(no gl.h header file in ${opengl_dir}/include/GL)
   fi
fi

if  test "x$OpenGL_ok" = "xno"
then
  AC_CHECK_HEADERS(GL/gl.h, [OpenGL_ok=yes])
fi

if  test "x$OpenGL_ok" = "xno"
then
# under SunOS ?
  AC_CHECK_HEADERS(/usr/openwin/share/include/GL/glxmd.h,
                  [OpenGL_ok=yes]
                  OGL_INCLUDES="-I/usr/openwin/share/include/")
fi

if  test "x$OpenGL_ok" = "xno"
then
# under IRIX ?
  AC_CHECK_HEADERS(/opt/graphics/OpenGL/include/GL/glxmd.h,
                  [OpenGL_ok=yes]
                  OGL_INCLUDES="-I/opt/graphics/OpenGL/include")
fi
if  test "x$OpenGL_ok" = "xno"
then
# some linux OpenGL servers hide the includes in /usr/X11R6/include/GL
  AC_CHECK_HEADERS(/usr/X11R6/include/GL/gl.h,
                  [OpenGL_ok=yes]
                  OGL_INCLUDES="-I/usr/X11R6/include")
fi

if  test "x$OpenGL_ok" = "xyes"
then
  AC_CHECKING(for OpenGL library)
  OpenGL_ok=no
  for i in $dirs; do
    if test -r "$i/libGL.so"; then
dnl      AC_MSG_RESULT(in $i)
      OGL_LIBS="-L$i"
      break
    fi
# under IRIX ?
    if test -r "$i/libGL.sl"; then
dnl      AC_MSG_RESULT(in $i)
      OGL_LIBS="-L$i"
      break
    fi
  done
  LDFLAGS_old="$LDFLAGS"
  LDFLAGS="$LDFLAGS $OGL_LIBS"
  AC_CHECK_LIB(GL,glBegin,OpenGL_ok=yes,OpenGL_ok=no)
  LDFLAGS="$LDFLAGS_old"
fi

if test "x$OpenGL_ok" = "xyes" ; then
  OGL_LIBS="$OGL_LIBS -lGL"
fi


OpenGLU_ok=no
LDFLAGS_old="$LDFLAGS"
LDFLAGS="$LDFLAGS $OGL_LIBS"
AC_CHECK_LIB(GLU,gluBeginSurface,OpenGLU_ok=yes,OpenGLU_ok=no)
LDFLAGS="$LDFLAGS_old"

if test "x$OpenGLU_ok" = "xyes" ; then
  OGL_LIBS="$OGL_LIBS -lGLU"
fi

# Save cache
AC_CACHE_SAVE

])dnl

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl

AC_DEFUN([CHECK_QT],[
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_CXX])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
AC_REQUIRE([AC_PROG_CXXCPP])dnl
AC_REQUIRE([CHECK_OPENGL])dnl

AC_CHECKING(for QT)
qt_ok=yes

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

if test "x$QTDIR" = "x"
then
   AC_MSG_RESULT(please define QTDIR variable)
   qt_ok=no
fi

if  test "x$qt_ok" = "xyes"
then
  if test -f ${QTDIR}/bin/moc
  then
    MOC=${QTDIR}/bin/moc
  else
    AC_PATH_PROG(MOC, moc)
  fi
  if test "x$MOC" = "x"
  then
    qt_ok=no
    AC_MSG_RESULT(moc qt-compiler not in PATH variable)
  else
    qt_ok=yes
    AC_MSG_RESULT(moc found)
  fi
fi

if  test "x$qt_ok" = "xyes"
then
  if test -f ${QTDIR}/bin/uic
  then
    UIC=${QTDIR}/bin/uic
  else
    AC_PATH_PROG(UIC, uic)
  fi
  if test "x$UIC" = "x"
  then
    qt_ok=no
    AC_MSG_RESULT(uic qt-interface compiler not in PATH variable)
  else
    qt_ok=yes
    AC_MSG_RESULT(uic found)
  fi
fi

AC_SUBST(QTDIR)
QT_ROOT=$QTDIR

if  test "x$qt_ok" = "xyes"
then
  AC_MSG_CHECKING(include of qt headers)

  CPPFLAGS_old=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS -I$QTDIR/include"

  AC_LANG_CPLUSPLUS
  AC_CHECK_HEADER(qaction.h,qt_ok=yes ,qt_ok=no)

  CPPFLAGS=$CPPFLAGS_old

  if  test "x$qt_ok" = "xno"
  then
    AC_MSG_RESULT(qt headers not found, or too old qt version, in $QTDIR/include)
    AC_MSG_RESULT(QTDIR environment variable may be wrong)
  else
    AC_MSG_RESULT(yes)
       QT_INCLUDES="-I${QT_ROOT}/include -DQT_THREAD_SUPPORT"
    QT_MT_INCLUDES="-I${QT_ROOT}/include -DQT_THREAD_SUPPORT"
  fi
fi

if  test "x$qt_ok" = "xyes"
then
  AC_MSG_CHECKING(linking qt library)
  LIBS_old=$LIBS
  LIBS="$LIBS -L$QTDIR/lib -lqt-mt $OGL_LIBS"

  CXXFLAGS_old=$CXXFLAGS
  CXXFLAGS="$CXXFLAGS -I$QTDIR/include"

  AC_CACHE_VAL(salome_cv_lib_qt,[
    AC_TRY_LINK(
#include <qapplication.h>
,   int n;
    char **s;
    QApplication a(n, s);
    a.exec();,
    eval "salome_cv_lib_qt=yes",eval "salome_cv_lib_qt=no")
  ])
  qt_ok="$salome_cv_lib_qt"

  if  test "x$qt_ok" = "xno"
  then
    AC_MSG_RESULT(unable to link with qt library)
    AC_MSG_RESULT(QTDIR environment variable may be wrong)
  else
    AC_MSG_RESULT(yes)
       QT_LIBS="-L$QTDIR/lib -lqt-mt"
    QT_MT_LIBS="-L$QTDIR/lib -lqt-mt"
  fi

  LIBS=$LIBS_old
  CXXFLAGS=$CXXFLAGS_old

fi

AC_SUBST(MOC)
AC_SUBST(UIC)

AC_SUBST(QT_ROOT)
AC_SUBST(QT_INCLUDES)
AC_SUBST(QT_LIBS)
AC_SUBST(QT_MT_LIBS)

AC_LANG_RESTORE

AC_MSG_RESULT(for qt: $qt_ok)

# Save cache
AC_CACHE_SAVE

])dnl
dnl

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl

AC_DEFUN([CHECK_VTK],[
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_CXX])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
AC_REQUIRE([AC_PROG_CXXCPP])dnl

AC_CHECKING(for VTK)

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

AC_SUBST(VTK_INCLUDES)
AC_SUBST(VTK_LIBS)
AC_SUBST(VTKPY_MODULES)

VTK_INCLUDES=""
VTK_LIBS=""
VTKPY_MODULES=""

vtk_ok=no

AC_PATH_X

if test "x$OpenGL_ok" != "xyes"
then
   AC_MSG_WARN(vtk needs OpenGL correct configuration, check configure output)
fi


LOCAL_INCLUDES="$OGL_INCLUDES"
LOCAL_LIBS="-lvtkCommon -lvtkGraphics -lvtkImaging -lvtkFiltering -lvtkIO -lvtkRendering -lvtkHybrid $OGL_LIBS -L$x_libraries -lX11 -lXt"
TRY_LINK_LIBS="-lvtkCommon $OGL_LIBS -L$x_libraries -lX11 -lXt"

if test -z $VTKHOME
then 
   AC_MSG_WARN(undefined VTKHOME variable which specify where vtk was compiled)
else
   LOCAL_INCLUDES="-I$VTKHOME/include/vtk $LOCAL_INCLUDES"
   LOCAL_LIBS="-L$VTKHOME/lib/vtk $LOCAL_LIBS"
   TRY_LINK_LIBS="-L$VTKHOME/lib/vtk $TRY_LINK_LIBS"
fi

dnl vtk headers
CPPFLAGS_old="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $LOCAL_INCLUDES -Wno-deprecated"

AC_CHECK_HEADER(vtkPlane.h,vtk_ok="yes",vtk_ok="no")

 CPPFLAGS="$CPPFLAGS_old"

 if  test "x$vtk_ok" = "xyes"
 then
   VTK_INCLUDES="$LOCAL_INCLUDES"

 dnl vtk libraries

   AC_MSG_CHECKING(linking VTK library)

  LIBS_old="$LIBS"
#  LIBS="$LIBS $TRY_LINK_LIBS"
  LIBS="$LIBS $LOCAL_LIBS"
  CPPFLAGS_old="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $VTK_INCLUDES -Wno-deprecated"

 dnl  VTKPY_MODULES="$VTKHOME/python"

   AC_CACHE_VAL(salome_cv_lib_vtk,[
     AC_TRY_LINK(
#include "vtkPlane.h"
,   vtkPlane *p = vtkPlane::New();,
    eval "salome_cv_lib_vtk=yes",eval "salome_cv_lib_vtk=no")
  ])
  vtk_ok="$salome_cv_lib_vtk"
  LIBS="$LIBS_old"
  CPPFLAGS="$CPPFLAGS_old"

fi

if  test "x$vtk_ok" = "xno"
then
  AC_MSG_RESULT("no")
  AC_MSG_WARN(unable to link with vtk library)
else
  AC_MSG_RESULT("yes")
  VTK_LIBS="$LOCAL_LIBS"
  VTK_MT_LIBS="$LOCAL_LIBS"
fi

AC_MSG_RESULT("for vtk: $vtk_ok")

AC_LANG_RESTORE

# Save cache
AC_CACHE_SAVE

])dnl



#  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
#  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
# 
#  This library is free software; you can redistribute it and/or 
#  modify it under the terms of the GNU Lesser General Public 
#  License as published by the Free Software Foundation; either 
#  version 2.1 of the License. 
# 
#  This library is distributed in the hope that it will be useful, 
#  but WITHOUT ANY WARRANTY; without even the implied warranty of 
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
#  Lesser General Public License for more details. 
# 
#  You should have received a copy of the GNU Lesser General Public 
#  License along with this library; if not, write to the Free Software 
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
# 
#  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
#
#
AC_DEFUN([CHECK_HDF5],[
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_CPP])dnl

AC_CHECKING(for HDF5)

AC_ARG_WITH(hdf5,
    [  --with-hdf5=DIR                 root directory path to hdf5 installation ],
    [HDF5HOME="$withval"
      AC_MSG_RESULT("select $withval as path to hdf5")
    ])

AC_SUBST(HDF5_INCLUDES)
AC_SUBST(HDF5_LIBS)
AC_SUBST(HDF5_MT_LIBS)

HDF5_INCLUDES=""
HDF5_LIBS=""
HDF5_MT_LIBS=""

hdf5_ok=no

LOCAL_INCLUDES=""
LOCAL_LIBS=""

if test -z $HDF5HOME
then
   AC_MSG_WARN(undefined HDF5HOME variable which specify hdf5 installation directory)
else
   LOCAL_INCLUDES="-I$HDF5HOME/include"
   LOCAL_LIBS="-L$HDF5HOME/lib"
fi

dnl hdf5 headers

CPPFLAGS_old="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $LOCAL_INCLUDES"
AC_CHECK_HEADER(hdf5.h,hdf5_ok=yes ,hdf5_ok=no)
CPPFLAGS="$CPPFLAGS_old"


if  test "x$hdf5_ok" = "xyes"
then

dnl hdf5 library

  LIBS_old="$LIBS"
  LIBS="$LIBS $LOCAL_LIBS"
  AC_CHECK_LIB(hdf5,H5open,hdf5_ok=yes,hdf5_ok=no)
  LIBS="$LIBS_old"

fi

if  test "x$hdf5_ok" = "xyes"
then
  HDF5_INCLUDES="$LOCAL_INCLUDES"
  HDF5_LIBS="$LOCAL_LIBS -lhdf5"
  HDF5_MT_LIBS="$LOCAL_LIBS -lhdf5"
fi

AC_MSG_RESULT(for hdf5: $hdf5_ok)

])dnl

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl

AC_DEFUN([CHECK_MED2],[
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
AC_REQUIRE([CHECK_HDF5])dnl

AC_CHECKING(for MED2)

AC_ARG_WITH(med2,
    [  --with-med2=DIR                 root directory path to med2 installation ],
    [MED2HOME="$withval"
      AC_MSG_RESULT("select $withval as path to med2")
    ])

AC_SUBST(MED2_INCLUDES)
AC_SUBST(MED2_LIBS)
AC_SUBST(MED2_MT_LIBS)

MED2_INCLUDES=""
MED2_LIBS=""
MED2_MT_LIBS=""

med2_ok=no

LOCAL_INCLUDES="$HDF5_INCLUDES"
LOCAL_LIBS="-lmed $HDF5_LIBS"

if test -z $MED2HOME
then
   AC_MSG_WARN(undefined MED2HOME variable which specify med2 installation directory)
else
   LOCAL_INCLUDES="$LOCAL_INCLUDES -I$MED2HOME/include"
   LOCAL_LIBS="-L$MED2HOME/lib $LOCAL_LIBS"
fi

dnl check med2 header

CPPFLAGS_old="$CPPFLAGS"
dnl we must test system : linux = -DPCLINUX
CPPFLAGS="$CPPFLAGS -DPCLINUX $LOCAL_INCLUDES"
AC_CHECK_HEADER(med.h,med2_ok=yes ,med2_ok=no)
CPPFLAGS="$CPPFLAGS_old"

if  test "x$med2_ok" = "xyes"
then

dnl check med2 library

  LIBS_old="$LIBS"
  LIBS="$LIBS $LOCAL_LIBS"
  AC_CHECK_LIB(med,MEDouvrir,med2_ok=yes,med2_ok=no)
  LIBS="$LIBS_old"

fi

if  test "x$med2_ok" = "xyes"
then
  MED2_INCLUDES="-DPCLINUX $LOCAL_INCLUDES"
  MED2_LIBS="$LOCAL_LIBS"
  MED2_MT_LIBS="$LOCAL_LIBS"
fi

AC_MSG_RESULT(for med2: $med2_ok)

])dnl

dnl  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
dnl  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
dnl 
dnl  This library is free software; you can redistribute it and/or 
dnl  modify it under the terms of the GNU Lesser General Public 
dnl  License as published by the Free Software Foundation; either 
dnl  version 2.1 of the License. 
dnl 
dnl  This library is distributed in the hope that it will be useful, 
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
dnl  Lesser General Public License for more details. 
dnl 
dnl  You should have received a copy of the GNU Lesser General Public 
dnl  License along with this library; if not, write to the Free Software 
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
dnl 
dnl  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
dnl
dnl
dnl
AC_DEFUN([CHECK_CAS],[
AC_REQUIRE([AC_PROG_CXX])dnl
AC_REQUIRE([AC_PROG_CXXCPP])dnl

AC_CHECKING(for OpenCascade)

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

AC_SUBST(CAS_CPPFLAGS)
AC_SUBST(CAS_CXXFLAGS)
AC_SUBST(CAS_KERNEL)
AC_SUBST(CAS_VIEWER)
AC_SUBST(CAS_MODELER)
AC_SUBST(CAS_OCAF)
AC_SUBST(CAS_DATAEXCHANGE)
AC_SUBST(CAS_LDFLAGS)

AC_SUBST(CAS_LDPATH)

CAS_CPPFLAGS=""
CAS_CXXFLAGS=""
CAS_LDFLAGS=""
occ_ok=no

dnl libraries directory location
case $host_os in
   linux*)
      casdir=Linux
      ;;
   freebsd*)
      casdir=Linux
      ;;
   irix5.*)
      casdir=Linux
      ;;
   irix6.*)
      casdir=Linux
      ;;
   osf4.*)
      casdir=Linux
      ;;
   solaris2.*)
      casdir=Linux
      ;;
   *)
      casdir=Linux
      ;;
esac

dnl were is OCC ?
if test -z $CASROOT; then
  AC_MSG_WARN(You must provide CASROOT variable : see OCC installation manual)
else
  occ_ok=yes
  OCC_VERSION_MAJOR=0
  ff=$CASROOT/inc/Standard_Version.hxx
  if test -f $ff ; then
    grep "define OCC_VERSION_MAJOR" $ff > /dev/null
    if test $? = 0 ; then
      OCC_VERSION_MAJOR=`grep "define OCC_VERSION_MAJOR" $ff | awk '{i=3 ; print $i}'`
    fi
  fi
fi

if test "x$occ_ok" = "xyes"; then

dnl cascade headers

  CPPFLAGS_old="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS -DLIN -DLINTEL -DCSFDB -DNO_CXX_EXCEPTION -DNo_exception -DHAVE_CONFIG_H -DHAVE_LIMITS_H -I$CASROOT/inc -Wno-deprecated -DHAVE_WOK_CONFIG_H"
  CXXFLAGS_old="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -funsigned-char"

  AC_CHECK_HEADER(Standard_Type.hxx,occ_ok=yes ,occ_ok=no)

  CPPFLAGS="$CPPFLAGS_old"
  CXXFLAGS="$CXXFLAGS_old"
fi

if test "x$occ_ok" = xyes ; then

  CAS_CPPFLAGS="-DOCC_VERSION_MAJOR=$OCC_VERSION_MAJOR -DLIN -DLINTEL -DCSFDB -DNO_CXX_EXCEPTION -DNo_exception -DHAVE_CONFIG_H -DHAVE_LIMITS_H -I$CASROOT/inc -DHAVE_WOK_CONFIG_H"
  CAS_CXXFLAGS="-funsigned-char"

  AC_MSG_CHECKING(for OpenCascade libraries)

  CPPFLAGS_old="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $CAS_CPPFLAGS -Wno-deprecated"
  CXXFLAGS_old="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $CAS_CXXFLAGS"
  LIBS_old="$LIBS"
  LIBS="$LIBS -L$CASROOT/$casdir/lib -lTKernel"
  AC_CACHE_VAL(salome_cv_lib_occ,[
    AC_TRY_LINK(
using namespace std;
#include <Standard_Type.hxx>
,   size_t size;
    const Standard_CString aName="toto";
    Standard_Type myST(aName) ; 
    myST.Find(aName);,
    eval "salome_cv_lib_occ=yes",eval "salome_cv_lib_occ=no")
  ])
  occ_ok="$salome_cv_lib_occ"

  CPPFLAGS="$CPPFLAGS_old"
  CXXFLAGS="$CXXFLAGS_old"
  LIBS="$LIBS_old"
fi

if test "x$occ_ok" = xno ; then
  AC_MSG_RESULT(no)
  AC_MSG_WARN(Opencascade libraries not found)
else
  AC_MSG_RESULT(yes)
  CAS_LDPATH="-L$CASROOT/$casdir/lib "
  CAS_KERNEL="$CAS_LDPATH -lTKernel -lTKMath"
  CAS_OCAF="$CAS_LDPATH -lPTKernel -lTKCAF -lFWOSPlugin -lTKPShape -lTKPCAF -lTKStdSchema -lTKShapeSchema -lPAppStdPlugin -lTKPAppStd -lTKCDF"
dnl  CAS_VIEWER="-L$CASROOT/$casdir/lib -lTKOpenGl -lTKV3d -lTKV2d -lTKService"
  CAS_VIEWER="$CAS_LDPATH -lTKOpenGl -lTKV3d -lTKService"
#  CAS_MODELER="-L$CASROOT/$casdir/lib -lTKG2d -lTKG3d -lTKGeomBase -lTKBRep -lTKGeomAlgo -lTKTopAlgo -lTKPrim -lTKBool -lTKHLR -lTKFillet -lTKFeat -lTKOffset"
  CAS_MODELER="$CAS_LDPATH -lTKG2d -lTKG3d -lTKGeomBase -lTKBRep -lTKGeomAlgo -lTKTopAlgo -lTKPrim -lTKBool -lTKHLR -lTKFillet -lTKOffset"
dnl  CAS_DATAEXCHANGE="-L$CASROOT/$casdir/lib -lTKXSBase -lTKIGES -lTKSTEP -lTKShHealing -lTKShHealingStd -lTKSTL -lTKVRML "
  CAS_DATAEXCHANGE="$CAS_LDPATH -lTKXSBase -lTKIGES -lTKSTEP -lTKShHealing -lTKShHealingStd"
  CAS_LDFLAGS="$CAS_KERNEL $CAS_OCAF $CAS_VIEWER $CAS_MODELER $CAS_DATAEXCHANGE"  
  
  
fi

AC_LANG_RESTORE

])dnl



dnl Copyright (C) 2003  CEA/DEN, EDF R&D

AC_DEFUN([CHECK_HTML_GENERATORS],[

#AC_CHECKING(for html generators)
AC_CHECKING(for doxygen)

doxygen_ok=yes

dnl were is doxygen ?

AC_PATH_PROG(DOXYGEN,doxygen) 
	
if test "x$DOXYGEN" = "x"
then
  doxygen_ok=no
  AC_MSG_RESULT(no)
  AC_MSG_WARN(doxygen not found)
else
  dnl AC_SUBST(DOXYGEN)
  AC_MSG_RESULT(yes)
fi

AC_CHECKING(for graphviz)

graphviz_ok=yes

dnl were is graphviz ?

AC_PATH_PROG(DOT,dot) 
	
if test "x$DOT" = "x" ; then
  graphviz_ok=no
  AC_MSG_RESULT(no)
  AC_MSG_WARN(graphviz not found)
else
  AC_MSG_RESULT(yes)
fi

])dnl
dnl

# Check availability of Salome's KERNEL binary distribution
#
# Author : Jerome Roy (CEA, 2003)
#

AC_DEFUN([CHECK_KERNEL],[

AC_CHECKING(for Kernel)

Kernel_ok=no

AC_ARG_WITH(kernel,
	    [  --with-kernel=DIR               root directory path of KERNEL build or installation],
	    KERNEL_DIR="$withval",KERNEL_DIR="")

if test "x$KERNEL_DIR" == "x" ; then

# no --with-kernel-dir option used

   if test "x$KERNEL_ROOT_DIR" != "x" ; then

    # KERNEL_ROOT_DIR environment variable defined
      KERNEL_DIR=$KERNEL_ROOT_DIR

   else

    # search Kernel binaries in PATH variable
      AC_PATH_PROG(TEMP, runSalome)
      if test "x$TEMP" != "x" ; then
         KERNEL_BIN_DIR=`dirname $TEMP`
         KERNEL_DIR=`dirname $KERNEL_BIN_DIR`
      fi
      
   fi
# 
fi

if test -f ${KERNEL_DIR}/bin/salome/runSalome ; then
   Kernel_ok=yes
   AC_MSG_RESULT(Using Kernel module distribution in ${KERNEL_DIR})

   if test "x$KERNEL_ROOT_DIR" == "x" ; then
      KERNEL_ROOT_DIR=${KERNEL_DIR}
   fi
   if test "x$KERNEL_SITE_DIR" == "x" ; then
      KERNEL_SITE_DIR=${KERNEL_ROOT_DIR}
   fi
   AC_SUBST(KERNEL_ROOT_DIR)
   AC_SUBST(KERNEL_SITE_DIR)

else
   AC_MSG_WARN("Cannot find compiled Kernel module distribution")
fi

AC_MSG_RESULT(for Kernel: $Kernel_ok)
 
])dnl
 

# Check availability of Geom binary distribution
#
# Author : Nicolas REJNERI (OPEN CASCADE, 2003)
#

AC_DEFUN([CHECK_GEOM],[

AC_CHECKING(for Geom)

Geom_ok=no

AC_ARG_WITH(geom,
	    [  --with-geom=DIR root directory path of GEOM installation ],
	    GEOM_DIR="$withval",GEOM_DIR="")

if test "x$GEOM_DIR" == "x" ; then

# no --with-geom-dir option used

   if test "x$GEOM_ROOT_DIR" != "x" ; then

    # GEOM_ROOT_DIR environment variable defined
      GEOM_DIR=$GEOM_ROOT_DIR

   else

    # search Geom binaries in PATH variable
      AC_PATH_PROG(TEMP, libGEOM_Swig.py)
      if test "x$TEMP" != "x" ; then
         GEOM_BIN_DIR=`dirname $TEMP`
         GEOM_DIR=`dirname $GEOM_BIN_DIR`
      fi
      
   fi
# 
fi

if test -f ${GEOM_DIR}/bin/salome/libGEOM_Swig.py ; then
   Geom_ok=yes
   AC_MSG_RESULT(Using Geom module distribution in ${GEOM_DIR})

   if test "x$GEOM_ROOT_DIR" == "x" ; then
      GEOM_ROOT_DIR=${GEOM_DIR}
   fi
   AC_SUBST(GEOM_ROOT_DIR)

else
   AC_MSG_WARN("Cannot find compiled Geom module distribution")
fi

AC_MSG_RESULT(for Geom: $Geom_ok)
 
])dnl
 

# Check availability of SMesh binary distribution
#
# Author : Nicolas REJNERI (OPEN CASCADE, 2003)
#

AC_DEFUN([CHECK_SMESH],[

AC_CHECKING(for SMesh)

SMesh_ok=no

AC_ARG_WITH(smesh,
	    [  --with-smesh=DIR root directory path of SMESH installation ],
	    SMESH_DIR="$withval",SMESH_DIR="")

if test "x$SMESH_DIR" == "x" ; then

# no --with-smesh option used

   if test "x$SMESH_ROOT_DIR" != "x" ; then

    # SMESH_ROOT_DIR environment variable defined
      SMESH_DIR=$SMESH_ROOT_DIR

   else

    # search SMESH binaries in PATH variable
      AC_PATH_PROG(TEMP, libSMESH_Swig.py)
      if test "x$TEMP" != "x" ; then
         SMESH_BIN_DIR=`dirname $TEMP`
         SMESH_DIR=`dirname $SMESH_BIN_DIR`
      fi
      
   fi
# 
fi

if test -f ${SMESH_DIR}/bin/salome/libSMESH_Swig.py ; then
   SMesh_ok=yes
   AC_MSG_RESULT(Using SMesh module distribution in ${SMESH_DIR})

   if test "x$SMESH_ROOT_DIR" == "x" ; then
      SMESH_ROOT_DIR=${SMESH_DIR}
   fi
   AC_SUBST(SMESH_ROOT_DIR)

else
   AC_MSG_WARN("Cannot find compiled SMesh module distribution")
fi

AC_MSG_RESULT(for SMesh: $SMesh_ok)
 
])dnl
 

AC_DEFUN([CHECK_GHS3D],[

AC_REQUIRE([AC_PROG_CXX])dnl
AC_REQUIRE([AC_PROG_CXXCPP])dnl

AC_CHECKING(for GHS3D executable)

AC_LANG_SAVE

AC_ARG_WITH(ghs3d,
	    [  --with-ghs3d=DIR root directory path of GHS33D installation],
	    GHS3D_HOME=$withval,GHS3D_HOME="")

GHS3D_ok=yes
GHS3D_HOME="/dn05/salome/GHS3D/"
GHS3D="/misc/dn05/salome/GHS3D/ghs3d3.1-1/bin/Linux/ghs3dV3.1-1"

#if test "x$GHS3D_HOME" == "x" ; then
#
#   if test "x$GHS3DHOME" != "x" ; then
#
#    # GHS3DHOME environment variable defined
#      GHS3D_HOME=$GHS3DHOME
#
#   fi
#fi


  if test "x$GHS3D_ok" == xno ; then
    AC_MSG_RESULT(no)
    AC_MSG_WARN(GHS3D libraries not found or not properly installed)
  else
    AC_MSG_RESULT(yes)
  fi
#fi

])dnl

