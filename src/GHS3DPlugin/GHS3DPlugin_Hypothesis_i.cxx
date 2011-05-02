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

// File      : GHS3DPlugin_Hypothesis_i.cxx
// Created   : Wed Apr  2 13:53:01 2008
// Author    : Edward AGAPOV (eap)
//
#include "GHS3DPlugin_Hypothesis_i.hxx"

#include "SMESH_Gen.hxx"
#include "SMESH_PythonDump.hxx"
//#include "SMESH_Mesh.hxx"
//#include "SMESH_ProxyMesh.hxx"
//#include <StdMeshers_QuadToTriaAdaptor.hxx>

#include "Utils_CorbaException.hxx"
#include "utilities.h"
#include "SMESH_Mesh_i.hxx"
#include "SMESH_Group_i.hxx"
#include "SMESH_Gen_i.hxx"
#include "SMESH_TypeDefs.hxx"

#ifndef GHS3D_VERSION
#define GHS3D_VERSION 41
#endif
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
//function : SetFEMCorrection
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetFEMCorrection(CORBA::Boolean toUseFem)
{
  ASSERT(myBaseImpl);
  this->GetImpl()->SetFEMCorrection(toUseFem);
  SMESH::TPythonDump() << _this() << ".SetFEMCorrection( " << toUseFem << " )";
}

//=======================================================================
//function : GetFEMCorrection
//=======================================================================

CORBA::Boolean GHS3DPlugin_Hypothesis_i::GetFEMCorrection()
{
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetFEMCorrection();
}

//=======================================================================
//function : SetToRemoveCentralPoint
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetToRemoveCentralPoint(CORBA::Boolean toRemove)
{
  ASSERT(myBaseImpl);
  this->GetImpl()->SetToRemoveCentralPoint(toRemove);
  SMESH::TPythonDump() << _this() << ".SetToRemoveCentralPoint( " << toRemove << " )";
}

//=======================================================================
//function : GetToRemoveCentralPoint
//=======================================================================

CORBA::Boolean GHS3DPlugin_Hypothesis_i::GetToRemoveCentralPoint()
{
  ASSERT(myBaseImpl);
  return this->GetImpl()->GetToRemoveCentralPoint();
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

//=======================================================================
//function : SetEnforcedVertex
//=======================================================================

void GHS3DPlugin_Hypothesis_i::SetEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size)
{
  ASSERT(myBaseImpl);
  this->GetImpl()->SetEnforcedVertex(x,y,z,size);
  SMESH::TPythonDump() << _this() << ".SetEnforcedVertex( " << x << ", " << y << ", " << z << ", " << size  << " )";
}

//=======================================================================
//function : GetEnforcedVertex
//=======================================================================

CORBA::Double GHS3DPlugin_Hypothesis_i::GetEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z)
  throw (SALOME::SALOME_Exception)
{
  ASSERT(myBaseImpl);
  try {
    return this->GetImpl()->GetEnforcedVertex(x,y,z);
  }
  catch (const std::invalid_argument& ex) {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = ex.what();
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis::GetEnforcedVertex(x,y,z)";
    ExDescription.lineNumber = 0;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  catch (SALOME_Exception& ex) {
    THROW_SALOME_CORBA_EXCEPTION( ex.what() ,SALOME::BAD_PARAM );
  }
  return 0;
}

//=======================================================================
//function : GetEnforcedVertices
//=======================================================================

GHS3DPlugin::GHS3DEnforcedVertexList* GHS3DPlugin_Hypothesis_i::GetEnforcedVertices()
{
  ASSERT(myBaseImpl);
  GHS3DPlugin::GHS3DEnforcedVertexList_var result = new GHS3DPlugin::GHS3DEnforcedVertexList();

  const ::GHS3DPlugin_Hypothesis::TEnforcedVertexValues sizeMaps = this->GetImpl()->_GetEnforcedVertices();
  int size = sizeMaps.size();
  result->length( size );

  ::GHS3DPlugin_Hypothesis::TEnforcedVertexValues::const_iterator it;
  int i = 0;
  for (it = sizeMaps.begin() ; it != sizeMaps.end(); it++ ) {
    GHS3DPlugin::GHS3DEnforcedVertex_var myVertex = new GHS3DPlugin::GHS3DEnforcedVertex();
    myVertex->x = it->first[0];
    myVertex->y = it->first[1];
    myVertex->z = it->first[2];
    myVertex->size = it->second;
    result[i]=myVertex;
    i++;
    }

  return result._retn();
}

//=======================================================================
//function : RemoveEnforcedVertex
//=======================================================================

void GHS3DPlugin_Hypothesis_i::RemoveEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z)
  throw (SALOME::SALOME_Exception)
{
  ASSERT(myBaseImpl);
  try {
    this->GetImpl()->RemoveEnforcedVertex(x,y,z);
    SMESH::TPythonDump() << _this() << ".RemoveEnforcedVertex( " << x << ", " << y << ", " << z << " )";
  }
  catch (const std::invalid_argument& ex) {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = ex.what();
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis::RemoveEnforcedVertex(x,y,z)";
    ExDescription.lineNumber = 408;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  catch (SALOME_Exception& ex) {
    THROW_SALOME_CORBA_EXCEPTION( ex.what() ,SALOME::BAD_PARAM );
  }
}

//=======================================================================
//function : ClearEnforcedVertices
//=======================================================================

void GHS3DPlugin_Hypothesis_i::ClearEnforcedVertices()
{
  ASSERT(myBaseImpl);
  this->GetImpl()->ClearEnforcedVertices();
  SMESH::TPythonDump () << _this() << ".ClearEnforcedVertices() ";
}

//=======================================================================
//function : ClearEnforcedMeshes
//=======================================================================

void GHS3DPlugin_Hypothesis_i::ClearEnforcedMeshes()
{
  ASSERT(myBaseImpl);
  this->GetImpl()->ClearEnforcedMeshes();
  SMESH::TPythonDump () << _this() << ".ClearEnforcedMeshes() ";
}

/*!
 * \brief Adds enforced elements of type elementType using another mesh/sub-mesh/mesh group theSource.
 */
void GHS3DPlugin_Hypothesis_i::SetEnforcedMesh(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType theType)
  throw (SALOME::SALOME_Exception)
{
#if GHS3D_VERSION >= 42
  _SetEnforcedMesh(theSource, theType, -1.0);
  SMESH_Mesh_i* theMesh_i = SMESH::DownCast<SMESH_Mesh_i*>( theSource);
  SMESH_Group_i* theGroup_i = SMESH::DownCast<SMESH_Group_i*>( theSource);
  if (theGroup_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMesh( " 
                          << theSource << ", " << theType << " )";
  }
  else if (theMesh_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMesh( " 
                          << theSource << ".GetMesh(), " << theType << " )";
  }
#else
  SALOME::ExceptionStruct ExDescription;
  ExDescription.text = "Bad version of GHS3D. It must >= 4.2.";
  ExDescription.type = SALOME::BAD_PARAM;
  ExDescription.sourceFile = "GHS3DPlugin_Hypothesis::SetEnforcedMesh(theSource, theType)";
  ExDescription.lineNumber = 463;
  throw SALOME::SALOME_Exception(ExDescription);
#endif
}

/*!
 * \brief Adds enforced elements of type elementType using another mesh/sub-mesh/mesh group theSource and a size.
 */
void GHS3DPlugin_Hypothesis_i::SetEnforcedMeshSize(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType theType, double theSize)
  throw (SALOME::SALOME_Exception)
{
  if (theSize <= 0) {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = "Size cannot be negative";
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis::SetEnforcedMeshSize(theSource, theType)";
    ExDescription.lineNumber = 475;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  
  _SetEnforcedMesh(theSource, theType, theSize);
  SMESH_Mesh_i* theMesh_i = SMESH::DownCast<SMESH_Mesh_i*>( theSource);
  SMESH_Group_i* theGroup_i = SMESH::DownCast<SMESH_Group_i*>( theSource);
  if (theGroup_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMeshSize( " 
                          << theSource << ", " << theType << ", " << theSize << " )";
  }
  else if (theMesh_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMeshSize( " 
                          << theSource << ".GetMesh(), " << theType << ", " << theSize << " )";
  }
}

void GHS3DPlugin_Hypothesis_i::_SetEnforcedMesh(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType theType, double theSize)
  throw (SALOME::SALOME_Exception)
{
  ASSERT(myBaseImpl);
  
  if (CORBA::is_nil( theSource ))
  {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = "The source mesh CORBA object is NULL";
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis::_SetEnforcedMesh(theSource, theType, theSize)";
    ExDescription.lineNumber = 502;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  
  if ((theType != SMESH::NODE) && (theType != SMESH::EDGE) && (theType != SMESH::FACE))
  {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = "Bad elementType";
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis::_SetEnforcedMesh(theSource, theType, theSize)";
    ExDescription.lineNumber = 502;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  
  SMESH::array_of_ElementType_var types = theSource->GetTypes();
//   MESSAGE("Required type is "<<theType);
//   MESSAGE("Available types:");
//   for (int i=0;i<types->length();i++){MESSAGE(types[i]);}
  if ( types->length() >= 1 && types[types->length()-1] <  theType)
  {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = "The source mesh has bad type";
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis::_SetEnforcedMesh(theSource, theType, theSize)";
    ExDescription.lineNumber = 502;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  
  SMESHDS_Mesh* theMeshDS;
  SMESH_Mesh_i* anImplPtr = SMESH::DownCast<SMESH_Mesh_i*>(theSource->GetMesh());
  if (anImplPtr)
    theMeshDS = anImplPtr->GetImpl().GetMeshDS();
  else
    return;
  
  SMESH_Mesh_i* theMesh_i = SMESH::DownCast<SMESH_Mesh_i*>( theSource);
  SMESH_Group_i* theGroup_i = SMESH::DownCast<SMESH_Group_i*>( theSource);
  TIDSortedElemSet theElemSet;

  if (theMesh_i)
  {
    try {
      this->GetImpl()->SetEnforcedMesh(anImplPtr->GetImpl(), theType, theSize);
    }
    catch (const std::invalid_argument& ex) {
      SALOME::ExceptionStruct ExDescription;
      ExDescription.text = ex.what();
      ExDescription.type = SALOME::BAD_PARAM;
      ExDescription.sourceFile = "GHS3DPlugin_Hypothesis::_SetEnforcedMesh(theSource, theType, theSize)";
      ExDescription.lineNumber = 502;
      throw SALOME::SALOME_Exception(ExDescription);
    }
    catch (SALOME_Exception& ex) {
      THROW_SALOME_CORBA_EXCEPTION( ex.what() ,SALOME::BAD_PARAM );
    }
//
//      SMESH::long_array_var anIDs = theMesh_i->GetElementsByType(theType);
//      if ( anIDs->length() == 0 ){MESSAGE("The source mesh is empty");}
//      for (int i=0; i<anIDs->length(); i++) {
//        CORBA::Long ind = anIDs[i];
//        const SMDS_MeshElement * elem = theMeshDS->FindElement(ind);
//        if (elem)
//          theElemSet.insert( elem );
//      }
////    }
//    MESSAGE("Add "<<theElemSet.size()<<" types["<<theType<<"] from source mesh");
  }
  else if (theGroup_i && types->length() == 1 && types[0] == theType)
  {
    SMESH::long_array_var anIDs = theGroup_i->GetListOfID();
    if ( anIDs->length() == 0 ){MESSAGE("The source group is empty");}
    for (int i=0; i<anIDs->length(); i++) {
      CORBA::Long ind = anIDs[i];
      if (theType == SMESH::NODE)
      {
        const SMDS_MeshNode * node = theMeshDS->FindNode(ind);
        if (node)
          theElemSet.insert( node );
      }
      else
      {
        const SMDS_MeshElement * elem = theMeshDS->FindElement(ind);
        if (elem)
          theElemSet.insert( elem );
      }
    }
    MESSAGE("Add "<<theElemSet.size()<<" types["<<theType<<"] from source group "<< theGroup_i->GetName());

    try {
      this->GetImpl()->SetEnforcedElements(theElemSet, theType, theSize);
    }
    catch (const std::invalid_argument& ex) {
      SALOME::ExceptionStruct ExDescription;
      ExDescription.text = ex.what();
      ExDescription.type = SALOME::BAD_PARAM;
      ExDescription.sourceFile = "GHS3DPlugin_Hypothesis::_SetEnforcedMesh(theSource, theType, theSize)";
      ExDescription.lineNumber = 502;
      throw SALOME::SALOME_Exception(ExDescription);
    }
    catch (SALOME_Exception& ex) {
      THROW_SALOME_CORBA_EXCEPTION( ex.what() ,SALOME::BAD_PARAM );
    }
  }
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

