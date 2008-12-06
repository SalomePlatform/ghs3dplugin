//  Copyright (C) 2004-2008  CEA/DEN, EDF R&D
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
// File      : GHS3DPlugin_Hypothesis_i.cxx
// Created   : Wed Apr  2 13:53:01 2008
// Author    : Edward AGAPOV (eap)
//
#include "GHS3DPlugin_Hypothesis_i.hxx"

#include <SMESH_Gen.hxx>
#include <SMESH_PythonDump.hxx>

#include <Utils_CorbaException.hxx>
#include <utilities.h>
#include <SMESH_Mesh_i.hxx>

//=======================================================================
//function : GHS3DPlugin_Hypothesis_i
//=======================================================================

GHS3DPlugin_Hypothesis_i::GHS3DPlugin_Hypothesis_i (PortableServer::POA_ptr thePOA,
                                                    int                     theStudyId,
                                                    ::SMESH_Gen*            theGenImpl)
  : SALOME::GenericObj_i( thePOA ), 
    SMESH_Hypothesis_i( thePOA )
{
  MESSAGE( "GHS3DPlugin_Hypothesis_i::GHS3DPlugin_Hypothesis_i" );
  myBaseImpl = new ::GHS3DPlugin_Hypothesis (theGenImpl->GetANewId(),
                                              theStudyId,
                                              theGenImpl);
}

//=======================================================================
//function : ~GHS3DPlugin_Hypothesis_i
//=======================================================================

GHS3DPlugin_Hypothesis_i::~GHS3DPlugin_Hypothesis_i()
{
  MESSAGE( "GHS3DPlugin_Hypothesis_i::~GHS3DPlugin_Hypothesis_i" );
}

//=======================================================================
//function : SetToMeshHoles
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetToMeshHoles(CORBA::Boolean toMesh)
{
  ASSERT(myBaseImpl);
  this->GetImpl()->SetToMeshHoles(toMesh);
  SMESH::TPythonDump() << _this() << ".SetToMeshHoles( " << toMesh << " )";
}

//=======================================================================
//function : GetToMeshHoles
//=======================================================================

CORBA::Boolean GHS3DPlugin_Hypothesis_i::GetToMeshHoles()
{
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetToMeshHoles();
}

//=======================================================================
//function : SetMaximumMemory
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetMaximumMemory(CORBA::Short MB)
   throw ( SALOME::SALOME_Exception )
{
  if ( MB == 0 )
    THROW_SALOME_CORBA_EXCEPTION( "Invalid memory size",SALOME::BAD_PARAM );
  ASSERT(myBaseImpl);
  this->GetImpl()->SetMaximumMemory(MB);
  SMESH::TPythonDump() << _this() << ".SetMaximumMemory( " << MB << " )";
}

//=======================================================================
//function : GetMaximumMemory
//=======================================================================

CORBA::Short GHS3DPlugin_Hypothesis_i::GetMaximumMemory()
{
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetMaximumMemory();
}

//=======================================================================
//function : SetInitialMemory
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetInitialMemory(CORBA::Short MB)
  throw ( SALOME::SALOME_Exception )
{
  if ( MB == 0 )
    THROW_SALOME_CORBA_EXCEPTION( "Invalid memory size",SALOME::BAD_PARAM );
  ASSERT(myBaseImpl);
  this->GetImpl()->SetInitialMemory(MB);
  SMESH::TPythonDump() << _this() << ".SetInitialMemory( " << MB << " )";
}

//=======================================================================
//function : GetInitialMemory
//=======================================================================

CORBA::Short GHS3DPlugin_Hypothesis_i::GetInitialMemory()
{
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetInitialMemory();
}

//=======================================================================
//function : SetOptimizationLevel
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetOptimizationLevel(CORBA::Short level)
  throw ( SALOME::SALOME_Exception )
{
  ::GHS3DPlugin_Hypothesis::OptimizationLevel l =
      (::GHS3DPlugin_Hypothesis::OptimizationLevel) level;
  if ( l < ::GHS3DPlugin_Hypothesis::None ||
       l > ::GHS3DPlugin_Hypothesis::Strong )
    THROW_SALOME_CORBA_EXCEPTION( "Invalid optimization level",SALOME::BAD_PARAM );
    
  ASSERT(myBaseImpl);
  this->GetImpl()->SetOptimizationLevel(l);
  SMESH::TPythonDump() << _this() << ".SetOptimizationLevel( " << level << " )";
}

//=======================================================================
//function : GetOptimizationLevel
//=======================================================================

CORBA::Short GHS3DPlugin_Hypothesis_i::GetOptimizationLevel()
{
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetOptimizationLevel();
}

//=======================================================================
//function : SetWorkingDirectory
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetWorkingDirectory(const char* path) throw ( SALOME::SALOME_Exception )
{
  if (!path )
    THROW_SALOME_CORBA_EXCEPTION( "Null working directory",SALOME::BAD_PARAM );

  string file(path);
  const char lastChar = *file.rbegin();
#ifdef WIN32
  if ( lastChar != '\\' ) file += '\\';
#else
  if ( lastChar != '/' ) file += '/';
#endif
  file += "GHS3D.log";
  SMESH_Mesh_i::PrepareForWriting (file.c_str());

  ASSERT(myBaseImpl);
  this->GetImpl()->SetWorkingDirectory(path);
  SMESH::TPythonDump() << _this() << ".SetWorkingDirectory( '" << path << "' )";
}

//=======================================================================
//function : GetWorkingDirectory
//=======================================================================

char* GHS3DPlugin_Hypothesis_i::GetWorkingDirectory()
{
  ASSERT(myBaseImpl);
  return CORBA::string_dup( this->GetImpl()->GetWorkingDirectory().c_str() );
}

//=======================================================================
//function : SetKeepFiles
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetKeepFiles(CORBA::Boolean toKeep)
{
  ASSERT(myBaseImpl);
  this->GetImpl()->SetKeepFiles(toKeep);
  SMESH::TPythonDump() << _this() << ".SetKeepFiles( " << toKeep << " )";
}

//=======================================================================
//function : GetKeepFiles
//=======================================================================

CORBA::Boolean GHS3DPlugin_Hypothesis_i::GetKeepFiles()
{
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetKeepFiles();
}

//=======================================================================
//function : SetVerboseLevel
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetVerboseLevel(CORBA::Short level)
  throw ( SALOME::SALOME_Exception )
{
  if (level < 0 || level > 10 )
    THROW_SALOME_CORBA_EXCEPTION( "Invalid verbose level, valid range is [0-10]",
                                  SALOME::BAD_PARAM );
  ASSERT(myBaseImpl);
  this->GetImpl()->SetVerboseLevel(level);
  SMESH::TPythonDump() << _this() << ".SetVerboseLevel( " << level << " )";
}

//=======================================================================
//function : GetVerboseLevel
//=======================================================================

CORBA::Short GHS3DPlugin_Hypothesis_i::GetVerboseLevel()
{
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetVerboseLevel();
}

//=======================================================================
//function : SetToCreateNewNodes
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetToCreateNewNodes(CORBA::Boolean toCreate)
{
  ASSERT(myBaseImpl);
  this->GetImpl()->SetToCreateNewNodes(toCreate);
  SMESH::TPythonDump() << _this() << ".SetToCreateNewNodes( " << toCreate << " )";
}

//=======================================================================
//function : GetToCreateNewNodes
//=======================================================================

CORBA::Boolean GHS3DPlugin_Hypothesis_i::GetToCreateNewNodes()
{
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetToCreateNewNodes();
}

//=======================================================================
//function : SetToUseBoundaryRecoveryVersion
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetToUseBoundaryRecoveryVersion(CORBA::Boolean toUse)
{
  ASSERT(myBaseImpl);
  this->GetImpl()->SetToUseBoundaryRecoveryVersion(toUse);
  SMESH::TPythonDump() << _this() << ".SetToUseBoundaryRecoveryVersion( " << toUse << " )";
}

//=======================================================================
//function : GetToUseBoundaryRecoveryVersion
//=======================================================================

CORBA::Boolean GHS3DPlugin_Hypothesis_i::GetToUseBoundaryRecoveryVersion()
{
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetToUseBoundaryRecoveryVersion();
}

//=======================================================================
//function : SetTextOption
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetTextOption(const char* option)
{
  ASSERT(myBaseImpl);
  this->GetImpl()->SetTextOption(option);
  SMESH::TPythonDump() << _this() << ".SetTextOption( '" << option << "' )";
}

//=======================================================================
//function : GetTextOption
//=======================================================================

char* GHS3DPlugin_Hypothesis_i::GetTextOption()
{
  ASSERT(myBaseImpl);
  return CORBA::string_dup( this->GetImpl()->GetTextOption().c_str() );
}

//=============================================================================
/*!
 *  Get implementation
 */
//=============================================================================

::GHS3DPlugin_Hypothesis* GHS3DPlugin_Hypothesis_i::GetImpl()
{
  return (::GHS3DPlugin_Hypothesis*)myBaseImpl;
}

//================================================================================
/*!
 * \brief Verify whether hypothesis supports given entity type 
 */
//================================================================================  

CORBA::Boolean GHS3DPlugin_Hypothesis_i::IsDimSupported( SMESH::Dimension type )
{
  return type == SMESH::DIM_3D;
}

