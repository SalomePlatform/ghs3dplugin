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
## no --with-ghs3d option used
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
