#  Copyright (C) 2004-2011  CEA/DEN, EDF R&D
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
salomeinclude_HEADERS = 

# Libraries targets
lib_LTLIBRARIES = libGHS3DPluginGUI.la

dist_libGHS3DPluginGUI_la_SOURCES = GHS3DPluginGUI_HypothesisCreator.cxx 

MOC_FILES = GHS3DPluginGUI_HypothesisCreator_moc.cxx
nodist_libGHS3DPluginGUI_la_SOURCES= $(MOC_FILES)

libGHS3DPluginGUI_la_CPPFLAGS =  \
	$(QT_INCLUDES)        \
	$(KERNEL_CXXFLAGS)    \
	$(GUI_CXXFLAGS)       \
	$(GEOM_CXXFLAGS)      \
	$(MED_CXXFLAGS)       \
	$(SMESH_CXXFLAGS)     \
	$(BOOST_CPPFLAGS)     \
	$(CORBA_CXXFLAGS)     \
	$(CORBA_INCLUDES)     \
	$(CAS_CPPFLAGS)       \
        -I$(srcdir)/..        \
	-I$(top_builddir)/idl

libGHS3DPluginGUI_la_LDFLAGS  = \
	../../idl/libSalomeIDLGHS3DPLUGIN.la \
	$(QT_LIBS) \
	$(KERNEL_LDFLAGS) -lSALOMELocalTrace \
	$(MED_LDFLAGS) -lSalomeIDLMED \
	$(SMESH_LDFLAGS) -lSMESH \
	$(GUI_LDFLAGS) -lsuit -lqtx -lSalomeApp \
	$(CAS_KERNEL)

# resources files
nodist_salomeres_DATA= \
	GHS3DPlugin_images.qm \
	GHS3DPlugin_msg_en.qm \
	GHS3DPlugin_msg_fr.qm
