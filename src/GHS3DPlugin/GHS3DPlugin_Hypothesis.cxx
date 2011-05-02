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

//=============================================================================
// File      : GHS3DPlugin_Hypothesis.cxx
// Created   : Wed Apr  2 12:36:29 2008
// Author    : Edward AGAPOV (eap)
//=============================================================================
//
#include "GHS3DPlugin_Hypothesis.hxx"
#include <SMESH_ProxyMesh.hxx>
#include <StdMeshers_QuadToTriaAdaptor.hxx>

#include <TCollection_AsciiString.hxx>

#ifdef WNT
#include <process.h>
#define getpid _getpid
#endif

//=======================================================================
//function : GHS3DPlugin_Hypothesis
//=======================================================================

GHS3DPlugin_Hypothesis::GHS3DPlugin_Hypothesis(int hypId, int studyId, SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen),
  myToMeshHoles(DefaultMeshHoles()),
  myMaximumMemory(-1),
  myInitialMemory(-1),
  myOptimizationLevel(DefaultOptimizationLevel()),
  myWorkingDirectory(DefaultWorkingDirectory()),
  myKeepFiles(DefaultKeepFiles()),
  myVerboseLevel(DefaultVerboseLevel()),
  myToCreateNewNodes(DefaultToCreateNewNodes()),
  myToUseBoundaryRecoveryVersion(DefaultToUseBoundaryRecoveryVersion()),
  myToUseFemCorrection(DefaultToUseFEMCorrection()),
  myToRemoveCentralPoint(DefaultToRemoveCentralPoint()),
  myEnforcedVertices(DefaultEnforcedVertices()),
  _enfNodes(DefaultIDSortedNodeSet()),
  _enfEdges(DefaultIDSortedElemSet()),
  _enfTriangles(DefaultIDSortedElemSet()),
  _enfQuadrangles(DefaultIDSortedElemSet())
{
  _name = "GHS3D_Parameters";
  _param_algo_dim = 3;
  _edgeID2nodeIDMap.clear();
  _triID2nodeIDMap.clear();
  _quadID2nodeIDMap.clear();
  _nodeIDToSizeMap.clear();
  _elementIDToSizeMap.clear();
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
    if ( myTextOption.find("-c 0"))
      return true;
    if ( myTextOption.find("-c 1"))
      return false;
  }
  return myToMeshHoles;
}

//=======================================================================
//function : SetMaximumMemory
//=======================================================================

void GHS3DPlugin_Hypothesis::SetMaximumMemory(short MB)
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

short GHS3DPlugin_Hypothesis::GetMaximumMemory() const
{
  return myMaximumMemory;
}

//=======================================================================
//function : SetInitialMemory
//=======================================================================

void GHS3DPlugin_Hypothesis::SetInitialMemory(short MB)
{
  if ( myInitialMemory != MB ) {
    myInitialMemory = MB;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetInitialMemory
//=======================================================================

short GHS3DPlugin_Hypothesis::GetInitialMemory() const
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
//function : SetTextOption
//=======================================================================

void GHS3DPlugin_Hypothesis::SetTextOption(const std::string& option)
{
  if ( myTextOption != option ) {
    myTextOption = option;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetTextOption
//=======================================================================

std::string GHS3DPlugin_Hypothesis::GetTextOption() const
{
  return myTextOption;
}

//=======================================================================
//function : SetEnforcedVertex
//=======================================================================

void GHS3DPlugin_Hypothesis::SetEnforcedVertex(double x, double y, double z, double size)
{
  std::vector<double> coord(3);
  coord[0] = x;
  coord[1] = y;
  coord[2] = z;
  myEnforcedVertices[coord] = size;
  NotifySubMeshesHypothesisModification();
}


//=======================================================================
//function : SetEnforcedMesh
//=======================================================================
void GHS3DPlugin_Hypothesis::SetEnforcedMesh(SMESH_Mesh& theMesh, SMESH::ElementType elementType, double size)
{
  TIDSortedElemSet theElemSet;
  SMDS_ElemIteratorPtr eIt;
/*
  if ((elementType == SMESH::FACE) && (theMesh.NbQuadrangles() > 0)) {
    SMESH_ProxyMesh::Ptr proxyMesh( new SMESH_ProxyMesh( theMesh ));

    StdMeshers_QuadToTriaAdaptor* aQuad2Trias = new StdMeshers_QuadToTriaAdaptor;
    aQuad2Trias->Compute( theMesh );
    proxyMesh.reset(aQuad2Trias );

//    std::cout << "proxyMesh->NbFaces(): " << proxyMesh->NbFaces() << std::endl;
//    eIt = proxyMesh->GetFaces();
//    if (eIt)
//      while ( eIt->more() )
//        theElemSet.insert( eIt->next() );
//    else {
//    std::cout << "********************** eIt == 0 *****************" << std::endl;
    eIt = theMesh.GetMeshDS()->elementsIterator(SMDSAbs_ElementType(elementType));
    while ( eIt->more() ) {
      const SMDS_MeshElement* elem = eIt->next();
        theElemSet.insert( elem );
    }
  }

  else
  {
  */
    eIt = theMesh.GetMeshDS()->elementsIterator(SMDSAbs_ElementType(elementType));
    while ( eIt->more() )
      theElemSet.insert( eIt->next() );
/*
  }
*/
  MESSAGE("Add "<<theElemSet.size()<<" types["<<elementType<<"] from source mesh");

  SetEnforcedElements( theElemSet, elementType, size);

}

//=======================================================================
//function : SetEnforcedElements
//=======================================================================
void GHS3DPlugin_Hypothesis::SetEnforcedElements(TIDSortedElemSet theElemSet, SMESH::ElementType elementType, double size)
{
  MESSAGE("GHS3DPlugin_Hypothesis::SetEnforcedElements");
  TIDSortedElemSet::const_iterator it = theElemSet.begin();
  const SMDS_MeshElement* elem;
  for (;it != theElemSet.end();++it)
  {
    elem = (*it);
//     MESSAGE("Element ID: " << (*it)->GetID());
    const SMDS_MeshNode* node = dynamic_cast<const SMDS_MeshNode*>(elem);
    switch (elementType) {
      case SMESH::NODE:
        if (node) {
//           MESSAGE("This is a node");
          _enfNodes.insert(node);
          _nodeIDToSizeMap.insert(make_pair(node->GetID(), size));
        }
        else {
//           MESSAGE("This is an element");
          _enfNodes.insert(elem->begin_nodes(),elem->end_nodes());
          SMDS_ElemIteratorPtr nodeIt = elem->nodesIterator();
          for (;nodeIt->more();)
            _nodeIDToSizeMap.insert(make_pair(nodeIt->next()->GetID(), size));
        }
        NotifySubMeshesHypothesisModification();
        break;
      case SMESH::EDGE:
        if (node) {
//           MESSAGE("This is a node");
        }
        else {
//           MESSAGE("This is an element");
          if (elem->GetType() == SMDSAbs_Edge) {
            _enfEdges.insert(elem);
//             _enfNodes.insert(elem->begin_nodes(),elem->end_nodes());
            _elementIDToSizeMap.insert(make_pair(elem->GetID(), size));
            SMDS_ElemIteratorPtr nodeIt = elem->nodesIterator();
            for (;nodeIt->more();) {
              node = dynamic_cast<const SMDS_MeshNode*>(nodeIt->next());
              _edgeID2nodeIDMap[elem->GetID()].push_back(node->GetID());
              _nodeIDToSizeMap.insert(make_pair(node->GetID(), size));
            }
          }
          else if (elem->GetType() > SMDSAbs_Edge) {
            SMDS_ElemIteratorPtr it = elem->edgesIterator();
            for (;it->more();) {
              const SMDS_MeshElement* anEdge = it->next();
              _enfEdges.insert(anEdge);
//               _enfNodes.insert(anEdge->begin_nodes(),anEdge->end_nodes());
              _elementIDToSizeMap.insert(make_pair(anEdge->GetID(), size));
              SMDS_ElemIteratorPtr nodeIt = anEdge->nodesIterator();
              for (;nodeIt->more();) {
                node = dynamic_cast<const SMDS_MeshNode*>(nodeIt->next());
                _edgeID2nodeIDMap[anEdge->GetID()].push_back(node->GetID());
                _nodeIDToSizeMap.insert(make_pair(node->GetID(), size));
              }
            }
          }
        }
        NotifySubMeshesHypothesisModification();
        break;
      case SMESH::FACE:
        if (node) {
//           MESSAGE("This is a node");
        }
        else {
//           MESSAGE("This is an element");
          if (elem->GetType() == SMDSAbs_Face)
          {
            if (elem->NbNodes() == 3) {
              _enfTriangles.insert(elem);
//               _enfNodes.insert(elem->begin_nodes(),elem->end_nodes());
              _elementIDToSizeMap.insert(make_pair(elem->GetID(), size));
              SMDS_ElemIteratorPtr nodeIt = elem->nodesIterator();
              for (;nodeIt->more();) {
                node = dynamic_cast<const SMDS_MeshNode*>(nodeIt->next());
                _triID2nodeIDMap[elem->GetID()].push_back(node->GetID());
                _nodeIDToSizeMap.insert(make_pair(node->GetID(), size));
              }
            }
            else if (elem->NbNodes() == 4) {
              _enfQuadrangles.insert(elem);
//               _enfNodes.insert(elem->begin_nodes(),elem->end_nodes());
              _elementIDToSizeMap.insert(make_pair(elem->GetID(), size));
              SMDS_ElemIteratorPtr nodeIt = elem->nodesIterator();
              for (;nodeIt->more();) {
                node = dynamic_cast<const SMDS_MeshNode*>(nodeIt->next());
                _quadID2nodeIDMap[elem->GetID()].push_back(node->GetID());
                _nodeIDToSizeMap.insert(make_pair(node->GetID(), size));
              }
            }
          }
          else if (elem->GetType() > SMDSAbs_Face) {
            SMDS_ElemIteratorPtr it = elem->facesIterator();
            for (;it->more();) {
              const SMDS_MeshElement* aFace = it->next();
              if (aFace->NbNodes() == 3) {
                _enfTriangles.insert(aFace);
//                 _enfNodes.insert(aFace->begin_nodes(),aFace->end_nodes());
                _elementIDToSizeMap.insert(make_pair(aFace->GetID(), size));
                SMDS_ElemIteratorPtr nodeIt = aFace->nodesIterator();
                for (;nodeIt->more();) {
                  node = dynamic_cast<const SMDS_MeshNode*>(nodeIt->next());
                  _triID2nodeIDMap[aFace->GetID()].push_back(node->GetID());
                  _nodeIDToSizeMap.insert(make_pair(node->GetID(), size));
                }
              }
              else if (aFace->NbNodes() == 4) {
                _enfQuadrangles.insert(aFace);
//                 _enfNodes.insert(aFace->begin_nodes(),aFace->end_nodes());
                _elementIDToSizeMap.insert(make_pair(aFace->GetID(), size));
                SMDS_ElemIteratorPtr nodeIt = aFace->nodesIterator();
                for (;nodeIt->more();) {
                  node = dynamic_cast<const SMDS_MeshNode*>(nodeIt->next());
                  _quadID2nodeIDMap[aFace->GetID()].push_back(node->GetID());
                  _nodeIDToSizeMap.insert(make_pair(node->GetID(), size));
                }
              }
            }
          }
        }
        NotifySubMeshesHypothesisModification();
        break;
    };
  }
}

//=======================================================================
//function : GetEnforcedVertex
//=======================================================================

double GHS3DPlugin_Hypothesis::GetEnforcedVertex(double x, double y, double z)
  throw (std::invalid_argument)
{
  std::vector<double> coord(3);
  coord[0] = x;
  coord[1] = y;
  coord[2] = z;
  if (myEnforcedVertices.count(coord)>0)
    return myEnforcedVertices[coord];
  std::ostringstream msg ;
  msg << "No enforced vertex at " << x << ", " << y << ", " << z;
  throw std::invalid_argument(msg.str());
}

//=======================================================================
//function : RemoveEnforcedVertex
//=======================================================================

void GHS3DPlugin_Hypothesis::RemoveEnforcedVertex(double x, double y, double z)
  throw (std::invalid_argument)
{
    std::vector<double> coord(3);
    coord[0] = x;
    coord[1] = y;
    coord[2] = z;
    TEnforcedVertexValues::iterator it = myEnforcedVertices.find(coord);
    if (it != myEnforcedVertices.end()) {
        myEnforcedVertices.erase(it);
        NotifySubMeshesHypothesisModification();
        return;
    }
    std::ostringstream msg ;
    msg << "No enforced vertex at " << x << ", " << y << ", " << z;
    throw std::invalid_argument(msg.str());
}

//=======================================================================
//function : ClearEnforcedVertices
//=======================================================================
void GHS3DPlugin_Hypothesis::ClearEnforcedVertices()
{
    myEnforcedVertices.clear();
    NotifySubMeshesHypothesisModification();
}

//=======================================================================
//function : ClearEnforcedMeshes
//=======================================================================
void GHS3DPlugin_Hypothesis::ClearEnforcedMeshes()
{
   _enfNodes.clear();
   _enfEdges.clear();
   _enfTriangles.clear();
   _enfQuadrangles.clear();
//    _edgeID2nodeIDMap.clear();
//    _triID2nodeIDMap.clear();
//    _quadID2nodeIDMap.clear();
//    _nodeIDToSizeMap.clear();
//    _elementIDToSizeMap.clear();
   NotifySubMeshesHypothesisModification();
}


//=======================================================================
//function : DefaultMeshHoles
//=======================================================================

bool GHS3DPlugin_Hypothesis::DefaultMeshHoles()
{
  return false; // PAL19680
}

//=======================================================================
//function : DefaultMaximumMemory
//=======================================================================

#ifndef WIN32
#include <sys/sysinfo.h>
#else
#include <windows.h>
#endif

short  GHS3DPlugin_Hypothesis::DefaultMaximumMemory()
{
#ifndef WIN32
  struct sysinfo si;
  int err = sysinfo( &si );
  if ( err == 0 ) {
    int ramMB = si.totalram * si.mem_unit / 1024 / 1024;
    return (int) ( 0.7 * ramMB );
  }
#else
  // See http://msdn.microsoft.com/en-us/library/aa366589.aspx
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof (statex);
  int err = GlobalMemoryStatusEx (&statex);
  if (err != 0) {
    int totMB = 
      statex.ullTotalPhys / 1024 / 1024 +
      statex.ullTotalPageFile / 1024 / 1024 +
      statex.ullTotalVirtual / 1024 / 1024;
    return (int) ( 0.7 * totMB );
  }
#endif
  return 1024;
}

//=======================================================================
//function : DefaultInitialMemory
//=======================================================================

short  GHS3DPlugin_Hypothesis::DefaultInitialMemory()
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
//function : DefaultEnforcedVertices
//=======================================================================

GHS3DPlugin_Hypothesis::TEnforcedVertexValues GHS3DPlugin_Hypothesis::DefaultEnforcedVertices()
{
  return GHS3DPlugin_Hypothesis::TEnforcedVertexValues();
}

TIDSortedNodeSet GHS3DPlugin_Hypothesis::DefaultIDSortedNodeSet()
{
  return TIDSortedNodeSet();
}

TIDSortedElemSet GHS3DPlugin_Hypothesis::DefaultIDSortedElemSet()
{
  return TIDSortedElemSet();
}

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
  if (!myTextOption.empty()) {
    save << "__OPTIONS_BEGIN__ ";
    save << myTextOption                      << " ";
    save << "__OPTIONS_END__ ";
  }
  

  TEnforcedVertexValues::iterator it  = myEnforcedVertices.begin();
  if (it != myEnforcedVertices.end()) {
    save << "__ENFORCED_VERTICES_BEGIN__ ";
    for ( ; it != myEnforcedVertices.end(); ++it ) {
        save << it->first[0] << " "
             << it->first[1] << " "
             << it->first[2] << " "
             << it->second << " ";
    }
    save << "__ENFORCED_VERTICES_END__ ";
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
    
    isOK = (load >> i);
    if (isOK)
        myToMeshHoles = i;
    else
        load.clear(ios::badbit | load.rdstate());
    
    isOK = (load >> i);
    if (isOK)
        myMaximumMemory = i;
    else
        load.clear(ios::badbit | load.rdstate());
    
    isOK = (load >> i);
    if (isOK)
        myInitialMemory = i;
    else
        load.clear(ios::badbit | load.rdstate());
    
    isOK = (load >> i);
    if (isOK)
        myOptimizationLevel = i;
    else
        load.clear(ios::badbit | load.rdstate());
    
    isOK = (load >> myWorkingDirectory);
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
        isOK = (load >> i);
        if (isOK)
            myKeepFiles = i;
        else
            load.clear(ios::badbit | load.rdstate());
    }
    
    isOK = (load >> i);
    if (isOK)
        myVerboseLevel = (short) i;
    else
        load.clear(ios::badbit | load.rdstate());
    
    isOK = (load >> i);
    if (isOK)
        myToCreateNewNodes = (bool) i;
    else
        load.clear(ios::badbit | load.rdstate());
    
    isOK = (load >> i);
    if (isOK)
        myToUseBoundaryRecoveryVersion = (bool) i;
    else
        load.clear(ios::badbit | load.rdstate());
    
    isOK = (load >> i);
    if (isOK)
        myToUseFemCorrection = (bool) i;
    else
        load.clear(ios::badbit | load.rdstate());
    
    isOK = (load >> i);
    if (isOK)
        myToRemoveCentralPoint = (bool) i;
    else
        load.clear(ios::badbit | load.rdstate());
    
    std::string separator;
    bool hasOptions = false;
    bool hasEnforcedVertices = false;
    isOK = (load >> separator);

    if (isOK) {
        if (separator == "__OPTIONS_BEGIN__")
            hasOptions = true;
        else if (separator == "__ENFORCED_VERTICES_BEGIN__")
            hasEnforcedVertices = true;
    }

    if (hasOptions) {
        std::string txt;
        while (isOK) {
            isOK = (load >> txt);
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
        isOK = (load >> separator);
        if (isOK)
            if (separator == "__ENFORCED_VERTICES_BEGIN__")
                hasEnforcedVertices = true;
    }

    if (hasEnforcedVertices) {
        std::string txt;
        double x,y,z,size;
        while (isOK) {
            isOK = (load >> txt);
            if (isOK) {
                if (txt == "__ENFORCED_VERTICES_END__") {
                    isOK = false;
                    break;
                }
                x = atof(txt.c_str());
                isOK = (load >> y >> z >> size);
            }
            if (isOK) {
                std::vector<double> coord;
                coord.push_back(x);
                coord.push_back(y);
                coord.push_back(z);
                myEnforcedVertices[ coord ] = size;
            }
        }
    }

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
 * \brief Return false
 */
//================================================================================

bool GHS3DPlugin_Hypothesis::SetParametersByDefaults(const TDefaults&  /*dflts*/,
                                                     const SMESH_Mesh* /*theMesh*/)
{
  return false;
}

//================================================================================
/*!
 * \brief Return command to run ghs3d mesher excluding file prefix (-f)
 */
//================================================================================

std::string GHS3DPlugin_Hypothesis::CommandToRun(const GHS3DPlugin_Hypothesis* hyp,
                                            const bool                    hasShapeToMesh)
{
  TCollection_AsciiString cmd( "ghs3d" ); // ghs3dV4.1_32bits (to permit the .mesh output)
  // check if any option is overridden by hyp->myTextOption
  bool m   = hyp ? ( hyp->myTextOption.find("-m")  == std::string::npos ) : true;
  bool M   = hyp ? ( hyp->myTextOption.find("-M")  == std::string::npos ) : true;
  bool c   = hyp ? ( hyp->myTextOption.find("-c")  == std::string::npos ) : true;
  bool o   = hyp ? ( hyp->myTextOption.find("-o")  == std::string::npos ) : true;
  bool p0  = hyp ? ( hyp->myTextOption.find("-p0") == std::string::npos ) : true;
  bool C   = hyp ? ( hyp->myTextOption.find("-C")  == std::string::npos ) : true;
  bool v   = hyp ? ( hyp->myTextOption.find("-v")  == std::string::npos ) : true;
  bool fem = hyp ? ( hyp->myTextOption.find("-FEM")== std::string::npos ) : true;
  bool rem = hyp ? ( hyp->myTextOption.find("-no_initial_central_point")== std::string::npos ) : true;

  // if use boundary recovery version, few options are allowed
  bool useBndRecovery = !C;
  if ( !useBndRecovery && hyp )
    useBndRecovery = hyp->myToUseBoundaryRecoveryVersion;

  // ghs3d needs to know amount of memory it may use (MB).
  // Default memory is defined at ghs3d installation but it may be not enough,
  // so allow to use about all available memory
  if ( m ) {
    short aMaximumMemory = hyp ? hyp->myMaximumMemory : -1;
    cmd += " -m ";
    if ( aMaximumMemory < 0 )
      cmd += DefaultMaximumMemory();
    else
      cmd += aMaximumMemory;
  }
  if ( M && !useBndRecovery ) {
    short aInitialMemory = hyp ? hyp->myInitialMemory : -1;
    cmd += " -M ";
    if ( aInitialMemory > 0 )
      cmd += aInitialMemory;
    else
      cmd += "100";
  }
  // component to mesh
  // 0 , all components to be meshed
  // 1 , only the main ( outermost ) component to be meshed
  if ( c && !useBndRecovery ) {
    // We always run GHS3D with "to mesh holes'==TRUE (see PAL19680)
    if ( hasShapeToMesh )
      cmd += " -c 0";
    else {
      bool aToMeshHoles = hyp ? hyp->myToMeshHoles : DefaultMeshHoles();
      if ( aToMeshHoles )
        cmd += " -c 0";
      else
        cmd += " -c 1";
    }
  }

  // optimization level
  if ( o && hyp && !useBndRecovery ) {
    if ( hyp->myOptimizationLevel >= 0 && hyp->myOptimizationLevel < 5 ) {
      const char* level[] = { "none" , "light" , "standard" , "standard+" , "strong" };
      cmd += " -o ";
      cmd += level[ hyp->myOptimizationLevel ];
    }
  }

  // to create internal nodes
  if ( p0 && hyp && !hyp->myToCreateNewNodes ) {
    cmd += " -p0";
  }

  // verbose mode
  if ( v && hyp ) {
    cmd += " -v ";
    cmd += hyp->myVerboseLevel;
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
    cmd += " -no_initial_central_point";
  }

  // options as text
  if ( hyp && !hyp->myTextOption.empty() ) {
    cmd += " ";
    cmd += (char*) hyp->myTextOption.c_str();
  }

#ifdef WNT
  cmd += " < NUL";
#endif

  return cmd.ToCString();
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
/*!
* \brief Return the enforced vertices
*/
//================================================================================

GHS3DPlugin_Hypothesis::TEnforcedVertexValues GHS3DPlugin_Hypothesis::GetEnforcedVertices(const GHS3DPlugin_Hypothesis* hyp)
{
    return hyp ? hyp->_GetEnforcedVertices():DefaultEnforcedVertices();
}

TIDSortedNodeSet GHS3DPlugin_Hypothesis::GetEnforcedNodes(const GHS3DPlugin_Hypothesis* hyp)
{
    return hyp ? hyp->_GetEnforcedNodes():DefaultIDSortedNodeSet();
}

TIDSortedElemSet GHS3DPlugin_Hypothesis::GetEnforcedEdges(const GHS3DPlugin_Hypothesis* hyp)
{
    return hyp ? hyp->_GetEnforcedEdges():DefaultIDSortedElemSet();
}

TIDSortedElemSet GHS3DPlugin_Hypothesis::GetEnforcedTriangles(const GHS3DPlugin_Hypothesis* hyp)
{
    return hyp ? hyp->_GetEnforcedTriangles():DefaultIDSortedElemSet();
}

TIDSortedElemSet GHS3DPlugin_Hypothesis::GetEnforcedQuadrangles(const GHS3DPlugin_Hypothesis* hyp)
{
    return hyp ? hyp->_GetEnforcedQuadrangles():DefaultIDSortedElemSet();
}

GHS3DPlugin_Hypothesis::TElemID2NodeIDMap GHS3DPlugin_Hypothesis::GetEdgeID2NodeIDMap(const GHS3DPlugin_Hypothesis* hyp)
{
    return hyp ? hyp->_GetEdgeID2NodeIDMap(): GHS3DPlugin_Hypothesis::TElemID2NodeIDMap();
}

GHS3DPlugin_Hypothesis::TElemID2NodeIDMap GHS3DPlugin_Hypothesis::GetTri2NodeMap(const GHS3DPlugin_Hypothesis* hyp)
{
    return hyp ? hyp->_GetTri2NodeMap(): GHS3DPlugin_Hypothesis::TElemID2NodeIDMap();
}

GHS3DPlugin_Hypothesis::TElemID2NodeIDMap GHS3DPlugin_Hypothesis::GetQuad2NodeMap(const GHS3DPlugin_Hypothesis* hyp)
{
    return hyp ? hyp->_GetQuad2NodeMap(): GHS3DPlugin_Hypothesis::TElemID2NodeIDMap();
}

GHS3DPlugin_Hypothesis::TID2SizeMap GHS3DPlugin_Hypothesis::GetNodeIDToSizeMap(const GHS3DPlugin_Hypothesis* hyp)
{
    return hyp ? hyp->_GetNodeIDToSizeMap(): GHS3DPlugin_Hypothesis::TID2SizeMap();
}

GHS3DPlugin_Hypothesis::TID2SizeMap GHS3DPlugin_Hypothesis::GetElementIDToSizeMap(const GHS3DPlugin_Hypothesis* hyp)
{
    return hyp ? hyp->_GetElementIDToSizeMap(): GHS3DPlugin_Hypothesis::TID2SizeMap();
}
