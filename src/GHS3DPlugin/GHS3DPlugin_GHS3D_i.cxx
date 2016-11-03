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

//  SMESH SMESH_I : idl implementation based on 'SMESH' unit's calsses
//  File   : GHS3DPlugin_GHS3D_i.cxx
//  Author : Edward AGAPOV
//  Module : GHS3DPlugin
//  $Header$
//
#include "GHS3DPlugin_GHS3D_i.hxx"

#include "GHS3DPlugin_GHS3D.hxx"
#include "GHS3DPlugin_Optimizer.hxx"

#include <SMESH_Gen.hxx>
#include <SMESH_Gen_i.hxx>
#include <SMESH_Mesh_i.hxx>
#include <SMESH_PythonDump.hxx>

#include <utilities.h>
#include <cstring>

//=============================================================================
/*!
 *  GHS3DPlugin_GHS3D_i::GHS3DPlugin_GHS3D_i
 *
 *  Constructor
 */
//=============================================================================

GHS3DPlugin_GHS3D_i::GHS3DPlugin_GHS3D_i (PortableServer::POA_ptr thePOA,
                                          int                     theStudyId,
                                          ::SMESH_Gen*            theGenImpl )
     : SALOME::GenericObj_i( thePOA ), 
       SMESH_Hypothesis_i( thePOA ), 
       SMESH_Algo_i( thePOA ),
       SMESH_3D_Algo_i( thePOA )
{
  myBaseImpl = new ::GHS3DPlugin_GHS3D (theGenImpl->GetANewId(),
                                        theStudyId,
                                        theGenImpl );
}

//=============================================================================
/*!
 *  GHS3DPlugin_GHS3D_i::~GHS3DPlugin_GHS3D_i
 *
 *  Destructor
 */
//=============================================================================

GHS3DPlugin_GHS3D_i::~GHS3DPlugin_GHS3D_i()
{
}

//=============================================================================
/*!
 *  GHS3DPlugin_GHS3D_i::GetImpl
 *
 *  Get implementation
 */
//=============================================================================

::GHS3DPlugin_GHS3D* GHS3DPlugin_GHS3D_i::GetImpl()
{
  return ( ::GHS3DPlugin_GHS3D* )myBaseImpl;
}

//=============================================================================
/*!
 *  GHS3DPlugin_GHS3D_i::~GHS3DPlugin_GHS3D_i
 *
 *  Destructor
 */
//=============================================================================

SMESH::SMESH_Mesh_ptr GHS3DPlugin_GHS3D_i::importGMFMesh(const char* theGMFFileName)
{
  SMESH_Gen_i* smeshGen = SMESH_Gen_i::GetSMESHGen();
  SMESH::SMESH_Mesh_ptr theMesh = smeshGen->CreateEmptyMesh();
  smeshGen->RemoveLastFromPythonScript(smeshGen->GetCurrentStudy()->StudyId());
  SALOMEDS::SObject_ptr theSMesh = smeshGen->ObjectToSObject(smeshGen->GetCurrentStudy(), theMesh);
#ifdef WINNT
#define SEP '\\'
#else
#define SEP '/'
#endif
  std::string strFileName (theGMFFileName);
  strFileName = strFileName.substr(strFileName.rfind(SEP)+1);
  strFileName.erase(strFileName.rfind('.'));
  smeshGen->SetName(theSMesh, strFileName.c_str());
  SMESH_Mesh_i* meshServant = dynamic_cast<SMESH_Mesh_i*>( smeshGen->GetServant( theMesh ).in() );
  ASSERT( meshServant );
  if ( meshServant ) {
    if (GetImpl()->importGMFMesh(theGMFFileName, meshServant->GetImpl()))
      SMESH::TPythonDump() << theSMesh << " = " << _this() << ".importGMFMesh( \"" << theGMFFileName << "\")";
  }
  return theMesh;
}

//=============================================================================
/*!
 *  GHS3DPlugin_Optimizer_i::GHS3DPlugin_Optimizer_i
 *
 *  Constructor
 */
//=============================================================================

GHS3DPlugin_Optimizer_i::GHS3DPlugin_Optimizer_i (PortableServer::POA_ptr thePOA,
                                                  int                     theStudyId,
                                                  ::SMESH_Gen*            theGenImpl )
  : SALOME::GenericObj_i( thePOA ),
    SMESH_Hypothesis_i( thePOA ),
    SMESH_Algo_i( thePOA ),
    SMESH_3D_Algo_i( thePOA )
{
  myBaseImpl = new ::GHS3DPlugin_Optimizer (theGenImpl->GetANewId(),
                                            theStudyId,
                                            theGenImpl );
}

