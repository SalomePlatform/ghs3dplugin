//  Copyright (C) 2004-2010  CEA/DEN, EDF R&D
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

//  SMESH SMESH_I : idl implementation based on 'SMESH' unit's calsses
//  File   : GHS3DPlugin_GHS3D_i.cxx
//  Author : Edward AGAPOV
//  Module : GHS3DPlugin
//  $Header$
//
#include "GHS3DPlugin_GHS3D_i.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Mesh_i.hxx"
#include "SMESH_Gen_i.hxx"
#include "GHS3DPlugin_GHS3D.hxx"
#include "SMESH_PythonDump.hxx"

#include "utilities.h"
#include <cstring>

using namespace std;

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
  MESSAGE( "GHS3DPlugin_GHS3D_i::GHS3DPlugin_GHS3D_i" );
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
  MESSAGE( "GHS3DPlugin_GHS3D_i::~GHS3DPlugin_GHS3D_i" );
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
  MESSAGE( "GHS3DPlugin_GHS3D_i::GetImpl" );
  return ( ::GHS3DPlugin_GHS3D* )myBaseImpl;
}

//=============================================================================
/*!
 *  GHS3DPlugin_GHS3D_i::~GHS3DPlugin_GHS3D_i
 *
 *  Destructor
 */
//=============================================================================

bool GHS3DPlugin_GHS3D_i::importGMFMesh(const char* theGMFFileName)
{
  MESSAGE( "GHS3DPlugin_GHS3D_i::importGMFMesh" );

  SMESH::SMESH_Mesh_ptr theMesh = SMESH_Gen_i::GetSMESHGen()->CreateEmptyMesh();
  SMESH_Gen_i::GetSMESHGen()->RemoveLastFromPythonScript(SMESH_Gen_i::GetSMESHGen()->GetCurrentStudy()->StudyId());
  SALOMEDS::SObject_ptr theSMesh = SMESH_Gen_i::GetSMESHGen()->ObjectToSObject(SMESH_Gen_i::GetSMESHGen()->GetCurrentStudy(), theMesh);
#ifdef WINNT
#define SEP '\\'
#else
#define SEP '/'
#endif
  string strFileName (theGMFFileName);
  strFileName = strFileName.substr(strFileName.rfind(SEP)+1);
  strFileName.erase(strFileName.rfind('.'));
  SMESH_Gen_i::GetSMESHGen()->SetName(theSMesh, strFileName.c_str());
  SMESH_Mesh_i* meshServant = dynamic_cast<SMESH_Mesh_i*>( SMESH_Gen_i::GetSMESHGen()->GetServant( theMesh ).in() );
  ASSERT( meshServant );
  if ( meshServant ) {
    bool res = GetImpl()->importGMFMesh(theGMFFileName, meshServant->GetImpl());
    SMESH::TPythonDump() << "isDone = " << _this() << ".importGMFMesh( \"" << theGMFFileName << "\")";
    return res;
  }
  return false;
}