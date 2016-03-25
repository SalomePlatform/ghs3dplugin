// Copyright (C) 2004-2016  CEA/DEN, EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

//  GHS3DPlugin GUI: GUI for plugged-in mesher GHS3DPlugin
//  File   : GHS3DPluginGUI_Enums.h
//  Author : Gilles DAVID (Euriware)
//  Module : GHS3DPlugin
//
#ifndef GHS3DPLUGINGUI_Enums_HeaderFile
#define GHS3DPLUGINGUI_Enums_HeaderFile

// tabs
enum {
  STD_TAB = 0,
  ADV_TAB,
  ENF_VER_TAB,
  ENF_MESH_TAB
};

// Enforced vertices array columns
enum {
  ENF_VER_NAME_COLUMN = 0,
  ENF_VER_X_COLUMN,
  ENF_VER_Y_COLUMN,
  ENF_VER_Z_COLUMN,
  ENF_VER_SIZE_COLUMN,
  ENF_VER_ENTRY_COLUMN,
  ENF_VER_COMPOUND_COLUMN,
  ENF_VER_GROUP_COLUMN,
  ENF_VER_NB_COLUMNS
};

// Enforced meshes array columns
enum {
  ENF_MESH_NAME_COLUMN = 0,
  ENF_MESH_ENTRY_COLUMN,
  ENF_MESH_CONSTRAINT_COLUMN,
  ENF_MESH_GROUP_COLUMN,
  ENF_MESH_NB_COLUMNS
};

// Enforced vertices widget inputs
enum {
  ENF_VER_WARNING = 0,
  ENF_VER_VERTEX = 0,
  ENF_VER_X_COORD,
  ENF_VER_Y_COORD,
  ENF_VER_Z_COORD,
  ENF_VER_SIZE,
  ENF_VER_GROUP,
  ENF_VER_BTN,
  ENF_VER_NB_LINES
};

// Enforced meshes widget inputs
enum {
  ENF_MESH_WARNING = 0,
  ENF_MESH_MESH = 0,
  ENF_MESH_CONSTRAINT,
  ENF_MESH_GROUP,
  ENF_MESH_BTN,
  ENF_MESH_NB_LINES
};


#endif
