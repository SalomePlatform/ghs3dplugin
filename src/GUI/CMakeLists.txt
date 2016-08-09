# Copyright (C) 2012-2016  CEA/DEN, EDF R&D
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#

INCLUDE(UseQtExt)

# --- options ---
# additional include directories
INCLUDE_DIRECTORIES(
  ${QT_INCLUDES}
  ${CAS_INCLUDE_DIRS}
  ${KERNEL_INCLUDE_DIRS}
  ${GUI_INCLUDE_DIRS}
  ${GEOM_INCLUDE_DIRS}
  ${SMESH_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${OMNIORB_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_BINARY_DIR}/idl
  ${PROJECT_SOURCE_DIR}/src/GHS3DPlugin
)

# additional preprocessor / compiler flags
ADD_DEFINITIONS(
  ${QT_DEFINITIONS}
  ${OMNIORB_DEFINITIONS}
  ${CAS_DEFINITIONS}
  ${BOOST_DEFINITIONS}
)

# libraries to link to
SET(_link_LIBRARIES
  ${QT_LIBRARIES}
  ${KERNEL_SALOMELocalTrace}
  ${GEOM_GEOM}
  ${SMESH_SMESH}
  ${SMESH_PluginUtils}
  ${SMESH_StdMeshersGUI}
  ${SMESH_SMESHFiltersSelection}
  ${GUI_suit}
  ${GUI_qtx}
  ${GUI_SalomeApp}
  ${CAS_KERNEL}
  SalomeIDLGHS3DPLUGIN
)

# --- headers ---

# header files / to be processed by moc
SET(_moc_HEADERS
  GHS3DPluginGUI_HypothesisCreator.h
  GHS3DPluginGUI_Dlg.h
)

# header files / no moc processed
SET(_other_HEADERS
  GHS3DPluginGUI_Enums.h
)

SET(GHS3DPluginGUI_HEADERS ${_moc_HEADERS} ${_other_HEADERS})

# --- sources ---

# sources / moc wrappings
QT_WRAP_MOC(_moc_SOURCES ${_moc_HEADERS})

# sources / static
SET(_other_SOURCES
  GHS3DPluginGUI.cxx
  GHS3DPluginGUI_HypothesisCreator.cxx
  GHS3DPluginGUI_AdvWidget.cxx
)

# --- resources ---

# resource files / to be processed by lrelease
SET(_ts_RESOURCES
  GHS3DPlugin_images.ts
  GHS3DPlugin_msg_en.ts
  GHS3DPlugin_msg_fr.ts
  GHS3DPlugin_msg_ja.ts
)

# resource files / to be processed by uic
SET(_uic_files
  GHS3DPluginGUI_AdvWidget_QTD.ui
)

# header files / uic wrappings
QT_WRAP_UIC(_uic_HEADERS ${_uic_files})

# sources / to compile
SET(GHS3DPluginGUI_SOURCES ${_other_SOURCES} ${_moc_SOURCES} ${_uic_HEADERS})

# --- rules ---

ADD_LIBRARY(GHS3DPluginGUI ${GHS3DPluginGUI_SOURCES})
TARGET_LINK_LIBRARIES(GHS3DPluginGUI ${_link_LIBRARIES} )
INSTALL(TARGETS GHS3DPluginGUI EXPORT ${PROJECT_NAME}TargetGroup DESTINATION ${SALOME_INSTALL_LIBS})

INSTALL(FILES ${GHS3DPluginGUI_HEADERS} DESTINATION ${SALOME_INSTALL_HEADERS})
QT_INSTALL_TS_RESOURCES("${_ts_RESOURCES}" "${SALOME_GHS3DPLUGIN_INSTALL_RES_DATA}")
