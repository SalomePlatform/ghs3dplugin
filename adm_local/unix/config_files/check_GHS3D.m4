AC_DEFUN([CHECK_GHS3D],[

GHS3D_ok=no

AC_EXEEXT
AC_CHECK_PROG(GHS3D, ghs3d$EXEEXT,found)

if test "x$GHS3D" == x ; then
  AC_MSG_WARN(ghs3d program not found in PATH variable)
else
  GHS3D_ok=yes
fi

AC_MSG_RESULT(for GHS3D: $GHS3D_ok)

])dnl
