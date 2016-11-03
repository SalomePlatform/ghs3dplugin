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

//=============================================================================
// File      : GHS3DPlugin_Hypothesis.cxx
// Created   : Wed Apr  2 12:36:29 2008
// Author    : Edward AGAPOV (eap)
//=============================================================================
//
#include "GHS3DPlugin_Hypothesis.hxx"

#include <SMESHDS_Mesh.hxx>

#include <TCollection_AsciiString.hxx>

#ifdef WIN32
#include <process.h>
#define getpid _getpid
#endif

using namespace std;

//=======================================================================
//function : GHS3DPlugin_Hypothesis
//=======================================================================

GHS3DPlugin_Hypothesis::GHS3DPlugin_Hypothesis(int hypId, int studyId, SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen),
  myToMeshHoles(DefaultMeshHoles()),
  myToMakeGroupsOfDomains(DefaultToMakeGroupsOfDomains()),
  myMaximumMemory(-1),
  myInitialMemory(-1),
  myOptimizationLevel(DefaultOptimizationLevel()),
  myKeepFiles(DefaultKeepFiles()),
  myWorkingDirectory(DefaultWorkingDirectory()),
  myVerboseLevel(DefaultVerboseLevel()),
  myToCreateNewNodes(DefaultToCreateNewNodes()),
  myToUseBoundaryRecoveryVersion(DefaultToUseBoundaryRecoveryVersion()),
  myToUseFemCorrection(DefaultToUseFEMCorrection()),
  myToRemoveCentralPoint(DefaultToRemoveCentralPoint()),
  myLogInStandardOutput(DefaultStandardOutputLog()),
  myGradation(DefaultGradation()),
  _enfVertexList(DefaultGHS3DEnforcedVertexList()),
  _enfVertexCoordsSizeList(DefaultGHS3DEnforcedVertexCoordsValues()),
  _enfVertexEntrySizeList(DefaultGHS3DEnforcedVertexEntryValues()),
  _coordsEnfVertexMap(DefaultCoordsGHS3DEnforcedVertexMap()),
  _geomEntryEnfVertexMap(DefaultGeomEntryGHS3DEnforcedVertexMap()),
  _enfMeshList(DefaultGHS3DEnforcedMeshList()),
  _entryEnfMeshMap(DefaultEntryGHS3DEnforcedMeshListMap()),
  _enfNodes(TIDSortedNodeGroupMap()),
  _enfEdges(TIDSortedElemGroupMap()),
  _enfTriangles(TIDSortedElemGroupMap()),
  _nodeIDToSizeMap(DefaultID2SizeMap()),
  _groupsToRemove(DefaultGroupsToRemove())
{
  _name = GetHypType();
  _param_algo_dim = 3;
}

//=======================================================================
//function : SetToMeshHoles
//=======================================================================

void GHS3DPlugin_Hypothesis::SetToMeshHoles(bool toMesh)
{
  if ( myToMeshHoles != toMesh ) {
    myToMeshHoles = toMesh;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetToMeshHoles
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetToMeshHoles(bool checkFreeOption) const
{
  if (checkFreeOption && !myTextOption.empty()) {
    if ( myTextOption.find("--components all"))
      return true;
    if ( myTextOption.find("--components outside_components"))
      return false;
  }
  return myToMeshHoles;
}

//=======================================================================
//function : SetToMakeGroupsOfDomains
//=======================================================================

void GHS3DPlugin_Hypothesis::SetToMakeGroupsOfDomains(bool toMakeGroups)
{
  if ( myToMakeGroupsOfDomains != toMakeGroups ) {
    myToMakeGroupsOfDomains = toMakeGroups;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetToMakeGroupsOfDomains
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetToMakeGroupsOfDomains() const
{
  return myToMakeGroupsOfDomains;
}

//=======================================================================
//function : GetToMakeGroupsOfDomains
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetToMakeGroupsOfDomains(const GHS3DPlugin_Hypothesis* hyp)
{
  bool res;
  if ( hyp ) res = /*hyp->GetToMeshHoles(true) &&*/ hyp->GetToMakeGroupsOfDomains();
  else       res = /*DefaultMeshHoles()        &&*/ DefaultToMakeGroupsOfDomains();
  return res;
}

//=======================================================================
//function : SetMaximumMemory
//=======================================================================

void GHS3DPlugin_Hypothesis::SetMaximumMemory(long MB)
{
  if ( myMaximumMemory != MB ) {
    myMaximumMemory = MB;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetMaximumMemory
//           * automatic memory adjustment mode. Default is zero
//=======================================================================

long GHS3DPlugin_Hypothesis::GetMaximumMemory() const
{
  return myMaximumMemory;
}

//=======================================================================
//function : SetInitialMemory
//=======================================================================

void GHS3DPlugin_Hypothesis::SetInitialMemory(long MB)
{
  if ( myInitialMemory != MB ) {
    myInitialMemory = MB;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetInitialMemory
//=======================================================================

long GHS3DPlugin_Hypothesis::GetInitialMemory() const
{
  return myInitialMemory;
}

//=======================================================================
//function : SetOptimizationLevel
//=======================================================================

void GHS3DPlugin_Hypothesis::SetOptimizationLevel(OptimizationLevel level)
{
  if ( myOptimizationLevel != level ) {
    myOptimizationLevel = level;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetOptimizationLevel
//=======================================================================

GHS3DPlugin_Hypothesis::OptimizationLevel GHS3DPlugin_Hypothesis::GetOptimizationLevel() const
{
  return (OptimizationLevel) myOptimizationLevel;
}

//=======================================================================
//function : SetWorkingDirectory
//=======================================================================

void GHS3DPlugin_Hypothesis::SetWorkingDirectory(const std::string& path)
{
  if ( myWorkingDirectory != path ) {
    myWorkingDirectory = path;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetWorkingDirectory
//=======================================================================

std::string GHS3DPlugin_Hypothesis::GetWorkingDirectory() const
{
  return myWorkingDirectory;
}

//=======================================================================
//function : SetKeepFiles
//=======================================================================

void GHS3DPlugin_Hypothesis::SetKeepFiles(bool toKeep)
{
  if ( myKeepFiles != toKeep ) {
    myKeepFiles = toKeep;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetKeepFiles
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetKeepFiles() const
{
  return myKeepFiles;
}

//=======================================================================
//function : SetVerboseLevel
//=======================================================================

void GHS3DPlugin_Hypothesis::SetVerboseLevel(short level)
{
  if ( myVerboseLevel != level ) {
    myVerboseLevel = level;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetVerboseLevel
//=======================================================================

short GHS3DPlugin_Hypothesis::GetVerboseLevel() const
{
  return myVerboseLevel;
}

//=======================================================================
//function : SetToCreateNewNodes
//=======================================================================

void GHS3DPlugin_Hypothesis::SetToCreateNewNodes(bool toCreate)
{
  if ( myToCreateNewNodes != toCreate ) {
    myToCreateNewNodes = toCreate;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetToCreateNewNodes
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetToCreateNewNodes() const
{
  return myToCreateNewNodes;
}

//=======================================================================
//function : SetToUseBoundaryRecoveryVersion
//=======================================================================

void GHS3DPlugin_Hypothesis::SetToUseBoundaryRecoveryVersion(bool toUse)
{
  if ( myToUseBoundaryRecoveryVersion != toUse ) {
    myToUseBoundaryRecoveryVersion = toUse;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetToUseBoundaryRecoveryVersion
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetToUseBoundaryRecoveryVersion() const
{
  return myToUseBoundaryRecoveryVersion;
}

//=======================================================================
//function : SetFEMCorrection
//=======================================================================

void GHS3DPlugin_Hypothesis::SetFEMCorrection(bool toUseFem)
{
  if ( myToUseFemCorrection != toUseFem ) {
    myToUseFemCorrection = toUseFem;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetFEMCorrection
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetFEMCorrection() const
{
  return myToUseFemCorrection;
}

//=======================================================================
//function : SetToRemoveCentralPoint
//=======================================================================

void GHS3DPlugin_Hypothesis::SetToRemoveCentralPoint(bool toRemove)
{
  if ( myToRemoveCentralPoint != toRemove ) {
    myToRemoveCentralPoint = toRemove;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetToRemoveCentralPoint
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetToRemoveCentralPoint() const
{
  return myToRemoveCentralPoint;
}

//=======================================================================
//function : SetAdvancedOption
//=======================================================================

void GHS3DPlugin_Hypothesis::SetAdvancedOption(const std::string& option)
{
  if ( myTextOption != option ) {
    myTextOption = option;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetAdvancedOption
//=======================================================================

std::string GHS3DPlugin_Hypothesis::GetAdvancedOption() const
{
  return myTextOption;
}

//=======================================================================
//function : SetGradation
//=======================================================================

void GHS3DPlugin_Hypothesis::SetGradation(double gradation)
{
  if ( myGradation != gradation ) {
    myGradation = gradation;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetGradation
//=======================================================================

double GHS3DPlugin_Hypothesis::GetGradation() const
{
  return myGradation;
}

//=======================================================================
//function : SetStandardOutputLog
//=======================================================================

void GHS3DPlugin_Hypothesis::SetStandardOutputLog(bool logInStandardOutput)
{
  if ( myLogInStandardOutput != logInStandardOutput ) {
    myLogInStandardOutput = logInStandardOutput;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetStandardOutputLog
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetStandardOutputLog() const
{
  return myLogInStandardOutput;
}

//=======================================================================
//function : SetRemoveLogOnSuccess
//=======================================================================

void GHS3DPlugin_Hypothesis::SetRemoveLogOnSuccess(bool removeLogOnSuccess)
{
  if ( myRemoveLogOnSuccess != removeLogOnSuccess ) {
    myRemoveLogOnSuccess = removeLogOnSuccess;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetRemoveLogOnSuccess
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetRemoveLogOnSuccess() const
{
  return myRemoveLogOnSuccess;
}

//=======================================================================
//function : SetEnforcedVertex
//=======================================================================

bool GHS3DPlugin_Hypothesis::SetEnforcedVertex(std::string theName,
                                               std::string theEntry,
                                               std::string theGroupName,
                                               double      size,
                                               double x, double y, double z,
                                               bool        isCompound)
{
  MESSAGE("GHS3DPlugin_Hypothesis::SetEnforcedVertex(\""<< theName << "\", \""<< theEntry << "\", \"" << theGroupName << "\", "
                                                      << size << ", " << x << ", " << y << ", " << z  << ", "<< isCompound << ")");

  bool toNotify = false;
  bool toCreate = true;

  TGHS3DEnforcedVertex *oldEnVertex;
  TGHS3DEnforcedVertex *newEnfVertex = new TGHS3DEnforcedVertex();
  newEnfVertex->name = theName;
  newEnfVertex->geomEntry = theEntry;
  newEnfVertex->coords.clear();
  if (!isCompound) {
    newEnfVertex->coords.push_back(x);
    newEnfVertex->coords.push_back(y);
    newEnfVertex->coords.push_back(z);
  }
  newEnfVertex->groupName = theGroupName;
  newEnfVertex->size = size;
  newEnfVertex->isCompound = isCompound;
  
  
  // update _enfVertexList
  TGHS3DEnforcedVertexList::iterator it = _enfVertexList.find(newEnfVertex);
  if (it != _enfVertexList.end()) {
    toCreate = false;
    oldEnVertex = (*it);
    MESSAGE("Enforced Vertex was found => Update");
    if (oldEnVertex->name != theName) {
      MESSAGE("Update name from \"" << oldEnVertex->name << "\" to \"" << theName << "\"");
      oldEnVertex->name = theName;
      toNotify = true;
    }
    if (oldEnVertex->groupName != theGroupName) {
      MESSAGE("Update group name from \"" << oldEnVertex->groupName << "\" to \"" << theGroupName << "\"");
      oldEnVertex->groupName = theGroupName;
      toNotify = true;
    }
    if (oldEnVertex->size != size) {
      MESSAGE("Update size from \"" << oldEnVertex->size << "\" to \"" << size << "\"");
      oldEnVertex->size = size;
      toNotify = true;
    }
    if (toNotify) {
      // update map coords / enf vertex if needed
      if (oldEnVertex->coords.size()) {
        _coordsEnfVertexMap[oldEnVertex->coords] = oldEnVertex;
        _enfVertexCoordsSizeList[oldEnVertex->coords] = size;
      }

      // update map geom entry / enf vertex if needed
      if (oldEnVertex->geomEntry != "") {
        _geomEntryEnfVertexMap[oldEnVertex->geomEntry] = oldEnVertex;
        _enfVertexEntrySizeList[oldEnVertex->geomEntry] = size;
      }
    }
  }

//   //////// CREATE ////////////
  if (toCreate) {
    toNotify = true;
    MESSAGE("Creating new enforced vertex");
    _enfVertexList.insert(newEnfVertex);
    if (theEntry == "") {
      _coordsEnfVertexMap[newEnfVertex->coords] = newEnfVertex;
      _enfVertexCoordsSizeList[newEnfVertex->coords] = size;
    }
    else {
      _geomEntryEnfVertexMap[newEnfVertex->geomEntry] = newEnfVertex;
      _enfVertexEntrySizeList[newEnfVertex->geomEntry] = size;
    }
  }

  if (toNotify)
    NotifySubMeshesHypothesisModification();

  MESSAGE("GHS3DPlugin_Hypothesis::SetEnforcedVertex END");
  return toNotify;
}


//=======================================================================
//function : SetEnforcedMesh
//=======================================================================
bool GHS3DPlugin_Hypothesis::SetEnforcedMesh(SMESH_Mesh& theMesh, SMESH::ElementType elementType, std::string name, std::string entry, std::string groupName)
{
  TIDSortedElemSet theElemSet;
  SMDS_ElemIteratorPtr eIt = theMesh.GetMeshDS()->elementsIterator(SMDSAbs_ElementType(elementType));
  while ( eIt->more() )
    theElemSet.insert( eIt->next() );
  MESSAGE("Add "<<theElemSet.size()<<" types["<<elementType<<"] from source mesh");
  bool added = SetEnforcedElements( theElemSet, elementType, groupName);
  if (added) {
    TGHS3DEnforcedMesh* newEnfMesh = new TGHS3DEnforcedMesh();
    newEnfMesh->persistID   = theMesh.GetMeshDS()->GetPersistentId();
    newEnfMesh->name        = name;
    newEnfMesh->entry       = entry;
    newEnfMesh->elementType = elementType;
    newEnfMesh->groupName   = groupName;
    
    TGHS3DEnforcedMeshList::iterator it = _enfMeshList.find(newEnfMesh);
    if (it == _enfMeshList.end()) {
      _entryEnfMeshMap[entry].insert(newEnfMesh);
      _enfMeshList.insert(newEnfMesh);
    }
    else {
      delete newEnfMesh;
    }
  }
  return added;
}

//=======================================================================
//function : SetEnforcedGroup
//=======================================================================
bool GHS3DPlugin_Hypothesis::SetEnforcedGroup(const SMESHDS_Mesh* theMeshDS, SMESH::long_array_var theIDs, SMESH::ElementType elementType, std::string name, std::string entry, std::string groupName)
{
  MESSAGE("GHS3DPlugin_Hypothesis::SetEnforcedGroup");
  TIDSortedElemSet theElemSet;
    if ( theIDs->length() == 0 ){MESSAGE("The source group is empty");}
    for ( CORBA::ULong i=0; i < theIDs->length(); i++) {
      CORBA::Long ind = theIDs[i];
      if (elementType == SMESH::NODE)
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

//   SMDS_ElemIteratorPtr it = theGroup->GetGroupDS()->GetElements();
//   while ( it->more() ) 
//     theElemSet.insert( it->next() );

  MESSAGE("Add "<<theElemSet.size()<<" types["<<elementType<<"] from source group ");
  bool added = SetEnforcedElements( theElemSet, elementType, groupName);
  if (added) {
    TGHS3DEnforcedMesh* newEnfMesh = new TGHS3DEnforcedMesh();
    newEnfMesh->name = name;
    newEnfMesh->entry = entry;
    newEnfMesh->elementType = elementType;
    newEnfMesh->groupName = groupName;
    
    TGHS3DEnforcedMeshList::iterator it = _enfMeshList.find(newEnfMesh);
    if (it == _enfMeshList.end()) {
      _entryEnfMeshMap[entry].insert(newEnfMesh);
      _enfMeshList.insert(newEnfMesh);
    }
  }
  return added;
}

//=======================================================================
//function : SetEnforcedElements
//=======================================================================
bool GHS3DPlugin_Hypothesis::SetEnforcedElements(TIDSortedElemSet theElemSet, SMESH::ElementType elementType, std::string groupName)
{
  MESSAGE("GHS3DPlugin_Hypothesis::SetEnforcedElements");
  TIDSortedElemSet::const_iterator it = theElemSet.begin();
  const SMDS_MeshElement* elem;
  const SMDS_MeshNode* node;
  bool added = true;
  pair<TIDSortedNodeGroupMap::iterator,bool> nodeRet;
  pair<TIDSortedElemGroupMap::iterator,bool> elemRet;

  for (;it != theElemSet.end();++it)
  {
    elem = (*it);
    switch (elementType) {
      case SMESH::NODE:
        node = dynamic_cast<const SMDS_MeshNode*>(elem);
        if (node) {
          nodeRet = _enfNodes.insert(make_pair(node,groupName));
          added = added && nodeRet.second;
          string msg = added ? "yes":"no";
          MESSAGE( "Node (" <<node->X()<<","<<node->Y()<<","<<node->Z()<< ") with ID " << node->GetID() <<" added ? " << msg);
        }
        else {
          SMDS_ElemIteratorPtr nodeIt = elem->nodesIterator();
          for (;nodeIt->more();) {
            node = dynamic_cast<const SMDS_MeshNode*>(nodeIt->next());
            nodeRet = _enfNodes.insert(make_pair(node,groupName));
            added = added && nodeRet.second;
          }
//          added = true;s
        }
        break;
      case SMESH::EDGE:
        if (elem->GetType() == SMDSAbs_Edge) {
          elemRet = _enfEdges.insert(make_pair(elem,groupName));
          added = added && elemRet.second;
        }
        else if (elem->GetType() > SMDSAbs_Edge) {
          SMDS_ElemIteratorPtr it = elem->edgesIterator();
          for (;it->more();) {
            const SMDS_MeshElement* anEdge = it->next();
            elemRet = _enfEdges.insert(make_pair(anEdge,groupName));
            added = added && elemRet.second;
          }
        }
        break;
      case SMESH::FACE:
        if (elem->GetType() == SMDSAbs_Face)
        {
          if (elem->NbCornerNodes() == 3) {
            elemRet = _enfTriangles.insert(make_pair(elem,groupName));
            added = added && elemRet.second;
          }
        }
        else if (elem->GetType() > SMDSAbs_Face) { // Group of faces
          SMDS_ElemIteratorPtr it = elem->facesIterator();
          for (;it->more();) {
            const SMDS_MeshElement* aFace = it->next();
            if (aFace->NbCornerNodes() == 3) {
              elemRet = _enfTriangles.insert(make_pair(aFace,groupName));
              added = added && elemRet.second;
            }
          }
        }
        break;
      default:
        break;
    };
  }
  if (added)
    NotifySubMeshesHypothesisModification();
  return added;
}


//=======================================================================
//function : GetEnforcedVertex
//=======================================================================

GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertex* GHS3DPlugin_Hypothesis::GetEnforcedVertex(double x, double y, double z)
  throw (std::invalid_argument)
{
  std::vector<double> coord(3);
  coord[0] = x;
  coord[1] = y;
  coord[2] = z;
  if (_coordsEnfVertexMap.count(coord)>0)
    return _coordsEnfVertexMap[coord];
  std::ostringstream msg ;
  msg << "No enforced vertex at " << x << ", " << y << ", " << z;
  throw std::invalid_argument(msg.str());
}

GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertex* GHS3DPlugin_Hypothesis::GetEnforcedVertex(const std::string theEntry)
  throw (std::invalid_argument)
{
  if (_geomEntryEnfVertexMap.count(theEntry)>0)
    return _geomEntryEnfVertexMap[theEntry];
  
  std::ostringstream msg ;
  msg << "No enforced vertex with entry " << theEntry;
  throw std::invalid_argument(msg.str());
}

//=======================================================================
//function : RemoveEnforcedVertex
//=======================================================================

bool GHS3DPlugin_Hypothesis::RemoveEnforcedVertex(double x, double y, double z, const std::string theEntry)
  throw (std::invalid_argument)
{
  bool toNotify = false;
  std::ostringstream msg;
  TGHS3DEnforcedVertex *oldEnfVertex;
  std::vector<double> coords(3);
  coords[0] = x;
  coords[1] = y;
  coords[2] = z;
  
  // check that enf vertex with given enf vertex entry exists
  TGeomEntryGHS3DEnforcedVertexMap::iterator it_enfVertexEntry = _geomEntryEnfVertexMap.find(theEntry);
  if (it_enfVertexEntry != _geomEntryEnfVertexMap.end()) {
    // Success
    MESSAGE("Found enforced vertex with geom entry " << theEntry);
    oldEnfVertex = it_enfVertexEntry->second;
    _geomEntryEnfVertexMap.erase(it_enfVertexEntry);
  } else {
    // Fail
    MESSAGE("Enforced vertex with geom entry " << theEntry << " not found");
    // check that enf vertex with given coords exists
    TCoordsGHS3DEnforcedVertexMap::iterator it_coords_enf = _coordsEnfVertexMap.find(coords);
    if (it_coords_enf != _coordsEnfVertexMap.end()) {
      // Success
      MESSAGE("Found enforced vertex with coords " << x << ", " << y << ", " << z);
      oldEnfVertex = it_coords_enf->second;
      _coordsEnfVertexMap.erase(it_coords_enf);
      _enfVertexCoordsSizeList.erase(_enfVertexCoordsSizeList.find(coords));
    } else {
      // Fail
      MESSAGE("Enforced vertex with coords " << x << ", " << y << ", " << z << " not found");
      throw std::invalid_argument(msg.str());
    }
  }

  MESSAGE("Remove enf vertex from _enfVertexList");

  // update _enfVertexList
  TGHS3DEnforcedVertexList::iterator it = _enfVertexList.find(oldEnfVertex);
  if (it != _enfVertexList.end()) {
    if ((*it)->groupName != "")
      _groupsToRemove.insert((*it)->groupName);
    _enfVertexList.erase(it);
    toNotify = true;
    MESSAGE("Done");
  }

  if (toNotify)
    NotifySubMeshesHypothesisModification();

  return toNotify;
}

//=======================================================================
//function : ClearEnforcedVertices
//=======================================================================
void GHS3DPlugin_Hypothesis::ClearEnforcedVertices()
{
  TGHS3DEnforcedVertexList::const_iterator it = _enfVertexList.begin();
  for(;it != _enfVertexList.end();++it) {
    if ((*it)->groupName != "")
      _groupsToRemove.insert((*it)->groupName);
  }
  _enfVertexList.clear();
  _coordsEnfVertexMap.clear();
  _geomEntryEnfVertexMap.clear();
  _enfVertexCoordsSizeList.clear();
  _enfVertexEntrySizeList.clear();
  NotifySubMeshesHypothesisModification();
}

//=======================================================================
//function : ClearEnforcedMeshes
//=======================================================================
void GHS3DPlugin_Hypothesis::ClearEnforcedMeshes()
{
  TGHS3DEnforcedMeshList::const_iterator it = _enfMeshList.begin();
  for(;it != _enfMeshList.end();++it) {
    if ((*it)->groupName != "")
      _groupsToRemove.insert((*it)->groupName);
  }
  _enfNodes.clear();
  _enfEdges.clear();
  _enfTriangles.clear();
  _nodeIDToSizeMap.clear();
  _enfMeshList.clear();
  _entryEnfMeshMap.clear();
  NotifySubMeshesHypothesisModification();
}

//================================================================================
/*!
 * \brief At mesh loading, restore enforced elements by just loaded enforced meshes
 */
//================================================================================

void GHS3DPlugin_Hypothesis::RestoreEnfElemsByMeshes()
{
  TGHS3DEnforcedMeshList::const_iterator it = _enfMeshList.begin();
  for(;it != _enfMeshList.end();++it) {
    TGHS3DEnforcedMesh* enfMesh = *it;
    if ( SMESH_Mesh* mesh = GetMeshByPersistentID( enfMesh->persistID ))
      SetEnforcedMesh( *mesh,
                       enfMesh->elementType,
                       enfMesh->name,
                       enfMesh->entry,
                       enfMesh->groupName );
    enfMesh->persistID = -1; // not to restore again
  }
}

//=======================================================================
//function : SetGroupsToRemove
//=======================================================================

void GHS3DPlugin_Hypothesis::ClearGroupsToRemove()
{
  _groupsToRemove.clear();
}


//=======================================================================
//function : DefaultMeshHoles
//=======================================================================

bool GHS3DPlugin_Hypothesis::DefaultMeshHoles()
{
  return false; // PAL19680
}

//=======================================================================
//function : DefaultToMakeGroupsOfDomains
//=======================================================================

bool GHS3DPlugin_Hypothesis::DefaultToMakeGroupsOfDomains()
{
  return false; // issue 0022172
}

//=======================================================================
//function : DefaultMaximumMemory
//=======================================================================

#ifndef WIN32
#include <sys/sysinfo.h>
#else
#include <windows.h>
#endif

long  GHS3DPlugin_Hypothesis::DefaultMaximumMemory()
{
#ifndef WIN32
  struct sysinfo si;
  long err = sysinfo( &si );
  if ( err == 0 ) {
    long ramMB = si.totalram * si.mem_unit / 1024 / 1024;
    return ( 0.7 * ramMB );
  }
#else
  // See http://msdn.microsoft.com/en-us/library/aa366589.aspx
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof (statex);
  long err = GlobalMemoryStatusEx (&statex);
  if (err != 0) {
    double totMB = (double)statex.ullAvailPhys / 1024. / 1024.;
    return (long)( 0.7 * totMB );
  }
#endif
  return 1024;
}

//=======================================================================
//function : DefaultInitialMemory
//=======================================================================

long  GHS3DPlugin_Hypothesis::DefaultInitialMemory()
{
  return DefaultMaximumMemory();
}

//=======================================================================
//function : DefaultOptimizationLevel
//=======================================================================

short  GHS3DPlugin_Hypothesis::DefaultOptimizationLevel()
{
  return Medium;
}

//=======================================================================
//function : DefaultWorkingDirectory
//=======================================================================

std::string GHS3DPlugin_Hypothesis::DefaultWorkingDirectory()
{
  TCollection_AsciiString aTmpDir;

  char *Tmp_dir = getenv("SALOME_TMP_DIR");
  if(Tmp_dir != NULL) {
    aTmpDir = Tmp_dir;
  }
  else {
#ifdef WIN32
    aTmpDir = TCollection_AsciiString("C:\\");
#else
    aTmpDir = TCollection_AsciiString("/tmp/");
#endif
  }
  return aTmpDir.ToCString();
}

//=======================================================================
//function : DefaultKeepFiles
//=======================================================================

bool   GHS3DPlugin_Hypothesis::DefaultKeepFiles()
{
  return false;
}

//=======================================================================
//function : DefaultRemoveLogOnSuccess
//=======================================================================

bool   GHS3DPlugin_Hypothesis::DefaultRemoveLogOnSuccess()
{
  return false;
}


//=======================================================================
//function : DefaultVerboseLevel
//=======================================================================

short  GHS3DPlugin_Hypothesis::DefaultVerboseLevel()
{
  return 10;
}

//=======================================================================
//function : DefaultToCreateNewNodes
//=======================================================================

bool GHS3DPlugin_Hypothesis::DefaultToCreateNewNodes()
{
  return true;
}

//=======================================================================
//function : DefaultToUseBoundaryRecoveryVersion
//=======================================================================

bool GHS3DPlugin_Hypothesis::DefaultToUseBoundaryRecoveryVersion()
{
  return false;
}

//=======================================================================
//function : DefaultToUseFEMCorrection
//=======================================================================

bool GHS3DPlugin_Hypothesis::DefaultToUseFEMCorrection()
{
  return false;
}

//=======================================================================
//function : DefaultToRemoveCentralPoint
//=======================================================================

bool GHS3DPlugin_Hypothesis::DefaultToRemoveCentralPoint()
{
  return false;
}

//=======================================================================
//function : DefaultGradation
//=======================================================================

double GHS3DPlugin_Hypothesis::DefaultGradation()
{
  return 1.05;
}

//=======================================================================
//function : DefaultStandardOutputLog
//=======================================================================

bool GHS3DPlugin_Hypothesis::DefaultStandardOutputLog()
{
  return false;
}

// //=======================================================================
// //function : DefaultID2SizeMap
// //=======================================================================
// 
// GHS3DPlugin_Hypothesis::TID2SizeMap GHS3DPlugin_Hypothesis::DefaultID2SizeMap()
// {
//   return GHS3DPlugin_Hypothesis::TID2SizeMap();
// }


//=======================================================================
//function : SaveTo
//=======================================================================

std::ostream & GHS3DPlugin_Hypothesis::SaveTo(std::ostream & save)
{
  save << (int) myToMeshHoles                 << " ";
  save << myMaximumMemory                     << " ";
  save << myInitialMemory                     << " ";
  save << myOptimizationLevel                 << " ";
  save << myWorkingDirectory                  << " ";
  save << (int)myKeepFiles                    << " ";
  save << myVerboseLevel                      << " ";
  save << (int)myToCreateNewNodes             << " ";
  save << (int)myToUseBoundaryRecoveryVersion << " ";
  save << (int)myToUseFemCorrection           << " ";
  save << (int)myToRemoveCentralPoint         << " ";
  save << myGradation                         << " ";
  save << myToMakeGroupsOfDomains             << " ";
  if (!myTextOption.empty()) {
    save << "__OPTIONS_BEGIN__ ";
    save << myTextOption                      << " ";
    save << "__OPTIONS_END__ ";
  }
  

  TGHS3DEnforcedVertexList::iterator it  = _enfVertexList.begin();
  if (it != _enfVertexList.end()) {
    save << " " << "__ENFORCED_VERTICES_BEGIN__ ";
    for ( ; it != _enfVertexList.end(); ++it ) {
      TGHS3DEnforcedVertex *enfVertex = (*it);
      save << " " << "__BEGIN_VERTEX__";
      if (!enfVertex->name.empty()) {
        save << " " << "__BEGIN_NAME__";
        save << " " << enfVertex->name;
        save << " " << "__END_NAME__";
      }
      if (!enfVertex->geomEntry.empty()) {
        save << " " << "__BEGIN_ENTRY__";
        save << " " << enfVertex->geomEntry;
        save << " " << enfVertex->isCompound;
        save << " " << "__END_ENTRY__";
      }
      if (!enfVertex->groupName.empty()) {
        save << " " << "__BEGIN_GROUP__";
        save << " " << enfVertex->groupName;
        save << " " << "__END_GROUP__";
      }
      if (enfVertex->coords.size()) {
        save << " " << "__BEGIN_COORDS__";
        for ( size_t i = 0; i < enfVertex->coords.size(); i++ )
          save << " " << enfVertex->coords[i];
        save << " " << "__END_COORDS__";
      }
      save << " " << "__BEGIN_SIZE__";
      save << " " << enfVertex->size;
      save << " " << "__END_SIZE__";
      save << " " << "__END_VERTEX__";
    }
    save << " " << "__ENFORCED_VERTICES_END__ ";
  }

  TGHS3DEnforcedMeshList::iterator it_mesh  = _enfMeshList.begin();
  if (it_mesh != _enfMeshList.end()) {
    save << " " << "__ENFORCED_MESHES_BEGIN__ ";
    for ( ; it_mesh != _enfMeshList.end(); ++it_mesh ) {
      TGHS3DEnforcedMesh *enfMesh = (*it_mesh);
      save << " " << "__BEGIN_ENF_MESH__";

      save << " " << "__BEGIN_NAME__";
      save << " " << enfMesh->name;
      save << " " << "__END_NAME__";

      save << " " << "__BEGIN_ENTRY__";
      save << " " << enfMesh->entry;
      save << " " << "__END_ENTRY__";

      save << " " << "__BEGIN_ELEM_TYPE__";
      save << " " << (int)enfMesh->elementType;
      save << " " << "__END_ELEM_TYPE__";

      if (!enfMesh->groupName.empty()) {
        save << " " << "__BEGIN_GROUP__";
        save << " " << enfMesh->groupName;
        save << " " << "__END_GROUP__";
      }
      save << " " << "__PERSIST_ID__";
      save << " " << enfMesh->persistID;
      save << " " << "__END_ENF_MESH__";
      std::cout << "Saving of enforced mesh " << enfMesh->name.c_str() << " done" << std::endl;
    }
    save << " "  << "__ENFORCED_MESHES_END__ ";
  }
  return save;
}

//=======================================================================
//function : LoadFrom
//=======================================================================

std::istream & GHS3DPlugin_Hypothesis::LoadFrom(std::istream & load)
{
  bool isOK = true;
  int i;
  double d;

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    myToMeshHoles = i;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> d);
  if (isOK)
    myMaximumMemory = d;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> d);
  if (isOK)
    myInitialMemory = d;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    myOptimizationLevel = i;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> myWorkingDirectory);
  if (isOK) {
    if ( myWorkingDirectory == "0") { // myWorkingDirectory was empty
      myKeepFiles = false;
      myWorkingDirectory.clear();
    }
    else if ( myWorkingDirectory == "1" ) {
      myKeepFiles = true;
      myWorkingDirectory.clear();
    }
  }
  else
    load.clear(ios::badbit | load.rdstate());

  if ( !myWorkingDirectory.empty() ) {
    isOK = static_cast<bool>(load >> i);
    if (isOK)
      myKeepFiles = i;
    else
      load.clear(ios::badbit | load.rdstate());
  }

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    myVerboseLevel = (short) i;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    myToCreateNewNodes = (bool) i;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    myToUseBoundaryRecoveryVersion = (bool) i;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    myToUseFemCorrection = (bool) i;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    myToRemoveCentralPoint = (bool) i;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> d);
  if (isOK)
    myGradation = d;
  else
    load.clear(ios::badbit | load.rdstate());

  std::string separator;
  bool hasOptions = false;
  bool hasEnforcedVertices = false;
  bool hasEnforcedMeshes = false;
  isOK = static_cast<bool>(load >> separator);

  if ( isOK && ( separator == "0" || separator == "1" ))
  {
    myToMakeGroupsOfDomains = ( separator == "1" );
    isOK = static_cast<bool>(load >> separator);
  }

  if (isOK) {
    if (separator == "__OPTIONS_BEGIN__")
      hasOptions = true;
    else if (separator == "__ENFORCED_VERTICES_BEGIN__")
      hasEnforcedVertices = true;
    else if (separator == "__ENFORCED_MESHES_BEGIN__")
      hasEnforcedMeshes = true;
  }

  if (hasOptions) {
    std::string txt;
    while (isOK) {
      isOK = static_cast<bool>(load >> txt);
      if (isOK) {
        if (txt == "__OPTIONS_END__") {
          if (!myTextOption.empty()) {
            // Remove last space
            myTextOption.erase(myTextOption.end()-1);
          }
          isOK = false;
          break;
        }
        myTextOption += txt;
        myTextOption += " ";
      }
    }
  }

  if (hasOptions) {
    isOK = static_cast<bool>(load >> separator);
    if (isOK && separator == "__ENFORCED_VERTICES_BEGIN__")
      hasEnforcedVertices = true;
    if (isOK && separator == "__ENFORCED_MESHES_BEGIN__")
      hasEnforcedMeshes = true;
  }

  if (hasEnforcedVertices) {
    std::string txt, name, entry, groupName;
    double size, coords[3];
    bool isCompound;
    bool hasCoords = false;
    isOK = static_cast<bool>(load >> txt);  // __BEGIN_VERTEX__
    while (isOK) {
      if (txt == "__ENFORCED_VERTICES_END__") {
        //isOK = false;
        break;
      }
      
      TGHS3DEnforcedVertex *enfVertex = new TGHS3DEnforcedVertex();
      while (isOK) {
        isOK = static_cast<bool>(load >> txt);
        if (txt == "__END_VERTEX__") {
          enfVertex->name = name;
          enfVertex->geomEntry = entry;
          enfVertex->isCompound = isCompound;
          enfVertex->groupName = groupName;
          enfVertex->coords.clear();
          if (hasCoords)
            enfVertex->coords.assign(coords,coords+3);

          _enfVertexList.insert(enfVertex);

          if (enfVertex->coords.size())
            _coordsEnfVertexMap[enfVertex->coords] = enfVertex;
          if (!enfVertex->geomEntry.empty())
            _geomEntryEnfVertexMap[enfVertex->geomEntry] = enfVertex;

          name.clear();
          entry.clear();
          groupName.clear();
          hasCoords = false;
          isOK = false;
        }

        if (txt == "__BEGIN_NAME__") {  // __BEGIN_NAME__
          while (isOK && (txt != "__END_NAME__")) {
            isOK = static_cast<bool>(load >> txt);
            if (txt != "__END_NAME__") {
              if (!name.empty())
                name += " ";
              name += txt;
            }
          }
          MESSAGE("name: " <<name);
        }

        if (txt == "__BEGIN_ENTRY__") {  // __BEGIN_ENTRY__
          isOK = static_cast<bool>(load >> entry);
          isOK = static_cast<bool>(load >> isCompound);
          isOK = static_cast<bool>(load >> txt); // __END_ENTRY__
          if (txt != "__END_ENTRY__")
            throw std::exception();
          MESSAGE("entry: " << entry);
        }

        if (txt == "__BEGIN_GROUP__") {  // __BEGIN_GROUP__
          while (isOK && (txt != "__END_GROUP__")) {
            isOK = static_cast<bool>(load >> txt);
            if (txt != "__END_GROUP__") {
              if (!groupName.empty())
                groupName += " ";
              groupName += txt;
            }
          }
          MESSAGE("groupName: " << groupName);
        }

        if (txt == "__BEGIN_COORDS__") {  // __BEGIN_COORDS__
          hasCoords = true;
          isOK = static_cast<bool>(load >> coords[0] >> coords[1] >> coords[2]);
          isOK = static_cast<bool>(load >> txt); // __END_COORDS__
          if (txt != "__END_COORDS__")
            throw std::exception();
          MESSAGE("coords: " << coords[0] <<","<< coords[1] <<","<< coords[2]);
        }

        if (txt == "__BEGIN_SIZE__") {  // __BEGIN_ENTRY__
          isOK = static_cast<bool>(load >> size);
          isOK = static_cast<bool>(load >> txt); // __END_ENTRY__
          if (txt != "__END_SIZE__") {
            throw std::exception();
          }
          MESSAGE("size: " << size);
        }
      }
      isOK = static_cast<bool>(load >> txt);  // __BEGIN_VERTEX__
    }
  }

  if (hasEnforcedVertices) {
    isOK = static_cast<bool>(load >> separator);
    if (isOK && separator == "__ENFORCED_MESHES_BEGIN__")
      hasEnforcedMeshes = true;
  }

  if (hasEnforcedMeshes) {
    std::string txt, name, entry, groupName;
    int elementType = -1, persistID = -1;
    isOK = static_cast<bool>(load >> txt);  // __BEGIN_ENF_MESH__
    while (isOK) {
      //                if (isOK) {
      if (txt == "__ENFORCED_MESHES_END__")
        isOK = false;

      TGHS3DEnforcedMesh *enfMesh = new TGHS3DEnforcedMesh();
      while (isOK) {
        isOK = static_cast<bool>(load >> txt);
        if (txt == "__END_ENF_MESH__") {
          enfMesh->name = name;
          enfMesh->entry = entry;
          enfMesh->elementType = (SMESH::ElementType)elementType;
          enfMesh->groupName = groupName;
          enfMesh->persistID = persistID;

          _enfMeshList.insert(enfMesh);
          //std::cout << "Restoring of enforced mesh " <<name  << " done" << std::endl;

          name.clear();
          entry.clear();
          elementType = -1;
          groupName.clear();
          persistID = -1;
          isOK = false;
        }

        if (txt == "__BEGIN_NAME__") {  // __BEGIN_NAME__
          while (isOK && (txt != "__END_NAME__")) {
            isOK = static_cast<bool>(load >> txt);
            if (txt != "__END_NAME__") {
              if (!name.empty())
                name += " ";
              name += txt;
            }
          }
          MESSAGE("name: " <<name);
        }

        if (txt == "__BEGIN_ENTRY__") {  // __BEGIN_ENTRY__
          isOK = static_cast<bool>(load >> entry);
          isOK = static_cast<bool>(load >> txt); // __END_ENTRY__
          if (txt != "__END_ENTRY__")
            throw std::exception();
          MESSAGE("entry: " << entry);
        }

        if (txt == "__BEGIN_ELEM_TYPE__") {  // __BEGIN_ELEM_TYPE__
          isOK = static_cast<bool>(load >> elementType);
          isOK = static_cast<bool>(load >> txt); // __END_ELEM_TYPE__
          if (txt != "__END_ELEM_TYPE__")
            throw std::exception();
          MESSAGE("elementType: " << elementType);
        }

        if (txt == "__BEGIN_GROUP__") {  // __BEGIN_GROUP__
          while (isOK && (txt != "__END_GROUP__")) {
            isOK = static_cast<bool>(load >> txt);
            if (txt != "__END_GROUP__") {
              if (!groupName.empty())
                groupName += " ";
              groupName += txt;
            }
          } // while
          MESSAGE("groupName: " << groupName);
        } // if

        if (txt == "__PERSIST_ID__") {
          isOK = static_cast<bool>(load >> persistID);
          MESSAGE("persistID: " << persistID);
        }
        //std::cout << "isOK: " << isOK << std::endl;
      } // while
      //                } // if
      isOK = static_cast<bool>(load >> txt);  // __BEGIN_ENF_MESH__
    } // while
  } // if

  return load;
}

//=======================================================================
//function : SetParametersByMesh
//=======================================================================

bool GHS3DPlugin_Hypothesis::SetParametersByMesh(const SMESH_Mesh* ,const TopoDS_Shape&)
{
  return false;
}


//================================================================================
/*!
 * \brief Sets myToMakeGroupsOfDomains depending on whether theMesh is on shape or not
 */
//================================================================================

bool GHS3DPlugin_Hypothesis::SetParametersByDefaults(const TDefaults&  dflts,
                                                     const SMESH_Mesh* /*theMesh*/)
{
  myToMakeGroupsOfDomains = ( !dflts._shape || dflts._shape->IsNull() );
  return true;
}

//================================================================================
/*!
 * \brief Return command to run MG-Tetra mesher excluding file prefix (-f)
 */
//================================================================================

std::string GHS3DPlugin_Hypothesis::CommandToRun(const GHS3DPlugin_Hypothesis* hyp,
                                                 const bool                    hasShapeToMesh,
                                                 const bool                    forExecutable)
{
  std::string cmd = GetExeName();
  // check if any option is overridden by hyp->myTextOption
  bool max_memory   = hyp ? ( hyp->myTextOption.find("--max_memory")  == std::string::npos ) : true;
  bool auto_memory   = hyp ? ( hyp->myTextOption.find("--automatic_memory")  == std::string::npos ) : true;
  bool comp   = hyp ? ( hyp->myTextOption.find("--components")  == std::string::npos ) : true;
  bool optim_level   = hyp ? ( hyp->myTextOption.find("--optimisation_level")  == std::string::npos ) : true;
  bool no_int_points  = hyp ? ( hyp->myTextOption.find("--no_internal_points") == std::string::npos ) : true;
  bool C   = hyp ? ( hyp->myTextOption.find("-C")  == std::string::npos ) : true;
  bool verbose   = hyp ? ( hyp->myTextOption.find("--verbose")  == std::string::npos ) : true;
  bool fem = hyp ? ( hyp->myTextOption.find("-FEM")== std::string::npos ) : true;
  bool rem = hyp ? ( hyp->myTextOption.find("--no_initial_central_point")== std::string::npos ) : true;
  bool gra = hyp ? ( hyp->myTextOption.find("-Dcpropa")== std::string::npos ) : true;

  // if use boundary recovery version, few options are allowed
  bool useBndRecovery = !C;
  if ( !useBndRecovery && hyp )
    useBndRecovery = hyp->myToUseBoundaryRecoveryVersion;

  // MG-Tetra needs to know amount of memory it may use (MB).
  // Default memory is defined at MG-Tetra installation but it may be not enough,
  // so allow to use about all available memory
  if ( max_memory ) {
    long aMaximumMemory = hyp ? hyp->myMaximumMemory : -1;
    cmd += " --max_memory ";
    if ( aMaximumMemory < 0 ) cmd += SMESH_Comment( DefaultMaximumMemory() );
    else                      cmd += SMESH_Comment( aMaximumMemory );
  }
  if ( auto_memory && !useBndRecovery ) {
    long aInitialMemory = hyp ? hyp->myInitialMemory : -1;
    cmd += " --automatic_memory ";
    if ( aInitialMemory > 0 ) cmd += SMESH_Comment( aInitialMemory );
    else                      cmd += "100";
  }
  // component to mesh
  if ( comp && !useBndRecovery ) {
    // We always run MG-Tetra with "to mesh holes'==TRUE (see PAL19680)
    if ( hasShapeToMesh )
      cmd += " --components all";
    else {
      bool aToMeshHoles = hyp ? hyp->myToMeshHoles : DefaultMeshHoles();
      if ( aToMeshHoles ) cmd += " --components all";
      else                cmd += " --components outside_components";
    }
  }
  const bool toCreateNewNodes = ( no_int_points && ( !hyp || hyp->myToCreateNewNodes ));

  // optimization level
  if ( !toCreateNewNodes ) {
    cmd += " --optimisation_level none"; // issue 22608
  }
  else if ( optim_level && hyp && !useBndRecovery ) {
    if ( hyp->myOptimizationLevel >= 0 && hyp->myOptimizationLevel < 5 ) {
      const char* level[] = { "none" , "light" , "standard" , "standard+" , "strong" };
      cmd += " --optimisation_level ";
      cmd += level[ hyp->myOptimizationLevel ];
    }
  }

  // to create internal nodes
  if ( no_int_points && !toCreateNewNodes ) {
    if ( forExecutable )
      cmd += " --no_internal_points";
    else
      cmd += " --internalpoints no";
  }

  // verbose mode
  if ( verbose && hyp ) {
    cmd += " --verbose " + SMESH_Comment( hyp->myVerboseLevel );
  }

  // boundary recovery version
  if ( useBndRecovery ) {
    cmd += " -C";
  }

  // to use FEM correction
  if ( fem && hyp && hyp->myToUseFemCorrection) {
    cmd += " -FEM";
  }

  // to remove initial central point.
  if ( rem && hyp && hyp->myToRemoveCentralPoint) {
    if ( forExecutable )
      cmd += " --no_initial_central_point";
    else
      cmd += " --centralpoint no";
  }

  // options as text
  if ( hyp && !hyp->myTextOption.empty() ) {
    cmd += " " + hyp->myTextOption;
  }

  // to define volumic gradation.
  if ( gra && hyp ) {
    if ( forExecutable )
      cmd += " -Dcpropa=" + SMESH_Comment( hyp->myGradation );
    else
      cmd += " --gradation " + SMESH_Comment( hyp->myGradation );
  }

#ifdef WIN32
  cmd += " < NUL";
#endif

  return cmd;
}

//================================================================================
/*!
 * \brief Return a unique file name
 */
//================================================================================

std::string GHS3DPlugin_Hypothesis::GetFileName(const GHS3DPlugin_Hypothesis* hyp)
{
  std::string aTmpDir = hyp ? hyp->GetWorkingDirectory() : DefaultWorkingDirectory();
  const char lastChar = *aTmpDir.rbegin();
#ifdef WIN32
    if(lastChar != '\\') aTmpDir+='\\';
#else
    if(lastChar != '/') aTmpDir+='/';
#endif      

  TCollection_AsciiString aGenericName = (char*)aTmpDir.c_str();
  aGenericName += "GHS3D_";
  aGenericName += getpid();
  aGenericName += "_";
  aGenericName += Abs((Standard_Integer)(long) aGenericName.ToCString());

  return aGenericName.ToCString();
}

//================================================================================
/*
 * Return the name of executable
 */
//================================================================================

std::string GHS3DPlugin_Hypothesis::GetExeName()
{
  return "mg-tetra.exe";
}

//================================================================================
/*!
* \brief Return the enforced vertices
*/
//================================================================================

GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexList GHS3DPlugin_Hypothesis::GetEnforcedVertices(const GHS3DPlugin_Hypothesis* hyp)
{
  return hyp ? hyp->_GetEnforcedVertices():DefaultGHS3DEnforcedVertexList();
}

GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues GHS3DPlugin_Hypothesis::GetEnforcedVerticesCoordsSize (const GHS3DPlugin_Hypothesis* hyp)
{  
  return hyp ? hyp->_GetEnforcedVerticesCoordsSize(): DefaultGHS3DEnforcedVertexCoordsValues();
}

GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexEntryValues GHS3DPlugin_Hypothesis::GetEnforcedVerticesEntrySize (const GHS3DPlugin_Hypothesis* hyp)
{  
  return hyp ? hyp->_GetEnforcedVerticesEntrySize(): DefaultGHS3DEnforcedVertexEntryValues();
}

GHS3DPlugin_Hypothesis::TCoordsGHS3DEnforcedVertexMap GHS3DPlugin_Hypothesis::GetEnforcedVerticesByCoords (const GHS3DPlugin_Hypothesis* hyp)
{  
  return hyp ? hyp->_GetEnforcedVerticesByCoords(): DefaultCoordsGHS3DEnforcedVertexMap();
}

GHS3DPlugin_Hypothesis::TGeomEntryGHS3DEnforcedVertexMap GHS3DPlugin_Hypothesis::GetEnforcedVerticesByEntry (const GHS3DPlugin_Hypothesis* hyp)
{  
  return hyp ? hyp->_GetEnforcedVerticesByEntry(): DefaultGeomEntryGHS3DEnforcedVertexMap();
}

GHS3DPlugin_Hypothesis::TIDSortedNodeGroupMap GHS3DPlugin_Hypothesis::GetEnforcedNodes(const GHS3DPlugin_Hypothesis* hyp)
{
  return hyp ? hyp->_GetEnforcedNodes():DefaultIDSortedNodeGroupMap();
}

GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap GHS3DPlugin_Hypothesis::GetEnforcedEdges(const GHS3DPlugin_Hypothesis* hyp)
{
  return hyp ? hyp->_GetEnforcedEdges():DefaultIDSortedElemGroupMap();
}

GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap GHS3DPlugin_Hypothesis::GetEnforcedTriangles(const GHS3DPlugin_Hypothesis* hyp)
{
  return hyp ? hyp->_GetEnforcedTriangles():DefaultIDSortedElemGroupMap();
}

GHS3DPlugin_Hypothesis::TID2SizeMap GHS3DPlugin_Hypothesis::GetNodeIDToSizeMap(const GHS3DPlugin_Hypothesis* hyp)
{
  return hyp ? hyp->_GetNodeIDToSizeMap(): DefaultID2SizeMap();
}

GHS3DPlugin_Hypothesis::TSetStrings GHS3DPlugin_Hypothesis::GetGroupsToRemove(const GHS3DPlugin_Hypothesis* hyp)
{
  return hyp ? hyp->_GetGroupsToRemove(): DefaultGroupsToRemove();
}
