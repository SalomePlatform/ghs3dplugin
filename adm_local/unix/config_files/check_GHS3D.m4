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
