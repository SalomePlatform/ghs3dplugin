#  Copyright (C) 2004-2010  CEA/DEN, EDF R&D
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
#  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#

# -* Makefile *- 
# Author : Edward AGAPOV (OCC)
# Modified by : Alexander BORODIN (OCN) - autotools usage
# Module : GHS3DPLUGIN
# Date : 10/01/2004
#
include $(top_srcdir)/adm_local/unix/make_common_starter.am

# header files 
salomeinclude_HEADERS = \
       GHS3DPlugin_Defs.hxx \
       GHS3DPlugin_GHS3D.hxx \
       GHS3DPlugin_GHS3D_i.hxx \
       GHS3DPlugin_Hypothesis.hxx \
       GHS3DPlugin_Hypothesis_i.hxx

# Libraries targets
lib_LTLIBRARIES = libGHS3DEngine.la

dist_libGHS3DEngine_la_SOURCES = \
  GHS3DPlugin_GHS3D.cxx \
  GHS3DPlugin_GHS3D_i.cxx \
  GHS3DPlugin_i.cxx \
  GHS3DPlugin_Hypothesis.cxx \
  GHS3DPlugin_Hypothesis_i.cxx

libGHS3DEngine_la_CPPFLAGS =  \
  $(KERNEL_CXXFLAGS)    \
  $(CAS_CPPFLAGS)       \
  $(GEOM_CXXFLAGS)      \
  $(MED_CXXFLAGS)       \
  $(SMESH_CXXFLAGS)     \
  $(VTK_INCLUDES) \
  $(BOOST_CPPFLAGS)     \
  $(CORBA_CXXFLAGS)     \
  $(CORBA_INCLUDES)     \
  -I$(top_builddir)/idl

libGHS3DEngine_la_LDFLAGS  = \
  ../../idl/libSalomeIDLGHS3DPLUGIN.la \
  $(CAS_KERNEL) -lTKBRep -lTKG2d -lTKG3d -lTKTopAlgo -lTKGeomBase -lTKGeomAlgo \
  $(MED_LDFLAGS) -lSalomeIDLMED \
  $(SMESH_LDFLAGS) -lSMESHimpl -lSMESHEngine -lSMESHDS -lSMDS -lStdMeshers \
  $(KERNEL_LDFLAGS) -lSalomeGenericObj -lSALOMELocalTrace -lSALOMEBasics
