// Copyright (C) 2004-2014  CEA/DEN, EDF R&D
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

//  File   : GHS3DPlugin_GHS3D_i.hxx
//  Author : Edward AGAPOV
//  Module : GHS3DPlugin
//  $Header$
//
#ifndef _GHS3DPlugin_GHS3D_I_HXX_
#define _GHS3DPlugin_GHS3D_I_HXX_

#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(GHS3DPlugin_Algorithm)
#include CORBA_SERVER_HEADER(SALOME_Exception)

#include "SMESH_3D_Algo_i.hxx"
#include "GHS3DPlugin_GHS3D.hxx"

// ======================================================
// GHS3D 3d algorithm
// ======================================================
class GHS3DPlugin_GHS3D_i:
  public virtual POA_GHS3DPlugin::GHS3DPlugin_GHS3D,
  public virtual SMESH_3D_Algo_i
{
public:
  // Constructor
  GHS3DPlugin_GHS3D_i (PortableServer::POA_ptr thePOA,
                       int                     theStudyId,
                       ::SMESH_Gen*            theGenImpl );
  // Destructor
  virtual ~GHS3DPlugin_GHS3D_i();
 
  // Get implementation
  ::GHS3DPlugin_GHS3D* GetImpl();

  virtual SMESH::SMESH_Mesh_ptr importGMFMesh(const char* theGMFFileName);
};

#endif
