#  Copyright (C) 2005  CEA/DEN, EDF R&D, OPEN CASCADE, PRINCIPIA R&D
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
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#
# -* Makefile *- 
#
# Author : Edward AGAPOV (OCC)
# Modified by : Alexander BORODIN (OCN) - autotools usage
# Module : GHS3DPLUGIN
# Date : 10/01/2004
#
#

include $(top_srcdir)/adm_local/unix/make_common_starter.am

# header files 
salomeinclude_HEADERS =

# Libraries targets
lib_LTLIBRARIES = libGHS3DEngine.la

dist_libGHS3DEngine_la_SOURCES = \
	GHS3DPlugin_GHS3D.cxx \
	GHS3DPlugin_GHS3D_i.cxx \
	GHS3DPlugin_i.cxx

libGHS3DEngine_la_CPPFLAGS = \
	$(KERNEL_CXXFLAGS) \
	$(GUI_CXXFLAGS) \
	$(GEOM_CXXFLAGS) \
	$(SMESH_CXXFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(CORBA_CXXFLAGS) \
	$(CORBA_INCLUDES) \
	$(CAS_CPPFLAGS) \
	-I$(top_builddir)/idl \
	-I$(top_builddir)/salome_adm/unix

libGHS3DEngine_la_LDFLAGS  = \
	../idl/libSalomeIDLGHS3DPLUGIN.la \
	$(SMESH_LDFLAGS) -lSMESHimpl -lSMESHEngine \
	$(KERNEL_LDFLAGS) -lSalomeGenericObj

# resources files
nodist_salomeres_DATA = GHS3DPlugin_icons.qm
