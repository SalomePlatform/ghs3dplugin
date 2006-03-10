#  Check availability of GHS3DPLUGIN binary distribution
#
#  Author : Marc Tajchman (CEA, 2002)
#------------------------------------------------------------

AC_DEFUN([CHECK_GHS3DPLUGIN],[

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
  AC_SUBST(GHS3DPLUGIN_ROOT_DIR)
else
  AC_MSG_WARN("Cannot find compiled GHS3DPLUGIN module distribution")
fi
  
AC_MSG_RESULT(for GHS3DPLUGIN: $GHS3dPlugin_ok)
 
])dnl
 
