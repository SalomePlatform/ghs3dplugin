# Copyright (C) 2013-2024  CEA, EDF, OPEN CASCADE
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

FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.py")
INCLUDE(examples.set)

SET(COMPONENT_NAME GHS3DPLUGIN)

SET(TEST_INSTALL_DIRECTORY ${SALOME_GHS3DPLUGIN_INSTALL_TESTS})

# make test
SALOME_GENERATE_TESTS_ENVIRONMENT(tests_env)

FOREACH(tfile ${EXAMPLE_NAMES})
  SET(TEST_NAME ${COMPONENT_NAME}_${tfile})
  ADD_TEST(NAME ${TEST_NAME}
           COMMAND ${PYTHON_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py)           
  SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES ENVIRONMENT "${tests_env}")
  SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES LABELS "${COMPONENT_NAME}")
  MESSAGE( ${SALOME_INSTALL_DOC})
  INSTALL(FILES ${tfile}.py DESTINATION ${SALOME_INSTALL_DOC}/examples/GHS3DPLUGIN)
ENDFOREACH()

# salome test
FOREACH(tfile ${EXAMPLE_NAMES})
  INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          DESTINATION ${TEST_INSTALL_DIRECTORY})
ENDFOREACH()

INSTALL(FILES CTestTestfileInstall.cmake
  DESTINATION ${TEST_INSTALL_DIRECTORY}
  RENAME CTestTestfile.cmake)

INSTALL(FILES examples.set DESTINATION ${TEST_INSTALL_DIRECTORY})