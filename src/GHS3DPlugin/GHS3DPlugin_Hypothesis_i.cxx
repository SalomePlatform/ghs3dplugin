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
#include "SMESHDS_GroupBase.hxx"

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

bool GHS3DPlugin_Hypothesis_i::SetEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size)
    throw (SALOME::SALOME_Exception) {
  ASSERT(myBaseImpl);
  MESSAGE("IDL : SetEnforcedVertex( "<< x << ", " << y << ", " << z << ", " << size << ")");
  return _SetEnforcedVertex(size, x, y, z);
}

bool GHS3DPlugin_Hypothesis_i::SetEnforcedVertexWithGroup(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size, const char* theGroupName)
    throw (SALOME::SALOME_Exception) {
  ASSERT(myBaseImpl);
  MESSAGE("IDL : SetEnforcedVertexWithGroup( "<< x << ", " << y << ", " << z << ", " << size << ", " << theGroupName << ")");
  return _SetEnforcedVertex(size, x, y, z, "", "", theGroupName);
}

bool GHS3DPlugin_Hypothesis_i::SetEnforcedVertexGeom(GEOM::GEOM_Object_ptr theVertex, CORBA::Double size)
    throw (SALOME::SALOME_Exception) {
  ASSERT(myBaseImpl);
  
  if ((theVertex->GetShapeType() != GEOM::VERTEX) && (theVertex->GetShapeType() != GEOM::COMPOUND)) {
    MESSAGE("theVertex shape type is not VERTEX or COMPOUND");
    THROW_SALOME_CORBA_EXCEPTION("theVertex shape type is not VERTEX or COMPOUND", SALOME::BAD_PARAM);
  }
  
  string theVertexEntry = theVertex->GetStudyEntry();
  if (theVertexEntry.empty()) {
    GEOM::GEOM_Gen_ptr geomGen = SMESH_Gen_i::GetGeomEngine();
    SMESH_Gen_i *smeshGen = SMESH_Gen_i::GetSMESHGen();
    string aName;
    if (theVertex->GetShapeType() == GEOM::VERTEX)
      aName = "Vertex_";
    if (theVertex->GetShapeType() == GEOM::COMPOUND)
      aName = "Compound_";
    aName += theVertex->GetEntry();
    SALOMEDS::SObject_ptr theSVertex = geomGen->PublishInStudy(smeshGen->GetCurrentStudy(), NULL, theVertex, aName.c_str());
    if (!theSVertex->_is_nil())
      theVertexEntry = theSVertex->GetID();
  }
  if (theVertexEntry.empty())
    THROW_SALOME_CORBA_EXCEPTION( "Geom object is not published in study" ,SALOME::BAD_PARAM );

  string theVertexName = theVertex->GetName();
  MESSAGE("IDL : SetEnforcedVertexGeom( "<< theVertexEntry << ", " << size<< ")");
  
  return _SetEnforcedVertex(size, 0, 0, 0, theVertexName.c_str(), theVertexEntry.c_str());
}

bool GHS3DPlugin_Hypothesis_i::SetEnforcedVertexGeomWithGroup(GEOM::GEOM_Object_ptr theVertex, CORBA::Double size, const char* theGroupName)
    throw (SALOME::SALOME_Exception) {
  ASSERT(myBaseImpl);
  
  if ((theVertex->GetShapeType() != GEOM::VERTEX) && (theVertex->GetShapeType() != GEOM::COMPOUND)) {
    MESSAGE("theVertex shape type is not VERTEX or COMPOUND");
    THROW_SALOME_CORBA_EXCEPTION("theVertex shape type is not VERTEX or COMPOUND", SALOME::BAD_PARAM);
  }
  
  string theVertexEntry = theVertex->GetStudyEntry();
  if (theVertexEntry.empty()) {
    GEOM::GEOM_Gen_ptr geomGen = SMESH_Gen_i::GetGeomEngine();
    SMESH_Gen_i *smeshGen = SMESH_Gen_i::GetSMESHGen();
    string aName;
    if (theVertex->GetShapeType() == GEOM::VERTEX)
      aName = "Vertex_";
    if (theVertex->GetShapeType() == GEOM::COMPOUND)
      aName = "Compound_";
    aName += theVertex->GetEntry();
    SALOMEDS::SObject_ptr theSVertex = geomGen->PublishInStudy(smeshGen->GetCurrentStudy(), NULL, theVertex, aName.c_str());
    if (!theSVertex->_is_nil())
      theVertexEntry = theSVertex->GetID();
  }
  if (theVertexEntry.empty())
    THROW_SALOME_CORBA_EXCEPTION( "Geom object is not published in study" ,SALOME::BAD_PARAM );

  string theVertexName = theVertex->GetName();
  MESSAGE("IDL : SetEnforcedVertexGeomWithGroup( "<< theVertexEntry << ", " << size<< ", " << theGroupName << ")");
  
  return _SetEnforcedVertex(size, 0, 0, 0, theVertexName.c_str(), theVertexEntry.c_str(), theGroupName);
}

bool GHS3DPlugin_Hypothesis_i:: _SetEnforcedVertex(CORBA::Double size, CORBA::Double x, CORBA::Double y, CORBA::Double z,
                                                   const char* theVertexName, const char* theVertexEntry, const char* theGroupName)
    throw (SALOME::SALOME_Exception) {
  ASSERT(myBaseImpl);
  MESSAGE("IDL : _SetEnforcedVertex(" << size << ", " << x << ", " << y << ", " << z << ", \"" << theVertexName << "\", \"" << theVertexEntry << "\", \"" << theGroupName << "\")");
  bool newValue = false;

  if (string(theVertexEntry).empty()) {
    ::GHS3DPlugin_Hypothesis::TCoordsGHS3DEnforcedVertexMap coordsList = this->GetImpl()->_GetEnforcedVerticesByCoords();
    std::vector<double> coords;
    coords.push_back(x);
    coords.push_back(y);
    coords.push_back(z);
    if (coordsList.find(coords) == coordsList.end()) {
      MESSAGE("Coords not found: add it in coordsList");
      newValue = true;
    } else {
      MESSAGE("Coords already found, compare names");
      ::GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertex *enfVertex = this->GetImpl()->GetEnforcedVertex(x, y, z);
      if ((enfVertex->name != theVertexName) || (enfVertex->groupName != theGroupName) || (enfVertex->size != size)) {
        MESSAGE("The names or size are different: update");
//          this->GetImpl()->ClearEnforcedVertex(theFaceEntry, x, y, z);
        newValue = true;
      }
      else {
        MESSAGE("The names and size are identical");
      }
    }

    if (newValue) {
      if (string(theVertexName).empty()) {
        if (string(theGroupName).empty())
          SMESH::TPythonDump() << _this() << ".SetEnforcedVertex(" << x << ", " << y << ", " << z << ", " << size << ")";
        else
          SMESH::TPythonDump() << _this() << ".SetEnforcedVertexWithGroup(" << x << ", " << y << ", " << z << ", " << size << ", \"" << theGroupName << "\")";
//       else
//         if (string(theGroupName).empty())
//           SMESH::TPythonDump() << _this() << ".SetEnforcedVertexNamed(" << theFaceEntry << ", " << x << ", " << y << ", " << z << ", \"" << theVertexName << "\")";
//         else
//           SMESH::TPythonDump() << _this() << ".SetEnforcedVertexNamedWithGroup(" << theFaceEntry << ", " << x << ", " << y << ", " << z << ", \"" 
//                                           << theVertexName << "\", \"" << theGroupName << "\")";
      }
    }
  } else {
    ::GHS3DPlugin_Hypothesis::TGeomEntryGHS3DEnforcedVertexMap enfVertexEntryList = this->GetImpl()->_GetEnforcedVerticesByEntry();
//     ::BLSURFPlugin_Hypothesis::TGeomEntryGHS3DEnforcedVertexMap::const_iterator it = enfVertexEntryList.find(theVertexEntry);
    if ( enfVertexEntryList.find(theVertexEntry) == enfVertexEntryList.end()) {
      MESSAGE("Geom entry not found: add it in enfVertexEntryList");
      newValue = true;
    }
    else {
      MESSAGE("Geom entry already found, compare names");
      ::GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertex *enfVertex = this->GetImpl()->GetEnforcedVertex(theVertexEntry);
      if ((enfVertex->name != theVertexName) || (enfVertex->groupName != theGroupName) || (enfVertex->size != size)) {
        MESSAGE("The names or size are different: update");
//          this->GetImpl()->ClearEnforcedVertex(theFaceEntry, x, y, z);
        newValue = true;
      }
      else {
        MESSAGE("The names and size are identical");
      }
    }

    if (newValue) {
      if (string(theGroupName).empty())
        SMESH::TPythonDump() << _this() << ".SetEnforcedVertexGeom(" << theVertexEntry << ", " << size << ")";
      else
        SMESH::TPythonDump() << _this() << ".SetEnforcedVertexGeomWithGroup(" << theVertexEntry << ", " << size << ", \"" << theGroupName << "\")";
    }
  }

  if (newValue)
    this->GetImpl()->SetEnforcedVertex(theVertexName, theVertexEntry, theGroupName, size, x, y, z);

  MESSAGE("IDL : SetEnforcedVertexEntry END");
  return newValue;
}

//=======================================================================
//function : GetEnforcedVertex
//=======================================================================

CORBA::Double GHS3DPlugin_Hypothesis_i::GetEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z)
  throw (SALOME::SALOME_Exception)
{
  ASSERT(myBaseImpl);
  try {
    return this->GetImpl()->GetEnforcedVertex(x,y,z)->size;
  }
  catch (const std::invalid_argument& ex) {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = ex.what();
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
    ExDescription.lineNumber = 513;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  catch (SALOME_Exception& ex) {
    THROW_SALOME_CORBA_EXCEPTION( ex.what() ,SALOME::BAD_PARAM );
  }
  return 0;
}

//=======================================================================
//function : GetEnforcedVertex
//=======================================================================

CORBA::Double GHS3DPlugin_Hypothesis_i::GetEnforcedVertexGeom(GEOM::GEOM_Object_ptr theVertex)
  throw (SALOME::SALOME_Exception)
{
  ASSERT(myBaseImpl);
  
  if ((theVertex->GetShapeType() != GEOM::VERTEX) && (theVertex->GetShapeType() != GEOM::COMPOUND)) {
    MESSAGE("theVertex shape type is not VERTEX or COMPOUND");
    THROW_SALOME_CORBA_EXCEPTION("theVertex shape type is not VERTEX or COMPOUND", SALOME::BAD_PARAM);
  }
  
  string theVertexEntry = theVertex->GetStudyEntry();
  if (theVertexEntry.empty()) {
    GEOM::GEOM_Gen_ptr geomGen = SMESH_Gen_i::GetGeomEngine();
    SMESH_Gen_i *smeshGen = SMESH_Gen_i::GetSMESHGen();
    string aName;
    if (theVertex->GetShapeType() == GEOM::VERTEX)
      aName = "Vertex_";
    if (theVertex->GetShapeType() == GEOM::COMPOUND)
      aName = "Compound_";
    aName += theVertex->GetEntry();
    SALOMEDS::SObject_ptr theSVertex = geomGen->PublishInStudy(smeshGen->GetCurrentStudy(), NULL, theVertex, aName.c_str());
    if (!theSVertex->_is_nil())
      theVertexEntry = theSVertex->GetID();
  }
  if (theVertexEntry.empty())
    THROW_SALOME_CORBA_EXCEPTION( "Geom object is not published in study" ,SALOME::BAD_PARAM );

  string theVertexName = theVertex->GetName();
  
  try {
    return this->GetImpl()->GetEnforcedVertex(theVertexName)->size;
  }
  catch (const std::invalid_argument& ex) {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = ex.what();
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
    ExDescription.lineNumber = 538;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  catch (SALOME_Exception& ex) {
    THROW_SALOME_CORBA_EXCEPTION( ex.what() ,SALOME::BAD_PARAM );
  }
}

//=======================================================================
//function : GetEnforcedVertices
//=======================================================================

GHS3DPlugin::GHS3DEnforcedVertexList* GHS3DPlugin_Hypothesis_i::GetEnforcedVertices()
{
  ASSERT(myBaseImpl);
  GHS3DPlugin::GHS3DEnforcedVertexList_var result = new GHS3DPlugin::GHS3DEnforcedVertexList();

  const ::GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexList enfVertexList = this->GetImpl()->_GetEnforcedVertices();
  result->length( enfVertexList.size() );

  ::GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexList::const_iterator it = enfVertexList.begin();

  for (int i = 0 ; it != enfVertexList.end(); ++it, ++i ) {
    ::GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertex* currentVertex = (*it);
    GHS3DPlugin::GHS3DEnforcedVertex_var enfVertex = new GHS3DPlugin::GHS3DEnforcedVertex();
    // Name
    enfVertex->name = CORBA::string_dup(currentVertex->name.c_str());
    // Geom Vertex Entry
    enfVertex->geomEntry = CORBA::string_dup(currentVertex->geomEntry.c_str());
    // Coords
    GHS3DPlugin::TCoords_var coords = new GHS3DPlugin::TCoords();
    coords->length(currentVertex->coords.size());
    for (int ind = 0; ind < currentVertex->coords.size(); ind++)
      coords[ind] = currentVertex->coords[ind];
    enfVertex->coords = coords;
    // Group Name
    enfVertex->groupName = CORBA::string_dup(currentVertex->groupName.c_str());
    // Size
    enfVertex->size = currentVertex->size;
    
    result[i]=enfVertex;
    }

  return result._retn();
}

//=======================================================================
//function : RemoveEnforcedVertex
//=======================================================================

bool GHS3DPlugin_Hypothesis_i::RemoveEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z)
  throw (SALOME::SALOME_Exception)
{
  ASSERT(myBaseImpl);
  bool res = false;
  try {
    res = this->GetImpl()->RemoveEnforcedVertex(x,y,z);
    SMESH::TPythonDump() << " isDone = " << _this() << ".RemoveEnforcedVertex( " << x << ", " << y << ", " << z << " )";
  }
  catch (const std::invalid_argument& ex) {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = ex.what();
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
    ExDescription.lineNumber = 625;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  catch (SALOME_Exception& ex) {
    THROW_SALOME_CORBA_EXCEPTION( ex.what() ,SALOME::BAD_PARAM );
  }
  return res;
}

bool GHS3DPlugin_Hypothesis_i::RemoveEnforcedVertexGeom(GEOM::GEOM_Object_ptr theVertex)
  throw (SALOME::SALOME_Exception)
{
  ASSERT(myBaseImpl);
  
  if ((theVertex->GetShapeType() != GEOM::VERTEX) && (theVertex->GetShapeType() != GEOM::COMPOUND)) {
    MESSAGE("theVertex shape type is not VERTEX or COMPOUND");
    THROW_SALOME_CORBA_EXCEPTION("theVertex shape type is not VERTEX or COMPOUND", SALOME::BAD_PARAM);
  }
  
  string theVertexEntry = theVertex->GetStudyEntry();
  if (theVertexEntry.empty()) {
    GEOM::GEOM_Gen_ptr geomGen = SMESH_Gen_i::GetGeomEngine();
    SMESH_Gen_i *smeshGen = SMESH_Gen_i::GetSMESHGen();
    string aName;
    if (theVertex->GetShapeType() == GEOM::VERTEX)
      aName = "Vertex_";
    if (theVertex->GetShapeType() == GEOM::COMPOUND)
      aName = "Compound_";
    aName += theVertex->GetEntry();
    SALOMEDS::SObject_ptr theSVertex = geomGen->PublishInStudy(smeshGen->GetCurrentStudy(), NULL, theVertex, aName.c_str());
    if (!theSVertex->_is_nil())
      theVertexEntry = theSVertex->GetID();
  }
  if (theVertexEntry.empty())
    THROW_SALOME_CORBA_EXCEPTION( "Geom object is not published in study" ,SALOME::BAD_PARAM );
  
  bool res = false;
  try {
    res = this->GetImpl()->RemoveEnforcedVertex(0,0,0, theVertexEntry.c_str());
    SMESH::TPythonDump() << "isDone = " << _this() << ".RemoveEnforcedVertexGeom( " << theVertexEntry.c_str() << " )";
  }
  catch (const std::invalid_argument& ex) {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = ex.what();
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
    ExDescription.lineNumber = 648;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  catch (SALOME_Exception& ex) {
    THROW_SALOME_CORBA_EXCEPTION( ex.what() ,SALOME::BAD_PARAM );
  }
  return res;
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
 * \brief Adds enforced elements of type elementType using another mesh/sub-mesh/mesh group theSource. The elements will be grouped in theGroupName.
 */
bool GHS3DPlugin_Hypothesis_i::SetEnforcedMeshWithGroup(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType theType, const char* theGroupName)
  throw (SALOME::SALOME_Exception)
{
#if GHS3D_VERSION >= 42
  bool res = _SetEnforcedMesh(theSource, theType, -1.0, theGroupName);
  SMESH_Mesh_i* theMesh_i = SMESH::DownCast<SMESH_Mesh_i*>( theSource);
  SMESH_Group_i* theGroup_i = SMESH::DownCast<SMESH_Group_i*>( theSource);
  SMESH_GroupOnGeom_i* theGroupOnGeom_i = SMESH::DownCast<SMESH_GroupOnGeom_i*>( theSource);
  if (theGroup_i or theGroupOnGeom_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMeshWithGroup( " 
                          << theSource << ", " << theType << ", \"" << theGroupName << "\" )";
  }
  else if (theMesh_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMeshWithGroup( " 
                          << theSource << ".GetMesh(), " << theType << ", \"" << theGroupName << "\" )";
  }
  return res;
#else
  SALOME::ExceptionStruct ExDescription;
  ExDescription.text = "Bad version of GHS3D. It must >= 4.2.";
  ExDescription.type = SALOME::BAD_PARAM;
  ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
  ExDescription.lineNumber = 719;
  throw SALOME::SALOME_Exception(ExDescription);
#endif
}

/*!
 * \brief Adds enforced elements of type elementType using another mesh/sub-mesh/mesh group theSource.
 */
bool GHS3DPlugin_Hypothesis_i::SetEnforcedMesh(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType theType)
  throw (SALOME::SALOME_Exception)
{
  MESSAGE("GHS3DPlugin_Hypothesis_i::SetEnforcedMesh");
#if GHS3D_VERSION >= 42
  bool res = _SetEnforcedMesh(theSource, theType, -1.0);
  SMESH_Mesh_i* theMesh_i = SMESH::DownCast<SMESH_Mesh_i*>( theSource);
  SMESH_Group_i* theGroup_i = SMESH::DownCast<SMESH_Group_i*>( theSource);
  SMESH_GroupOnGeom_i* theGroupOnGeom_i = SMESH::DownCast<SMESH_GroupOnGeom_i*>( theSource);
  if (theGroup_i or theGroupOnGeom_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMesh( " 
                          << theSource << ", " << theType << " )";
  }
  else if (theMesh_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMesh( " 
                          << theSource << ".GetMesh(), " << theType << " )";
  }
  return res;
#else
  SALOME::ExceptionStruct ExDescription;
  ExDescription.text = "Bad version of GHS3D. It must >= 4.2.";
  ExDescription.type = SALOME::BAD_PARAM;
  ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
  ExDescription.lineNumber = 750;
  throw SALOME::SALOME_Exception(ExDescription);
#endif
}

/*!
 * \brief Adds enforced elements of type elementType using another mesh/sub-mesh/mesh group theSource and a size. The elements will be grouped in theGroupName.
 */
bool GHS3DPlugin_Hypothesis_i::SetEnforcedMeshSizeWithGroup(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType theType, double theSize, const char* theGroupName)
  throw (SALOME::SALOME_Exception)
{
  if (theSize <= 0) {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = "Size cannot be negative";
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
    ExDescription.lineNumber = 781;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  
  bool res = _SetEnforcedMesh(theSource, theType, theSize,theGroupName);
  SMESH_Mesh_i* theMesh_i = SMESH::DownCast<SMESH_Mesh_i*>( theSource);
  SMESH_Group_i* theGroup_i = SMESH::DownCast<SMESH_Group_i*>( theSource);
  SMESH_GroupOnGeom_i* theGroupOnGeom_i = SMESH::DownCast<SMESH_GroupOnGeom_i*>( theSource);
  if (theGroup_i or theGroupOnGeom_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMeshSizeWithGroup( " 
                          << theSource << ", " << theType << ", " << theSize << ", \"" << theGroupName << "\" )";
  }
  else if (theMesh_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMeshSizeWithGroup( " 
                          << theSource << ".GetMesh(), " << theType << ", " << theSize << ", \"" << theGroupName << "\" )";
  }
  return res;
}

/*!
 * \brief Adds enforced elements of type elementType using another mesh/sub-mesh/mesh group theSource and a size.
 */
bool GHS3DPlugin_Hypothesis_i::SetEnforcedMeshSize(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType theType, double theSize)
  throw (SALOME::SALOME_Exception)
{
  if (theSize <= 0) {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = "Size cannot be negative";
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
    ExDescription.lineNumber = 812;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  
  bool res = _SetEnforcedMesh(theSource, theType, theSize);
  SMESH_Mesh_i* theMesh_i = SMESH::DownCast<SMESH_Mesh_i*>( theSource);
  SMESH_Group_i* theGroup_i = SMESH::DownCast<SMESH_Group_i*>( theSource);
  SMESH_GroupOnGeom_i* theGroupOnGeom_i = SMESH::DownCast<SMESH_GroupOnGeom_i*>( theSource);
  if (theGroup_i or theGroupOnGeom_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMeshSize( " 
                          << theSource << ", " << theType << ", " << theSize << " )";
  }
  else if (theMesh_i)
  {
    SMESH::TPythonDump () << _this() << ".SetEnforcedMeshSize( " 
                          << theSource << ".GetMesh(), " << theType << ", " << theSize << " )";
  }
  return res;
}

bool GHS3DPlugin_Hypothesis_i::_SetEnforcedMesh(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType theType, double theSize, const char* theGroupName)
  throw (SALOME::SALOME_Exception)
{
  MESSAGE("GHS3DPlugin_Hypothesis_i::_SetEnforcedMesh");
  ASSERT(myBaseImpl);
  
  if (CORBA::is_nil( theSource ))
  {
    SALOME::ExceptionStruct ExDescription;
    ExDescription.text = "The source mesh CORBA object is NULL";
    ExDescription.type = SALOME::BAD_PARAM;
    ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
    ExDescription.lineNumber = 840;
    throw SALOME::SALOME_Exception(ExDescription);
  }
  
  if ((theType != SMESH::NODE) && (theType != SMESH::EDGE) && (theType != SMESH::FACE))
  {
    return 0;
//     SALOME::ExceptionStruct ExDescription;
//     ExDescription.text = "Bad elementType";
//     ExDescription.type = SALOME::BAD_PARAM;
//     ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
//     ExDescription.lineNumber = 840;
//     throw SALOME::SALOME_Exception(ExDescription);
  }
  
  SMESH::array_of_ElementType_var types = theSource->GetTypes();
  MESSAGE("Required type is "<<theType);
  MESSAGE("Available types:");
  for (int i=0;i<types->length();i++){MESSAGE(types[i]);}
  if ( types->length() >= 1 && types[types->length()-1] <  theType)
  {
    return 0;
//     SALOME::ExceptionStruct ExDescription;
//     ExDescription.text = "The source mesh has bad type";
//     ExDescription.type = SALOME::BAD_PARAM;
//     ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
//     ExDescription.lineNumber = 840;
//     throw SALOME::SALOME_Exception(ExDescription);
  }
  
  SMESH_Mesh_i* theMesh_i = SMESH::DownCast<SMESH_Mesh_i*>( theSource);
  SMESH_Group_i* theGroup_i = SMESH::DownCast<SMESH_Group_i*>( theSource);
  SMESH_GroupOnGeom_i* theGroupOnGeom_i = SMESH::DownCast<SMESH_GroupOnGeom_i*>( theSource);
  TIDSortedElemSet theElemSet;

  if (theMesh_i)
  {
    try {
      return this->GetImpl()->SetEnforcedMesh(theMesh_i->GetImpl(), theType, theSize, theGroupName);
    }
    catch (const std::invalid_argument& ex) {
      SALOME::ExceptionStruct ExDescription;
      ExDescription.text = ex.what();
      ExDescription.type = SALOME::BAD_PARAM;
      ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
      ExDescription.lineNumber = 840;
      throw SALOME::SALOME_Exception(ExDescription);
    }
    catch (SALOME_Exception& ex) {
      THROW_SALOME_CORBA_EXCEPTION( ex.what() ,SALOME::BAD_PARAM );
    }
  }
  else if (theGroup_i && types->length() == 1 && types[0] == theType)
  {
    MESSAGE("The source is a group")
    try {
      return this->GetImpl()->SetEnforcedGroup(theGroup_i->GetGroupDS()->GetMesh(),theGroup_i->GetListOfID(), theType, theSize, theGroupName);
    }
    catch (const std::invalid_argument& ex) {
      SALOME::ExceptionStruct ExDescription;
      ExDescription.text = ex.what();
      ExDescription.type = SALOME::BAD_PARAM;
      ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
      ExDescription.lineNumber = 840;
      throw SALOME::SALOME_Exception(ExDescription);
    }
    catch (SALOME_Exception& ex) {
      THROW_SALOME_CORBA_EXCEPTION( ex.what() ,SALOME::BAD_PARAM );
    }
  }
  else if (theGroupOnGeom_i && types->length() == 1 && types[0] == theType)
  {
    MESSAGE("The source is a group on geom")
    try {
      return this->GetImpl()->SetEnforcedGroup(theGroupOnGeom_i->GetGroupDS()->GetMesh(),theGroupOnGeom_i->GetListOfID(), theType, theSize, theGroupName);
    }
    catch (const std::invalid_argument& ex) {
      SALOME::ExceptionStruct ExDescription;
      ExDescription.text = ex.what();
      ExDescription.type = SALOME::BAD_PARAM;
      ExDescription.sourceFile = "GHS3DPlugin_Hypothesis_i.cxx";
      ExDescription.lineNumber = 840;
      throw SALOME::SALOME_Exception(ExDescription);
    }
    catch (SALOME_Exception& ex) {
      THROW_SALOME_CORBA_EXCEPTION( ex.what() ,SALOME::BAD_PARAM );
    }
  }
  return false;
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

