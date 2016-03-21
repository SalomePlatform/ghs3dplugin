dnl Copyright (C) 2004-2016  CEA/DEN, EDF R&D
dnl
dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public
dnl License as published by the Free Software Foundation; either
dnl version 2.1 of the License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
dnl
dnl See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
dnl

#  Check availability of GHS3DPLUGIN binary distribution
#
#  Author : Marc Tajchman (CEA, 2002)
#------------------------------------------------------------

AC_DEFUN([CHECK_GHS3DPLUGIN],[

GHS3DPLUGIN_LDFLAGS=""
GHS3DPLUGIN_CXXFLAGS=""

AC_CHECKING(for GHS3dPlugin)

GHS3dPlugin_ok=no

AC_ARG_WITH(ghs,
	    --with-ghs3dPlugin=DIR  root directory path of GHS3DPLUGIN build or installation,
	    GHS3DPLUGIN_DIR="$withval",GHS3DPLUGIN_DIR="")

if test "x$GHS3DPLUGIN_DIR" = "x" ; then

# no --with-gui-dir option used

  if test "x$GHS3DPLUGIN_ROOT_DIR" != "x" ; then

    # SALOME_ROOT_DIR environment variable defined
    GHS3DPLUGIN_DIR=$GHS3DPLUGIN_ROOT_DIR

  else

    # search Salome binaries in PATH variable
    AC_PATH_PROG(TEMP, libGHS3DEngine.so)
    if test "x$TEMP" != "x" ; then
      GHS3DPLUGIN_DIR=`dirname $TEMP`
    fi

  fi

fi

if test -f ${GHS3DPLUGIN_DIR}/lib/salome/libGHS3DEngine.so  ; then
  GHS3dPlugin_ok=yes
  AC_MSG_RESULT(Using GHS3DPLUGIN module distribution in ${GHS3DPLUGIN_DIR})

  if test "x$GHS3DPLUGIN_ROOT_DIR" == "x" ; then
    GHS3DPLUGIN_ROOT_DIR=${GHS3DPLUGIN_DIR}
  fi
  GHS3DPLUGIN_CXXFLAGS+=-I${GHS3DPLUGIN_ROOT_DIR}/include/salome
  GHS3DPLUGIN_LDFLAGS+=-L${GHS3DPLUGIN_ROOT_DIR}/lib${LIB_LOCATION_SUFFIX}/salome
  AC_SUBST(GHS3DPLUGIN_ROOT_DIR)
  AC_SUBST(GHS3DPLUGIN_LDFLAGS)
  AC_SUBST(GHS3DPLUGIN_CXXFLAGS)
else
  AC_MSG_WARN("Cannot find compiled GHS3DPLUGIN module distribution")
fi
  
AC_MSG_RESULT(for GHS3DPLUGIN: $GHS3dPlugin_ok)
 
])dnl
 
