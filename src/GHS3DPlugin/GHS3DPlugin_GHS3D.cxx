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
// File      : GHS3DPlugin_GHS3D.cxx
// Created   : 
// Author    : Edward AGAPOV, modified by Lioka RAZAFINDRAZAKA (CEA) 09/02/2007
// Project   : SALOME
//=============================================================================
//
#include "GHS3DPlugin_GHS3D.hxx"
#include "GHS3DPlugin_Hypothesis.hxx"
#include "MG_Tetra_API.hxx"

#include <SMDS_FaceOfNodes.hxx>
#include <SMDS_LinearEdge.hxx>
#include <SMDS_MeshElement.hxx>
#include <SMDS_MeshNode.hxx>
#include <SMDS_VolumeOfNodes.hxx>
#include <SMESHDS_Group.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMESH_Comment.hxx>
#include <SMESH_File.hxx>
#include <SMESH_Group.hxx>
#include <SMESH_HypoFilter.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESH_MeshAlgos.hxx>
#include <SMESH_MeshEditor.hxx>
#include <SMESH_MesherHelper.hxx>
#include <SMESH_OctreeNode.hxx>
#include <SMESH_subMeshEventListener.hxx>
#include <StdMeshers_QuadToTriaAdaptor.hxx>
#include <StdMeshers_ViscousLayers.hxx>

#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepClass3d.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Precision.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <Standard_ProgramError.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>

#include <Basics_Utils.hxx>
#include <utilities.h>

#include <algorithm>
#include <errno.h>

#ifdef _DEBUG_
//#define _MY_DEBUG_
#endif

#define castToNode(n) static_cast<const SMDS_MeshNode *>( n );

using namespace std;

#define HOLE_ID -1

// flags returning state of enforced entities, returned from writeGMFFile
enum InvalidEnforcedFlags { FLAG_BAD_ENF_VERT = 1,
                            FLAG_BAD_ENF_NODE = 2,
                            FLAG_BAD_ENF_EDGE = 4,
                            FLAG_BAD_ENF_TRIA = 8
};
static std::string flagsToErrorStr( int anInvalidEnforcedFlags )
{
  std::string str;
  if ( anInvalidEnforcedFlags != 0 )
  {
    if ( anInvalidEnforcedFlags & FLAG_BAD_ENF_VERT )
      str = "There are enforced vertices incorrectly defined.\n";
    if ( anInvalidEnforcedFlags & FLAG_BAD_ENF_NODE )
      str += "There are enforced nodes incorrectly defined.\n";
    if ( anInvalidEnforcedFlags & FLAG_BAD_ENF_EDGE )
      str += "There are enforced edge incorrectly defined.\n";
    if ( anInvalidEnforcedFlags & FLAG_BAD_ENF_TRIA )
      str += "There are enforced triangles incorrectly defined.\n";
  }
  return str;
}

typedef const list<const SMDS_MeshFace*> TTriaList;

static const char theDomainGroupNamePrefix[] = "Domain_";

static void removeFile( const TCollection_AsciiString& fileName )
{
  try {
    SMESH_File( fileName.ToCString() ).remove();
  }
  catch ( ... ) {
    MESSAGE("Can't remove file: " << fileName.ToCString() << " ; file does not exist or permission denied");
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

GHS3DPlugin_GHS3D::GHS3DPlugin_GHS3D(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_3D_Algo(hypId, studyId, gen), _isLibUsed( false )
{
  _name = Name();
  _shapeType = (1 << TopAbs_SHELL) | (1 << TopAbs_SOLID);// 1 bit /shape type
  _onlyUnaryInput = false; // Compute() will be called on a compound of solids
  _iShape=0;
  _nbShape=0;
  _compatibleHypothesis.push_back( GHS3DPlugin_Hypothesis::GetHypType());
  _compatibleHypothesis.push_back( StdMeshers_ViscousLayers::GetHypType() );
  _requireShape = false; // can work without shape

  _smeshGen_i = SMESH_Gen_i::GetSMESHGen();
  CORBA::Object_var anObject = _smeshGen_i->GetNS()->Resolve("/myStudyManager");
  SALOMEDS::StudyManager_var aStudyMgr = SALOMEDS::StudyManager::_narrow(anObject);

  _study = NULL;
  _study = aStudyMgr->GetStudyByID(_studyId);
  
  _computeCanceled = false;
  _progressAdvance = 1e-4;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

GHS3DPlugin_GHS3D::~GHS3DPlugin_GHS3D()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool GHS3DPlugin_GHS3D::CheckHypothesis ( SMESH_Mesh&         aMesh,
                                          const TopoDS_Shape& aShape,
                                          Hypothesis_Status&  aStatus )
{
  aStatus = SMESH_Hypothesis::HYP_OK;

  _hyp = 0;
  _viscousLayersHyp = 0;
  _keepFiles = false;
  _removeLogOnSuccess = true;
  _logInStandardOutput = false;

  const list <const SMESHDS_Hypothesis * >& hyps =
    GetUsedHypothesis(aMesh, aShape, /*ignoreAuxiliary=*/false);
  list <const SMESHDS_Hypothesis* >::const_iterator h = hyps.begin();
  for ( ; h != hyps.end(); ++h )
  {
    if ( !_hyp )
      _hyp = dynamic_cast< const GHS3DPlugin_Hypothesis*> ( *h );
    if ( !_viscousLayersHyp )
      _viscousLayersHyp = dynamic_cast< const StdMeshers_ViscousLayers*> ( *h );
  }
  if ( _hyp )
  {
    _keepFiles = _hyp->GetKeepFiles();
    _removeLogOnSuccess = _hyp->GetRemoveLogOnSuccess();
    _logInStandardOutput = _hyp->GetStandardOutputLog();
  }

  if ( _viscousLayersHyp )
    error( _viscousLayersHyp->CheckHypothesis( aMesh, aShape, aStatus ));

  return aStatus == HYP_OK;
}


//=======================================================================
//function : entryToShape
//purpose  : 
//=======================================================================

TopoDS_Shape GHS3DPlugin_GHS3D::entryToShape(std::string entry)
{
  if ( _study->_is_nil() )
    throw SALOME_Exception("MG-Tetra plugin can't work w/o publishing in the study");
  GEOM::GEOM_Object_var aGeomObj;
  TopoDS_Shape S = TopoDS_Shape();
  SALOMEDS::SObject_var aSObj = _study->FindObjectID( entry.c_str() );
  if (!aSObj->_is_nil() ) {
    CORBA::Object_var obj = aSObj->GetObject();
    aGeomObj = GEOM::GEOM_Object::_narrow(obj);
    aSObj->UnRegister();
  }
  if ( !aGeomObj->_is_nil() )
    S = _smeshGen_i->GeomObjectToShape( aGeomObj.in() );
  return S;
}

//================================================================================
/*!
 * \brief returns id of a solid if a triangle defined by the nodes is a temporary face on a
 * side facet of pyramid and defines sub-domian outside the pyramid; else returns HOLE_ID
 */
//================================================================================

static int checkTmpFace(const SMDS_MeshNode* node1,
                        const SMDS_MeshNode* node2,
                        const SMDS_MeshNode* node3)
{
  // find a pyramid sharing the 3 nodes
  SMDS_ElemIteratorPtr vIt1 = node1->GetInverseElementIterator(SMDSAbs_Volume);
  while ( vIt1->more() )
  {
    const SMDS_MeshElement* pyram = vIt1->next();
    if ( pyram->NbCornerNodes() != 5 ) continue;
    int i2, i3;
    if ( (i2 = pyram->GetNodeIndex( node2 )) >= 0 &&
         (i3 = pyram->GetNodeIndex( node3 )) >= 0 )
    {
      // Triangle defines sub-domian inside the pyramid if it's
      // normal points out of the pyram

      // make i2 and i3 hold indices of base nodes of the pyram while
      // keeping the nodes order in the triangle
      const int iApex = 4;
      if ( i2 == iApex )
        i2 = i3, i3 = pyram->GetNodeIndex( node1 );
      else if ( i3 == iApex )
        i3 = i2, i2 = pyram->GetNodeIndex( node1 );

      int i3base = (i2+1) % 4; // next index after i2 within the pyramid base
      bool isDomainInPyramid = ( i3base != i3 );
      return isDomainInPyramid ? HOLE_ID : pyram->getshapeId();
    }
  }
  return HOLE_ID;
}

//=======================================================================
//function : findShapeID
//purpose  : find the solid corresponding to MG-Tetra sub-domain following
//           the technique proposed in MG-Tetra manual (available within
//           MG-Tetra installation) in chapter "B.4 Subdomain (sub-region) assignment".
//           In brief: normal of the triangle defined by the given nodes
//           points out of the domain it is associated to
//=======================================================================

static int findShapeID(SMESH_Mesh&          mesh,
                       const SMDS_MeshNode* node1,
                       const SMDS_MeshNode* node2,
                       const SMDS_MeshNode* node3,
                       const bool           toMeshHoles)
{
  const int invalidID = 0;
  SMESHDS_Mesh* meshDS = mesh.GetMeshDS();

  // face the nodes belong to
  vector<const SMDS_MeshNode *> nodes(3);
  nodes[0] = node1;
  nodes[1] = node2;
  nodes[2] = node3;
  const SMDS_MeshElement * face = meshDS->FindElement( nodes, SMDSAbs_Face, /*noMedium=*/true);
  if ( !face )
    return checkTmpFace(node1, node2, node3);
#ifdef _DEBUG_
  std::cout << "bnd face " << face->GetID() << " - ";
#endif
  // geom face the face assigned to
  SMESH_MeshEditor editor(&mesh);
  int geomFaceID = editor.FindShape( face );
  if ( !geomFaceID )
    return checkTmpFace(node1, node2, node3);
  TopoDS_Shape shape = meshDS->IndexToShape( geomFaceID );
  if ( shape.IsNull() || shape.ShapeType() != TopAbs_FACE )
    return invalidID;
  TopoDS_Face geomFace = TopoDS::Face( shape );

  // solids bounded by geom face
  TopTools_IndexedMapOfShape solids, shells;
  TopTools_ListIteratorOfListOfShape ansIt = mesh.GetAncestors(geomFace);
  for ( ; ansIt.More(); ansIt.Next() ) {
    switch ( ansIt.Value().ShapeType() ) {
    case TopAbs_SOLID:
      solids.Add( ansIt.Value() ); break;
    case TopAbs_SHELL:
      shells.Add( ansIt.Value() ); break;
    default:;
    }
  }
  // analyse found solids
  if ( solids.Extent() == 0 || shells.Extent() == 0)
    return invalidID;

  const TopoDS_Solid& solid1 = TopoDS::Solid( solids(1) );
  if ( solids.Extent() == 1 )
  {
    if ( toMeshHoles )
      return meshDS->ShapeToIndex( solid1 );

    // - Are we at a hole boundary face?
    if ( shells(1).IsSame( BRepClass3d::OuterShell( solid1 )) )
    { // - No, but maybe a hole is bound by two shapes? Does shells(1) touch another shell?
      bool touch = false;
      TopExp_Explorer eExp( shells(1), TopAbs_EDGE );
      // check if any edge of shells(1) belongs to another shell
      for ( ; eExp.More() && !touch; eExp.Next() ) {
        ansIt = mesh.GetAncestors( eExp.Current() );
        for ( ; ansIt.More() && !touch; ansIt.Next() ) {
          if ( ansIt.Value().ShapeType() == TopAbs_SHELL )
            touch = ( !ansIt.Value().IsSame( shells(1) ));
        }
      }
      if (!touch)
        return meshDS->ShapeToIndex( solid1 );
    }
  }
  // find orientation of geom face within the first solid
  TopExp_Explorer fExp( solid1, TopAbs_FACE );
  for ( ; fExp.More(); fExp.Next() )
    if ( geomFace.IsSame( fExp.Current() )) {
      geomFace = TopoDS::Face( fExp.Current() );
      break;
    }
  if ( !fExp.More() )
    return invalidID; // face not found

  // normale to triangle
  gp_Pnt node1Pnt ( node1->X(), node1->Y(), node1->Z() );
  gp_Pnt node2Pnt ( node2->X(), node2->Y(), node2->Z() );
  gp_Pnt node3Pnt ( node3->X(), node3->Y(), node3->Z() );
  gp_Vec vec12( node1Pnt, node2Pnt );
  gp_Vec vec13( node1Pnt, node3Pnt );
  gp_Vec meshNormal = vec12 ^ vec13;
  if ( meshNormal.SquareMagnitude() < DBL_MIN )
    return invalidID;

  // get normale to geomFace at any node
  bool geomNormalOK = false;
  gp_Vec geomNormal;
  SMESH_MesherHelper helper( mesh ); helper.SetSubShape( geomFace );
  for ( int i = 0; !geomNormalOK && i < 3; ++i )
  {
    // find UV of i-th node on geomFace
    const SMDS_MeshNode* nNotOnSeamEdge = 0;
    if ( helper.IsSeamShape( nodes[i]->getshapeId() )) {
      if ( helper.IsSeamShape( nodes[(i+1)%3]->getshapeId() ))
        nNotOnSeamEdge = nodes[(i+2)%3];
      else
        nNotOnSeamEdge = nodes[(i+1)%3];
    }
    bool uvOK = true;
    gp_XY uv = helper.GetNodeUV( geomFace, nodes[i], nNotOnSeamEdge, &uvOK );
    // check that uv is correct
    if (uvOK) {
      double tol = 1e-6;
      TopoDS_Shape nodeShape = helper.GetSubShapeByNode( nodes[i], meshDS );
      if ( !nodeShape.IsNull() )
        switch ( nodeShape.ShapeType() )
        {
        case TopAbs_FACE:   tol = BRep_Tool::Tolerance( TopoDS::Face( nodeShape )); break;
        case TopAbs_EDGE:   tol = BRep_Tool::Tolerance( TopoDS::Edge( nodeShape )); break;
        case TopAbs_VERTEX: tol = BRep_Tool::Tolerance( TopoDS::Vertex( nodeShape )); break;
        default:;
        }
      gp_Pnt nodePnt ( nodes[i]->X(), nodes[i]->Y(), nodes[i]->Z() );
      BRepAdaptor_Surface surface( geomFace );
      uvOK = ( nodePnt.Distance( surface.Value( uv.X(), uv.Y() )) < 2 * tol );
      if ( uvOK ) {
        // normale to geomFace at UV
        gp_Vec du, dv;
        surface.D1( uv.X(), uv.Y(), nodePnt, du, dv );
        geomNormal = du ^ dv;
        if ( geomFace.Orientation() == TopAbs_REVERSED )
          geomNormal.Reverse();
        geomNormalOK = ( geomNormal.SquareMagnitude() > DBL_MIN * 1e3 );
      }
    }
  }
  if ( !geomNormalOK)
    return invalidID;

  // compare normals
  bool isReverse = ( meshNormal * geomNormal ) < 0;
  if ( !isReverse )
    return meshDS->ShapeToIndex( solid1 );

  if ( solids.Extent() == 1 )
    return HOLE_ID; // we are inside a hole
  else
    return meshDS->ShapeToIndex( solids(2) );
}

//=======================================================================
//function : addElemInMeshGroup
//purpose  : Update or create groups in mesh
//=======================================================================

static void addElemInMeshGroup(SMESH_Mesh*             theMesh,
                               const SMDS_MeshElement* anElem,
                               std::string&            groupName,
                               std::set<std::string>&  groupsToRemove)
{
  if ( !anElem ) return; // issue 0021776

  bool groupDone = false;
  SMESH_Mesh::GroupIteratorPtr grIt = theMesh->GetGroups();
  while (grIt->more()) {
    SMESH_Group * group = grIt->next();
    if ( !group ) continue;
    SMESHDS_GroupBase* groupDS = group->GetGroupDS();
    if ( !groupDS ) continue;
    if ( groupDS->GetType()==anElem->GetType() &&groupName.compare(group->GetName())==0) {
      SMESHDS_Group* aGroupDS = static_cast<SMESHDS_Group*>( groupDS );
      aGroupDS->SMDSGroup().Add(anElem);
      groupDone = true;
      break;
    }
  }
  
  if (!groupDone)
  {
    int groupId;
    SMESH_Group* aGroup = theMesh->AddGroup(anElem->GetType(), groupName.c_str(), groupId);
    aGroup->SetName( groupName.c_str() );
    SMESHDS_Group* aGroupDS = static_cast<SMESHDS_Group*>( aGroup->GetGroupDS() );
    aGroupDS->SMDSGroup().Add(anElem);
    groupDone = true;
  }
  if (!groupDone)
    throw SALOME_Exception(LOCALIZED("A given element was not added to a group"));
}


//=======================================================================
//function : updateMeshGroups
//purpose  : Update or create groups in mesh
//=======================================================================

static void updateMeshGroups(SMESH_Mesh* theMesh, std::set<std::string> groupsToRemove)
{
  SMESH_Mesh::GroupIteratorPtr grIt = theMesh->GetGroups();
  while (grIt->more()) {
    SMESH_Group * group = grIt->next();
    if ( !group ) continue;
    SMESHDS_GroupBase* groupDS = group->GetGroupDS();
    if ( !groupDS ) continue;
    std::string currentGroupName = (string)group->GetName();
    if (groupDS->IsEmpty() && groupsToRemove.find(currentGroupName) != groupsToRemove.end()) {
      // Previous group created by enforced elements
      theMesh->RemoveGroup(groupDS->GetID());
    }
  }
}

//=======================================================================
//function : removeEmptyGroupsOfDomains
//purpose  : remove empty groups named "Domain_nb" created due to
//           "To make groups of domains" option.
//=======================================================================

static void removeEmptyGroupsOfDomains(SMESH_Mesh* mesh,
                                       bool        notEmptyAsWell = false)
{
  const char* refName = theDomainGroupNamePrefix;
  const size_t refLen = strlen( theDomainGroupNamePrefix );

  std::list<int> groupIDs = mesh->GetGroupIds();
  std::list<int>::const_iterator id = groupIDs.begin();
  for ( ; id != groupIDs.end(); ++id )
  {
    SMESH_Group* group = mesh->GetGroup( *id );
    if ( !group || ( !group->GetGroupDS()->IsEmpty() && !notEmptyAsWell ))
      continue;
    const char* name = group->GetName();
    char* end;
    // check the name
    if ( strncmp( name, refName, refLen ) == 0 && // starts from refName;
         isdigit( *( name + refLen )) &&          // refName is followed by a digit;
         strtol( name + refLen, &end, 10) >= 0 && // there are only digits ...
         *end == '\0')                            // ... till a string end.
    {
      mesh->RemoveGroup( *id );
    }
  }
}

//================================================================================
/*!
 * \brief Create the groups corresponding to domains
 */
//================================================================================

static void makeDomainGroups( std::vector< std::vector< const SMDS_MeshElement* > >& elemsOfDomain,
                              SMESH_MesherHelper*                                    theHelper)
{
  // int nbDomains = 0;
  // for ( size_t i = 0; i < elemsOfDomain.size(); ++i )
  //   nbDomains += ( elemsOfDomain[i].size() > 0 );

  // if ( nbDomains > 1 )
  for ( size_t iDomain = 0; iDomain < elemsOfDomain.size(); ++iDomain )
  {
    std::vector< const SMDS_MeshElement* > & elems = elemsOfDomain[ iDomain ];
    if ( elems.empty() ) continue;

    // find existing groups
    std::vector< SMESH_Group* > groupOfType( SMDSAbs_NbElementTypes, (SMESH_Group*)NULL );
    const std::string domainName = ( SMESH_Comment( theDomainGroupNamePrefix ) << iDomain );
    SMESH_Mesh::GroupIteratorPtr groupIt = theHelper->GetMesh()->GetGroups();
    while ( groupIt->more() )
    {
      SMESH_Group* group = groupIt->next();
      if ( domainName == group->GetName() &&
           dynamic_cast< SMESHDS_Group* >( group->GetGroupDS()) )
        groupOfType[ group->GetGroupDS()->GetType() ] = group;
    }
    // create and fill the groups
    size_t iElem = 0;
    int groupID;
    do
    {
      SMESH_Group* group = groupOfType[ elems[ iElem ]->GetType() ];
      if ( !group )
        group = theHelper->GetMesh()->AddGroup( elems[ iElem ]->GetType(),
                                                domainName.c_str(), groupID );
      SMDS_MeshGroup& groupDS =
        static_cast< SMESHDS_Group* >( group->GetGroupDS() )->SMDSGroup();

      while ( iElem < elems.size() && groupDS.Add( elems[iElem] ))
        ++iElem;

    } while ( iElem < elems.size() );
  }
}

//=======================================================================
//function : readGMFFile
//purpose  : read GMF file w/o geometry associated to mesh
//=======================================================================

static bool readGMFFile(MG_Tetra_API*                   MGOutput,
                        const char*                     theFile,
                        GHS3DPlugin_GHS3D*              theAlgo,
                        SMESH_MesherHelper*             theHelper,
                        std::vector <const SMDS_MeshNode*> &    theNodeByGhs3dId,
                        std::vector <const SMDS_MeshElement*> & theFaceByGhs3dId,
                        map<const SMDS_MeshNode*,int> & theNodeToGhs3dIdMap,
                        std::vector<std::string> &      aNodeGroupByGhs3dId,
                        std::vector<std::string> &      anEdgeGroupByGhs3dId,
                        std::vector<std::string> &      aFaceGroupByGhs3dId,
                        std::set<std::string> &         groupsToRemove,
                        bool                            toMakeGroupsOfDomains=false,
                        bool                            toMeshHoles=true)
{
  std::string tmpStr;
  SMESHDS_Mesh* theMeshDS = theHelper->GetMeshDS();
  const bool hasGeom = ( theHelper->GetMesh()->HasShapeToMesh() );

  int nbInitialNodes = theNodeByGhs3dId.size();
  int nbMeshNodes = theMeshDS->NbNodes();
  
  const bool isQuadMesh = 
    theHelper->GetMesh()->NbEdges( ORDER_QUADRATIC ) ||
    theHelper->GetMesh()->NbFaces( ORDER_QUADRATIC ) ||
    theHelper->GetMesh()->NbVolumes( ORDER_QUADRATIC );
    
#ifdef _DEBUG_
  std::cout << "theNodeByGhs3dId.size(): " << nbInitialNodes << std::endl;
  std::cout << "theHelper->GetMesh()->NbNodes(): " << nbMeshNodes << std::endl;
  std::cout << "isQuadMesh: " << isQuadMesh << std::endl;
#endif
  
  // ---------------------------------
  // Read generated elements and nodes
  // ---------------------------------

  int nbElem = 0, nbRef = 0;
  int aGMFNodeID = 0;
  std::vector< const SMDS_MeshNode*> GMFNode;
#ifdef _DEBUG_
  std::map<int, std::set<int> > subdomainId2tetraId;
#endif
  std::map <GmfKwdCod,int> tabRef;
  const bool force3d = !hasGeom;
  const int  noID    = 0;

  tabRef[GmfVertices]       = 3; // for new nodes and enforced nodes
  tabRef[GmfCorners]        = 1;
  tabRef[GmfEdges]          = 2; // for enforced edges
  tabRef[GmfRidges]         = 1;
  tabRef[GmfTriangles]      = 3; // for enforced faces
  tabRef[GmfQuadrilaterals] = 4;
  tabRef[GmfTetrahedra]     = 4; // for new tetras
  tabRef[GmfHexahedra]      = 8;

  int ver, dim;
  int InpMsh = MGOutput->GmfOpenMesh( theFile, GmfRead, &ver, &dim);
  if (!InpMsh)
    return false;

  // Read ids of domains
  vector< int > solidIDByDomain;
  if ( hasGeom )
  {
    int solid1; // id used in case of 1 domain or some reading failure
    if ( theHelper->GetSubShape().ShapeType() == TopAbs_SOLID )
      solid1 = theHelper->GetSubShapeID();
    else
      solid1 = theMeshDS->ShapeToIndex
        ( TopExp_Explorer( theHelper->GetSubShape(), TopAbs_SOLID ).Current() );

    int nbDomains = MGOutput->GmfStatKwd( InpMsh, GmfSubDomainFromGeom );
    if ( nbDomains > 1 )
    {
      solidIDByDomain.resize( nbDomains+1, theHelper->GetSubShapeID() );
      int faceNbNodes, faceIndex, orientation, domainNb;
      MGOutput->GmfGotoKwd( InpMsh, GmfSubDomainFromGeom );
      for ( int i = 0; i < nbDomains; ++i )
      {
        faceIndex = 0;
        MGOutput->GmfGetLin( InpMsh, GmfSubDomainFromGeom,
                   &faceNbNodes, &faceIndex, &orientation, &domainNb, i);
        solidIDByDomain[ domainNb ] = 1;
        if ( 0 < faceIndex && faceIndex-1 < (int)theFaceByGhs3dId.size() )
        {
          const SMDS_MeshElement* face = theFaceByGhs3dId[ faceIndex-1 ];
          const SMDS_MeshNode* nn[3] = { face->GetNode(0),
                                         face->GetNode(1),
                                         face->GetNode(2) };
          if ( orientation < 0 )
            std::swap( nn[1], nn[2] );
          solidIDByDomain[ domainNb ] =
            findShapeID( *theHelper->GetMesh(), nn[0], nn[1], nn[2], toMeshHoles );
          if ( solidIDByDomain[ domainNb ] > 0 )
          {
#ifdef _DEBUG_
            std::cout << "solid " << solidIDByDomain[ domainNb ] << std::endl;
#endif
            const TopoDS_Shape& foundShape = theMeshDS->IndexToShape( solidIDByDomain[ domainNb ] );
            if ( ! theHelper->IsSubShape( foundShape, theHelper->GetSubShape() ))
              solidIDByDomain[ domainNb ] = HOLE_ID;
          }
        }
      }
    }
    if ( solidIDByDomain.size() < 2 )
      solidIDByDomain.resize( 2, solid1 );
  }

  // Issue 0020682. Avoid creating nodes and tetras at place where
  // volumic elements already exist
  SMESH_ElementSearcher* elemSearcher = 0;
  std::vector< const SMDS_MeshElement* > foundVolumes;
  if ( !hasGeom && theHelper->GetMesh()->NbVolumes() > 0 )
    elemSearcher = SMESH_MeshAlgos::GetElementSearcher( *theMeshDS );
  auto_ptr< SMESH_ElementSearcher > elemSearcherDeleter( elemSearcher );

  // IMP 0022172: [CEA 790] create the groups corresponding to domains
  std::vector< std::vector< const SMDS_MeshElement* > > elemsOfDomain;

  int nbVertices = MGOutput->GmfStatKwd( InpMsh, GmfVertices ) - nbInitialNodes;
  if ( nbVertices < 0 )
    return false;
  GMFNode.resize( nbVertices + 1 );

  std::map <GmfKwdCod,int>::const_iterator it = tabRef.begin();
  for ( ; it != tabRef.end() ; ++it)
  {
    if(theAlgo->computeCanceled()) {
      return false;
    }
    int dummy, solidID;
    GmfKwdCod token = it->first;
    nbRef           = it->second;

    nbElem = MGOutput->GmfStatKwd( InpMsh, token);
    if (nbElem > 0) {
      MGOutput->GmfGotoKwd( InpMsh, token);
      std::cout << "Read " << nbElem;
    }
    else
      continue;

    std::vector<int> id (nbElem*tabRef[token]); // node ids
    std::vector<int> domainID( nbElem ); // domain

    if (token == GmfVertices) {
      (nbElem <= 1) ? tmpStr = " vertex" : tmpStr = " vertices";
//       std::cout << nbInitialNodes << " from input mesh " << std::endl;

      // Remove orphan nodes from previous enforced mesh which was cleared
//       if ( nbElem < nbMeshNodes ) {
//         const SMDS_MeshNode* node;
//         SMDS_NodeIteratorPtr nodeIt = theMeshDS->nodesIterator();
//         while ( nodeIt->more() )
//         {
//           node = nodeIt->next();
//           if (theNodeToGhs3dIdMap.find(node) != theNodeToGhs3dIdMap.end())
//             theMeshDS->RemoveNode(node);
//         }
//       }

      
      int aGMFID;

      float VerTab_f[3];
      double x, y, z;
      const SMDS_MeshNode * aGMFNode;

      for ( int iElem = 0; iElem < nbElem; iElem++ ) {
        if(theAlgo->computeCanceled()) {
          return false;
        }
        if (ver == GmfFloat) {
          MGOutput->GmfGetLin( InpMsh, token, &VerTab_f[0], &VerTab_f[1], &VerTab_f[2], &dummy);
          x = VerTab_f[0];
          y = VerTab_f[1];
          z = VerTab_f[2];
        }
        else {
          MGOutput->GmfGetLin( InpMsh, token, &x, &y, &z, &dummy);
        }
        if (iElem >= nbInitialNodes) {
          if ( elemSearcher &&
                elemSearcher->FindElementsByPoint( gp_Pnt(x,y,z), SMDSAbs_Volume, foundVolumes))
            aGMFNode = 0;
          else
            aGMFNode = theHelper->AddNode(x, y, z);
          
          aGMFID = iElem -nbInitialNodes +1;
          GMFNode[ aGMFID ] = aGMFNode;
          if (aGMFID-1 < (int)aNodeGroupByGhs3dId.size() && !aNodeGroupByGhs3dId.at(aGMFID-1).empty())
            addElemInMeshGroup(theHelper->GetMesh(), aGMFNode, aNodeGroupByGhs3dId.at(aGMFID-1), groupsToRemove);
        }
      }
    }
    else if (token == GmfCorners && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " corner" : tmpStr = " corners";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        MGOutput->GmfGetLin( InpMsh, token, &id[iElem*tabRef[token]]);
    }
    else if (token == GmfRidges && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " ridge" : tmpStr = " ridges";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        MGOutput->GmfGetLin( InpMsh, token, &id[iElem*tabRef[token]]);
    }
    else if (token == GmfEdges && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " edge" : tmpStr = " edges";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        MGOutput->GmfGetLin( InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &domainID[iElem]);
    }
    else if (token == GmfTriangles && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " triangle" : tmpStr = " triangles";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        MGOutput->GmfGetLin( InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &domainID[iElem]);
    }
    else if (token == GmfQuadrilaterals && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " Quadrilateral" : tmpStr = " Quadrilaterals";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        MGOutput->GmfGetLin( InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &id[iElem*tabRef[token]+3], &domainID[iElem]);
    }
    else if (token == GmfTetrahedra && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " Tetrahedron" : tmpStr = " Tetrahedra";
      for ( int iElem = 0; iElem < nbElem; iElem++ ) {
        MGOutput->GmfGetLin( InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &id[iElem*tabRef[token]+3], &domainID[iElem]);
#ifdef _DEBUG_
        subdomainId2tetraId[dummy].insert(iElem+1);
#endif
      }
    }
    else if (token == GmfHexahedra && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " Hexahedron" : tmpStr = " Hexahedra";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        MGOutput->GmfGetLin( InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &id[iElem*tabRef[token]+3],
                  &id[iElem*tabRef[token]+4], &id[iElem*tabRef[token]+5], &id[iElem*tabRef[token]+6], &id[iElem*tabRef[token]+7], &domainID[iElem]);
    }
    std::cout << tmpStr << std::endl;
    std::cout << std::endl;

    switch (token) {
    case GmfCorners:
    case GmfRidges:
    case GmfEdges:
    case GmfTriangles:
    case GmfQuadrilaterals:
    case GmfTetrahedra:
    case GmfHexahedra:
    {
      std::vector< const SMDS_MeshNode* > node( nbRef );
      std::vector< int >          nodeID( nbRef );
      std::vector< SMDS_MeshNode* > enfNode( nbRef );
      const SMDS_MeshElement* aCreatedElem;

      for ( int iElem = 0; iElem < nbElem; iElem++ )
      {
        if(theAlgo->computeCanceled()) {
          return false;
        }
        // Check if elem is already in input mesh. If yes => skip
        bool fullyCreatedElement = false; // if at least one of the nodes was created
        for ( int iRef = 0; iRef < nbRef; iRef++ )
        {
          aGMFNodeID = id[iElem*tabRef[token]+iRef]; // read nbRef aGMFNodeID
          if (aGMFNodeID <= nbInitialNodes) // input nodes
          {
            aGMFNodeID--;
            node[ iRef ] = theNodeByGhs3dId[aGMFNodeID];
          }
          else
          {
            fullyCreatedElement = true;
            aGMFNodeID -= nbInitialNodes;
            nodeID[ iRef ] = aGMFNodeID ;
            node  [ iRef ] = GMFNode[ aGMFNodeID ];
          }
        }
        aCreatedElem = 0;
        switch (token)
        {
        case GmfEdges:
          if (fullyCreatedElement) {
            aCreatedElem = theHelper->AddEdge( node[0], node[1], noID, force3d );
            if (anEdgeGroupByGhs3dId.size() && !anEdgeGroupByGhs3dId[iElem].empty())
              addElemInMeshGroup(theHelper->GetMesh(), aCreatedElem, anEdgeGroupByGhs3dId[iElem], groupsToRemove);
          }
          break;
        case GmfTriangles:
          if (fullyCreatedElement) {
            aCreatedElem = theHelper->AddFace( node[0], node[1], node[2], noID, force3d );
            if (aFaceGroupByGhs3dId.size() && !aFaceGroupByGhs3dId[iElem].empty())
              addElemInMeshGroup(theHelper->GetMesh(), aCreatedElem, aFaceGroupByGhs3dId[iElem], groupsToRemove);
          }
          break;
        case GmfQuadrilaterals:
          if (fullyCreatedElement) {
            aCreatedElem = theHelper->AddFace( node[0], node[1], node[2], node[3], noID, force3d );
          }
          break;
        case GmfTetrahedra:
          if ( hasGeom )
          {
            solidID = solidIDByDomain[ domainID[iElem]];
            if ( solidID != HOLE_ID )
            {
              aCreatedElem = theHelper->AddVolume( node[1], node[0], node[2], node[3],
                                                   noID, force3d );
              theMeshDS->SetMeshElementOnShape( aCreatedElem, solidID );
              for ( int iN = 0; iN < 4; ++iN )
                if ( node[iN]->getshapeId() < 1 )
                  theMeshDS->SetNodeInVolume( node[iN], solidID );
            }
          }
          else
          {
            if ( elemSearcher ) {
              // Issue 0020682. Avoid creating nodes and tetras at place where
              // volumic elements already exist
              if ( !node[1] || !node[0] || !node[2] || !node[3] )
                continue;
              if ( elemSearcher->FindElementsByPoint((SMESH_TNodeXYZ(node[0]) +
                                                      SMESH_TNodeXYZ(node[1]) +
                                                      SMESH_TNodeXYZ(node[2]) +
                                                      SMESH_TNodeXYZ(node[3]) ) / 4.,
                                                     SMDSAbs_Volume, foundVolumes ))
                break;
            }
            aCreatedElem = theHelper->AddVolume( node[1], node[0], node[2], node[3],
                                                 noID, force3d );
          }
          break;
        case GmfHexahedra:
          if ( hasGeom )
          {
            solidID = solidIDByDomain[ domainID[iElem]];
            if ( solidID != HOLE_ID )
            {
              aCreatedElem = theHelper->AddVolume( node[0], node[3], node[2], node[1],
                                                   node[4], node[7], node[6], node[5],
                                                   noID, force3d );
              theMeshDS->SetMeshElementOnShape( aCreatedElem, solidID );
              for ( int iN = 0; iN < 8; ++iN )
                if ( node[iN]->getshapeId() < 1 )
                  theMeshDS->SetNodeInVolume( node[iN], solidID );
            }
          }
          else
          {
            if ( elemSearcher ) {
              // Issue 0020682. Avoid creating nodes and tetras at place where
              // volumic elements already exist
              if ( !node[1] || !node[0] || !node[2] || !node[3] || !node[4] || !node[5] || !node[6] || !node[7])
                continue;
              if ( elemSearcher->FindElementsByPoint((SMESH_TNodeXYZ(node[0]) +
                                                      SMESH_TNodeXYZ(node[1]) +
                                                      SMESH_TNodeXYZ(node[2]) +
                                                      SMESH_TNodeXYZ(node[3]) +
                                                      SMESH_TNodeXYZ(node[4]) +
                                                      SMESH_TNodeXYZ(node[5]) +
                                                      SMESH_TNodeXYZ(node[6]) +
                                                      SMESH_TNodeXYZ(node[7])) / 8.,
                                                     SMDSAbs_Volume, foundVolumes ))
                break;
            }
            aCreatedElem = theHelper->AddVolume( node[0], node[3], node[2], node[1],
                                                 node[4], node[7], node[6], node[5],
                                                 noID, force3d );
          }
          break;
        default: continue;
        } // switch (token)

        if ( aCreatedElem && toMakeGroupsOfDomains )
        {
          if ( domainID[iElem] >= (int) elemsOfDomain.size() )
            elemsOfDomain.resize( domainID[iElem] + 1 );
          elemsOfDomain[ domainID[iElem] ].push_back( aCreatedElem );
        }
      } // loop on elements of one type
      break;
    } // case ...
    default:;
    } // switch (token)
  } // loop on tabRef

  // remove nodes in holes
  if ( hasGeom )
  {
    for ( int i = 1; i <= nbVertices; ++i )
      if ( GMFNode[i]->NbInverseElements() == 0 )
        theMeshDS->RemoveFreeNode( GMFNode[i], /*sm=*/0, /*fromGroups=*/false );
  }

  MGOutput->GmfCloseMesh( InpMsh);

  // 0022172: [CEA 790] create the groups corresponding to domains
  if ( toMakeGroupsOfDomains )
    makeDomainGroups( elemsOfDomain, theHelper );

#ifdef _DEBUG_
  std::map<int, std::set<int> >::const_iterator subdomainIt = subdomainId2tetraId.begin();
  TCollection_AsciiString aSubdomainFileName = theFile;
  aSubdomainFileName = aSubdomainFileName + ".subdomain";
  ofstream aSubdomainFile  ( aSubdomainFileName.ToCString()  , ios::out);

  aSubdomainFile << "Nb subdomains " << subdomainId2tetraId.size() << std::endl;
  for(;subdomainIt != subdomainId2tetraId.end() ; ++subdomainIt) {
    int subdomainId = subdomainIt->first;
    std::set<int> tetraIds = subdomainIt->second;
    std::set<int>::const_iterator tetraIdsIt = tetraIds.begin();
    aSubdomainFile << subdomainId << std::endl;
    for(;tetraIdsIt != tetraIds.end() ; ++tetraIdsIt) {
      aSubdomainFile << (*tetraIdsIt) << " ";
    }
    aSubdomainFile << std::endl;
  }
  aSubdomainFile.close();
#endif  
  
  return true;
}


static bool writeGMFFile(MG_Tetra_API*                                   MGInput,
                         const char*                                     theMeshFileName,
                         const char*                                     theRequiredFileName,
                         const char*                                     theSolFileName,
                         const SMESH_ProxyMesh&                          theProxyMesh,
                         SMESH_MesherHelper&                             theHelper,
                         std::vector <const SMDS_MeshNode*> &            theNodeByGhs3dId,
                         std::vector <const SMDS_MeshElement*> &         theFaceByGhs3dId,
                         std::map<const SMDS_MeshNode*,int> &            aNodeToGhs3dIdMap,
                         std::vector<std::string> &                      aNodeGroupByGhs3dId,
                         std::vector<std::string> &                      anEdgeGroupByGhs3dId,
                         std::vector<std::string> &                      aFaceGroupByGhs3dId,
                         GHS3DPlugin_Hypothesis::TIDSortedNodeGroupMap & theEnforcedNodes,
                         GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap & theEnforcedEdges,
                         GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap & theEnforcedTriangles,
                         std::map<std::vector<double>, std::string> &    enfVerticesWithGroup,
                         GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues & theEnforcedVertices,
                         int &                                           theInvalidEnforcedFlags)
{
  std::string tmpStr;
  int idx, idxRequired = 0, idxSol = 0;
  const int dummyint = 0;
  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues::const_iterator vertexIt;
  std::vector<double> enfVertexSizes;
  const SMDS_MeshElement* elem;
  TIDSortedElemSet anElemSet, theKeptEnforcedEdges, theKeptEnforcedTriangles;
  SMDS_ElemIteratorPtr nodeIt;
  std::vector <const SMDS_MeshNode*> theEnforcedNodeByGhs3dId;
  map<const SMDS_MeshNode*,int> anEnforcedNodeToGhs3dIdMap, anExistingEnforcedNodeToGhs3dIdMap;
  std::vector< const SMDS_MeshElement* > foundElems;
  map<const SMDS_MeshNode*,TopAbs_State> aNodeToTopAbs_StateMap;
  int nbFoundElems;
  GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap::iterator elemIt;
  TIDSortedElemSet::iterator elemSetIt;
  bool isOK;
  SMESH_Mesh* theMesh = theHelper.GetMesh();
  const bool hasGeom = theMesh->HasShapeToMesh();
  SMESHUtils::Deleter< SMESH_ElementSearcher > pntCls
    ( SMESH_MeshAlgos::GetElementSearcher(*theMesh->GetMeshDS()));
  
  int nbEnforcedVertices = theEnforcedVertices.size();
  theInvalidEnforcedFlags = 0;

  // count faces
  int nbFaces = theProxyMesh.NbFaces();
  int nbNodes;
  theFaceByGhs3dId.reserve( nbFaces );
  
  // groups management
  int usedEnforcedNodes = 0;
  std::string gn = "";

  if ( nbFaces == 0 )
    return false;
  
  idx = MGInput->GmfOpenMesh( theMeshFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
  if (!idx)
    return false;
  
  /* ========================== FACES ========================== */
  /* TRIANGLES ========================== */
  SMDS_ElemIteratorPtr eIt =
    hasGeom ? theProxyMesh.GetFaces( theHelper.GetSubShape()) : theProxyMesh.GetFaces();
  while ( eIt->more() )
  {
    elem = eIt->next();
    anElemSet.insert(elem);
    nodeIt = elem->nodesIterator();
    nbNodes = elem->NbCornerNodes();
    while ( nodeIt->more() && nbNodes--)
    {
      // find MG-Tetra ID
      const SMDS_MeshNode* node = castToNode( nodeIt->next() );
      int newId = aNodeToGhs3dIdMap.size() + 1; // MG-Tetra ids count from 1
      aNodeToGhs3dIdMap.insert( make_pair( node, newId ));
    }
  }
  
  /* EDGES ========================== */
  
  // Iterate over the enforced edges
  for(elemIt = theEnforcedEdges.begin() ; elemIt != theEnforcedEdges.end() ; ++elemIt) {
    elem = elemIt->first;
    isOK = true;
    nodeIt = elem->nodesIterator();
    nbNodes = 2;
    while ( nodeIt->more() && nbNodes-- ) {
      // find MG-Tetra ID
      const SMDS_MeshNode* node = castToNode( nodeIt->next() );
      // Test if point is inside shape to mesh
      gp_Pnt myPoint(node->X(),node->Y(),node->Z());
      TopAbs_State result = pntCls->GetPointState( myPoint );
      if ( result == TopAbs_OUT ) {
        isOK = false;
        theInvalidEnforcedFlags |= FLAG_BAD_ENF_EDGE;
        break;
      }
      aNodeToTopAbs_StateMap.insert( make_pair( node, result ));
    }
    if (isOK) {
      nodeIt = elem->nodesIterator();
      nbNodes = 2;
      int newId = -1;
      while ( nodeIt->more() && nbNodes-- ) {
        // find MG-Tetra ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        gp_Pnt myPoint(node->X(),node->Y(),node->Z());
        nbFoundElems = pntCls->FindElementsByPoint(myPoint, SMDSAbs_Node, foundElems);
#ifdef _MY_DEBUG_
        std::cout << "Node at "<<node->X()<<", "<<node->Y()<<", "<<node->Z()<<std::endl;
        std::cout << "Nb nodes found : "<<nbFoundElems<<std::endl;
#endif
        if (nbFoundElems ==0) {
          if ((*aNodeToTopAbs_StateMap.find(node)).second == TopAbs_IN) {
            newId = aNodeToGhs3dIdMap.size() + anEnforcedNodeToGhs3dIdMap.size() + 1; // MG-Tetra ids count from 1
            anEnforcedNodeToGhs3dIdMap.insert( make_pair( node, newId ));
          }
        }
        else if (nbFoundElems ==1) {
          const SMDS_MeshNode* existingNode = (SMDS_MeshNode*) foundElems.at(0);
          newId = (*aNodeToGhs3dIdMap.find(existingNode)).second;
          anExistingEnforcedNodeToGhs3dIdMap.insert( make_pair( node, newId ));
        }
        else
          isOK = false;
#ifdef _MY_DEBUG_
        std::cout << "MG-Tetra node ID: "<<newId<<std::endl;
#endif
      }
      if (isOK)
        theKeptEnforcedEdges.insert(elem);
      else
        theInvalidEnforcedFlags |= FLAG_BAD_ENF_EDGE;
    }
  }
  
  /* ENFORCED TRIANGLES ========================== */
  
  // Iterate over the enforced triangles
  for(elemIt = theEnforcedTriangles.begin() ; elemIt != theEnforcedTriangles.end() ; ++elemIt) {
    elem = elemIt->first;
    isOK = true;
    nodeIt = elem->nodesIterator();
    nbNodes = 3;
    while ( nodeIt->more() && nbNodes--) {
      // find MG-Tetra ID
      const SMDS_MeshNode* node = castToNode( nodeIt->next() );
      // Test if point is inside shape to mesh
      gp_Pnt myPoint(node->X(),node->Y(),node->Z());
      TopAbs_State result = pntCls->GetPointState( myPoint );
      if ( result == TopAbs_OUT ) {
        isOK = false;
        theInvalidEnforcedFlags |= FLAG_BAD_ENF_TRIA;
        break;
      }
      aNodeToTopAbs_StateMap.insert( make_pair( node, result ));
    }
    if (isOK) {
      nodeIt = elem->nodesIterator();
      nbNodes = 3;
      int newId = -1;
      while ( nodeIt->more() && nbNodes--) {
        // find MG-Tetra ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        gp_Pnt myPoint(node->X(),node->Y(),node->Z());
        nbFoundElems = pntCls->FindElementsByPoint(myPoint, SMDSAbs_Node, foundElems);
#ifdef _MY_DEBUG_
        std::cout << "Nb nodes found : "<<nbFoundElems<<std::endl;
#endif
        if (nbFoundElems ==0) {
          if ((*aNodeToTopAbs_StateMap.find(node)).second == TopAbs_IN) {
            newId = aNodeToGhs3dIdMap.size() + anEnforcedNodeToGhs3dIdMap.size() + 1; // MG-Tetra ids count from 1
            anEnforcedNodeToGhs3dIdMap.insert( make_pair( node, newId ));
          }
        }
        else if (nbFoundElems ==1) {
          const SMDS_MeshNode* existingNode = (SMDS_MeshNode*) foundElems.at(0);
          newId = (*aNodeToGhs3dIdMap.find(existingNode)).second;
          anExistingEnforcedNodeToGhs3dIdMap.insert( make_pair( node, newId ));
        }
        else
          isOK = false;
#ifdef _MY_DEBUG_
        std::cout << "MG-Tetra node ID: "<<newId<<std::endl;
#endif
      }
      if (isOK)
        theKeptEnforcedTriangles.insert(elem);
      else
        theInvalidEnforcedFlags |= FLAG_BAD_ENF_TRIA;
    }
  }
  
  // put nodes to theNodeByGhs3dId vector
#ifdef _MY_DEBUG_
  std::cout << "aNodeToGhs3dIdMap.size(): "<<aNodeToGhs3dIdMap.size()<<std::endl;
#endif
  theNodeByGhs3dId.resize( aNodeToGhs3dIdMap.size() );
  map<const SMDS_MeshNode*,int>::const_iterator n2id = aNodeToGhs3dIdMap.begin();
  for ( ; n2id != aNodeToGhs3dIdMap.end(); ++ n2id)
  {
//     std::cout << "n2id->first: "<<n2id->first<<std::endl;
    theNodeByGhs3dId[ n2id->second - 1 ] = n2id->first; // MG-Tetra ids count from 1
  }

  // put nodes to anEnforcedNodeToGhs3dIdMap vector
#ifdef _MY_DEBUG_
  std::cout << "anEnforcedNodeToGhs3dIdMap.size(): "<<anEnforcedNodeToGhs3dIdMap.size()<<std::endl;
#endif
  theEnforcedNodeByGhs3dId.resize( anEnforcedNodeToGhs3dIdMap.size());
  n2id = anEnforcedNodeToGhs3dIdMap.begin();
  for ( ; n2id != anEnforcedNodeToGhs3dIdMap.end(); ++ n2id)
  {
    if (n2id->second > (int)aNodeToGhs3dIdMap.size()) {
      theEnforcedNodeByGhs3dId[ n2id->second - aNodeToGhs3dIdMap.size() - 1 ] = n2id->first; // MG-Tetra ids count from 1
    }
  }
  
  
  /* ========================== NODES ========================== */
  vector<const SMDS_MeshNode*> theOrderedNodes, theRequiredNodes;
  std::set< std::vector<double> > nodesCoords;
  vector<const SMDS_MeshNode*>::const_iterator ghs3dNodeIt = theNodeByGhs3dId.begin();
  vector<const SMDS_MeshNode*>::const_iterator after  = theNodeByGhs3dId.end();
  
  (theNodeByGhs3dId.size() <= 1) ? tmpStr = " node" : " nodes";
  std::cout << theNodeByGhs3dId.size() << tmpStr << " from mesh ..." << std::endl;
  for ( ; ghs3dNodeIt != after; ++ghs3dNodeIt )
  {
    const SMDS_MeshNode* node = *ghs3dNodeIt;
    std::vector<double> coords;
    coords.push_back(node->X());
    coords.push_back(node->Y());
    coords.push_back(node->Z());
    nodesCoords.insert(coords);
    theOrderedNodes.push_back(node);
  }
  
  // Iterate over the enforced nodes given by enforced elements
  ghs3dNodeIt = theEnforcedNodeByGhs3dId.begin();
  after  = theEnforcedNodeByGhs3dId.end();
  (theEnforcedNodeByGhs3dId.size() <= 1) ? tmpStr = " node" : " nodes";
  std::cout << theEnforcedNodeByGhs3dId.size() << tmpStr << " from enforced elements ..." << std::endl;
  for ( ; ghs3dNodeIt != after; ++ghs3dNodeIt )
  {
    const SMDS_MeshNode* node = *ghs3dNodeIt;
    std::vector<double> coords;
    coords.push_back(node->X());
    coords.push_back(node->Y());
    coords.push_back(node->Z());
#ifdef _MY_DEBUG_
    std::cout << "Node at " << node->X()<<", " <<node->Y()<<", " <<node->Z();
#endif
    
    if (nodesCoords.find(coords) != nodesCoords.end()) {
      // node already exists in original mesh
#ifdef _MY_DEBUG_
      std::cout << " found" << std::endl;
#endif
      continue;
    }
    
    if (theEnforcedVertices.find(coords) != theEnforcedVertices.end()) {
      // node already exists in enforced vertices
#ifdef _MY_DEBUG_
      std::cout << " found" << std::endl;
#endif
      continue;
    }
    
//     gp_Pnt myPoint(node->X(),node->Y(),node->Z());
//     nbFoundElems = pntCls->FindElementsByPoint(myPoint, SMDSAbs_Node, foundElems);
//     if (nbFoundElems ==0) {
//       std::cout << " not found" << std::endl;
//       if ((*aNodeToTopAbs_StateMap.find(node)).second == TopAbs_IN) {
//         nodesCoords.insert(coords);
//         theOrderedNodes.push_back(node);
//       }
//     }
//     else {
//       std::cout << " found in initial mesh" << std::endl;
//       const SMDS_MeshNode* existingNode = (SMDS_MeshNode*) foundElems.at(0);
//       nodesCoords.insert(coords);
//       theOrderedNodes.push_back(existingNode);
//     }
    
#ifdef _MY_DEBUG_
    std::cout << " not found" << std::endl;
#endif
    
    nodesCoords.insert(coords);
    theOrderedNodes.push_back(node);
//     theRequiredNodes.push_back(node);
  }
  
  
  // Iterate over the enforced nodes
  GHS3DPlugin_Hypothesis::TIDSortedNodeGroupMap::const_iterator enfNodeIt;
  (theEnforcedNodes.size() <= 1) ? tmpStr = " node" : " nodes";
  std::cout << theEnforcedNodes.size() << tmpStr << " from enforced nodes ..." << std::endl;
  for(enfNodeIt = theEnforcedNodes.begin() ; enfNodeIt != theEnforcedNodes.end() ; ++enfNodeIt)
  {
    const SMDS_MeshNode* node = enfNodeIt->first;
    std::vector<double> coords;
    coords.push_back(node->X());
    coords.push_back(node->Y());
    coords.push_back(node->Z());
#ifdef _MY_DEBUG_
    std::cout << "Node at " << node->X()<<", " <<node->Y()<<", " <<node->Z();
#endif
    
    // Test if point is inside shape to mesh
    gp_Pnt myPoint(node->X(),node->Y(),node->Z());
    TopAbs_State result = pntCls->GetPointState( myPoint );
    if ( result == TopAbs_OUT ) {
#ifdef _MY_DEBUG_
      std::cout << " out of volume" << std::endl;
#endif
      theInvalidEnforcedFlags |= FLAG_BAD_ENF_NODE;
      continue;
    }
    
    if (nodesCoords.find(coords) != nodesCoords.end()) {
#ifdef _MY_DEBUG_
      std::cout << " found in nodesCoords" << std::endl;
#endif
//       theRequiredNodes.push_back(node);
      continue;
    }

    if (theEnforcedVertices.find(coords) != theEnforcedVertices.end()) {
#ifdef _MY_DEBUG_
      std::cout << " found in theEnforcedVertices" << std::endl;
#endif
      continue;
    }
    
//     nbFoundElems = pntCls->FindElementsByPoint(myPoint, SMDSAbs_Node, foundElems);
//     if (nbFoundElems ==0) {
//       std::cout << " not found" << std::endl;
//       if (result == TopAbs_IN) {
//         nodesCoords.insert(coords);
//         theRequiredNodes.push_back(node);
//       }
//     }
//     else {
//       std::cout << " found in initial mesh" << std::endl;
//       const SMDS_MeshNode* existingNode = (SMDS_MeshNode*) foundElems.at(0);
// //       nodesCoords.insert(coords);
//       theRequiredNodes.push_back(existingNode);
//     }
//     
//     
//     
//     if (pntCls->FindElementsByPoint(myPoint, SMDSAbs_Node, foundElems) == 0)
//       continue;

//     if ( result != TopAbs_IN )
//       continue;
    
#ifdef _MY_DEBUG_
    std::cout << " not found" << std::endl;
#endif
    nodesCoords.insert(coords);
//     theOrderedNodes.push_back(node);
    theRequiredNodes.push_back(node);
  }
  int requiredNodes = theRequiredNodes.size();
  
  int solSize = 0;
  std::vector<std::vector<double> > ReqVerTab;
  if (nbEnforcedVertices) {
//    ReqVerTab.clear();
    (nbEnforcedVertices <= 1) ? tmpStr = " node" : " nodes";
    std::cout << nbEnforcedVertices << tmpStr << " from enforced vertices ..." << std::endl;
    // Iterate over the enforced vertices
    for(vertexIt = theEnforcedVertices.begin() ; vertexIt != theEnforcedVertices.end() ; ++vertexIt) {
      double x = vertexIt->first[0];
      double y = vertexIt->first[1];
      double z = vertexIt->first[2];
      // Test if point is inside shape to mesh
      gp_Pnt myPoint(x,y,z);
      TopAbs_State result = pntCls->GetPointState( myPoint );
      if ( result == TopAbs_OUT )
      {
        std::cout << "Warning: enforced vertex at ( " << x << "," << y << "," << z << " ) is out of the meshed domain!!!" << std::endl;
        theInvalidEnforcedFlags |= FLAG_BAD_ENF_VERT;
        //continue;
      }
      std::vector<double> coords;
      coords.push_back(x);
      coords.push_back(y);
      coords.push_back(z);
      ReqVerTab.push_back(coords);
      enfVertexSizes.push_back(vertexIt->second);
      solSize++;
    }
  }
  
  
  // GmfVertices
  std::cout << "Begin writting required nodes in GmfVertices" << std::endl;
  std::cout << "Nb vertices: " << theOrderedNodes.size() << std::endl;
  MGInput->GmfSetKwd( idx, GmfVertices, theOrderedNodes.size()/*+solSize*/);
  for (ghs3dNodeIt = theOrderedNodes.begin();ghs3dNodeIt != theOrderedNodes.end();++ghs3dNodeIt) {
    MGInput->GmfSetLin( idx, GmfVertices, (*ghs3dNodeIt)->X(), (*ghs3dNodeIt)->Y(), (*ghs3dNodeIt)->Z(), dummyint);
  }

  std::cout << "End writting required nodes in GmfVertices" << std::endl;

  if (requiredNodes + solSize) {
    std::cout << "Begin writting in req and sol file" << std::endl;
    aNodeGroupByGhs3dId.resize( requiredNodes + solSize );
    idxRequired = MGInput->GmfOpenMesh( theRequiredFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
    if (!idxRequired) {
      return false;
    }
    idxSol = MGInput->GmfOpenMesh( theSolFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
    if (!idxSol) {
      return false;
    }
    int TypTab[] = {GmfSca};
    double ValTab[] = {0.0};
    MGInput->GmfSetKwd( idxRequired, GmfVertices, requiredNodes + solSize);
    MGInput->GmfSetKwd( idxSol, GmfSolAtVertices, requiredNodes + solSize, 1, TypTab);
//     int usedEnforcedNodes = 0;
//     std::string gn = "";
    for (ghs3dNodeIt = theRequiredNodes.begin();ghs3dNodeIt != theRequiredNodes.end();++ghs3dNodeIt) {
      MGInput->GmfSetLin( idxRequired, GmfVertices, (*ghs3dNodeIt)->X(), (*ghs3dNodeIt)->Y(), (*ghs3dNodeIt)->Z(), dummyint);
      MGInput->GmfSetLin( idxSol, GmfSolAtVertices, ValTab);
      if (theEnforcedNodes.find((*ghs3dNodeIt)) != theEnforcedNodes.end())
        gn = theEnforcedNodes.find((*ghs3dNodeIt))->second;
      aNodeGroupByGhs3dId[usedEnforcedNodes] = gn;
      usedEnforcedNodes++;
    }

    for (int i=0;i<solSize;i++) {
      std::cout << ReqVerTab[i][0] <<" "<< ReqVerTab[i][1] << " "<< ReqVerTab[i][2] << std::endl;
#ifdef _MY_DEBUG_
      std::cout << "enfVertexSizes.at("<<i<<"): " << enfVertexSizes.at(i) << std::endl;
#endif
      double solTab[] = {enfVertexSizes.at(i)};
      MGInput->GmfSetLin( idxRequired, GmfVertices, ReqVerTab[i][0], ReqVerTab[i][1], ReqVerTab[i][2], dummyint);
      MGInput->GmfSetLin( idxSol, GmfSolAtVertices, solTab);
      aNodeGroupByGhs3dId[usedEnforcedNodes] = enfVerticesWithGroup.find(ReqVerTab[i])->second;
#ifdef _MY_DEBUG_
      std::cout << "aNodeGroupByGhs3dId["<<usedEnforcedNodes<<"] = \""<<aNodeGroupByGhs3dId[usedEnforcedNodes]<<"\""<<std::endl;
#endif
      usedEnforcedNodes++;
    }
    std::cout << "End writting in req and sol file" << std::endl;
  }

  int nedge[2], ntri[3];
    
  // GmfEdges
  int usedEnforcedEdges = 0;
  if (theKeptEnforcedEdges.size()) {
    anEdgeGroupByGhs3dId.resize( theKeptEnforcedEdges.size() );
//    idxRequired = MGInput->GmfOpenMesh( theRequiredFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
//    if (!idxRequired)
//      return false;
    MGInput->GmfSetKwd( idx, GmfEdges, theKeptEnforcedEdges.size());
//    MGInput->GmfSetKwd( idxRequired, GmfEdges, theKeptEnforcedEdges.size());
    for(elemSetIt = theKeptEnforcedEdges.begin() ; elemSetIt != theKeptEnforcedEdges.end() ; ++elemSetIt) {
      elem = (*elemSetIt);
      nodeIt = elem->nodesIterator();
      int index=0;
      while ( nodeIt->more() ) {
        // find MG-Tetra ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        map< const SMDS_MeshNode*,int >::iterator it = anEnforcedNodeToGhs3dIdMap.find(node);
        if (it == anEnforcedNodeToGhs3dIdMap.end()) {
          it = anExistingEnforcedNodeToGhs3dIdMap.find(node);
          if (it == anEnforcedNodeToGhs3dIdMap.end())
            throw "Node not found";
        }
        nedge[index] = it->second;
        index++;
      }
      MGInput->GmfSetLin( idx, GmfEdges, nedge[0], nedge[1], dummyint);
      anEdgeGroupByGhs3dId[usedEnforcedEdges] = theEnforcedEdges.find(elem)->second;
//      MGInput->GmfSetLin( idxRequired, GmfEdges, nedge[0], nedge[1], dummyint);
      usedEnforcedEdges++;
    }
  }


  if (usedEnforcedEdges) {
    MGInput->GmfSetKwd( idx, GmfRequiredEdges, usedEnforcedEdges);
    for (int enfID=1;enfID<=usedEnforcedEdges;enfID++) {
      MGInput->GmfSetLin( idx, GmfRequiredEdges, enfID);
    }
  }

  // GmfTriangles
  int usedEnforcedTriangles = 0;
  if (anElemSet.size()+theKeptEnforcedTriangles.size()) {
    aFaceGroupByGhs3dId.resize( anElemSet.size()+theKeptEnforcedTriangles.size() );
    MGInput->GmfSetKwd( idx, GmfTriangles, anElemSet.size()+theKeptEnforcedTriangles.size());
    int k=0;
    for(elemSetIt = anElemSet.begin() ; elemSetIt != anElemSet.end() ; ++elemSetIt,++k) {
      elem = (*elemSetIt);
      theFaceByGhs3dId.push_back( elem );
      nodeIt = elem->nodesIterator();
      int index=0;
      for ( int j = 0; j < 3; ++j ) {
        // find MG-Tetra ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        map< const SMDS_MeshNode*,int >::iterator it = aNodeToGhs3dIdMap.find(node);
        if (it == aNodeToGhs3dIdMap.end())
          throw "Node not found";
        ntri[index] = it->second;
        index++;
      }
      MGInput->GmfSetLin( idx, GmfTriangles, ntri[0], ntri[1], ntri[2], dummyint);
      aFaceGroupByGhs3dId[k] = "";
    }
    if ( !theHelper.GetMesh()->HasShapeToMesh() )
      SMESHUtils::FreeVector( theFaceByGhs3dId );
    if (theKeptEnforcedTriangles.size()) {
      for(elemSetIt = theKeptEnforcedTriangles.begin() ; elemSetIt != theKeptEnforcedTriangles.end() ; ++elemSetIt,++k) {
        elem = (*elemSetIt);
        nodeIt = elem->nodesIterator();
        int index=0;
        for ( int j = 0; j < 3; ++j ) {
          // find MG-Tetra ID
          const SMDS_MeshNode* node = castToNode( nodeIt->next() );
          map< const SMDS_MeshNode*,int >::iterator it = anEnforcedNodeToGhs3dIdMap.find(node);
          if (it == anEnforcedNodeToGhs3dIdMap.end()) {
            it = anExistingEnforcedNodeToGhs3dIdMap.find(node);
            if (it == anEnforcedNodeToGhs3dIdMap.end())
              throw "Node not found";
          }
          ntri[index] = it->second;
          index++;
        }
        MGInput->GmfSetLin( idx, GmfTriangles, ntri[0], ntri[1], ntri[2], dummyint);
        aFaceGroupByGhs3dId[k] = theEnforcedTriangles.find(elem)->second;
        usedEnforcedTriangles++;
      }
    }
  }

  
  if (usedEnforcedTriangles) {
    MGInput->GmfSetKwd( idx, GmfRequiredTriangles, usedEnforcedTriangles);
    for (int enfID=1;enfID<=usedEnforcedTriangles;enfID++)
      MGInput->GmfSetLin( idx, GmfRequiredTriangles, anElemSet.size()+enfID);
  }

  MGInput->GmfCloseMesh(idx);
  if (idxRequired)
    MGInput->GmfCloseMesh(idxRequired);
  if (idxSol)
    MGInput->GmfCloseMesh(idxSol);

  return true;
}

//=============================================================================
/*!
 *Here we are going to use the MG-Tetra mesher with geometry
 */
//=============================================================================

bool GHS3DPlugin_GHS3D::Compute(SMESH_Mesh&         theMesh,
                                const TopoDS_Shape& theShape)
{
  bool Ok(false);
  TopExp_Explorer expBox ( theShape, TopAbs_SOLID );

  // a unique working file name
  // to avoid access to the same files by eg different users
  _genericName = GHS3DPlugin_Hypothesis::GetFileName(_hyp);
  TCollection_AsciiString aGenericName((char*) _genericName.c_str() );
  TCollection_AsciiString aGenericNameRequired = aGenericName + "_required";

  TCollection_AsciiString aLogFileName    = aGenericName + ".log";    // log
  TCollection_AsciiString aResultFileName;

  TCollection_AsciiString aGMFFileName, aRequiredVerticesFileName, aSolFileName, aResSolFileName;
  aGMFFileName              = aGenericName + ".mesh"; // GMF mesh file
  aResultFileName           = aGenericName + "Vol.mesh"; // GMF mesh file
  aResSolFileName           = aGenericName + "Vol.sol"; // GMF mesh file
  aRequiredVerticesFileName = aGenericNameRequired + ".mesh"; // GMF required vertices mesh file
  aSolFileName              = aGenericNameRequired + ".sol"; // GMF solution file
  
  std::map <int,int> aNodeId2NodeIndexMap, aSmdsToGhs3dIdMap, anEnforcedNodeIdToGhs3dIdMap;
  std::map <int, int> nodeID2nodeIndexMap;
  std::map<std::vector<double>, std::string> enfVerticesWithGroup;
  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues coordsSizeMap = GHS3DPlugin_Hypothesis::GetEnforcedVerticesCoordsSize(_hyp);
  GHS3DPlugin_Hypothesis::TIDSortedNodeGroupMap enforcedNodes = GHS3DPlugin_Hypothesis::GetEnforcedNodes(_hyp);
  GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap enforcedEdges = GHS3DPlugin_Hypothesis::GetEnforcedEdges(_hyp);
  GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap enforcedTriangles = GHS3DPlugin_Hypothesis::GetEnforcedTriangles(_hyp);
  GHS3DPlugin_Hypothesis::TID2SizeMap nodeIDToSizeMap = GHS3DPlugin_Hypothesis::GetNodeIDToSizeMap(_hyp);

  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexList enfVertices = GHS3DPlugin_Hypothesis::GetEnforcedVertices(_hyp);
  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexList::const_iterator enfVerIt = enfVertices.begin();
  std::vector<double> coords;

  for ( ; enfVerIt != enfVertices.end() ; ++enfVerIt)
  {
    GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertex* enfVertex = (*enfVerIt);
    if (enfVertex->coords.size()) {
      coordsSizeMap.insert(make_pair(enfVertex->coords,enfVertex->size));
      enfVerticesWithGroup.insert(make_pair(enfVertex->coords,enfVertex->groupName));
    }
    else {
      TopoDS_Shape GeomShape = entryToShape(enfVertex->geomEntry);
      for (TopoDS_Iterator it (GeomShape); it.More(); it.Next()){
        coords.clear();
        if (it.Value().ShapeType() == TopAbs_VERTEX){
          gp_Pnt aPnt = BRep_Tool::Pnt(TopoDS::Vertex(it.Value()));
          coords.push_back(aPnt.X());
          coords.push_back(aPnt.Y());
          coords.push_back(aPnt.Z());
          if (coordsSizeMap.find(coords) == coordsSizeMap.end()) {
            coordsSizeMap.insert(make_pair(coords,enfVertex->size));
            enfVerticesWithGroup.insert(make_pair(coords,enfVertex->groupName));
          }
        }
      }
    }
  }
  int nbEnforcedVertices = coordsSizeMap.size();
  int nbEnforcedNodes = enforcedNodes.size();

  std::string tmpStr;
  (nbEnforcedNodes <= 1) ? tmpStr = "node" : "nodes";
  std::cout << nbEnforcedNodes << " enforced " << tmpStr << " from hypo" << std::endl;
  (nbEnforcedVertices <= 1) ? tmpStr = "vertex" : "vertices";
  std::cout << nbEnforcedVertices << " enforced " << tmpStr << " from hypo" << std::endl;

  SMESH_MesherHelper helper( theMesh );
  helper.SetSubShape( theShape );

  std::vector <const SMDS_MeshNode*> aNodeByGhs3dId, anEnforcedNodeByGhs3dId;
  std::vector <const SMDS_MeshElement*> aFaceByGhs3dId;
  std::map<const SMDS_MeshNode*,int> aNodeToGhs3dIdMap;
  std::vector<std::string> aNodeGroupByGhs3dId, anEdgeGroupByGhs3dId, aFaceGroupByGhs3dId;

  MG_Tetra_API mgTetra( _computeCanceled, _progress );

  _isLibUsed = mgTetra.IsLibrary();
  if ( theMesh.NbQuadrangles() > 0 )
    _progressAdvance /= 10;
  if ( _viscousLayersHyp )
    _progressAdvance /= 10;

  // proxyMesh must live till readGMFFile() as a proxy face can be used by
  // MG-Tetra for domain indication
  SMESH_ProxyMesh::Ptr proxyMesh( new SMESH_ProxyMesh( theMesh ));

  // make prisms on quadrangles
  if ( theMesh.NbQuadrangles() > 0 )
  {
    vector<SMESH_ProxyMesh::Ptr> components;
    for (expBox.ReInit(); expBox.More(); expBox.Next())
    {
      if ( _viscousLayersHyp )
      {
        proxyMesh = _viscousLayersHyp->Compute( theMesh, expBox.Current() );
        if ( !proxyMesh )
          return false;
      }
      StdMeshers_QuadToTriaAdaptor* q2t = new StdMeshers_QuadToTriaAdaptor;
      Ok = q2t->Compute( theMesh, expBox.Current(), proxyMesh.get() );
      components.push_back( SMESH_ProxyMesh::Ptr( q2t ));
      if ( !Ok )
        return false;
    }
    proxyMesh.reset( new SMESH_ProxyMesh( components ));
  }
  // build viscous layers
  else if ( _viscousLayersHyp )
  {
    proxyMesh = _viscousLayersHyp->Compute( theMesh, theShape );
    if ( !proxyMesh )
      return false;
  }

  int anInvalidEnforcedFlags = 0;
  Ok = writeGMFFile(&mgTetra,
                    aGMFFileName.ToCString(),
                    aRequiredVerticesFileName.ToCString(),
                    aSolFileName.ToCString(),
                    *proxyMesh, helper,
                    aNodeByGhs3dId, aFaceByGhs3dId, aNodeToGhs3dIdMap,
                    aNodeGroupByGhs3dId, anEdgeGroupByGhs3dId, aFaceGroupByGhs3dId,
                    enforcedNodes, enforcedEdges, enforcedTriangles,
                    enfVerticesWithGroup, coordsSizeMap, anInvalidEnforcedFlags);

  // Write aSmdsToGhs3dIdMap to temp file
  TCollection_AsciiString aSmdsToGhs3dIdMapFileName;
  aSmdsToGhs3dIdMapFileName = aGenericName + ".ids";  // ids relation
  ofstream aIdsFile  ( aSmdsToGhs3dIdMapFileName.ToCString()  , ios::out);
  Ok = aIdsFile.rdbuf()->is_open();
  if (!Ok) {
    INFOS( "Can't write into " << aSmdsToGhs3dIdMapFileName);
    return error(SMESH_Comment("Can't write into ") << aSmdsToGhs3dIdMapFileName);
  }
  INFOS( "Writing ids relation into " << aSmdsToGhs3dIdMapFileName);
  aIdsFile << "Smds MG-Tetra" << std::endl;
  map <int,int>::const_iterator myit;
  for (myit=aSmdsToGhs3dIdMap.begin() ; myit != aSmdsToGhs3dIdMap.end() ; ++myit) {
    aIdsFile << myit->first << " " << myit->second << std::endl;
  }

  aIdsFile.close();

  if ( ! Ok ) {
    if ( !_keepFiles ) {
      removeFile( aGMFFileName );
      removeFile( aRequiredVerticesFileName );
      removeFile( aSolFileName );
      removeFile( aSmdsToGhs3dIdMapFileName );
    }
    return error(COMPERR_BAD_INPUT_MESH);
  }
  removeFile( aResultFileName ); // needed for boundary recovery module usage

  // -----------------
  // run MG-Tetra mesher
  // -----------------

  TCollection_AsciiString cmd = GHS3DPlugin_Hypothesis::CommandToRun( _hyp, true, mgTetra.IsExecutable() ).c_str();

  if ( mgTetra.IsExecutable() )
  {
    cmd += TCollection_AsciiString(" --in ") + aGMFFileName;
    if ( nbEnforcedVertices + nbEnforcedNodes)
      cmd += TCollection_AsciiString(" --required_vertices ") + aGenericNameRequired;
    cmd += TCollection_AsciiString(" --out ") + aResultFileName;
  }
  if ( !_logInStandardOutput )
  {
    mgTetra.SetLogFile( aLogFileName.ToCString() );
    cmd += TCollection_AsciiString(" 1>" ) + aLogFileName;  // dump into file
  }
  std::cout << std::endl;
  std::cout << "MG-Tetra execution..." << std::endl;
  std::cout << cmd << std::endl;

  _computeCanceled = false;

  std::string errStr;
  Ok = mgTetra.Compute( cmd.ToCString(), errStr ); // run

  if ( _logInStandardOutput && mgTetra.IsLibrary() )
    std::cout << std::endl << mgTetra.GetLog() << std::endl;
  if ( Ok )
    std::cout << std::endl << "End of MG-Tetra execution !" << std::endl;

  // --------------
  // read a result
  // --------------

  GHS3DPlugin_Hypothesis::TSetStrings groupsToRemove = GHS3DPlugin_Hypothesis::GetGroupsToRemove(_hyp);
  bool toMeshHoles =
    _hyp ? _hyp->GetToMeshHoles(true) : GHS3DPlugin_Hypothesis::DefaultMeshHoles();
  const bool toMakeGroupsOfDomains = GHS3DPlugin_Hypothesis::GetToMakeGroupsOfDomains( _hyp );

  helper.IsQuadraticSubMesh( theShape );
  helper.SetElementsOnShape( false );

  Ok = readGMFFile(&mgTetra,
                   aResultFileName.ToCString(),
                   this,
                   &helper, aNodeByGhs3dId, aFaceByGhs3dId, aNodeToGhs3dIdMap,
                   aNodeGroupByGhs3dId, anEdgeGroupByGhs3dId, aFaceGroupByGhs3dId,
                   groupsToRemove, toMakeGroupsOfDomains, toMeshHoles);

  removeEmptyGroupsOfDomains( helper.GetMesh(), /*notEmptyAsWell =*/ !toMakeGroupsOfDomains );



  // ---------------------
  // remove working files
  // ---------------------

  if ( Ok )
  {
    if ( anInvalidEnforcedFlags )
      error( COMPERR_WARNING, flagsToErrorStr( anInvalidEnforcedFlags ));
    if ( _removeLogOnSuccess )
      removeFile( aLogFileName );
    // if ( _hyp && _hyp->GetToMakeGroupsOfDomains() )
    //   error( COMPERR_WARNING, "'toMakeGroupsOfDomains' is ignored since the mesh is on shape" );
  }
  else if ( mgTetra.HasLog() )
  {
    if( _computeCanceled )
      error( "interruption initiated by user" );
    else
    {
      // get problem description from the log file
      _Ghs2smdsConvertor conv( aNodeByGhs3dId, proxyMesh );
      error( getErrorDescription( _logInStandardOutput ? 0 : aLogFileName.ToCString(),
                                  mgTetra.GetLog(), conv ));
    }
  }
  else if ( !errStr.empty() )
  {
    // the log file is empty
    removeFile( aLogFileName );
    INFOS( "MG-Tetra Error, " << errStr);
    error(COMPERR_ALGO_FAILED, errStr);
  }

  if ( !_keepFiles ) {
    if (! Ok && _computeCanceled )
      removeFile( aLogFileName );
    removeFile( aGMFFileName );
    removeFile( aRequiredVerticesFileName );
    removeFile( aSolFileName );
    removeFile( aResSolFileName );
    removeFile( aResultFileName );
    removeFile( aSmdsToGhs3dIdMapFileName );
  }
  if ( mgTetra.IsExecutable() )
  {
    std::cout << "<" << aResultFileName.ToCString() << "> MG-Tetra output file ";
    if ( !Ok )
      std::cout << "not ";
    std::cout << "treated !" << std::endl;
    std::cout << std::endl;
  }
  else
  {
    std::cout << "MG-Tetra " << ( Ok ? "succeeded" : "failed") << std::endl;
  }
  return Ok;
}

//=============================================================================
/*!
 *Here we are going to use the MG-Tetra mesher w/o geometry
 */
//=============================================================================
bool GHS3DPlugin_GHS3D::Compute(SMESH_Mesh&         theMesh,
                                SMESH_MesherHelper* theHelper)
{
  theHelper->IsQuadraticSubMesh( theHelper->GetSubShape() );

  // a unique working file name
  // to avoid access to the same files by eg different users
  _genericName = GHS3DPlugin_Hypothesis::GetFileName(_hyp);
  TCollection_AsciiString aGenericName((char*) _genericName.c_str() );
  TCollection_AsciiString aGenericNameRequired = aGenericName + "_required";

  TCollection_AsciiString aLogFileName    = aGenericName + ".log";    // log
  TCollection_AsciiString aResultFileName;
  bool Ok;

  TCollection_AsciiString aGMFFileName, aRequiredVerticesFileName, aSolFileName, aResSolFileName;
  aGMFFileName              = aGenericName + ".mesh"; // GMF mesh file
  aResultFileName           = aGenericName + "Vol.mesh"; // GMF mesh file
  aResSolFileName           = aGenericName + "Vol.sol"; // GMF mesh file
  aRequiredVerticesFileName = aGenericNameRequired + ".mesh"; // GMF required vertices mesh file
  aSolFileName              = aGenericNameRequired + ".sol"; // GMF solution file

  std::map <int, int> nodeID2nodeIndexMap;
  std::map<std::vector<double>, std::string> enfVerticesWithGroup;
  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues coordsSizeMap;
  TopoDS_Shape GeomShape;
  std::vector<double> coords;
  gp_Pnt aPnt;
  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertex* enfVertex;

  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexList enfVertices = GHS3DPlugin_Hypothesis::GetEnforcedVertices(_hyp);
  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexList::const_iterator enfVerIt = enfVertices.begin();

  for ( ; enfVerIt != enfVertices.end() ; ++enfVerIt)
  {
    enfVertex = (*enfVerIt);
    if (enfVertex->coords.size()) {
      coordsSizeMap.insert(make_pair(enfVertex->coords,enfVertex->size));
      enfVerticesWithGroup.insert(make_pair(enfVertex->coords,enfVertex->groupName));
    }
    else {
      GeomShape = entryToShape(enfVertex->geomEntry);
      for (TopoDS_Iterator it (GeomShape); it.More(); it.Next()){
        coords.clear();
        if (it.Value().ShapeType() == TopAbs_VERTEX){
          aPnt = BRep_Tool::Pnt(TopoDS::Vertex(it.Value()));
          coords.push_back(aPnt.X());
          coords.push_back(aPnt.Y());
          coords.push_back(aPnt.Z());
          if (coordsSizeMap.find(coords) == coordsSizeMap.end()) {
            coordsSizeMap.insert(make_pair(coords,enfVertex->size));
            enfVerticesWithGroup.insert(make_pair(coords,enfVertex->groupName));
          }
        }
      }
    }
  }

  GHS3DPlugin_Hypothesis::TIDSortedNodeGroupMap     enforcedNodes = GHS3DPlugin_Hypothesis::GetEnforcedNodes(_hyp);
  GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap     enforcedEdges = GHS3DPlugin_Hypothesis::GetEnforcedEdges(_hyp);
  GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap enforcedTriangles = GHS3DPlugin_Hypothesis::GetEnforcedTriangles(_hyp);
  GHS3DPlugin_Hypothesis::TID2SizeMap             nodeIDToSizeMap = GHS3DPlugin_Hypothesis::GetNodeIDToSizeMap(_hyp);

  std::string tmpStr;

  int nbEnforcedVertices = coordsSizeMap.size();
  int nbEnforcedNodes = enforcedNodes.size();
  (nbEnforcedNodes <= 1) ? tmpStr = "node" : tmpStr = "nodes";
  std::cout << nbEnforcedNodes << " enforced " << tmpStr << " from hypo" << std::endl;
  (nbEnforcedVertices <= 1) ? tmpStr = "vertex" : tmpStr = "vertices";
  std::cout << nbEnforcedVertices << " enforced " << tmpStr << " from hypo" << std::endl;

  std::vector <const SMDS_MeshNode*> aNodeByGhs3dId, anEnforcedNodeByGhs3dId;
  std::vector <const SMDS_MeshElement*> aFaceByGhs3dId;
  std::map<const SMDS_MeshNode*,int> aNodeToGhs3dIdMap;
  std::vector<std::string> aNodeGroupByGhs3dId, anEdgeGroupByGhs3dId, aFaceGroupByGhs3dId;


  MG_Tetra_API mgTetra( _computeCanceled, _progress );

  _isLibUsed = mgTetra.IsLibrary();
  if ( theMesh.NbQuadrangles() > 0 )
    _progressAdvance /= 10;

  // proxyMesh must live till readGMFFile() as a proxy face can be used by
  // MG-Tetra for domain indication
  SMESH_ProxyMesh::Ptr proxyMesh( new SMESH_ProxyMesh( theMesh ));
  if ( theMesh.NbQuadrangles() > 0 )
  {
    StdMeshers_QuadToTriaAdaptor* aQuad2Trias = new StdMeshers_QuadToTriaAdaptor;
    Ok = aQuad2Trias->Compute( theMesh );
    proxyMesh.reset( aQuad2Trias );
    if ( !Ok )
      return false;
  }

  int anInvalidEnforcedFlags = 0;
  Ok = writeGMFFile(&mgTetra,
                    aGMFFileName.ToCString(), aRequiredVerticesFileName.ToCString(), aSolFileName.ToCString(),
                    *proxyMesh, *theHelper,
                    aNodeByGhs3dId, aFaceByGhs3dId, aNodeToGhs3dIdMap,
                    aNodeGroupByGhs3dId, anEdgeGroupByGhs3dId, aFaceGroupByGhs3dId,
                    enforcedNodes, enforcedEdges, enforcedTriangles,
                    enfVerticesWithGroup, coordsSizeMap, anInvalidEnforcedFlags);

  // -----------------
  // run MG-Tetra mesher
  // -----------------

  TCollection_AsciiString cmd = GHS3DPlugin_Hypothesis::CommandToRun( _hyp, false, mgTetra.IsExecutable() ).c_str();

  if ( mgTetra.IsExecutable() )
  {
    cmd += TCollection_AsciiString(" --in ") + aGMFFileName;
    if ( nbEnforcedVertices + nbEnforcedNodes)
      cmd += TCollection_AsciiString(" --required_vertices ") + aGenericNameRequired;
    cmd += TCollection_AsciiString(" --out ") + aResultFileName;
  }
  if ( !_logInStandardOutput )
  {
    mgTetra.SetLogFile( aLogFileName.ToCString() );
    cmd += TCollection_AsciiString(" 1>" ) + aLogFileName;  // dump into file
  }
  std::cout << std::endl;
  std::cout << "MG-Tetra execution..." << std::endl;
  std::cout << cmd << std::endl;

  _computeCanceled = false;

  std::string errStr;
  Ok = mgTetra.Compute( cmd.ToCString(), errStr ); // run

  if ( _logInStandardOutput && mgTetra.IsLibrary() )
    std::cout << std::endl << mgTetra.GetLog() << std::endl;
  if ( Ok )
    std::cout << std::endl << "End of MG-Tetra execution !" << std::endl;

  // --------------
  // read a result
  // --------------
  GHS3DPlugin_Hypothesis::TSetStrings groupsToRemove = GHS3DPlugin_Hypothesis::GetGroupsToRemove(_hyp);
  const bool toMakeGroupsOfDomains = GHS3DPlugin_Hypothesis::GetToMakeGroupsOfDomains( _hyp );

  Ok = Ok && readGMFFile(&mgTetra,
                         aResultFileName.ToCString(),
                         this,
                         theHelper, aNodeByGhs3dId, aFaceByGhs3dId, aNodeToGhs3dIdMap,
                         aNodeGroupByGhs3dId, anEdgeGroupByGhs3dId, aFaceGroupByGhs3dId,
                         groupsToRemove, toMakeGroupsOfDomains);

  updateMeshGroups(theHelper->GetMesh(), groupsToRemove);
  removeEmptyGroupsOfDomains( theHelper->GetMesh(), /*notEmptyAsWell =*/ !toMakeGroupsOfDomains );

  if ( Ok ) {
    GHS3DPlugin_Hypothesis* that = (GHS3DPlugin_Hypothesis*)this->_hyp;
    if (that)
      that->ClearGroupsToRemove();
  }
  // ---------------------
  // remove working files
  // ---------------------

  if ( Ok )
  {
    if ( anInvalidEnforcedFlags )
      error( COMPERR_WARNING, flagsToErrorStr( anInvalidEnforcedFlags ));
    if ( _removeLogOnSuccess )
      removeFile( aLogFileName );

    //if ( !toMakeGroupsOfDomains && _hyp && _hyp->GetToMakeGroupsOfDomains() )
    //error( COMPERR_WARNING, "'toMakeGroupsOfDomains' is ignored since 'toMeshHoles' is OFF." );
  }
  else if ( mgTetra.HasLog() )
  {
    if( _computeCanceled )
      error( "interruption initiated by user" );
    else
    {
      // get problem description from the log file
      _Ghs2smdsConvertor conv( aNodeByGhs3dId, proxyMesh );
      error( getErrorDescription( _logInStandardOutput ? 0 : aLogFileName.ToCString(),
                                  mgTetra.GetLog(), conv ));
    }
  }
  else {
    // the log file is empty
    removeFile( aLogFileName );
    INFOS( "MG-Tetra Error, " << errStr);
    error(COMPERR_ALGO_FAILED, errStr);
  }

  if ( !_keepFiles )
  {
    if (! Ok && _computeCanceled)
      removeFile( aLogFileName );
    removeFile( aGMFFileName );
    removeFile( aResultFileName );
    removeFile( aRequiredVerticesFileName );
    removeFile( aSolFileName );
    removeFile( aResSolFileName );
  }
  return Ok;
}

void GHS3DPlugin_GHS3D::CancelCompute()
{
  _computeCanceled = true;
#ifdef WIN32
#else
  std::string cmd = "ps xo pid,args | grep " + _genericName;
  //cmd += " | grep -e \"^ *[0-9]\\+ \\+" + GHS3DPlugin_Hypothesis::GetExeName() + "\"";
  cmd += " | awk '{print $1}' | xargs kill -9 > /dev/null 2>&1";
  system( cmd.c_str() );
#endif
}

//================================================================================
/*!
 * \brief Provide human readable text by error code reported by MG-Tetra
 */
//================================================================================

static const char* translateError(const int errNum)
{
  switch ( errNum ) {
  case 0:
    return "The surface mesh includes a face of type other than edge, "
      "triangle or quadrilateral. This face type is not supported.";
  case 1:
    return "Not enough memory for the face table.";
  case 2:
    return "Not enough memory.";
  case 3:
    return "Not enough memory.";
  case 4:
    return "Face is ignored.";
  case 5:
    return "End of file. Some data are missing in the file.";
  case 6:
    return "Read error on the file. There are wrong data in the file.";
  case 7:
    return "the metric file is inadequate (dimension other than 3).";
  case 8:
    return "the metric file is inadequate (values not per vertices).";
  case 9:
    return "the metric file contains more than one field.";
  case 10:
    return "the number of values in the \".bb\" (metric file) is incompatible with the expected"
      "value of number of mesh vertices in the \".noboite\" file.";
  case 12:
    return "Too many sub-domains.";
  case 13:
    return "the number of vertices is negative or null.";
  case 14:
    return "the number of faces is negative or null.";
  case 15:
    return "A face has a null vertex.";
  case 22:
    return "incompatible data.";
  case 131:
    return "the number of vertices is negative or null.";
  case 132:
    return "the number of vertices is negative or null (in the \".mesh\" file).";
  case 133:
    return "the number of faces is negative or null.";
  case 1000:
    return "A face appears more than once in the input surface mesh.";
  case 1001:
    return "An edge appears more than once in the input surface mesh.";
  case 1002:
    return "A face has a vertex negative or null.";
  case 1003:
    return "NOT ENOUGH MEMORY.";
  case 2000:
    return "Not enough available memory.";
  case 2002:
    return "Some initial points cannot be inserted. The surface mesh is probably very bad "
      "in terms of quality or the input list of points is wrong.";
  case 2003:
    return "Some vertices are too close to one another or coincident.";
  case 2004:
    return "Some vertices are too close to one another or coincident.";
  case 2012:
    return "A vertex cannot be inserted.";
  case 2014:
    return "There are at least two points considered as coincident.";
  case 2103:
    return "Some vertices are too close to one another or coincident.";
  case 3000:
    return "The surface mesh regeneration step has failed.";
  case 3009:
    return "Constrained edge cannot be enforced.";
  case 3019:
    return "Constrained face cannot be enforced.";
  case 3029:
    return "Missing faces.";
  case 3100:
    return "No guess to start the definition of the connected component(s).";
  case 3101:
    return "The surface mesh includes at least one hole. The domain is not well defined.";
  case 3102:
    return "Impossible to define a component.";
  case 3103:
    return "The surface edge intersects another surface edge.";
  case 3104:
    return "The surface edge intersects the surface face.";
  case 3105:
    return "One boundary point lies within a surface face.";
  case 3106:
    return "One surface edge intersects a surface face.";
  case 3107:
    return "One boundary point lies within a surface edge.";
  case 3108:
    return "Insufficient memory ressources detected due to a bad quality surface mesh leading "
      "to too many swaps.";
  case 3109:
    return "Edge is unique (i.e., bounds a hole in the surface).";
  case 3122:
    return "Presumably, the surface mesh is not compatible with the domain being processed.";
  case 3123:
    return "Too many components, too many sub-domain.";
  case 3209:
    return "The surface mesh includes at least one hole. "
      "Therefore there is no domain properly defined.";
  case 3300:
    return "Statistics.";
  case 3400:
    return "Statistics.";
  case 3500:
    return "Warning, it is dramatically tedious to enforce the boundary items.";
  case 4000:
    return "Not enough memory at this time, nevertheless, the program continues. "
      "The expected mesh will be correct but not really as large as required.";
  case 4002:
    return "see above error code, resulting quality may be poor.";
  case 4003:
    return "Not enough memory at this time, nevertheless, the program continues (warning).";
  case 8000:
    return "Unknown face type.";
  case 8005:
  case 8006:
    return "End of file. Some data are missing in the file.";
  case 9000:
    return "A too small volume element is detected.";
  case 9001:
    return "There exists at least a null or negative volume element.";
  case 9002:
    return "There exist null or negative volume elements.";
  case 9003:
    return "A too small volume element is detected. A face is considered being degenerated.";
  case 9100:
    return "Some element is suspected to be very bad shaped or wrong.";
  case 9102:
    return "A too bad quality face is detected. This face is considered degenerated.";
  case 9112:
    return "A too bad quality face is detected. This face is degenerated.";
  case 9122:
    return "Presumably, the surface mesh is not compatible with the domain being processed.";
  case 9999:
    return "Abnormal error occured, contact hotline.";
  case 23600:
    return "Not enough memory for the face table.";
  case 23601:
    return "The algorithm cannot run further. "
      "The surface mesh is probably very bad in terms of quality.";
  case 23602:
    return "Bad vertex number.";
  case 1001200:
    return "Cannot close mesh file NomFil.";
  case 1002010:
    return "There are wrong data.";
  case 1002120:
    return "The number of faces is negative or null.";
  case 1002170:
    return "The number of vertices is negative or null in the '.sol' file.";
  case 1002190:
    return "The number of tetrahedra is negative or null.";
  case 1002210:
    return "The number of vertices is negative or null.";
  case 1002211:
    return "A face has a vertex negative or null.";
  case 1002270:
    return "The field is not a size in file NomFil.";
  case 1002280:
    return "A count is wrong in the enclosing box in the .boite.mesh input "
      "file (option '--read_boite').";
  case 1002290:
    return "A tetrahedron has a vertex with a negative number.";
  case 1002300:
    return "the 'MeshVersionFormatted' is not 1 or 2 in the '.mesh' file or the '.sol'.";
 case 1002370:
   return "The number of values in the '.sol' (metric file) is incompatible with "
     "the expected value of number of mesh vertices in the '.mesh' file.";
  case 1003000:
    return "Not enough memory.";
  case 1003020:
    return "Not enough memory for the face table.";
  case 1003050:
    return "Insufficient memory ressources detected due to a bad quality "
      "surface mesh leading to too many swaps.";
  case 1005010:
    return "The surface coordinates of a vertex are differing from the "
      "volume coordinates, probably due to a precision problem.";
  case 1005050:
    return "Invalid dimension. Dimension 3 expected.";
  case 1005100:
    return "A point has a tag 0. This point is probably outside the domain which has been meshed.";
  case 1005103:
    return "The vertices of an element are too close to one another or coincident.";
  case 1005104:
    return "There are at least two points whose distance is very small, and considered as coincident.";
  case 1005105:
    return "Two vertices are too close to one another or coincident.";
  case 1005106:
    return "A vertex cannot be inserted.";
  case 1005107:
    return "Two vertices are too close to one another or coincident. Note : When "
      "this error occurs during the overconstrained processing phase, this is only "
      "a warning which means that it is difficult to break some overconstrained facets.";
  case 1005110:
    return "Two surface edges are intersecting.";
  case 1005120:
    return "A surface edge intersects a surface face.";
  case 1005150:
    return "A boundary point lies within a surface face.";
  case 1005160:
    return "A boundary point lies within a surface edge.";
  case 1005200:
    return "A surface mesh appears more than once in the input surface mesh.";
  case 1005210:
    return "An edge appears more than once in the input surface mesh.";
  case 1005225:
    return "Surface with unvalid triangles.";
  case 1005270:
    return "The metric in the '.sol' file contains more than one field.";
  case 1005300:
    return "The surface mesh includes at least one hole. The domain is not well defined.";
  case 1005301:
    return "Presumably, the surface mesh is not compatible with the domain being processed (warning).";
  case 1005302:
    return "Probable faces overlapping somewher.";
  case 1005320:
    return "The quadratic version does not work with prescribed free edges.";
  case 1005321:
    return "The quadratic version does not work with a volume mesh.";
  case 1005370:
    return "The metric in the '.sol' file is inadequate (values not per vertices).";
  case 1005371:
    return "The number of vertices in the '.sol' is different from the one in the "
      "'.mesh' file for the required vertices (option '--required_vertices').";
  case 1005372:
    return "More than one type in file NomFil. The type must be equal to 1 in the '.sol'"
      "for the required vertices (option '--required_vertices').";
  case 1005515:
    return "Bad vertex number.";
  case 1005560:
    return "No guess to start the definition of the connected component(s).";
  case 1005602:
    return "Some initial points cannot be inserted.";
  case 1005620:
    return "A too bad quality face is detected. This face is considered degenerated.";
  case 1005621:
    return "A too bad quality face is detected. This face is degenerated.";
  case 1005622:
    return "The algorithm cannot run further.";
  case 1005690:
    return "A too small volume element is detected.";
  case 1005691:
    return "A tetrahedra is suspected to be very bad shaped or wrong.";
  case 1005692:
    return "There is at least a null or negative volume element. The resulting mesh"
      "may be inappropriate.";
  case 1005693:
    return "There are some null or negative volume element. The resulting mesh may"
      "be inappropriate.";
  case 1005820:
    return "An edge is unique (i.e., bounds a hole in the surface).";
  case 1007000:
    return "Abnormal or internal error.";
  case 1007010:
    return "Too many components with respect to too many sub-domain.";
  case 1007400:
    return "An internal error has been encountered or a signal has been received. "
      "Current mesh will not be saved.";
  case 1008491:
    return "Impossible to define a component.";
  case 1008410:
    return "There are some overconstrained edges.";
  case 1008420:
    return "There are some overconstrained facets.";
  case 1008422:
    return "Give the number of missing faces (information given when regeneration phase failed).";
  case 1008423:
    return "A constrained face cannot be enforced (information given when regeneration phase failed).";
  case 1008441:
    return "A constrained edge cannot be enforced.";
  case 1008460:
    return "It is dramatically tedious to enforce the boundary items.";
  case 1008480:
    return "The surface mesh regeneration step has failed. A .boite.mesh and .boite.map files are created.";
  case 1008490:
    return "Invalid resulting mesh.";
  case 1008495:
    return "P2 correction not successful.";
  case 1009000:
    return "Program has received an interruption or a termination signal sent by the "
      "user or the system administrator. Current mesh will not be saved.";
  }
  return "";
}

//================================================================================
/*!
 * \brief Retrieve from a string given number of integers
 */
//================================================================================

static char* getIds( char* ptr, int nbIds, vector<int>& ids )
{
  ids.clear();
  ids.reserve( nbIds );
  while ( nbIds )
  {
    while ( !isdigit( *ptr )) ++ptr;
    if ( ptr[-1] == '-' ) --ptr;
    ids.push_back( strtol( ptr, &ptr, 10 ));
    --nbIds;
  }
  return ptr;
}

//================================================================================
/*!
 * \brief Retrieve problem description form a log file
 *  \retval bool - always false
 */
//================================================================================

SMESH_ComputeErrorPtr
GHS3DPlugin_GHS3D::getErrorDescription(const char*                logFile,
                                       const std::string&         log,
                                       const _Ghs2smdsConvertor & toSmdsConvertor,
                                       const bool                 isOk/* = false*/ )
{
  SMESH_ComputeErrorPtr err = SMESH_ComputeError::New( COMPERR_ALGO_FAILED );

  char* ptr = const_cast<char*>( log.c_str() );
  char* buf = ptr, * bufEnd = ptr + log.size();


  SMESH_Comment errDescription;

  enum { NODE = 1, EDGE, TRIA, VOL, SKIP_ID = 1 };

  // look for MeshGems version
  // Since "MG-TETRA -- MeshGems 1.1-3 (January, 2013)" error codes change.
  // To discriminate old codes from new ones we add 1000000 to the new codes.
  // This way value of the new codes is same as absolute value of codes printed
  // in the log after "MGMESSAGE" string.
  int versionAddition = 0;
  {
    char* verPtr = ptr;
    while ( ++verPtr < bufEnd )
    {
      if ( strncmp( verPtr, "MG-TETRA -- MeshGems ", 21 ) != 0 )
        continue;
      if ( strcmp( verPtr, "MG-TETRA -- MeshGems 1.1-3 " ) >= 0 )
        versionAddition = 1000000;
      ptr = verPtr;
      break;
    }
  }

  // look for errors "ERR #"

  set<string> foundErrorStr; // to avoid reporting same error several times
  set<int>    elemErrorNums; // not to report different types of errors with bad elements
  while ( ++ptr < bufEnd )
  {
    if ( strncmp( ptr, "ERR ", 4 ) != 0 )
      continue;

    list<const SMDS_MeshElement*>& badElems = err->myBadElements;
    vector<int> nodeIds;

    ptr += 4;
    char* errBeg = ptr;
    int   errNum = strtol(ptr, &ptr, 10) + versionAddition;
    // we treat errors enumerated in [SALOME platform 0019316] issue
    // and all errors from a new (Release 1.1) MeshGems User Manual
    switch ( errNum ) {
    case 0015: // The face number (numfac) with vertices (f 1, f 2, f 3) has a null vertex.
    case 1005620 : // a too bad quality face is detected. This face is considered degenerated.
      ptr = getIds(ptr, SKIP_ID, nodeIds);
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 1005621 : // a too bad quality face is detected. This face is degenerated.
      // hence the is degenerated it is invisible, add its edges in addition
      ptr = getIds(ptr, SKIP_ID, nodeIds);
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      {
        vector<int> edgeNodes( nodeIds.begin(), --nodeIds.end() ); // 01
        badElems.push_back( toSmdsConvertor.getElement(edgeNodes));
        edgeNodes[1] = nodeIds[2]; // 02
        badElems.push_back( toSmdsConvertor.getElement(edgeNodes));
        edgeNodes[0] = nodeIds[1]; // 12
      }      
      break;
    case 1000: // Face (f 1, f 2, f 3) appears more than once in the input surface mesh.
      // ERR  1000 :  1 3 2
    case 1002: // Face (f 1, f 2, f 3) has a vertex negative or null
    case 3019: // Constrained face (f 1, f 2, f 3) cannot be enforced
    case 1002211: // a face has a vertex negative or null.
    case 1005200 : // a surface mesh appears more than once in the input surface mesh.
    case 1008423 : // a constrained face cannot be enforced (regeneration phase failed).
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 1001: // Edge (e1, e2) appears more than once in the input surface mesh
    case 3009: // Constrained edge (e1, e2) cannot be enforced (warning).
      // ERR  3109 :  EDGE  5 6 UNIQUE
    case 3109: // Edge (e1, e2) is unique (i.e., bounds a hole in the surface)
    case 1005210 : // an edge appears more than once in the input surface mesh.
    case 1005820 : // an edge is unique (i.e., bounds a hole in the surface).
    case 1008441 : // a constrained edge cannot be enforced.
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 2004: // Vertex v1 and vertex v2 are too close to one another or coincident (warning).
    case 2014: // at least two points whose distance is dist, i.e., considered as coincident
    case 2103: // Vertex v1 and vertex v2 are too close to one another or coincident (warning).
      // ERR  2103 :  16 WITH  3
    case 1005105 : // two vertices are too close to one another or coincident.
    case 1005107: // Two vertices are too close to one another or coincident.
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 2012: // Vertex v1 cannot be inserted (warning).
    case 1005106 : // a vertex cannot be inserted.
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3103: // The surface edge (e1, e2) intersects another surface edge (e3, e4)
    case 1005110 : // two surface edges are intersecting.
      // ERR  3103 :  1 2 WITH  7 3
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3104: // The surface edge (e1, e2) intersects the surface face (f 1, f 2, f 3)
      // ERR  3104 :  9 10 WITH  1 2 3
    case 3106: // One surface edge (say e1, e2) intersects a surface face (f 1, f 2, f 3)
    case 1005120 : // a surface edge intersects a surface face.
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3105: // One boundary point (say p1) lies within a surface face (f 1, f 2, f 3)
      // ERR  3105 :  8 IN  2 3 5
    case 1005150 : // a boundary point lies within a surface face.
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3107: // One boundary point (say p1) lies within a surface edge (e1, e2) (stop).
      // ERR  3107 :  2 IN  4 1
    case 1005160 : // a boundary point lies within a surface edge.
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 9000: // ERR  9000
      //  ELEMENT  261 WITH VERTICES :  7 396 -8 242
      //  VOLUME   : -1.11325045E+11 W.R.T. EPSILON   0.
      // A too small volume element is detected. Are reported the index of the element,
      // its four vertex indices, its volume and the tolerance threshold value
      ptr = getIds(ptr, SKIP_ID, nodeIds);
      ptr = getIds(ptr, VOL, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      // even if all nodes found, volume it most probably invisible,
      // add its faces to demonstrate it anyhow
      {
        vector<int> faceNodes( nodeIds.begin(), --nodeIds.end() ); // 012
        badElems.push_back( toSmdsConvertor.getElement(faceNodes));
        faceNodes[2] = nodeIds[3]; // 013
        badElems.push_back( toSmdsConvertor.getElement(faceNodes));
        faceNodes[1] = nodeIds[2]; // 023
        badElems.push_back( toSmdsConvertor.getElement(faceNodes));
        faceNodes[0] = nodeIds[1]; // 123
        badElems.push_back( toSmdsConvertor.getElement(faceNodes));
      }
      break;
    case 9001: // ERR  9001
      //  %% NUMBER OF NEGATIVE VOLUME TETS  :  1
      //  %% THE LARGEST NEGATIVE TET        :   1.75376581E+11
      //  %%  NUMBER OF NULL VOLUME TETS     :  0
      // There exists at least a null or negative volume element
      break;
    case 9002:
      // There exist n null or negative volume elements
      break;
    case 9003:
      // A too small volume element is detected
      break;
    case 9102:
      // A too bad quality face is detected. This face is considered degenerated,
      // its index, its three vertex indices together with its quality value are reported
      break; // same as next
    case 9112: // ERR  9112
      //  FACE   2 WITH VERTICES :  4 2 5
      //  SMALL INRADIUS :   0.
      // A too bad quality face is detected. This face is degenerated,
      // its index, its three vertex indices together with its inradius are reported
      ptr = getIds(ptr, SKIP_ID, nodeIds);
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      // add triangle edges as it most probably has zero area and hence invisible
      {
        vector<int> edgeNodes(2);
        edgeNodes[0] = nodeIds[0]; edgeNodes[1] = nodeIds[1]; // 0-1
        badElems.push_back( toSmdsConvertor.getElement(edgeNodes));
        edgeNodes[1] = nodeIds[2]; // 0-2
        badElems.push_back( toSmdsConvertor.getElement(edgeNodes));
        edgeNodes[0] = nodeIds[1]; // 1-2
        badElems.push_back( toSmdsConvertor.getElement(edgeNodes));
      }
      break;
    case 1005103 : // the vertices of an element are too close to one another or coincident.
      ptr = getIds(ptr, TRIA, nodeIds);
      if ( nodeIds.back() == 0 ) // index of the third vertex of the element (0 for an edge)
        nodeIds.resize( EDGE );
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    }

    bool isNewError = foundErrorStr.insert( string( errBeg, ptr )).second;
    if ( !isNewError )
      continue; // not to report same error several times

//     const SMDS_MeshElement* nullElem = 0;
//     bool allElemsOk = ( find( badElems.begin(), badElems.end(), nullElem) == badElems.end());

//     if ( allElemsOk && !badElems.empty() && !elemErrorNums.empty() ) {
//       bool oneMoreErrorType = elemErrorNums.insert( errNum ).second;
//       if ( oneMoreErrorType )
//         continue; // not to report different types of errors with bad elements
//     }

    // make error text
    string text = translateError( errNum );
    if ( errDescription.find( text ) == text.npos ) {
      if ( !errDescription.empty() )
        errDescription << "\n";
      errDescription << text;
    }

  } // end while

  if ( errDescription.empty() ) { // no errors found
    char msgLic1[] = "connection to server failed";
    char msgLic2[] = " Dlim ";
    if ( search( &buf[0], bufEnd, msgLic1, msgLic1 + strlen(msgLic1)) != bufEnd ||
         search( &buf[0], bufEnd, msgLic2, msgLic2 + strlen(msgLic2)) != bufEnd )
      errDescription << "Licence problems.";
    else
    {
      char msg2[] = "SEGMENTATION FAULT";
      if ( search( &buf[0], bufEnd, msg2, msg2 + strlen(msg2)) != bufEnd )
        errDescription << "MG-Tetra: SEGMENTATION FAULT. ";
    }
  }

  if ( !isOk && logFile && logFile[0] )
  {
    if ( errDescription.empty() )
      errDescription << "See " << logFile << " for problem description";
    else
      errDescription << "\nSee " << logFile << " for more information";
  }

  err->myComment = errDescription;

  if ( err->myComment.empty() && err->myBadElements.empty() )
    err = SMESH_ComputeError::New(); // OK

  return err;
}

//================================================================================
/*!
 * \brief Creates _Ghs2smdsConvertor
 */
//================================================================================

_Ghs2smdsConvertor::_Ghs2smdsConvertor( const map <int,const SMDS_MeshNode*> & ghs2NodeMap,
                                        SMESH_ProxyMesh::Ptr                   mesh)
  :_ghs2NodeMap( & ghs2NodeMap ), _nodeByGhsId( 0 ), _mesh( mesh )
{
}

//================================================================================
/*!
 * \brief Creates _Ghs2smdsConvertor
 */
//================================================================================

_Ghs2smdsConvertor::_Ghs2smdsConvertor( const vector <const SMDS_MeshNode*> &  nodeByGhsId,
                                        SMESH_ProxyMesh::Ptr                   mesh)
  : _ghs2NodeMap( 0 ), _nodeByGhsId( &nodeByGhsId ), _mesh( mesh )
{
}

//================================================================================
/*!
 * \brief Return SMDS element by ids of MG-Tetra nodes
 */
//================================================================================

const SMDS_MeshElement* _Ghs2smdsConvertor::getElement(const vector<int>& ghsNodes) const
{
  size_t nbNodes = ghsNodes.size();
  vector<const SMDS_MeshNode*> nodes( nbNodes, 0 );
  for ( size_t i = 0; i < nbNodes; ++i ) {
    int ghsNode = ghsNodes[ i ];
    if ( _ghs2NodeMap ) {
      map <int,const SMDS_MeshNode*>::const_iterator in = _ghs2NodeMap->find( ghsNode);
      if ( in == _ghs2NodeMap->end() )
        return 0;
      nodes[ i ] = in->second;
    }
    else {
      if ( ghsNode < 1 || ghsNode > (int)_nodeByGhsId->size() )
        return 0;
      nodes[ i ] = (*_nodeByGhsId)[ ghsNode-1 ];
    }
  }
  if ( nbNodes == 1 )
    return nodes[0];

  if ( nbNodes == 2 ) {
    const SMDS_MeshElement* edge= SMDS_Mesh::FindEdge( nodes[0], nodes[1] );
    if ( !edge || edge->GetID() < 1 || _mesh->IsTemporary( edge ))
      edge = new SMDS_LinearEdge( nodes[0], nodes[1] );
    return edge;
  }
  if ( nbNodes == 3 ) {
    const SMDS_MeshElement* face = SMDS_Mesh::FindFace( nodes );
    if ( !face || face->GetID() < 1 || _mesh->IsTemporary( face ))
      face = new SMDS_FaceOfNodes( nodes[0], nodes[1], nodes[2] );
    return face;
  }
  if ( nbNodes == 4 )
    return new SMDS_VolumeOfNodes( nodes[0], nodes[1], nodes[2], nodes[3] );

  return 0;
}


//=============================================================================
/*!
 *
 */
//=============================================================================
bool GHS3DPlugin_GHS3D::Evaluate(SMESH_Mesh& aMesh,
                                 const TopoDS_Shape& aShape,
                                 MapShapeNbElems& aResMap)
{
  int nbtri = 0, nbqua = 0;
  double fullArea = 0.0;
  for (TopExp_Explorer exp(aShape, TopAbs_FACE); exp.More(); exp.Next()) {
    TopoDS_Face F = TopoDS::Face( exp.Current() );
    SMESH_subMesh *sm = aMesh.GetSubMesh(F);
    MapShapeNbElemsItr anIt = aResMap.find(sm);
    if( anIt==aResMap.end() ) {
      SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
      smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,
                                            "Submesh can not be evaluated",this));
      return false;
    }
    std::vector<int> aVec = (*anIt).second;
    nbtri += Max(aVec[SMDSEntity_Triangle],aVec[SMDSEntity_Quad_Triangle]);
    nbqua += Max(aVec[SMDSEntity_Quadrangle],aVec[SMDSEntity_Quad_Quadrangle]);
    GProp_GProps G;
    BRepGProp::SurfaceProperties(F,G);
    double anArea = G.Mass();
    fullArea += anArea;
  }

  // collect info from edges
  int nb0d_e = 0, nb1d_e = 0;
  bool IsQuadratic = false;
  bool IsFirst = true;
  TopTools_MapOfShape tmpMap;
  for (TopExp_Explorer exp(aShape, TopAbs_EDGE); exp.More(); exp.Next()) {
    TopoDS_Edge E = TopoDS::Edge(exp.Current());
    if( tmpMap.Contains(E) )
      continue;
    tmpMap.Add(E);
    SMESH_subMesh *aSubMesh = aMesh.GetSubMesh(exp.Current());
    MapShapeNbElemsItr anIt = aResMap.find(aSubMesh);
    std::vector<int> aVec = (*anIt).second;
    nb0d_e += aVec[SMDSEntity_Node];
    nb1d_e += Max(aVec[SMDSEntity_Edge],aVec[SMDSEntity_Quad_Edge]);
    if(IsFirst) {
      IsQuadratic = (aVec[SMDSEntity_Quad_Edge] > aVec[SMDSEntity_Edge]);
      IsFirst = false;
    }
  }
  tmpMap.Clear();

  double ELen = sqrt(2.* ( fullArea/(nbtri+nbqua*2) ) / sqrt(3.0) );

  GProp_GProps G;
  BRepGProp::VolumeProperties(aShape,G);
  double aVolume = G.Mass();
  double tetrVol = 0.1179*ELen*ELen*ELen;
  double CoeffQuality = 0.9;
  int nbVols = int(aVolume/tetrVol/CoeffQuality);
  int nb1d_f = (nbtri*3 + nbqua*4 - nb1d_e) / 2;
  int nb1d_in = (int) ( nbVols*6 - nb1d_e - nb1d_f ) / 5;
  std::vector<int> aVec(SMDSEntity_Last);
  for(int i=SMDSEntity_Node; i<SMDSEntity_Last; i++) aVec[i]=0;
  if( IsQuadratic ) {
    aVec[SMDSEntity_Node] = nb1d_in/6 + 1 + nb1d_in;
    aVec[SMDSEntity_Quad_Tetra] = nbVols - nbqua*2;
    aVec[SMDSEntity_Quad_Pyramid] = nbqua;
  }
  else {
    aVec[SMDSEntity_Node] = nb1d_in/6 + 1;
    aVec[SMDSEntity_Tetra] = nbVols - nbqua*2;
    aVec[SMDSEntity_Pyramid] = nbqua;
  }
  SMESH_subMesh *sm = aMesh.GetSubMesh(aShape);
  aResMap.insert(std::make_pair(sm,aVec));

  return true;
}

bool GHS3DPlugin_GHS3D::importGMFMesh(const char* theGMFFileName, SMESH_Mesh& theMesh)
{
  SMESH_ComputeErrorPtr err = theMesh.GMFToMesh( theGMFFileName, /*makeRequiredGroups =*/ true );

  theMesh.GetMeshDS()->Modified();

  return ( !err || err->IsOK());
}

namespace
{
  //================================================================================
  /*!
   * \brief Sub-mesh event listener setting enforced elements as soon as an enforced
   *        mesh is loaded
   */
  struct _EnforcedMeshRestorer : public SMESH_subMeshEventListener
  {
    _EnforcedMeshRestorer():
      SMESH_subMeshEventListener( /*isDeletable = */true, Name() )
    {}

    //================================================================================
    /*!
     * \brief Returns an ID of listener
     */
    static const char* Name() { return "GHS3DPlugin_GHS3D::_EnforcedMeshRestorer"; }

    //================================================================================
    /*!
     * \brief Treat events of the subMesh
     */
    void ProcessEvent(const int                       event,
                      const int                       eventType,
                      SMESH_subMesh*                  subMesh,
                      SMESH_subMeshEventListenerData* data,
                      const SMESH_Hypothesis*         hyp)
    {
      if ( SMESH_subMesh::SUBMESH_LOADED == event &&
           SMESH_subMesh::COMPUTE_EVENT  == eventType &&
           data &&
           !data->mySubMeshes.empty() )
      {
        // An enforced mesh (subMesh->_father) has been loaded from hdf file
        if ( GHS3DPlugin_Hypothesis* hyp = GetGHSHypothesis( data->mySubMeshes.front() ))
          hyp->RestoreEnfElemsByMeshes();
      }
    }
    //================================================================================
    /*!
     * \brief Returns GHS3DPlugin_Hypothesis used to compute a subMesh
     */
    static GHS3DPlugin_Hypothesis* GetGHSHypothesis( SMESH_subMesh* subMesh )
    {
      SMESH_HypoFilter ghsHypFilter
        ( SMESH_HypoFilter::HasName( GHS3DPlugin_Hypothesis::GetHypType() ));
      return (GHS3DPlugin_Hypothesis* )
        subMesh->GetFather()->GetHypothesis( subMesh->GetSubShape(),
                                             ghsHypFilter,
                                             /*visitAncestors=*/true);
    }
  };

  //================================================================================
  /*!
   * \brief Sub-mesh event listener removing empty groups created due to "To make
   *        groups of domains".
   */
  struct _GroupsOfDomainsRemover : public SMESH_subMeshEventListener
  {
    _GroupsOfDomainsRemover():
      SMESH_subMeshEventListener( /*isDeletable = */true,
                                  "GHS3DPlugin_GHS3D::_GroupsOfDomainsRemover" ) {}
    /*!
     * \brief Treat events of the subMesh
     */
    void ProcessEvent(const int                       event,
                      const int                       eventType,
                      SMESH_subMesh*                  subMesh,
                      SMESH_subMeshEventListenerData* data,
                      const SMESH_Hypothesis*         hyp)
    {
      if (SMESH_subMesh::ALGO_EVENT == eventType &&
          !subMesh->GetAlgo() )
      {
        removeEmptyGroupsOfDomains( subMesh->GetFather(), /*notEmptyAsWell=*/true );
      }
    }
  };
}

//================================================================================
/*!
 * \brief Set an event listener to set enforced elements as soon as an enforced
 *        mesh is loaded
 */
//================================================================================

void GHS3DPlugin_GHS3D::SubmeshRestored(SMESH_subMesh* subMesh)
{
  if ( GHS3DPlugin_Hypothesis* hyp = _EnforcedMeshRestorer::GetGHSHypothesis( subMesh ))
  {
    GHS3DPlugin_Hypothesis::TGHS3DEnforcedMeshList enfMeshes = hyp->_GetEnforcedMeshes();
    GHS3DPlugin_Hypothesis::TGHS3DEnforcedMeshList::iterator it = enfMeshes.begin();
    for(;it != enfMeshes.end();++it) {
      GHS3DPlugin_Hypothesis::TGHS3DEnforcedMesh* enfMesh = *it;
      if ( SMESH_Mesh* mesh = GetMeshByPersistentID( enfMesh->persistID ))
      {
        SMESH_subMesh* smToListen = mesh->GetSubMesh( mesh->GetShapeToMesh() );
        // a listener set to smToListen will care of hypothesis stored in SMESH_EventListenerData
        subMesh->SetEventListener( new _EnforcedMeshRestorer(),
                                   SMESH_subMeshEventListenerData::MakeData( subMesh ),
                                   smToListen);
      }
    }
  }
}

//================================================================================
/*!
 * \brief Sets an event listener removing empty groups created due to "To make
 *        groups of domains".
 * \param subMesh - submesh where algo is set
 *
 * This method is called when a submesh gets HYP_OK algo_state.
 * After being set, event listener is notified on each event of a submesh.
 */
//================================================================================

void GHS3DPlugin_GHS3D::SetEventListener(SMESH_subMesh* subMesh)
{
  subMesh->SetEventListener( new _GroupsOfDomainsRemover(), 0, subMesh );
}

//================================================================================
/*!
 * \brief If possible, returns progress of computation [0.,1.]
 */
//================================================================================

double GHS3DPlugin_GHS3D::GetProgress() const
{
  if ( _isLibUsed )
  {
    // this->_progress is advanced by MG_Tetra_API according to messages from MG library
    // but sharply. Advance it a bit to get smoother advancement.
    GHS3DPlugin_GHS3D* me = const_cast<GHS3DPlugin_GHS3D*>( this );
    if ( _progress < 0.1 ) // the first message is at 10%
      me->_progress = GetProgressByTic();
    else if ( _progress < 0.98 )
      me->_progress += _progressAdvance;
    return _progress;
  }

  return -1;
}
