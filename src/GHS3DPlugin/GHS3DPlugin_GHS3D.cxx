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

//=============================================================================
// File      : GHS3DPlugin_GHS3D.cxx
// Created   : 
// Author    : Edward AGAPOV, modified by Lioka RAZAFINDRAZAKA (CEA) 09/02/2007
// Project   : SALOME
//=============================================================================
//
#include "GHS3DPlugin_GHS3D.hxx"
#include "GHS3DPlugin_Hypothesis.hxx"

#include <SMDS_FaceOfNodes.hxx>
#include <SMDS_MeshElement.hxx>
#include <SMDS_MeshNode.hxx>
#include <SMDS_VolumeOfNodes.hxx>
#include <SMESHDS_Group.hxx>
#include <SMESH_Comment.hxx>
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
#include <OSD_File.hxx>
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

#ifdef WIN32
#include <io.h>
#else
#include <sys/sysinfo.h>
#endif
#include <algorithm>

#define castToNode(n) static_cast<const SMDS_MeshNode *>( n );

extern "C"
{
#ifndef WIN32
#include <unistd.h>
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
}

#define HOLE_ID -1

typedef const list<const SMDS_MeshFace*> TTriaList;

static const char theDomainGroupNamePrefix[] = "Domain_";

static void removeFile( const TCollection_AsciiString& fileName )
{
  try {
    OSD_File( fileName ).Remove();
  }
  catch ( Standard_ProgramError ) {
    MESSAGE("Can't remove file: " << fileName.ToCString() << " ; file does not exist or permission denied");
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

GHS3DPlugin_GHS3D::GHS3DPlugin_GHS3D(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_3D_Algo(hypId, studyId, gen)
{
  MESSAGE("GHS3DPlugin_GHS3D::GHS3DPlugin_GHS3D");
  _name = Name();
  _shapeType = (1 << TopAbs_SHELL) | (1 << TopAbs_SOLID);// 1 bit /shape type
  _onlyUnaryInput = false; // Compute() will be called on a compound of solids
  _iShape=0;
  _nbShape=0;
  _compatibleHypothesis.push_back( GHS3DPlugin_Hypothesis::GetHypType());
  _compatibleHypothesis.push_back( StdMeshers_ViscousLayers::GetHypType() );
  _requireShape = false; // can work without shape_studyId

  smeshGen_i = SMESH_Gen_i::GetSMESHGen();
  CORBA::Object_var anObject = smeshGen_i->GetNS()->Resolve("/myStudyManager");
  SALOMEDS::StudyManager_var aStudyMgr = SALOMEDS::StudyManager::_narrow(anObject);

  MESSAGE("studyid = " << _studyId);

  myStudy = NULL;
  myStudy = aStudyMgr->GetStudyByID(_studyId);
  if (myStudy)
    MESSAGE("myStudy->StudyId() = " << myStudy->StudyId());
  
  _compute_canceled = false;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

GHS3DPlugin_GHS3D::~GHS3DPlugin_GHS3D()
{
  MESSAGE("GHS3DPlugin_GHS3D::~GHS3DPlugin_GHS3D");
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
  MESSAGE("GHS3DPlugin_GHS3D::entryToShape "<<entry );
  GEOM::GEOM_Object_var aGeomObj;
  TopoDS_Shape S = TopoDS_Shape();
  SALOMEDS::SObject_var aSObj = myStudy->FindObjectID( entry.c_str() );
  if (!aSObj->_is_nil() ) {
    CORBA::Object_var obj = aSObj->GetObject();
    aGeomObj = GEOM::GEOM_Object::_narrow(obj);
    aSObj->UnRegister();
  }
  if ( !aGeomObj->_is_nil() )
    S = smeshGen_i->GeomObjectToShape( aGeomObj.in() );
  return S;
}

//=======================================================================
//function : findShape
//purpose  : 
//=======================================================================

static TopoDS_Shape findShape(const SMDS_MeshNode *aNode[],
                              TopoDS_Shape        aShape,
                              const TopoDS_Shape  shape[],
                              double**            box,
                              const int           nShape,
                              TopAbs_State *      state = 0)
{
  gp_XYZ aPnt(0,0,0);
  int j, iShape, nbNode = 4;

  for ( j=0; j<nbNode; j++ ) {
    gp_XYZ p ( aNode[j]->X(), aNode[j]->Y(), aNode[j]->Z() );
    if ( aNode[j]->GetPosition()->GetTypeOfPosition() == SMDS_TOP_3DSPACE ) {
      aPnt = p;
      break;
    }
    aPnt += p / nbNode;
  }

  BRepClass3d_SolidClassifier SC (aShape, aPnt, Precision::Confusion());
  if (state) *state = SC.State();
  if ( SC.State() != TopAbs_IN || aShape.IsNull() || aShape.ShapeType() != TopAbs_SOLID) {
    for (iShape = 0; iShape < nShape; iShape++) {
      aShape = shape[iShape];
      if ( !( aPnt.X() < box[iShape][0] || box[iShape][1] < aPnt.X() ||
              aPnt.Y() < box[iShape][2] || box[iShape][3] < aPnt.Y() ||
              aPnt.Z() < box[iShape][4] || box[iShape][5] < aPnt.Z()) ) {
        BRepClass3d_SolidClassifier SC (aShape, aPnt, Precision::Confusion());
        if (state) *state = SC.State();
        if (SC.State() == TopAbs_IN)
          break;
      }
    }
  }
  return aShape;
}

//=======================================================================
//function : readMapIntLine
//purpose  : 
//=======================================================================

static char* readMapIntLine(char* ptr, int tab[]) {
  long int intVal;
  std::cout << std::endl;

  for ( int i=0; i<17; i++ ) {
    intVal = strtol(ptr, &ptr, 10);
    if ( i < 3 )
      tab[i] = intVal;
  }
  return ptr;
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
    bool uvOK;
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

// //=======================================================================
// //function : countShape
// //purpose  :
// //=======================================================================
// 
// template < class Mesh, class Shape >
// static int countShape( Mesh* mesh, Shape shape ) {
//   TopExp_Explorer expShape ( mesh->ShapeToMesh(), shape );
//   TopTools_MapOfShape mapShape;
//   int nbShape = 0;
//   for ( ; expShape.More(); expShape.Next() ) {
//     if (mapShape.Add(expShape.Current())) {
//       nbShape++;
//     }
//   }
//   return nbShape;
// }
// 
// //=======================================================================
// //function : getShape
// //purpose  :
// //=======================================================================
// 
// template < class Mesh, class Shape, class Tab >
// void getShape(Mesh* mesh, Shape shape, Tab *t_Shape) {
//   TopExp_Explorer expShape ( mesh->ShapeToMesh(), shape );
//   TopTools_MapOfShape mapShape;
//   for ( int i=0; expShape.More(); expShape.Next() ) {
//     if (mapShape.Add(expShape.Current())) {
//       t_Shape[i] = expShape.Current();
//       i++;
//     }
//   }
//   return;
// }
// 
// // //=======================================================================
// // //function : findEdgeID
// // //purpose  :
// // //=======================================================================
// 
// static int findEdgeID(const SMDS_MeshNode* aNode,
//                       const SMESHDS_Mesh*  theMesh,
//                       const int            nEdge,
//                       const TopoDS_Shape*  t_Edge) {
// 
//   TopoDS_Shape aPntShape, foundEdge;
//   TopoDS_Vertex aVertex;
//   gp_Pnt aPnt( aNode->X(), aNode->Y(), aNode->Z() );
// 
//   int foundInd, ind;
//   double nearest = RealLast(), *t_Dist;
//   double epsilon = Precision::Confusion();
// 
//   t_Dist = new double[ nEdge ];
//   aPntShape = BRepBuilderAPI_MakeVertex( aPnt ).Shape();
//   aVertex   = TopoDS::Vertex( aPntShape );
// 
//   for ( ind=0; ind < nEdge; ind++ ) {
//     BRepExtrema_DistShapeShape aDistance ( aVertex, t_Edge[ind] );
//     t_Dist[ind] = aDistance.Value();
//     if ( t_Dist[ind] < nearest ) {
//       nearest   = t_Dist[ind];
//       foundEdge = t_Edge[ind];
//       foundInd  = ind;
//       if ( nearest < epsilon )
//         ind = nEdge;
//     }
//   }
// 
//   delete [] t_Dist;
//   return theMesh->ShapeToIndex( foundEdge );
// }
// 
// 
// // =======================================================================
// // function : readGMFFile
// // purpose  : read GMF file with geometry associated to mesh
// // =======================================================================
// 
// static bool readGMFFile(const int                       fileOpen,
//                         const char*                     theFileName, 
//                         SMESH_Mesh&                     theMesh,
//                         const int                       nbShape,
//                         const TopoDS_Shape*             tabShape,
//                         double**                        tabBox,
//                         map <int,const SMDS_MeshNode*>& theGhs3dIdToNodeMap,
//                         bool                            toMeshHoles,
//                         int                             nbEnforcedVertices,
//                         int                             nbEnforcedNodes)
// {
//   TopoDS_Shape aShape;
//   TopoDS_Vertex aVertex;
//   SMESHDS_Mesh* theMeshDS = theMesh.GetMeshDS();
//   int nbElem = 0, nbRef = 0, IdShapeRef = 1;
//   int *tabID;
//   int aGMFNodeID = 0;
//   int compoundID =
//     nbShape ? theMeshDS->ShapeToIndex( tabShape[0] ) : theMeshDS->ShapeToIndex( theMeshDS->ShapeToMesh() );
//   int tetraShapeID = compoundID;
//   double epsilon = Precision::Confusion();
//   int *nodeAssigne, *GMFNodeAssigne;
//   SMDS_MeshNode** GMFNode;
//   TopoDS_Shape *tabCorner, *tabEdge;
//   std::map <GmfKwdCod,int> tabRef;
//   
//   
//   int ver, dim;
//   MESSAGE("Read " << theFileName << " file");
//   int InpMsh = GmfOpenMesh(theFileName, GmfRead, &ver, &dim);
//   if (!InpMsh)
//     return false;
//   
//   // ===========================
//   // Fill the tabID array: BEGIN
//   // ===========================
//   
//   /*
//   The output .mesh file does not contain yet the subdomain-info (Ghs3D 4.2)
//   */
//   Kernel_Utils::Localizer loc;
//   struct stat status;
//   size_t      length;
// 
//   char *ptr, *mapPtr;
//   char *tetraPtr;
//   int *tab = new int[3];
//   
//   // Read the file state
//   fstat(fileOpen, &status);
//   length   = status.st_size;
//   
//   // Mapping the result file into memory
// #ifdef WIN32
//   HANDLE fd = CreateFile(theFileName, GENERIC_READ, FILE_SHARE_READ,
//                          NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//   HANDLE hMapObject = CreateFileMapping(fd, NULL, PAGE_READONLY,
//                                         0, (DWORD)length, NULL);
//   ptr = ( char* ) MapViewOfFile(hMapObject, FILE_MAP_READ, 0, 0, 0 );
// #else
//   ptr = (char *) mmap(0,length,PROT_READ,MAP_PRIVATE,fileOpen,0);
// #endif
//   mapPtr = ptr;
// 
//   ptr      = readMapIntLine(ptr, tab);
//   tetraPtr = ptr;
// 
//   nbElem            = tab[0];
//   int nbNodes       = tab[1];
//   
//   for (int i=0; i < 4*nbElem; i++)
//     strtol(ptr, &ptr, 10);
//   
//   for (int iNode=1; iNode <= nbNodes; iNode++)
//     for (int iCoor=0; iCoor < 3; iCoor++)
//       strtod(ptr, &ptr);
// 
//     
//   // Reading the number of triangles which corresponds to the number of sub-domains
//   int nbTriangle = strtol(ptr, &ptr, 10);
// 
//   
//   // The keyword does not exist yet => to update when it is created
// //   int nbSubdomains = GmfStatKwd(InpMsh, GmfSubdomain);
// //   int id_tri[3];
// 
// 
//   tabID = new int[nbTriangle];
//   for (int i=0; i < nbTriangle; i++) {
//     tabID[i] = 0;
//     int nodeId1, nodeId2, nodeId3;
//     // find the solid corresponding to MG-Tetra sub-domain following
//     // the technique proposed in MG-Tetra manual in chapter
//     // "B.4 Subdomain (sub-region) assignment"
// 
//     nodeId1 = strtol(ptr, &ptr, 10);
//     nodeId2 = strtol(ptr, &ptr, 10);
//     nodeId3 = strtol(ptr, &ptr, 10);
// 
// //   // The keyword does not exist yet => to update when it is created
// //     GmfGetLin(InpMsh, GmfSubdomain, &id_tri[0], &id_tri[1], &id_tri[2]);
// //     nodeId1 = id_tri[0];
// //     nodeId2 = id_tri[1];
// //     nodeId3 = id_tri[2];
// 
//     if ( nbTriangle > 1 ) {
//       // get the nodes indices
//       const SMDS_MeshNode* n1 = theGhs3dIdToNodeMap[ nodeId1 ];
//       const SMDS_MeshNode* n2 = theGhs3dIdToNodeMap[ nodeId2 ];
//       const SMDS_MeshNode* n3 = theGhs3dIdToNodeMap[ nodeId3 ];
//       try {
//         OCC_CATCH_SIGNALS;
//         tabID[i] = findShapeID( theMesh, n1, n2, n3, toMeshHoles );
//         // -- 0020330: Pb with MG-Tetra as a submesh
//         // check that found shape is to be meshed
//         if ( tabID[i] > 0 ) {
//           const TopoDS_Shape& foundShape = theMeshDS->IndexToShape( tabID[i] );
//           bool isToBeMeshed = false;
//           for ( int iS = 0; !isToBeMeshed && iS < nbShape; ++iS )
//             isToBeMeshed = foundShape.IsSame( tabShape[ iS ]);
//           if ( !isToBeMeshed )
//             tabID[i] = HOLE_ID;
//         }
//         // END -- 0020330: Pb with MG-Tetra as a submesh
// #ifdef _DEBUG_
//         std::cout << i+1 << " subdomain: findShapeID() returns " << tabID[i] << std::endl;
// #endif
//       }
//       catch ( Standard_Failure & ex)
//       {
// #ifdef _DEBUG_
//         std::cout << i+1 << " subdomain: Exception caugt: " << ex.GetMessageString() << std::endl;
// #endif
//       }
//       catch (...) {
// #ifdef _DEBUG_
//         std::cout << i+1 << " subdomain: unknown exception caught " << std::endl;
// #endif
//       }
//     }
//   }
//   
//   // ===========================
//   // Fill the tabID array: END
//   // ===========================
//   
// 
//   tabRef[GmfVertices]       = 3;
//   tabRef[GmfCorners]        = 1;
//   tabRef[GmfEdges]          = 2;
//   tabRef[GmfRidges]         = 1;
//   tabRef[GmfTriangles]      = 3;
// //   tabRef[GmfQuadrilaterals] = 4;
//   tabRef[GmfTetrahedra]     = 4;
// //   tabRef[GmfHexahedra]      = 8;
//   
//   SMDS_NodeIteratorPtr itOnGMFInputNode = theMeshDS->nodesIterator();
//   while ( itOnGMFInputNode->more() )
//     theMeshDS->RemoveNode( itOnGMFInputNode->next() );
// 
//   
//   int nbVertices = GmfStatKwd(InpMsh, GmfVertices);
//   int nbCorners = max(countShape( theMeshDS, TopAbs_VERTEX ) , GmfStatKwd(InpMsh, GmfCorners));
//   int nbShapeEdge = countShape( theMeshDS, TopAbs_EDGE );
// 
//   tabCorner       = new TopoDS_Shape[ nbCorners ];
//   tabEdge         = new TopoDS_Shape[ nbShapeEdge ];
//   nodeAssigne     = new int[ nbVertices + 1 ];
//   GMFNodeAssigne  = new int[ nbVertices + 1 ];
//   GMFNode         = new SMDS_MeshNode*[ nbVertices + 1 ];
// 
//   getShape(theMeshDS, TopAbs_VERTEX, tabCorner);
//   getShape(theMeshDS, TopAbs_EDGE,   tabEdge);
// 
//   std::map <GmfKwdCod,int>::const_iterator it = tabRef.begin();
//   for ( ; it != tabRef.end() ; ++it)
//   {
// //     int dummy;
//     GmfKwdCod token = it->first;
//     nbRef    = it->second;
// 
//     nbElem = GmfStatKwd(InpMsh, token);
//     if (nbElem > 0) {
//       GmfGotoKwd(InpMsh, token);
//       std::cout << "Read " << nbElem;
//     }
//     else
//       continue;
// 
//     int id[nbElem*tabRef[token]];
//     int ghs3dShapeID[nbElem];
// 
//     if (token == GmfVertices) {
//       std::cout << " vertices" << std::endl;
//       int aGMFID;
// 
//       float VerTab_f[nbElem][3];
//       double VerTab_d[nbElem][3];
//       SMDS_MeshNode * aGMFNode;
// 
//       for ( int iElem = 0; iElem < nbElem; iElem++ ) {
//         aGMFID = iElem + 1;
//         if (ver == GmfFloat) {
//           GmfGetLin(InpMsh, token, &VerTab_f[nbElem][0], &VerTab_f[nbElem][1], &VerTab_f[nbElem][2], &ghs3dShapeID[iElem]);
//           aGMFNode = theMeshDS->AddNode(VerTab_f[nbElem][0], VerTab_f[nbElem][1], VerTab_f[nbElem][2]);
//         }
//         else {
//           GmfGetLin(InpMsh, token, &VerTab_d[nbElem][0], &VerTab_d[nbElem][1], &VerTab_d[nbElem][2], &ghs3dShapeID[iElem]);
//           aGMFNode = theMeshDS->AddNode(VerTab_d[nbElem][0], VerTab_d[nbElem][1], VerTab_d[nbElem][2]);
//         }
//         GMFNode[ aGMFID ] = aGMFNode;
//         nodeAssigne[ aGMFID ] = 0;
//         GMFNodeAssigne[ aGMFID ] = 0;
//       }
//     }
//     else if (token == GmfCorners && nbElem > 0) {
//       std::cout << " corners" << std::endl;
//       for ( int iElem = 0; iElem < nbElem; iElem++ )
//         GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]]);
//     }
//     else if (token == GmfRidges && nbElem > 0) {
//       std::cout << " ridges" << std::endl;
//       for ( int iElem = 0; iElem < nbElem; iElem++ )
//         GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]]);
//     }
//     else if (token == GmfEdges && nbElem > 0) {
//       std::cout << " edges" << std::endl;
//       for ( int iElem = 0; iElem < nbElem; iElem++ )
//         GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &ghs3dShapeID[iElem]);
//     }
//     else if (token == GmfTriangles && nbElem > 0) {
//       std::cout << " triangles" << std::endl;
//       for ( int iElem = 0; iElem < nbElem; iElem++ )
//         GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &ghs3dShapeID[iElem]);
//     }
// //     else if (token == GmfQuadrilaterals && nbElem > 0) {
// //       std::cout << " Quadrilaterals" << std::endl;
// //       for ( int iElem = 0; iElem < nbElem; iElem++ )
// //         GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &id[iElem*tabRef[token]+3], &ghs3dShapeID[iElem]);
// //     }
//     else if (token == GmfTetrahedra && nbElem > 0) {
//       std::cout << " Tetrahedra" << std::endl;
//       for ( int iElem = 0; iElem < nbElem; iElem++ )
//         GmfGetLin(InpMsh, token, 
//                   &id[iElem*tabRef[token]], 
//                   &id[iElem*tabRef[token]+1], 
//                   &id[iElem*tabRef[token]+2], 
//                   &id[iElem*tabRef[token]+3], 
//                   &ghs3dShapeID[iElem]);
//     }
// //     else if (token == GmfHexahedra && nbElem > 0) {
// //       std::cout << " Hexahedra" << std::endl;
// //       for ( int iElem = 0; iElem < nbElem; iElem++ )
// //         GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &id[iElem*tabRef[token]+3],
// //                   &id[iElem*tabRef[token]+4], &id[iElem*tabRef[token]+5], &id[iElem*tabRef[token]+6], &id[iElem*tabRef[token]+7], &ghs3dShapeID[iElem]);
// //     }
// 
//     switch (token) {
//     case GmfCorners:
//     case GmfRidges:
//     case GmfEdges:
//     case GmfTriangles:
// //     case GmfQuadrilaterals:
//     case GmfTetrahedra:
// //     case GmfHexahedra:
//     {
//       int nodeDim, shapeID, *nodeID;
//       const SMDS_MeshNode** node;
// //       std::vector< SMDS_MeshNode* > enfNode( nbRef );
//       SMDS_MeshElement * aGMFElement;
//       
//       node    = new const SMDS_MeshNode*[nbRef];
//       nodeID  = new int[ nbRef ];
// 
//       for ( int iElem = 0; iElem < nbElem; iElem++ )
//       {
//         for ( int iRef = 0; iRef < nbRef; iRef++ )
//         {
//           aGMFNodeID = id[iElem*tabRef[token]+iRef]; // read nbRef aGMFNodeID
//           node  [ iRef ] = GMFNode[ aGMFNodeID ];
//           nodeID[ iRef ] = aGMFNodeID;
//         }
// 
//         switch (token)
//         {
//         case GmfCorners: {
//           nodeDim = 1;
//           gp_Pnt GMFPnt ( node[0]->X(), node[0]->Y(), node[0]->Z() );
//           for ( int i=0; i<nbElem; i++ ) {
//             aVertex = TopoDS::Vertex( tabCorner[i] );
//             gp_Pnt aPnt = BRep_Tool::Pnt( aVertex );
//             if ( aPnt.Distance( GMFPnt ) < epsilon )
//               break;
//           }
//           break;
//         }
//         case GmfEdges: {
//           nodeDim = 2;
//           aGMFElement = theMeshDS->AddEdge( node[0], node[1] );
//           int iNode = 1;
//           if ( GMFNodeAssigne[ nodeID[0] ] == 0 || GMFNodeAssigne[ nodeID[0] ] == 2 )
//             iNode = 0;
//           shapeID = findEdgeID( node[iNode], theMeshDS, nbShapeEdge, tabEdge );
//           break;
//         }
//         case GmfRidges:
//           break;
//         case GmfTriangles: {
//           nodeDim = 3;
//           aGMFElement = theMeshDS->AddFace( node[0], node[1], node[2]);
//           shapeID = -1;
//           break;
//         }
// //         case GmfQuadrilaterals: {
// //           nodeDim = 4;
// //           aGMFElement = theMeshDS->AddFace( node[0], node[1], node[2], node[3] );
// //           shapeID = -1;
// //           break;
// //         }
//         case GmfTetrahedra: {
//           
//           // IN WORK
//           TopoDS_Shape aSolid;
//           // We always run MG-Tetra with "to mesh holes"==TRUE but we must not create
//           // tetras within holes depending on hypo option,
//           // so we first check if aTet is inside a hole and then create it 
//           if ( nbTriangle > 1 ) {
//             tetraShapeID = HOLE_ID; // negative tetraShapeID means not to create tetras if !toMeshHoles
//             int aGhs3dShapeID = ghs3dShapeID[iElem] - IdShapeRef;
//             if ( tabID[ aGhs3dShapeID ] == 0 ) {
//               TopAbs_State state;
//               aSolid = findShape(node, aSolid, tabShape, tabBox, nbShape, &state);
//               if ( toMeshHoles || state == TopAbs_IN )
//                 tetraShapeID = theMeshDS->ShapeToIndex( aSolid );
//               tabID[ aGhs3dShapeID ] = tetraShapeID;
//             }
//             else
//               tetraShapeID = tabID[ aGhs3dShapeID ];
//           }
//           else if ( nbShape > 1 ) {
//             // Case where nbTriangle == 1 while nbShape == 2 encountered
//             // with compound of 2 boxes and "To mesh holes"==False,
//             // so there are no subdomains specified for each tetrahedron.
//             // Try to guess a solid by a node already bound to shape
//             tetraShapeID = 0;
//             for ( int i=0; i<4 && tetraShapeID==0; i++ ) {
//               if ( nodeAssigne[ nodeID[i] ] == 1 &&
//                   node[i]->GetPosition()->GetTypeOfPosition() == SMDS_TOP_3DSPACE &&
//                   node[i]->getshapeId() > 1 )
//               {
//                 tetraShapeID = node[i]->getshapeId();
//               }
//             }
//             if ( tetraShapeID==0 ) {
//               aSolid = findShape(node, aSolid, tabShape, tabBox, nbShape);
//               tetraShapeID = theMeshDS->ShapeToIndex( aSolid );
//             }
//           }
//           // set new nodes and tetrahedron onto the shape
//           for ( int i=0; i<4; i++ ) {
//             if ( nodeAssigne[ nodeID[i] ] == 0 ) {
//               if ( tetraShapeID != HOLE_ID )
//                 theMeshDS->SetNodeInVolume( node[i], tetraShapeID );
//               nodeAssigne[ nodeID[i] ] = tetraShapeID;
//             }
//           }
//           if ( toMeshHoles || tetraShapeID != HOLE_ID ) {
//             aGMFElement = theMeshDS->AddVolume( node[1], node[0], node[2], node[3] );
//             theMeshDS->SetMeshElementOnShape( aGMFElement, tetraShapeID );
//           }
//           
//           // IN WORK
//           
//           nodeDim = 5;
//           break;
//         }
// //         case GmfHexahedra: {
// //           nodeDim = 6;
// //           aGMFElement = theMeshDS->AddVolume( node[0], node[3], node[2], node[1],
// //                                             node[4], node[7], node[6], node[5] );
// //           break;
// //         }
//         default: continue;
//         }
//         if (token != GmfRidges)
//         {
//           for ( int i=0; i<nbRef; i++ ) {
//               if ( GMFNodeAssigne[ nodeID[i] ] == 0 ) {
//                 if      ( token == GmfCorners )   theMeshDS->SetNodeOnVertex( node[0], aVertex );
//                 else if ( token == GmfEdges )     theMeshDS->SetNodeOnEdge( node[i], shapeID );
//                 else if ( token == GmfTriangles ) theMeshDS->SetNodeOnFace( node[i], shapeID );
//                 GMFNodeAssigne[ nodeID[i] ] = nodeDim;
//               }
//             }
//             if ( token != "Corners" )
//               theMeshDS->SetMeshElementOnShape( aGMFElement, shapeID );
//         }
//       } // for
//       
//       if ( !toMeshHoles ) {
//         map <int,const SMDS_MeshNode*>::iterator itOnNode = theGhs3dIdToNodeMap.find( nbVertices-(nbEnforcedVertices+nbEnforcedNodes) );
//         for ( ; itOnNode != theGhs3dIdToNodeMap.end(); ++itOnNode) {
//           if ( nodeAssigne[ itOnNode->first ] == HOLE_ID )
//             theMeshDS->RemoveFreeNode( itOnNode->second, 0 );
//         }
//       }
//       
//       delete [] node;
//       delete [] nodeID;
//       break;
//       } // case GmfTetrahedra
//     } // switch(token)
//   } // for
//   cout << std::endl;
//   
// #ifdef WIN32
//   UnmapViewOfFile(mapPtr);
//   CloseHandle(hMapObject);
//   CloseHandle(fd);
// #else
//   munmap(mapPtr, length);
// #endif
//   close(fileOpen);
//   
//   delete [] tabID;
//   delete [] tabCorner;
//   delete [] tabEdge;
//   delete [] nodeAssigne;
//   delete [] GMFNodeAssigne;
//   delete [] GMFNode;
//   
//   return true;
// }


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
//       MESSAGE("Successfully added enforced element to existing group " << groupName);
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
//     MESSAGE("Successfully created enforced vertex group " << groupName);
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
      MESSAGE("Delete previous group created by removed enforced elements: " << group->GetName())
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

static bool readGMFFile(const char*                     theFile,
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
  const SMDS_MeshNode** GMFNode;
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
  MESSAGE("Read " << theFile << " file");
  int InpMsh = GmfOpenMesh(theFile, GmfRead, &ver, &dim);
  if (!InpMsh)
    return false;
  MESSAGE("Done ");

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

    int nbDomains = GmfStatKwd( InpMsh, GmfSubDomainFromGeom );
    if ( nbDomains > 1 )
    {
      solidIDByDomain.resize( nbDomains+1, theHelper->GetSubShapeID() );
      int faceNbNodes, faceIndex, orientation, domainNb;
      GmfGotoKwd( InpMsh, GmfSubDomainFromGeom );
      for ( int i = 0; i < nbDomains; ++i )
      {
        faceIndex = 0;
        GmfGetLin( InpMsh, GmfSubDomainFromGeom,
                   &faceNbNodes, &faceIndex, &orientation, &domainNb);
        solidIDByDomain[ domainNb ] = 1;
        if ( 0 < faceIndex && faceIndex-1 < theFaceByGhs3dId.size() )
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

  int nbVertices = GmfStatKwd(InpMsh, GmfVertices) - nbInitialNodes;
  GMFNode = new const SMDS_MeshNode*[ nbVertices + 1 ];

  std::map <GmfKwdCod,int>::const_iterator it = tabRef.begin();
  for ( ; it != tabRef.end() ; ++it)
  {
    if(theAlgo->computeCanceled()) {
      GmfCloseMesh(InpMsh);
      delete [] GMFNode;
      return false;
    }
    int dummy, solidID;
    GmfKwdCod token = it->first;
    nbRef           = it->second;

    nbElem = GmfStatKwd(InpMsh, token);
    if (nbElem > 0) {
      GmfGotoKwd(InpMsh, token);
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
          GmfCloseMesh(InpMsh);
          delete [] GMFNode;
          return false;
        }
        if (ver == GmfFloat) {
          GmfGetLin(InpMsh, token, &VerTab_f[0], &VerTab_f[1], &VerTab_f[2], &dummy);
          x = VerTab_f[0];
          y = VerTab_f[1];
          z = VerTab_f[2];
        }
        else {
          GmfGetLin(InpMsh, token, &x, &y, &z, &dummy);
        }
        if (iElem >= nbInitialNodes) {
          if ( elemSearcher &&
                elemSearcher->FindElementsByPoint( gp_Pnt(x,y,z), SMDSAbs_Volume, foundVolumes))
            aGMFNode = 0;
          else
            aGMFNode = theHelper->AddNode(x, y, z);
          
          aGMFID = iElem -nbInitialNodes +1;
          GMFNode[ aGMFID ] = aGMFNode;
          if (aGMFID-1 < aNodeGroupByGhs3dId.size() && !aNodeGroupByGhs3dId.at(aGMFID-1).empty())
            addElemInMeshGroup(theHelper->GetMesh(), aGMFNode, aNodeGroupByGhs3dId.at(aGMFID-1), groupsToRemove);
        }
      }
    }
    else if (token == GmfCorners && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " corner" : tmpStr = " corners";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]]);
    }
    else if (token == GmfRidges && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " ridge" : tmpStr = " ridges";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]]);
    }
    else if (token == GmfEdges && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " edge" : tmpStr = " edges";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &domainID[iElem]);
    }
    else if (token == GmfTriangles && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " triangle" : tmpStr = " triangles";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &domainID[iElem]);
    }
    else if (token == GmfQuadrilaterals && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " Quadrilateral" : tmpStr = " Quadrilaterals";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &id[iElem*tabRef[token]+3], &domainID[iElem]);
    }
    else if (token == GmfTetrahedra && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " Tetrahedron" : tmpStr = " Tetrahedra";
      for ( int iElem = 0; iElem < nbElem; iElem++ ) {
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &id[iElem*tabRef[token]+3], &domainID[iElem]);
#ifdef _DEBUG_
        subdomainId2tetraId[dummy].insert(iElem+1);
//         MESSAGE("subdomainId2tetraId["<<dummy<<"].insert("<<iElem+1<<")");
#endif
      }
    }
    else if (token == GmfHexahedra && nbElem > 0) {
      (nbElem <= 1) ? tmpStr = " Hexahedron" : tmpStr = " Hexahedra";
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &id[iElem*tabRef[token]+3],
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
          GmfCloseMesh(InpMsh);
          delete [] GMFNode;
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
    } // switch (token)
  } // loop on tabRef

  // remove nodes in holes
  if ( hasGeom )
  {
    for ( int i = 1; i <= nbVertices; ++i )
      if ( GMFNode[i]->NbInverseElements() == 0 )
        theMeshDS->RemoveFreeNode( GMFNode[i], /*sm=*/0, /*fromGroups=*/false );
  }

  GmfCloseMesh(InpMsh);
  delete [] GMFNode;

  // 0022172: [CEA 790] create the groups corresponding to domains
  if ( toMakeGroupsOfDomains )
    makeDomainGroups( elemsOfDomain, theHelper );

#ifdef _DEBUG_
  MESSAGE("Nb subdomains " << subdomainId2tetraId.size());
  std::map<int, std::set<int> >::const_iterator subdomainIt = subdomainId2tetraId.begin();
  TCollection_AsciiString aSubdomainFileName = theFile;
  aSubdomainFileName = aSubdomainFileName + ".subdomain";
  ofstream aSubdomainFile  ( aSubdomainFileName.ToCString()  , ios::out);

  aSubdomainFile << "Nb subdomains " << subdomainId2tetraId.size() << std::endl;
  for(;subdomainIt != subdomainId2tetraId.end() ; ++subdomainIt) {
    int subdomainId = subdomainIt->first;
    std::set<int> tetraIds = subdomainIt->second;
    MESSAGE("Subdomain #"<<subdomainId<<": "<<tetraIds.size()<<" tetrahedrons");
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


static bool writeGMFFile(const char*                                     theMeshFileName,
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
                         GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues & theEnforcedVertices)
{
  MESSAGE("writeGMFFile w/o geometry");
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
  auto_ptr< SMESH_ElementSearcher > pntCls
    ( SMESH_MeshAlgos::GetElementSearcher(*theMesh->GetMeshDS()));
  
  int nbEnforcedVertices = theEnforcedVertices.size();
  
  // count faces
  int nbFaces = theProxyMesh.NbFaces();
  int nbNodes;
  theFaceByGhs3dId.reserve( nbFaces );
  
  // groups management
  int usedEnforcedNodes = 0;
  std::string gn = "";

  if ( nbFaces == 0 )
    return false;
  
  idx = GmfOpenMesh(theMeshFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
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
#ifdef _DEBUG_
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
#ifdef _DEBUG_
        std::cout << "MG-Tetra node ID: "<<newId<<std::endl;
#endif
      }
      if (isOK)
        theKeptEnforcedEdges.insert(elem);
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
#ifdef _DEBUG_
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
#ifdef _DEBUG_
        std::cout << "MG-Tetra node ID: "<<newId<<std::endl;
#endif
      }
      if (isOK)
        theKeptEnforcedTriangles.insert(elem);
    }
  }
  
  // put nodes to theNodeByGhs3dId vector
#ifdef _DEBUG_
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
#ifdef _DEBUG_
  std::cout << "anEnforcedNodeToGhs3dIdMap.size(): "<<anEnforcedNodeToGhs3dIdMap.size()<<std::endl;
#endif
  theEnforcedNodeByGhs3dId.resize( anEnforcedNodeToGhs3dIdMap.size());
  n2id = anEnforcedNodeToGhs3dIdMap.begin();
  for ( ; n2id != anEnforcedNodeToGhs3dIdMap.end(); ++ n2id)
  {
    if (n2id->second > aNodeToGhs3dIdMap.size()) {
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
#ifdef _DEBUG_
    std::cout << "Node at " << node->X()<<", " <<node->Y()<<", " <<node->Z();
#endif
    
    if (nodesCoords.find(coords) != nodesCoords.end()) {
      // node already exists in original mesh
#ifdef _DEBUG_
      std::cout << " found" << std::endl;
#endif
      continue;
    }
    
    if (theEnforcedVertices.find(coords) != theEnforcedVertices.end()) {
      // node already exists in enforced vertices
#ifdef _DEBUG_
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
    
#ifdef _DEBUG_
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
#ifdef _DEBUG_
    std::cout << "Node at " << node->X()<<", " <<node->Y()<<", " <<node->Z();
#endif
    
    // Test if point is inside shape to mesh
    gp_Pnt myPoint(node->X(),node->Y(),node->Z());
    TopAbs_State result = pntCls->GetPointState( myPoint );
    if ( result == TopAbs_OUT ) {
#ifdef _DEBUG_
      std::cout << " out of volume" << std::endl;
#endif
      continue;
    }
    
    if (nodesCoords.find(coords) != nodesCoords.end()) {
#ifdef _DEBUG_
      std::cout << " found in nodesCoords" << std::endl;
#endif
//       theRequiredNodes.push_back(node);
      continue;
    }

    if (theEnforcedVertices.find(coords) != theEnforcedVertices.end()) {
#ifdef _DEBUG_
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
    
#ifdef _DEBUG_
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
        continue;
      //if (pntCls->FindElementsByPoint(myPoint, SMDSAbs_Node, foundElems) == 0)
      //continue;

//       if ( result != TopAbs_IN )
//         continue;
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
  GmfSetKwd(idx, GmfVertices, theOrderedNodes.size()/*+solSize*/);
  for (ghs3dNodeIt = theOrderedNodes.begin();ghs3dNodeIt != theOrderedNodes.end();++ghs3dNodeIt) {
    GmfSetLin(idx, GmfVertices, (*ghs3dNodeIt)->X(), (*ghs3dNodeIt)->Y(), (*ghs3dNodeIt)->Z(), dummyint);
  }

  std::cout << "End writting required nodes in GmfVertices" << std::endl;

  if (requiredNodes + solSize) {
    std::cout << "Begin writting in req and sol file" << std::endl;
    aNodeGroupByGhs3dId.resize( requiredNodes + solSize );
    idxRequired = GmfOpenMesh(theRequiredFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
    if (!idxRequired) {
      GmfCloseMesh(idx);
      return false;
    }
    idxSol = GmfOpenMesh(theSolFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
    if (!idxSol) {
      GmfCloseMesh(idx);
      if (idxRequired)
        GmfCloseMesh(idxRequired);
      return false;
    }
    int TypTab[] = {GmfSca};
    double ValTab[] = {0.0};
    GmfSetKwd(idxRequired, GmfVertices, requiredNodes + solSize);
    GmfSetKwd(idxSol, GmfSolAtVertices, requiredNodes + solSize, 1, TypTab);
//     int usedEnforcedNodes = 0;
//     std::string gn = "";
    for (ghs3dNodeIt = theRequiredNodes.begin();ghs3dNodeIt != theRequiredNodes.end();++ghs3dNodeIt) {
      GmfSetLin(idxRequired, GmfVertices, (*ghs3dNodeIt)->X(), (*ghs3dNodeIt)->Y(), (*ghs3dNodeIt)->Z(), dummyint);
      GmfSetLin(idxSol, GmfSolAtVertices, ValTab);
      if (theEnforcedNodes.find((*ghs3dNodeIt)) != theEnforcedNodes.end())
        gn = theEnforcedNodes.find((*ghs3dNodeIt))->second;
      aNodeGroupByGhs3dId[usedEnforcedNodes] = gn;
      usedEnforcedNodes++;
    }

    for (int i=0;i<solSize;i++) {
      std::cout << ReqVerTab[i][0] <<" "<< ReqVerTab[i][1] << " "<< ReqVerTab[i][2] << std::endl;
#ifdef _DEBUG_
      std::cout << "enfVertexSizes.at("<<i<<"): " << enfVertexSizes.at(i) << std::endl;
#endif
      double solTab[] = {enfVertexSizes.at(i)};
      GmfSetLin(idxRequired, GmfVertices, ReqVerTab[i][0], ReqVerTab[i][1], ReqVerTab[i][2], dummyint);
      GmfSetLin(idxSol, GmfSolAtVertices, solTab);
      aNodeGroupByGhs3dId[usedEnforcedNodes] = enfVerticesWithGroup.find(ReqVerTab[i])->second;
#ifdef _DEBUG_
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
//    idxRequired = GmfOpenMesh(theRequiredFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
//    if (!idxRequired)
//      return false;
    GmfSetKwd(idx, GmfEdges, theKeptEnforcedEdges.size());
//    GmfSetKwd(idxRequired, GmfEdges, theKeptEnforcedEdges.size());
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
      GmfSetLin(idx, GmfEdges, nedge[0], nedge[1], dummyint);
      anEdgeGroupByGhs3dId[usedEnforcedEdges] = theEnforcedEdges.find(elem)->second;
//      GmfSetLin(idxRequired, GmfEdges, nedge[0], nedge[1], dummyint);
      usedEnforcedEdges++;
    }
//    GmfCloseMesh(idxRequired);
  }


  if (usedEnforcedEdges) {
    GmfSetKwd(idx, GmfRequiredEdges, usedEnforcedEdges);
    for (int enfID=1;enfID<=usedEnforcedEdges;enfID++) {
      GmfSetLin(idx, GmfRequiredEdges, enfID);
    }
  }

  // GmfTriangles
  int usedEnforcedTriangles = 0;
  if (anElemSet.size()+theKeptEnforcedTriangles.size()) {
    aFaceGroupByGhs3dId.resize( anElemSet.size()+theKeptEnforcedTriangles.size() );
    GmfSetKwd(idx, GmfTriangles, anElemSet.size()+theKeptEnforcedTriangles.size());
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
      GmfSetLin(idx, GmfTriangles, ntri[0], ntri[1], ntri[2], dummyint);
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
        GmfSetLin(idx, GmfTriangles, ntri[0], ntri[1], ntri[2], dummyint);
        aFaceGroupByGhs3dId[k] = theEnforcedTriangles.find(elem)->second;
        usedEnforcedTriangles++;
      }
    }
  }

  
  if (usedEnforcedTriangles) {
    GmfSetKwd(idx, GmfRequiredTriangles, usedEnforcedTriangles);
    for (int enfID=1;enfID<=usedEnforcedTriangles;enfID++)
      GmfSetLin(idx, GmfRequiredTriangles, anElemSet.size()+enfID);
  }

  GmfCloseMesh(idx);
  if (idxRequired)
    GmfCloseMesh(idxRequired);
  if (idxSol)
    GmfCloseMesh(idxSol);
  
  return true;
  
}

// static bool writeGMFFile(const char*                                    theMeshFileName,
//                         const char*                                     theRequiredFileName,
//                         const char*                                     theSolFileName,
//                         SMESH_MesherHelper&                             theHelper,
//                         const SMESH_ProxyMesh&                          theProxyMesh,
//                         std::map <int,int> &                            theNodeId2NodeIndexMap,
//                         std::map <int,int> &                            theSmdsToGhs3dIdMap,
//                         std::map <int,const SMDS_MeshNode*> &           theGhs3dIdToNodeMap,
//                         TIDSortedNodeSet &                              theEnforcedNodes,
//                         TIDSortedElemSet &                              theEnforcedEdges,
//                         TIDSortedElemSet &                              theEnforcedTriangles,
// //                         TIDSortedElemSet &                              theEnforcedQuadrangles,
//                         GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues & theEnforcedVertices)
// {
//   MESSAGE("writeGMFFile with geometry");
//   int idx, idxRequired, idxSol;
//   int nbv, nbev, nben, aGhs3dID = 0;
//   const int dummyint = 0;
//   GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues::const_iterator vertexIt;
//   std::vector<double> enfVertexSizes;
//   TIDSortedNodeSet::const_iterator enfNodeIt;
//   const SMDS_MeshNode* node;
//   SMDS_NodeIteratorPtr nodeIt;
// 
//   idx = GmfOpenMesh(theMeshFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
//   if (!idx)
//     return false;
//   
//   SMESHDS_Mesh * theMeshDS = theHelper.GetMeshDS();
//   
//   /* ========================== NODES ========================== */
//   // NB_NODES
//   nbv = theMeshDS->NbNodes();
//   if ( nbv == 0 )
//     return false;
//   nbev = theEnforcedVertices.size();
//   nben = theEnforcedNodes.size();
//   
//   // Issue 020674: EDF 870 SMESH: Mesh generated by Netgen not usable by MG-Tetra
//   // The problem is in nodes on degenerated edges, we need to skip nodes which are free
//   // and replace not-free nodes on edges by the node on vertex
//   TNodeNodeMap n2nDegen; // map a node on degenerated edge to a node on vertex
//   TNodeNodeMap::iterator n2nDegenIt;
//   if ( theHelper.HasDegeneratedEdges() )
//   {
//     set<int> checkedSM;
//     for (TopExp_Explorer e(theMeshDS->ShapeToMesh(), TopAbs_EDGE ); e.More(); e.Next())
//     {
//       SMESH_subMesh* sm = theHelper.GetMesh()->GetSubMesh( e.Current() );
//       if ( checkedSM.insert( sm->GetId() ).second && theHelper.IsDegenShape(sm->GetId() ))
//       {
//         if ( SMESHDS_SubMesh* smDS = sm->GetSubMeshDS() )
//         {
//           TopoDS_Shape vertex = TopoDS_Iterator( e.Current() ).Value();
//           const SMDS_MeshNode* vNode = SMESH_Algo::VertexNode( TopoDS::Vertex( vertex ), theMeshDS);
//           {
//             SMDS_NodeIteratorPtr nIt = smDS->GetNodes();
//             while ( nIt->more() )
//               n2nDegen.insert( make_pair( nIt->next(), vNode ));
//           }
//         }
//       }
//     }
//   }
//   
//   const bool isQuadMesh = 
//     theHelper.GetMesh()->NbEdges( ORDER_QUADRATIC ) ||
//     theHelper.GetMesh()->NbFaces( ORDER_QUADRATIC ) ||
//     theHelper.GetMesh()->NbVolumes( ORDER_QUADRATIC );
// 
//   std::vector<std::vector<double> > VerTab;
//   std::set<std::vector<double> > VerMap;
//   VerTab.clear();
//   std::vector<double> aVerTab;
//   // Loop from 1 to NB_NODES
// 
//   nodeIt = theMeshDS->nodesIterator();
//   
//   while ( nodeIt->more() )
//   {
//     node = nodeIt->next();
//     if ( isQuadMesh && theHelper.IsMedium( node )) // Issue 0021238
//       continue;
//     if ( n2nDegen.count( node ) ) // Issue 0020674
//       continue;
// 
//     std::vector<double> coords;
//     coords.push_back(node->X());
//     coords.push_back(node->Y());
//     coords.push_back(node->Z());
//     if (VerMap.find(coords) != VerMap.end()) {
//       aGhs3dID = theSmdsToGhs3dIdMap[node->GetID()];
//       theGhs3dIdToNodeMap[theSmdsToGhs3dIdMap[node->GetID()]] = node;
//       continue;
//     }
//     VerTab.push_back(coords);
//     VerMap.insert(coords);
//     aGhs3dID++;
//     theSmdsToGhs3dIdMap.insert( make_pair( node->GetID(), aGhs3dID ));
//     theGhs3dIdToNodeMap.insert( make_pair( aGhs3dID, node ));
//   }
//   
//   
//   /* ENFORCED NODES ========================== */
//   if (nben) {
//     std::cout << "Add " << nben << " enforced nodes to input .mesh file" << std::endl;
//     for(enfNodeIt = theEnforcedNodes.begin() ; enfNodeIt != theEnforcedNodes.end() ; ++enfNodeIt) {
//       double x = (*enfNodeIt)->X();
//       double y = (*enfNodeIt)->Y();
//       double z = (*enfNodeIt)->Z();
//       // Test if point is inside shape to mesh
//       gp_Pnt myPoint(x,y,z);
//       BRepClass3d_SolidClassifier scl(theMeshDS->ShapeToMesh());
//       scl.Perform(myPoint, 1e-7);
//       TopAbs_State result = scl.State();
//       if ( result != TopAbs_IN )
//         continue;
//       std::vector<double> coords;
//       coords.push_back(x);
//       coords.push_back(y);
//       coords.push_back(z);
//       if (theEnforcedVertices.find(coords) != theEnforcedVertices.end())
//         continue;
//       if (VerMap.find(coords) != VerMap.end())
//         continue;
//       VerTab.push_back(coords);
//       VerMap.insert(coords);
//       aGhs3dID++;
//       theNodeId2NodeIndexMap.insert( make_pair( (*enfNodeIt)->GetID(), aGhs3dID ));
//     }
//   }
//     
//   
//   /* ENFORCED VERTICES ========================== */
//   int solSize = 0;
//   std::vector<std::vector<double> > ReqVerTab;
//   ReqVerTab.clear();
//   if (nbev) {
//     std::cout << "Add " << nbev << " enforced vertices to input .mesh file" << std::endl;
//     for(vertexIt = theEnforcedVertices.begin() ; vertexIt != theEnforcedVertices.end() ; ++vertexIt) {
//       double x = vertexIt->first[0];
//       double y = vertexIt->first[1];
//       double z = vertexIt->first[2];
//       // Test if point is inside shape to mesh
//       gp_Pnt myPoint(x,y,z);
//       BRepClass3d_SolidClassifier scl(theMeshDS->ShapeToMesh());
//       scl.Perform(myPoint, 1e-7);
//       TopAbs_State result = scl.State();
//       if ( result != TopAbs_IN )
//         continue;
//       enfVertexSizes.push_back(vertexIt->second);
//       std::vector<double> coords;
//       coords.push_back(x);
//       coords.push_back(y);
//       coords.push_back(z);
//       if (VerMap.find(coords) != VerMap.end())
//         continue;
//       ReqVerTab.push_back(coords);
//       VerMap.insert(coords);
//       solSize++;
//     }
//   }
// 
//   
//   /* ========================== FACES ========================== */
//   
//   int nbTriangles = 0/*, nbQuadrangles = 0*/, aSmdsID;
//   TopTools_IndexedMapOfShape facesMap, trianglesMap/*, quadranglesMap*/;
//   TIDSortedElemSet::const_iterator elemIt;
//   const SMESHDS_SubMesh* theSubMesh;
//   TopoDS_Shape aShape;
//   SMDS_ElemIteratorPtr itOnSubMesh, itOnSubFace;
//   const SMDS_MeshElement* aFace;
//   map<int,int>::const_iterator itOnMap;
//   std::vector<std::vector<int> > tt, qt,et;
//   tt.clear();
//   qt.clear();
//   et.clear();
//   std::vector<int> att, aqt, aet;
//   
//   TopExp::MapShapes( theMeshDS->ShapeToMesh(), TopAbs_FACE, facesMap );
// 
//   for ( int i = 1; i <= facesMap.Extent(); ++i )
//     if (( theSubMesh  = theProxyMesh.GetSubMesh( facesMap(i))))
//     {
//       SMDS_ElemIteratorPtr it = theSubMesh->GetElements();
//       while (it->more())
//       {
//         const SMDS_MeshElement *elem = it->next();
//         int nbCornerNodes = elem->NbCornerNodes();
//         if (nbCornerNodes == 3)
//         {
//           trianglesMap.Add(facesMap(i));
//           nbTriangles ++;
//         }
// //         else if (nbCornerNodes == 4)
// //         {
// //           quadranglesMap.Add(facesMap(i));
// //           nbQuadrangles ++;
// //         }
//       }
//     }
//     
//   /* TRIANGLES ========================== */
//   if (nbTriangles) {
//     for ( int i = 1; i <= trianglesMap.Extent(); i++ )
//     {
//       aShape = trianglesMap(i);
//       theSubMesh = theProxyMesh.GetSubMesh(aShape);
//       if ( !theSubMesh ) continue;
//       itOnSubMesh = theSubMesh->GetElements();
//       while ( itOnSubMesh->more() )
//       {
//         aFace = itOnSubMesh->next();
//         itOnSubFace = aFace->nodesIterator();
//         att.clear();
//         for ( int j = 0; j < 3; ++j ) {
//           // find MG-Tetra ID
//           node = castToNode( itOnSubFace->next() );
//           if (( n2nDegenIt = n2nDegen.find( node )) != n2nDegen.end() )
//             node = n2nDegenIt->second;
//           aSmdsID = node->GetID();
//           itOnMap = theSmdsToGhs3dIdMap.find( aSmdsID );
//           ASSERT( itOnMap != theSmdsToGhs3dIdMap.end() );
//           att.push_back((*itOnMap).second);
//         }
//         tt.push_back(att);
//       }
//     }
//   }
// 
//   if (theEnforcedTriangles.size()) {
//     std::cout << "Add " << theEnforcedTriangles.size() << " enforced triangles to input .mesh file" << std::endl;
//     // Iterate over the enforced triangles
//     for(elemIt = theEnforcedTriangles.begin() ; elemIt != theEnforcedTriangles.end() ; ++elemIt) {
//       aFace = (*elemIt);
//       itOnSubFace = aFace->nodesIterator();
//       bool isOK = true;
//       att.clear();
//       
//       for ( int j = 0; j < 3; ++j ) {
//         node = castToNode( itOnSubFace->next() );
//         if (( n2nDegenIt = n2nDegen.find( node )) != n2nDegen.end() )
//           node = n2nDegenIt->second;
// //         std::cout << node;
//         double x = node->X();
//         double y = node->Y();
//         double z = node->Z();
//         // Test if point is inside shape to mesh
//         gp_Pnt myPoint(x,y,z);
//         BRepClass3d_SolidClassifier scl(theMeshDS->ShapeToMesh());
//         scl.Perform(myPoint, 1e-7);
//         TopAbs_State result = scl.State();
//         if ( result != TopAbs_IN ) {
//           isOK = false;
//           theEnforcedTriangles.erase(elemIt);
//           continue;
//         }
//         std::vector<double> coords;
//         coords.push_back(x);
//         coords.push_back(y);
//         coords.push_back(z);
//         if (VerMap.find(coords) != VerMap.end()) {
//           att.push_back(theNodeId2NodeIndexMap[node->GetID()]);
//           continue;
//         }
//         VerTab.push_back(coords);
//         VerMap.insert(coords);
//         aGhs3dID++;
//         theNodeId2NodeIndexMap.insert( make_pair( node->GetID(), aGhs3dID ));
//         att.push_back(aGhs3dID);
//       }
//       if (isOK)
//         tt.push_back(att);
//     }
//   }
// 
// 
//   /* ========================== EDGES ========================== */
// 
//   if (theEnforcedEdges.size()) {
//     // Iterate over the enforced edges
//     std::cout << "Add " << theEnforcedEdges.size() << " enforced edges to input .mesh file" << std::endl;
//     for(elemIt = theEnforcedEdges.begin() ; elemIt != theEnforcedEdges.end() ; ++elemIt) {
//       aFace = (*elemIt);
//       bool isOK = true;
//       itOnSubFace = aFace->nodesIterator();
//       aet.clear();
//       for ( int j = 0; j < 2; ++j ) {
//         node = castToNode( itOnSubFace->next() );
//         if (( n2nDegenIt = n2nDegen.find( node )) != n2nDegen.end() )
//           node = n2nDegenIt->second;
//         double x = node->X();
//         double y = node->Y();
//         double z = node->Z();
//         // Test if point is inside shape to mesh
//         gp_Pnt myPoint(x,y,z);
//         BRepClass3d_SolidClassifier scl(theMeshDS->ShapeToMesh());
//         scl.Perform(myPoint, 1e-7);
//         TopAbs_State result = scl.State();
//         if ( result != TopAbs_IN ) {
//           isOK = false;
//           theEnforcedEdges.erase(elemIt);
//           continue;
//         }
//         std::vector<double> coords;
//         coords.push_back(x);
//         coords.push_back(y);
//         coords.push_back(z);
//         if (VerMap.find(coords) != VerMap.end()) {
//           aet.push_back(theNodeId2NodeIndexMap[node->GetID()]);
//           continue;
//         }
//         VerTab.push_back(coords);
//         VerMap.insert(coords);
//         
//         aGhs3dID++;
//         theNodeId2NodeIndexMap.insert( make_pair( node->GetID(), aGhs3dID ));
//         aet.push_back(aGhs3dID);
//       }
//       if (isOK)
//         et.push_back(aet);
//     }
//   }
// 
// 
//   /* Write vertices number */
//   MESSAGE("Number of vertices: "<<aGhs3dID);
//   MESSAGE("Size of vector: "<<VerTab.size());
//   GmfSetKwd(idx, GmfVertices, aGhs3dID/*+solSize*/);
//   for (int i=0;i<aGhs3dID;i++)
//     GmfSetLin(idx, GmfVertices, VerTab[i][0], VerTab[i][1], VerTab[i][2], dummyint);
// //   for (int i=0;i<solSize;i++) {
// //     std::cout << ReqVerTab[i][0] <<" "<< ReqVerTab[i][1] << " "<< ReqVerTab[i][2] << std::endl;
// //     GmfSetLin(idx, GmfVertices, ReqVerTab[i][0], ReqVerTab[i][1], ReqVerTab[i][2], dummyint);
// //   }
// 
//   if (solSize) {
//     idxRequired = GmfOpenMesh(theRequiredFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
//     if (!idxRequired) {
//       GmfCloseMesh(idx);
//       return false;
//     }
//     idxSol = GmfOpenMesh(theSolFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
//     if (!idxSol){
//       GmfCloseMesh(idx);
//       if (idxRequired)
//         GmfCloseMesh(idxRequired);
//       return false;
//     }
//     
//     int TypTab[] = {GmfSca};
//     GmfSetKwd(idxRequired, GmfVertices, solSize);
//     GmfSetKwd(idxSol, GmfSolAtVertices, solSize, 1, TypTab);
//     
//     for (int i=0;i<solSize;i++) {
//       double solTab[] = {enfVertexSizes.at(i)};
//       GmfSetLin(idxRequired, GmfVertices, ReqVerTab[i][0], ReqVerTab[i][1], ReqVerTab[i][2], dummyint);
//       GmfSetLin(idxSol, GmfSolAtVertices, solTab);
//     }
//     GmfCloseMesh(idxRequired);
//     GmfCloseMesh(idxSol);
//   }
//   
//   /* Write triangles number */
//   if (tt.size()) {
//     GmfSetKwd(idx, GmfTriangles, tt.size());
//     for (int i=0;i<tt.size();i++)
//       GmfSetLin(idx, GmfTriangles, tt[i][0], tt[i][1], tt[i][2], dummyint);
//   }  
//   
//   /* Write edges number */
//   if (et.size()) {
//     GmfSetKwd(idx, GmfEdges, et.size());
//     for (int i=0;i<et.size();i++)
//       GmfSetLin(idx, GmfEdges, et[i][0], et[i][1], dummyint);
//   }
// 
//   /* QUADRANGLES ========================== */
//   // TODO: add pyramids ?
// //   if (nbQuadrangles) {
// //     for ( int i = 1; i <= quadranglesMap.Extent(); i++ )
// //     {
// //       aShape = quadranglesMap(i);
// //       theSubMesh = theProxyMesh.GetSubMesh(aShape);
// //       if ( !theSubMesh ) continue;
// //       itOnSubMesh = theSubMesh->GetElements();
// //       for ( int j = 0; j < 4; ++j )
// //       {
// //         aFace = itOnSubMesh->next();
// //         itOnSubFace = aFace->nodesIterator();
// //         aqt.clear();
// //         while ( itOnSubFace->more() ) {
// //           // find MG-Tetra ID
// //           aSmdsID = itOnSubFace->next()->GetID();
// //           itOnMap = theSmdsToGhs3dIdMap.find( aSmdsID );
// //           ASSERT( itOnMap != theSmdsToGhs3dIdMap.end() );
// //           aqt.push_back((*itOnMap).second);
// //         }
// //         qt.push_back(aqt);
// //       }
// //     }
// //   }
// // 
// //   if (theEnforcedQuadrangles.size()) {
// //     // Iterate over the enforced triangles
// //     for(elemIt = theEnforcedQuadrangles.begin() ; elemIt != theEnforcedQuadrangles.end() ; ++elemIt) {
// //       aFace = (*elemIt);
// //       bool isOK = true;
// //       itOnSubFace = aFace->nodesIterator();
// //       aqt.clear();
// //       for ( int j = 0; j < 4; ++j ) {
// //         int aNodeID = itOnSubFace->next()->GetID();
// //         itOnMap = theNodeId2NodeIndexMap.find(aNodeID);
// //         if (itOnMap != theNodeId2NodeIndexMap.end())
// //           aqt.push_back((*itOnMap).second);
// //         else {
// //           isOK = false;
// //           theEnforcedQuadrangles.erase(elemIt);
// //           break;
// //         }
// //       }
// //       if (isOK)
// //         qt.push_back(aqt);
// //     }
// //   }
// //  
//   
// //   /* Write quadrilaterals number */
// //   if (qt.size()) {
// //     GmfSetKwd(idx, GmfQuadrilaterals, qt.size());
// //     for (int i=0;i<qt.size();i++)
// //       GmfSetLin(idx, GmfQuadrilaterals, qt[i][0], qt[i][1], qt[i][2], qt[i][3], dummyint);
// //   }
// 
//   GmfCloseMesh(idx);
//   return true;
// }


//=======================================================================
//function : writeFaces
//purpose  : 
//=======================================================================

static bool writeFaces (ofstream &              theFile,
                        const SMESH_ProxyMesh&  theMesh,
                        const TopoDS_Shape&     theShape,
                        const map <int,int> &   theSmdsToGhs3dIdMap,
                        const map <int,int> &   theEnforcedNodeIdToGhs3dIdMap,
                        GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap & theEnforcedEdges,
                        GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap & theEnforcedTriangles)
{
  // record structure:
  //
  // NB_ELEMS DUMMY_INT
  // Loop from 1 to NB_ELEMS
  // NB_NODES NODE_NB_1 NODE_NB_2 ... (NB_NODES + 1) times: DUMMY_INT

  TopoDS_Shape aShape;
  const SMESHDS_SubMesh* theSubMesh;
  const SMDS_MeshElement* aFace;
  const char* space    = "  ";
  const int   dummyint = 0;
  map<int,int>::const_iterator itOnMap;
  SMDS_ElemIteratorPtr itOnSubMesh, itOnSubFace;
  int nbNodes, aSmdsID;

  TIDSortedElemSet::const_iterator elemIt;
  int nbEnforcedEdges       = theEnforcedEdges.size();
  int nbEnforcedTriangles   = theEnforcedTriangles.size();

  // count triangles bound to geometry
  int nbTriangles = 0;

  TopTools_IndexedMapOfShape facesMap, trianglesMap;
  TopExp::MapShapes( theShape, TopAbs_FACE, facesMap );
  
  int nbFaces = facesMap.Extent();

  for ( int i = 1; i <= nbFaces; ++i )
    if (( theSubMesh  = theMesh.GetSubMesh( facesMap(i))))
      nbTriangles += theSubMesh->NbElements();
  std::string tmpStr;
  (nbFaces == 0 || nbFaces == 1) ? tmpStr = " shape " : tmpStr = " shapes " ;
  std::cout << "    " << nbFaces << tmpStr << "of 2D dimension";
  int nbEnforcedElements = nbEnforcedEdges+nbEnforcedTriangles;
  if (nbEnforcedElements > 0) {
    (nbEnforcedElements == 1) ? tmpStr = "shape:" : tmpStr = "shapes:";
    std::cout << " and" << std::endl;
    std::cout << "    " << nbEnforcedElements 
                        << " enforced " << tmpStr << std::endl;
  }
  else
    std::cout << std::endl;
  if (nbEnforcedEdges) {
    (nbEnforcedEdges == 1) ? tmpStr = "edge" : tmpStr = "edges";
    std::cout << "      " << nbEnforcedEdges << " enforced " << tmpStr << std::endl;
  }
  if (nbEnforcedTriangles) {
    (nbEnforcedTriangles == 1) ? tmpStr = "triangle" : tmpStr = "triangles";
    std::cout << "      " << nbEnforcedTriangles << " enforced " << tmpStr << std::endl;
  }
  std::cout << std::endl;

//   theFile << space << nbTriangles << space << dummyint << std::endl;
  std::ostringstream globalStream, localStream, aStream;

  for ( int i = 1; i <= facesMap.Extent(); i++ )
  {
    aShape = facesMap(i);
    theSubMesh = theMesh.GetSubMesh(aShape);
    if ( !theSubMesh ) continue;
    itOnSubMesh = theSubMesh->GetElements();
    while ( itOnSubMesh->more() )
    {
      aFace = itOnSubMesh->next();
      nbNodes = aFace->NbCornerNodes();

      localStream << nbNodes << space;

      itOnSubFace = aFace->nodesIterator();
      for ( int j = 0; j < 3; ++j ) {
        // find MG-Tetra ID
        aSmdsID = itOnSubFace->next()->GetID();
        itOnMap = theSmdsToGhs3dIdMap.find( aSmdsID );
        // if ( itOnMap == theSmdsToGhs3dIdMap.end() ) {
        //   cout << "not found node: " << aSmdsID << endl;
        //   return false;
        // }
        ASSERT( itOnMap != theSmdsToGhs3dIdMap.end() );

        localStream << (*itOnMap).second << space ;
      }

      // (NB_NODES + 1) times: DUMMY_INT
      for ( int j=0; j<=nbNodes; j++)
        localStream << dummyint << space ;

      localStream << std::endl;
    }
  }
  
  globalStream << localStream.str();
  localStream.str("");

  //
  //        FACES : END
  //

//   //
//   //        ENFORCED EDGES : BEGIN
//   //
//   
//   // Iterate over the enforced edges
//   int usedEnforcedEdges = 0;
//   bool isOK;
//   for(elemIt = theEnforcedEdges.begin() ; elemIt != theEnforcedEdges.end() ; ++elemIt) {
//     aFace = (*elemIt);
//     isOK = true;
//     itOnSubFace = aFace->nodesIterator();
//     aStream.str("");
//     aStream << "2" << space ;
//     for ( int j = 0; j < 2; ++j ) {
//       aSmdsID = itOnSubFace->next()->GetID();
//       itOnMap = theEnforcedNodeIdToGhs3dIdMap.find(aSmdsID);
//       if (itOnMap != theEnforcedNodeIdToGhs3dIdMap.end())
//         aStream << (*itOnMap).second << space;
//       else {
//         isOK = false;
//         break;
//       }
//     }
//     if (isOK) {
//       for ( int j=0; j<=2; j++)
//         aStream << dummyint << space ;
// //       aStream << dummyint << space << dummyint;
//       localStream << aStream.str() << std::endl;
//       usedEnforcedEdges++;
//     }
//   }
//   
//   if (usedEnforcedEdges) {
//     globalStream << localStream.str();
//     localStream.str("");
//   }
// 
//   //
//   //        ENFORCED EDGES : END
//   //
//   //
// 
//   //
//   //        ENFORCED TRIANGLES : BEGIN
//   //
//     // Iterate over the enforced triangles
//   int usedEnforcedTriangles = 0;
//   for(elemIt = theEnforcedTriangles.begin() ; elemIt != theEnforcedTriangles.end() ; ++elemIt) {
//     aFace = (*elemIt);
//     nbNodes = aFace->NbCornerNodes();
//     isOK = true;
//     itOnSubFace = aFace->nodesIterator();
//     aStream.str("");
//     aStream << nbNodes << space ;
//     for ( int j = 0; j < 3; ++j ) {
//       aSmdsID = itOnSubFace->next()->GetID();
//       itOnMap = theEnforcedNodeIdToGhs3dIdMap.find(aSmdsID);
//       if (itOnMap != theEnforcedNodeIdToGhs3dIdMap.end())
//         aStream << (*itOnMap).second << space;
//       else {
//         isOK = false;
//         break;
//       }
//     }
//     if (isOK) {
//       for ( int j=0; j<=3; j++)
//         aStream << dummyint << space ;
//       localStream << aStream.str() << std::endl;
//       usedEnforcedTriangles++;
//     }
//   }
//   
//   if (usedEnforcedTriangles) {
//     globalStream << localStream.str();
//     localStream.str("");
//   }
// 
//   //
//   //        ENFORCED TRIANGLES : END
//   //
  
  theFile
  << nbTriangles/*+usedEnforcedTriangles+usedEnforcedEdges*/
  << " 0" << std::endl
  << globalStream.str();

  return true;
}

//=======================================================================
//function : writePoints
//purpose  : 
//=======================================================================

static bool writePoints (ofstream &                       theFile,
                         SMESH_MesherHelper&              theHelper,
                         map <int,int> &                  theSmdsToGhs3dIdMap,
                         map <int,int> &                  theEnforcedNodeIdToGhs3dIdMap,
                         map <int,const SMDS_MeshNode*> & theGhs3dIdToNodeMap,
                         GHS3DPlugin_Hypothesis::TID2SizeMap & theNodeIDToSizeMap,
                         GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues & theEnforcedVertices,
                         GHS3DPlugin_Hypothesis::TIDSortedNodeGroupMap & theEnforcedNodes,
                         GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap & theEnforcedEdges,
                         GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap & theEnforcedTriangles)
{
  // record structure:
  //
  // NB_NODES
  // Loop from 1 to NB_NODES
  //   X Y Z DUMMY_INT

  SMESHDS_Mesh * theMeshDS = theHelper.GetMeshDS();
  int nbNodes = theMeshDS->NbNodes();
  if ( nbNodes == 0 )
    return false;
  
  int nbEnforcedVertices = theEnforcedVertices.size();
  int nbEnforcedNodes    = theEnforcedNodes.size();
  
  const TopoDS_Shape shapeToMesh = theMeshDS->ShapeToMesh();
  
  int aGhs3dID = 1;
  SMDS_NodeIteratorPtr nodeIt = theMeshDS->nodesIterator();
  const SMDS_MeshNode* node;

  // Issue 020674: EDF 870 SMESH: Mesh generated by Netgen not usable by MG-Tetra
  // The problem is in nodes on degenerated edges, we need to skip nodes which are free
  // and replace not-free nodes on degenerated edges by the node on vertex
  TNodeNodeMap n2nDegen; // map a node on degenerated edge to a node on vertex
  TNodeNodeMap::iterator n2nDegenIt;
  if ( theHelper.HasDegeneratedEdges() )
  {
    set<int> checkedSM;
    for (TopExp_Explorer e(theMeshDS->ShapeToMesh(), TopAbs_EDGE ); e.More(); e.Next())
    {
      SMESH_subMesh* sm = theHelper.GetMesh()->GetSubMesh( e.Current() );
      if ( checkedSM.insert( sm->GetId() ).second && theHelper.IsDegenShape(sm->GetId() ))
      {
        if ( SMESHDS_SubMesh* smDS = sm->GetSubMeshDS() )
        {
          TopoDS_Shape vertex = TopoDS_Iterator( e.Current() ).Value();
          const SMDS_MeshNode* vNode = SMESH_Algo::VertexNode( TopoDS::Vertex( vertex ), theMeshDS);
          {
            SMDS_NodeIteratorPtr nIt = smDS->GetNodes();
            while ( nIt->more() )
              n2nDegen.insert( make_pair( nIt->next(), vNode ));
          }
        }
      }
    }
    nbNodes -= n2nDegen.size();
  }

  const bool isQuadMesh = 
    theHelper.GetMesh()->NbEdges( ORDER_QUADRATIC ) ||
    theHelper.GetMesh()->NbFaces( ORDER_QUADRATIC ) ||
    theHelper.GetMesh()->NbVolumes( ORDER_QUADRATIC );
  if ( isQuadMesh )
  {
    // descrease nbNodes by nb of medium nodes
    while ( nodeIt->more() )
    {
      node = nodeIt->next();
      if ( !theHelper.IsDegenShape( node->getshapeId() ))
        nbNodes -= int( theHelper.IsMedium( node ));
    }
    nodeIt = theMeshDS->nodesIterator();
  }

  const char* space    = "  ";
  const int   dummyint = 0;

  std::string tmpStr;
  (nbNodes == 0 || nbNodes == 1) ? tmpStr = " node" : tmpStr = " nodes";
  // NB_NODES
  std::cout << std::endl;
  std::cout << "The initial 2D mesh contains :" << std::endl;
  std::cout << "    " << nbNodes << tmpStr << std::endl;
  if (nbEnforcedVertices > 0) {
    (nbEnforcedVertices == 1) ? tmpStr = "vertex" : tmpStr = "vertices";
    std::cout << "    " << nbEnforcedVertices << " enforced " << tmpStr << std::endl;
  }
  if (nbEnforcedNodes > 0) {
    (nbEnforcedNodes == 1) ? tmpStr = "node" : tmpStr = "nodes";
    std::cout << "    " << nbEnforcedNodes << " enforced " << tmpStr << std::endl;
  }
  std::cout << std::endl;
  std::cout << "Start writing in 'points' file ..." << std::endl;

  theFile << nbNodes << std::endl;

  // Loop from 1 to NB_NODES

  while ( nodeIt->more() )
  {
    node = nodeIt->next();
    if ( isQuadMesh && theHelper.IsMedium( node )) // Issue 0021238
      continue;
    if ( n2nDegen.count( node ) ) // Issue 0020674
      continue;

    theSmdsToGhs3dIdMap.insert( make_pair( node->GetID(), aGhs3dID ));
    theGhs3dIdToNodeMap.insert( make_pair( aGhs3dID, node ));
    aGhs3dID++;

    // X Y Z DUMMY_INT
    theFile
    << node->X() << space 
    << node->Y() << space 
    << node->Z() << space 
    << dummyint;

    theFile << std::endl;

  }
  
  // Iterate over the enforced nodes
  std::map<int,double> enfVertexIndexSizeMap;
  if (nbEnforcedNodes) {
    GHS3DPlugin_Hypothesis::TIDSortedNodeGroupMap::const_iterator nodeIt = theEnforcedNodes.begin();
    for( ; nodeIt != theEnforcedNodes.end() ; ++nodeIt) {
      double x = nodeIt->first->X();
      double y = nodeIt->first->Y();
      double z = nodeIt->first->Z();
      // Test if point is inside shape to mesh
      gp_Pnt myPoint(x,y,z);
      BRepClass3d_SolidClassifier scl(shapeToMesh);
      scl.Perform(myPoint, 1e-7);
      TopAbs_State result = scl.State();
      if ( result != TopAbs_IN )
        continue;
      std::vector<double> coords;
      coords.push_back(x);
      coords.push_back(y);
      coords.push_back(z);
      if (theEnforcedVertices.find(coords) != theEnforcedVertices.end())
        continue;
        
//      double size = theNodeIDToSizeMap.find(nodeIt->first->GetID())->second;
  //       theGhs3dIdToNodeMap.insert( make_pair( nbNodes + i, (*nodeIt) ));
  //       MESSAGE("Adding enforced node (" << x << "," << y <<"," << z << ")");
      // X Y Z PHY_SIZE DUMMY_INT
      theFile
      << x << space 
      << y << space 
      << z << space
      << -1 << space
      << dummyint << space;
      theFile << std::endl;
      theEnforcedNodeIdToGhs3dIdMap.insert( make_pair( nodeIt->first->GetID(), aGhs3dID ));
      enfVertexIndexSizeMap[aGhs3dID] = -1;
      aGhs3dID++;
  //     else
  //         MESSAGE("Enforced vertex (" << x << "," << y <<"," << z << ") is not inside the geometry: it was not added ");
    }
  }
  
  if (nbEnforcedVertices) {
    // Iterate over the enforced vertices
    GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues::const_iterator vertexIt = theEnforcedVertices.begin();
    for( ; vertexIt != theEnforcedVertices.end() ; ++vertexIt) {
      double x = vertexIt->first[0];
      double y = vertexIt->first[1];
      double z = vertexIt->first[2];
      // Test if point is inside shape to mesh
      gp_Pnt myPoint(x,y,z);
      BRepClass3d_SolidClassifier scl(shapeToMesh);
      scl.Perform(myPoint, 1e-7);
      TopAbs_State result = scl.State();
      if ( result != TopAbs_IN )
        continue;
      MESSAGE("Adding enforced vertex (" << x << "," << y <<"," << z << ") = " << vertexIt->second);
      // X Y Z PHY_SIZE DUMMY_INT
      theFile
      << x << space 
      << y << space 
      << z << space
      << vertexIt->second << space 
      << dummyint << space;
      theFile << std::endl;
      enfVertexIndexSizeMap[aGhs3dID] = vertexIt->second;
      aGhs3dID++;
    }
  }
  
  
  std::cout << std::endl;
  std::cout << "End writing in 'points' file." << std::endl;

  return true;
}

//=======================================================================
//function : readResultFile
//purpose  : readResultFile with geometry
//=======================================================================

// static bool readResultFile(const int                       fileOpen,
// #ifdef WIN32
//                            const char*                     fileName,
// #endif
//                            GHS3DPlugin_GHS3D*              theAlgo,
//                            SMESH_MesherHelper&             theHelper,
//                            TopoDS_Shape                    tabShape[],
//                            double**                        tabBox,
//                            const int                       nbShape,
//                            map <int,const SMDS_MeshNode*>& theGhs3dIdToNodeMap,
//                            std::map <int,int> &            theNodeId2NodeIndexMap,
//                            bool                            toMeshHoles,
//                            int                             nbEnforcedVertices,
//                            int                             nbEnforcedNodes,
//                            GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap & theEnforcedEdges,
//                            GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap & theEnforcedTriangles,
//                            bool                            toMakeGroupsOfDomains)
// {
//   MESSAGE("GHS3DPlugin_GHS3D::readResultFile()");
//   Kernel_Utils::Localizer loc;
//   struct stat status;
//   size_t      length;
  
//   std::string tmpStr;

//   char *ptr, *mapPtr;
//   char *tetraPtr;
//   char *shapePtr;

//   SMESHDS_Mesh* theMeshDS = theHelper.GetMeshDS();

//   int nbElems, nbNodes, nbInputNodes;
//   int nbTriangle;
//   int ID, shapeID, ghs3dShapeID;
//   int IdShapeRef = 1;
//   int compoundID =
//     nbShape ? theMeshDS->ShapeToIndex( tabShape[0] ) : theMeshDS->ShapeToIndex( theMeshDS->ShapeToMesh() );

//   int *tab, *tabID, *nodeID, *nodeAssigne;
//   double *coord;
//   const SMDS_MeshNode **node;

//   tab    = new int[3];
//   nodeID = new int[4];
//   coord  = new double[3];
//   node   = new const SMDS_MeshNode*[4];

//   TopoDS_Shape aSolid;
//   SMDS_MeshNode * aNewNode;
//   map <int,const SMDS_MeshNode*>::iterator itOnNode;
//   SMDS_MeshElement* aTet;
// #ifdef _DEBUG_
//   set<int> shapeIDs;
// #endif

//   // Read the file state
//   fstat(fileOpen, &status);
//   length   = status.st_size;

//   // Mapping the result file into memory
// #ifdef WIN32
//   HANDLE fd = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ,
//                          NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//   HANDLE hMapObject = CreateFileMapping(fd, NULL, PAGE_READONLY,
//                                         0, (DWORD)length, NULL);
//   ptr = ( char* ) MapViewOfFile(hMapObject, FILE_MAP_READ, 0, 0, 0 );
// #else
//   ptr = (char *) mmap(0,length,PROT_READ,MAP_PRIVATE,fileOpen,0);
// #endif
//   mapPtr = ptr;

//   ptr      = readMapIntLine(ptr, tab);
//   tetraPtr = ptr;

//   nbElems      = tab[0];
//   nbNodes      = tab[1];
//   nbInputNodes = tab[2];

//   nodeAssigne = new int[ nbNodes+1 ];

//   if (nbShape > 0)
//     aSolid = tabShape[0];

//   // Reading the nodeId
//   for (int i=0; i < 4*nbElems; i++)
//     strtol(ptr, &ptr, 10);

//   MESSAGE("nbInputNodes: "<<nbInputNodes);
//   MESSAGE("nbEnforcedVertices: "<<nbEnforcedVertices);
//   MESSAGE("nbEnforcedNodes: "<<nbEnforcedNodes);
//   // Reading the nodeCoor and update the nodeMap
//   for (int iNode=1; iNode <= nbNodes; iNode++) {
//     if(theAlgo->computeCanceled())
//       return false;
//     for (int iCoor=0; iCoor < 3; iCoor++)
//       coord[ iCoor ] = strtod(ptr, &ptr);
//     nodeAssigne[ iNode ] = 1;
//     if ( iNode > (nbInputNodes-(nbEnforcedVertices+nbEnforcedNodes)) ) {
//       // Creating SMESH nodes
//       // - for enforced vertices
//       // - for vertices of forced edges
//       // - for MG-Tetra nodes
//       nodeAssigne[ iNode ] = 0;
//       aNewNode = theMeshDS->AddNode( coord[0],coord[1],coord[2] );
//       theGhs3dIdToNodeMap.insert(theGhs3dIdToNodeMap.end(), make_pair( iNode, aNewNode ));
//     }
//   }

//   // Reading the number of triangles which corresponds to the number of sub-domains
//   nbTriangle = strtol(ptr, &ptr, 10);

//   tabID = new int[nbTriangle];
//   for (int i=0; i < nbTriangle; i++) {
//     if(theAlgo->computeCanceled())
//       return false;
//     tabID[i] = 0;
//     // find the solid corresponding to MG-Tetra sub-domain following
//     // the technique proposed in MG-Tetra manual in chapter
//     // "B.4 Subdomain (sub-region) assignment"
//     int nodeId1 = strtol(ptr, &ptr, 10);
//     int nodeId2 = strtol(ptr, &ptr, 10);
//     int nodeId3 = strtol(ptr, &ptr, 10);
//     if ( nbTriangle > 1 ) {
//       const SMDS_MeshNode* n1 = theGhs3dIdToNodeMap[ nodeId1 ];
//       const SMDS_MeshNode* n2 = theGhs3dIdToNodeMap[ nodeId2 ];
//       const SMDS_MeshNode* n3 = theGhs3dIdToNodeMap[ nodeId3 ];
//       if (!n1 || !n2 || !n3) {
//         tabID[i] = HOLE_ID;
//         continue;
//       }
//       try {
//         OCC_CATCH_SIGNALS;
//         tabID[i] = findShapeID( *theHelper.GetMesh(), n1, n2, n3, toMeshHoles );
//         // -- 0020330: Pb with MG-Tetra as a submesh
//         // check that found shape is to be meshed
//         if ( tabID[i] > 0 ) {
//           const TopoDS_Shape& foundShape = theMeshDS->IndexToShape( tabID[i] );
//           bool isToBeMeshed = false;
//           for ( int iS = 0; !isToBeMeshed && iS < nbShape; ++iS )
//             isToBeMeshed = foundShape.IsSame( tabShape[ iS ]);
//           if ( !isToBeMeshed )
//             tabID[i] = HOLE_ID;
//         }
//         // END -- 0020330: Pb with MG-Tetra as a submesh
// #ifdef _DEBUG_
//         std::cout << i+1 << " subdomain: findShapeID() returns " << tabID[i] << std::endl;
// #endif
//       }
//       catch ( Standard_Failure & ex)
//       {
// #ifdef _DEBUG_
//         std::cout << i+1 << " subdomain: Exception caugt: " << ex.GetMessageString() << std::endl;
// #endif
//       }
//       catch (...) {
// #ifdef _DEBUG_
//         std::cout << i+1 << " subdomain: unknown exception caught " << std::endl;
// #endif
//       }
//     }
//   }

//   shapePtr = ptr;

//   if ( nbTriangle <= nbShape ) // no holes
//     toMeshHoles = true; // not avoid creating tetras in holes

//   // IMP 0022172: [CEA 790] create the groups corresponding to domains
//   std::vector< std::vector< const SMDS_MeshElement* > > elemsOfDomain( Max( nbTriangle, nbShape ));

//   // Associating the tetrahedrons to the shapes
//   shapeID = compoundID;
//   for (int iElem = 0; iElem < nbElems; iElem++) {
//     if(theAlgo->computeCanceled())
//       return false;
//     for (int iNode = 0; iNode < 4; iNode++) {
//       ID = strtol(tetraPtr, &tetraPtr, 10);
//       itOnNode = theGhs3dIdToNodeMap.find(ID);
//       node[ iNode ] = itOnNode->second;
//       nodeID[ iNode ] = ID;
//     }
//     // We always run MG-Tetra with "to mesh holes"==TRUE but we must not create
//     // tetras within holes depending on hypo option,
//     // so we first check if aTet is inside a hole and then create it 
//     //aTet = theMeshDS->AddVolume( node[1], node[0], node[2], node[3] );
//     ghs3dShapeID = 0; // domain ID
//     if ( nbTriangle > 1 ) {
//       shapeID = HOLE_ID; // negative shapeID means not to create tetras if !toMeshHoles
//       ghs3dShapeID = strtol(shapePtr, &shapePtr, 10) - IdShapeRef;
//       if ( tabID[ ghs3dShapeID ] == 0 ) {
//         TopAbs_State state;
//         aSolid = findShape(node, aSolid, tabShape, tabBox, nbShape, &state);
//         if ( toMeshHoles || state == TopAbs_IN )
//           shapeID = theMeshDS->ShapeToIndex( aSolid );
//         tabID[ ghs3dShapeID ] = shapeID;
//       }
//       else
//         shapeID = tabID[ ghs3dShapeID ];
//     }
//     else if ( nbShape > 1 ) {
//       // Case where nbTriangle == 1 while nbShape == 2 encountered
//       // with compound of 2 boxes and "To mesh holes"==False,
//       // so there are no subdomains specified for each tetrahedron.
//       // Try to guess a solid by a node already bound to shape
//       shapeID = 0;
//       for ( int i=0; i<4 && shapeID==0; i++ ) {
//         if ( nodeAssigne[ nodeID[i] ] == 1 &&
//              node[i]->GetPosition()->GetTypeOfPosition() == SMDS_TOP_3DSPACE &&
//              node[i]->getshapeId() > 1 )
//         {
//           shapeID = node[i]->getshapeId();
//         }
//       }
//       if ( shapeID==0 ) {
//         aSolid = findShape(node, aSolid, tabShape, tabBox, nbShape);
//         shapeID = theMeshDS->ShapeToIndex( aSolid );
//       }
//     }
//     // set new nodes and tetrahedron onto the shape
//     for ( int i=0; i<4; i++ ) {
//       if ( nodeAssigne[ nodeID[i] ] == 0 ) {
//         if ( shapeID != HOLE_ID )
//           theMeshDS->SetNodeInVolume( node[i], shapeID );
//         nodeAssigne[ nodeID[i] ] = shapeID;
//       }
//     }
//     if ( toMeshHoles || shapeID != HOLE_ID ) {
//       aTet = theHelper.AddVolume( node[1], node[0], node[2], node[3],
//                                   /*id=*/0, /*force3d=*/false);
//       theMeshDS->SetMeshElementOnShape( aTet, shapeID );
//       if ( toMakeGroupsOfDomains )
//       {
//         if ( int( elemsOfDomain.size() ) < ghs3dShapeID+1 )
//           elemsOfDomain.resize( ghs3dShapeID+1 );
//         elemsOfDomain[ ghs3dShapeID ].push_back( aTet );
//       }
//     }
// #ifdef _DEBUG_
//     shapeIDs.insert( shapeID );
// #endif
//   }
//   if ( toMakeGroupsOfDomains )
//     makeDomainGroups( elemsOfDomain, &theHelper );
  
//   // Add enforced elements
//   GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap::const_iterator elemIt;
//   const SMDS_MeshElement* anElem;
//   SMDS_ElemIteratorPtr itOnEnfElem;
//   map<int,int>::const_iterator itOnMap;
//   shapeID = compoundID;
//   // Enforced edges
//   if (theEnforcedEdges.size()) {
//     (theEnforcedEdges.size() <= 1) ? tmpStr = " enforced edge" : " enforced edges";
//     std::cout << "Add " << theEnforcedEdges.size() << tmpStr << std::endl;
//     std::vector< const SMDS_MeshNode* > node( 2 );
//     // Iterate over the enforced edges
//     for(elemIt = theEnforcedEdges.begin() ; elemIt != theEnforcedEdges.end() ; ++elemIt) {
//       anElem = elemIt->first;
//       bool addElem = true;
//       itOnEnfElem = anElem->nodesIterator();
//       for ( int j = 0; j < 2; ++j ) {
//         int aNodeID = itOnEnfElem->next()->GetID();
//         itOnMap = theNodeId2NodeIndexMap.find(aNodeID);
//         if (itOnMap != theNodeId2NodeIndexMap.end()) {
//           itOnNode = theGhs3dIdToNodeMap.find((*itOnMap).second);
//           if (itOnNode != theGhs3dIdToNodeMap.end()) {
//             node.push_back((*itOnNode).second);
// //             shapeID =(*itOnNode).second->getshapeId();
//           }
//           else
//             addElem = false;
//         }
//         else
//           addElem = false;
//       }
//       if (addElem) {
//         aTet = theHelper.AddEdge( node[0], node[1], 0,  false);
//         theMeshDS->SetMeshElementOnShape( aTet, shapeID );
//       }
//     }
//   }
//   // Enforced faces
//   if (theEnforcedTriangles.size()) {
//     (theEnforcedTriangles.size() <= 1) ? tmpStr = " enforced triangle" : " enforced triangles";
//     std::cout << "Add " << theEnforcedTriangles.size() << " enforced triangles" << std::endl;
//     std::vector< const SMDS_MeshNode* > node( 3 );
//     // Iterate over the enforced triangles
//     for(elemIt = theEnforcedTriangles.begin() ; elemIt != theEnforcedTriangles.end() ; ++elemIt) {
//       anElem = elemIt->first;
//       bool addElem = true;
//       itOnEnfElem = anElem->nodesIterator();
//       for ( int j = 0; j < 3; ++j ) {
//         int aNodeID = itOnEnfElem->next()->GetID();
//         itOnMap = theNodeId2NodeIndexMap.find(aNodeID);
//         if (itOnMap != theNodeId2NodeIndexMap.end()) {
//           itOnNode = theGhs3dIdToNodeMap.find((*itOnMap).second);
//           if (itOnNode != theGhs3dIdToNodeMap.end()) {
//             node.push_back((*itOnNode).second);
// //             shapeID =(*itOnNode).second->getshapeId();
//           }
//           else
//             addElem = false;
//         }
//         else
//           addElem = false;
//       }
//       if (addElem) {
//         aTet = theHelper.AddFace( node[0], node[1], node[2], 0,  false);
//         theMeshDS->SetMeshElementOnShape( aTet, shapeID );
//       }
//     }
//   }

//   // Remove nodes of tetras inside holes if !toMeshHoles
//   if ( !toMeshHoles ) {
//     itOnNode = theGhs3dIdToNodeMap.find( nbInputNodes );
//     for ( ; itOnNode != theGhs3dIdToNodeMap.end(); ++itOnNode) {
//       ID = itOnNode->first;
//       if ( nodeAssigne[ ID ] == HOLE_ID )
//         theMeshDS->RemoveFreeNode( itOnNode->second, 0 );
//     }
//   }

  
//   if ( nbElems ) {
//     (nbElems <= 1) ? tmpStr = " tetrahedra" : " tetrahedrons";
//     cout << nbElems << tmpStr << " have been associated to " << nbShape;
//     (nbShape <= 1) ? tmpStr = " shape" : " shapes";
//     cout << tmpStr << endl;
//   }
// #ifdef WIN32
//   UnmapViewOfFile(mapPtr);
//   CloseHandle(hMapObject);
//   CloseHandle(fd);
// #else
//   munmap(mapPtr, length);
// #endif
//   close(fileOpen);

//   delete [] tab;
//   delete [] tabID;
//   delete [] nodeID;
//   delete [] coord;
//   delete [] node;
//   delete [] nodeAssigne;

// #ifdef _DEBUG_
//   shapeIDs.erase(-1);
//   if ( shapeIDs.size() != nbShape ) {
//     (shapeIDs.size() <= 1) ? tmpStr = " solid" : " solids";
//     std::cout << "Only " << shapeIDs.size() << tmpStr << " of " << nbShape << " found" << std::endl;
//     for (int i=0; i<nbShape; i++) {
//       shapeID = theMeshDS->ShapeToIndex( tabShape[i] );
//       if ( shapeIDs.find( shapeID ) == shapeIDs.end() )
//         std::cout << "  Solid #" << shapeID << " not found" << std::endl;
//     }
//   }
// #endif

//   return true;
// }


//=============================================================================
/*!
 *Here we are going to use the MG-Tetra mesher with geometry
 */
//=============================================================================

bool GHS3DPlugin_GHS3D::Compute(SMESH_Mesh&         theMesh,
                                const TopoDS_Shape& theShape)
{
  bool Ok(false);
  //SMESHDS_Mesh* meshDS = theMesh.GetMeshDS();

  // we count the number of shapes
  // _nbShape = countShape( meshDS, TopAbs_SOLID ); -- 0020330: Pb with MG-Tetra as a submesh
  // _nbShape = 0;
  TopExp_Explorer expBox ( theShape, TopAbs_SOLID );
  // for ( ; expBox.More(); expBox.Next() )
  //   _nbShape++;

  // create bounding box for every shape inside the compound

  // int iShape = 0;
  // TopoDS_Shape* tabShape;
  // double**      tabBox;
  // tabShape = new TopoDS_Shape[_nbShape];
  // tabBox   = new double*[_nbShape];
  // for (int i=0; i<_nbShape; i++)
  //   tabBox[i] = new double[6];
  // Standard_Real Xmin, Ymin, Zmin, Xmax, Ymax, Zmax;

  // for (expBox.ReInit(); expBox.More(); expBox.Next()) {
  //   tabShape[iShape] = expBox.Current();
  //   Bnd_Box BoundingBox;
  //   BRepBndLib::Add(expBox.Current(), BoundingBox);
  //   BoundingBox.Get(Xmin, Ymin, Zmin, Xmax, Ymax, Zmax);
  //   tabBox[iShape][0] = Xmin; tabBox[iShape][1] = Xmax;
  //   tabBox[iShape][2] = Ymin; tabBox[iShape][3] = Ymax;
  //   tabBox[iShape][4] = Zmin; tabBox[iShape][5] = Zmax;
  //   iShape++;
  // }

  // a unique working file name
  // to avoid access to the same files by eg different users
  _genericName = GHS3DPlugin_Hypothesis::GetFileName(_hyp);
  TCollection_AsciiString aGenericName((char*) _genericName.c_str() );
  TCollection_AsciiString aGenericNameRequired = aGenericName + "_required";

  TCollection_AsciiString aLogFileName    = aGenericName + ".log";    // log
  TCollection_AsciiString aResultFileName;

  TCollection_AsciiString aGMFFileName, aRequiredVerticesFileName, aSolFileName, aResSolFileName;
//#ifdef _DEBUG_
  aGMFFileName              = aGenericName + ".mesh"; // GMF mesh file
  aResultFileName           = aGenericName + "Vol.mesh"; // GMF mesh file
  aResSolFileName           = aGenericName + "Vol.sol"; // GMF mesh file
  aRequiredVerticesFileName = aGenericNameRequired + ".mesh"; // GMF required vertices mesh file
  aSolFileName              = aGenericNameRequired + ".sol"; // GMF solution file
//#else
//  aGMFFileName    = aGenericName + ".meshb"; // GMF mesh file
//  aResultFileName = aGenericName + "Vol.meshb"; // GMF mesh file
//  aRequiredVerticesFileName    = aGenericNameRequired + ".meshb"; // GMF required vertices mesh file
//  aSolFileName    = aGenericNameRequired + ".solb"; // GMF solution file
//#endif
  
  std::map <int,int> aNodeId2NodeIndexMap, aSmdsToGhs3dIdMap, anEnforcedNodeIdToGhs3dIdMap;
  //std::map <int,const SMDS_MeshNode*> aGhs3dIdToNodeMap;
  std::map <int, int> nodeID2nodeIndexMap;
  std::map<std::vector<double>, std::string> enfVerticesWithGroup;
  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues coordsSizeMap = GHS3DPlugin_Hypothesis::GetEnforcedVerticesCoordsSize(_hyp);
  GHS3DPlugin_Hypothesis::TIDSortedNodeGroupMap enforcedNodes = GHS3DPlugin_Hypothesis::GetEnforcedNodes(_hyp);
  GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap enforcedEdges = GHS3DPlugin_Hypothesis::GetEnforcedEdges(_hyp);
  GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap enforcedTriangles = GHS3DPlugin_Hypothesis::GetEnforcedTriangles(_hyp);
//   TIDSortedElemSet enforcedQuadrangles = GHS3DPlugin_Hypothesis::GetEnforcedQuadrangles(_hyp);
  GHS3DPlugin_Hypothesis::TID2SizeMap nodeIDToSizeMap = GHS3DPlugin_Hypothesis::GetNodeIDToSizeMap(_hyp);

  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexList enfVertices = GHS3DPlugin_Hypothesis::GetEnforcedVertices(_hyp);
  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexList::const_iterator enfVerIt = enfVertices.begin();
  std::vector<double> coords;

  for ( ; enfVerIt != enfVertices.end() ; ++enfVerIt)
  {
    GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertex* enfVertex = (*enfVerIt);
//     if (enfVertex->geomEntry.empty() && enfVertex->coords.size()) {
    if (enfVertex->coords.size()) {
      coordsSizeMap.insert(make_pair(enfVertex->coords,enfVertex->size));
      enfVerticesWithGroup.insert(make_pair(enfVertex->coords,enfVertex->groupName));
//       MESSAGE("enfVerticesWithGroup.insert(make_pair(("<<enfVertex->coords[0]<<","<<enfVertex->coords[1]<<","<<enfVertex->coords[2]<<"),\""<<enfVertex->groupName<<"\"))");
    }
    else {
//     if (!enfVertex->geomEntry.empty()) {
      TopoDS_Shape GeomShape = entryToShape(enfVertex->geomEntry);
//       GeomType = GeomShape.ShapeType();

//       if (!enfVertex->isCompound) {
// //       if (GeomType == TopAbs_VERTEX) {
//         coords.clear();
//         aPnt = BRep_Tool::Pnt(TopoDS::Vertex(GeomShape));
//         coords.push_back(aPnt.X());
//         coords.push_back(aPnt.Y());
//         coords.push_back(aPnt.Z());
//         if (coordsSizeMap.find(coords) == coordsSizeMap.end()) {
//           coordsSizeMap.insert(make_pair(coords,enfVertex->size));
//           enfVerticesWithGroup.insert(make_pair(coords,enfVertex->groupName));
//         }
//       }
//
//       // Group Management
//       else {
//       if (GeomType == TopAbs_COMPOUND){
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
//               MESSAGE("enfVerticesWithGroup.insert(make_pair(("<<coords[0]<<","<<coords[1]<<","<<coords[2]<<"),\""<<enfVertex->groupName<<"\"))");
            }
          }
        }
//       }
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

  // proxyMesh must live till readGMFFile() as a proxy face can be used by
  // MG-Tetra for domain indication
  //{
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
        q2t->Compute( theMesh, expBox.Current(), proxyMesh.get() );
        components.push_back( SMESH_ProxyMesh::Ptr( q2t ));
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

    // Ok = (writePoints( aPointsFile, helper, 
    //                    aSmdsToGhs3dIdMap, anEnforcedNodeIdToGhs3dIdMap, aGhs3dIdToNodeMap, 
    //                    nodeIDToSizeMap,
    //                    coordsSizeMap, enforcedNodes, enforcedEdges, enforcedTriangles)
    //       &&
    //       writeFaces ( aFacesFile, *proxyMesh, theShape, 
    //                    aSmdsToGhs3dIdMap, anEnforcedNodeIdToGhs3dIdMap,
    //                    enforcedEdges, enforcedTriangles ));
    Ok = writeGMFFile(aGMFFileName.ToCString(), aRequiredVerticesFileName.ToCString(), aSolFileName.ToCString(),
                      *proxyMesh, helper,
                      aNodeByGhs3dId, aFaceByGhs3dId, aNodeToGhs3dIdMap,
                      aNodeGroupByGhs3dId, anEdgeGroupByGhs3dId, aFaceGroupByGhs3dId,
                      enforcedNodes, enforcedEdges, enforcedTriangles, /*enforcedQuadrangles,*/
                      enfVerticesWithGroup, coordsSizeMap);
    //}

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

  TCollection_AsciiString cmd( (char*)GHS3DPlugin_Hypothesis::CommandToRun( _hyp ).c_str() );
  
  cmd += TCollection_AsciiString(" --in ") + aGMFFileName;
  if ( nbEnforcedVertices + nbEnforcedNodes)
    cmd += TCollection_AsciiString(" --required_vertices ") + aGenericNameRequired;
  cmd += TCollection_AsciiString(" --out ") + aResultFileName;
  if ( !_logInStandardOutput )
    cmd += TCollection_AsciiString(" 1>" ) + aLogFileName;  // dump into file

  std::cout << std::endl;
  std::cout << "MG-Tetra execution..." << std::endl;
  std::cout << cmd << std::endl;

  _compute_canceled = false;

  system( cmd.ToCString() ); // run

  std::cout << std::endl;
  std::cout << "End of MG-Tetra execution !" << std::endl;

  // --------------
  // read a result
  // --------------

  // Mapping the result file

  // int fileOpen;
  // fileOpen = open( aResultFileName.ToCString(), O_RDONLY);
  // if ( fileOpen < 0 ) {
  //   std::cout << std::endl;
  //   std::cout << "Can't open the " << aResultFileName.ToCString() << " MG-Tetra output file" << std::endl;
  //   std::cout << "Log: " << aLogFileName << std::endl;
  //   Ok = false;
  // }
  // else {
    GHS3DPlugin_Hypothesis::TSetStrings groupsToRemove = GHS3DPlugin_Hypothesis::GetGroupsToRemove(_hyp);
    bool toMeshHoles =
      _hyp ? _hyp->GetToMeshHoles(true) : GHS3DPlugin_Hypothesis::DefaultMeshHoles();
    const bool toMakeGroupsOfDomains = GHS3DPlugin_Hypothesis::GetToMakeGroupsOfDomains( _hyp );

    helper.IsQuadraticSubMesh( theShape );
    helper.SetElementsOnShape( false );

//     Ok = readResultFile( fileOpen,
// #ifdef WIN32
//                          aResultFileName.ToCString(),
// #endif
//                          this,
//                          /*theMesh, */helper, tabShape, tabBox, _nbShape, 
//                          aGhs3dIdToNodeMap, aNodeId2NodeIndexMap,
//                          toMeshHoles, 
//                          nbEnforcedVertices, nbEnforcedNodes, 
//                          enforcedEdges, enforcedTriangles,
//                          toMakeGroupsOfDomains );
                         
    Ok = readGMFFile(aResultFileName.ToCString(),
                     this,
                     &helper, aNodeByGhs3dId, aFaceByGhs3dId, aNodeToGhs3dIdMap,
                     aNodeGroupByGhs3dId, anEdgeGroupByGhs3dId, aFaceGroupByGhs3dId,
                     groupsToRemove, toMakeGroupsOfDomains, toMeshHoles);

    removeEmptyGroupsOfDomains( helper.GetMesh(), /*notEmptyAsWell =*/ !toMakeGroupsOfDomains );
    //}




  // ---------------------
  // remove working files
  // ---------------------

  if ( Ok )
  {
    if ( _removeLogOnSuccess )
      removeFile( aLogFileName );

    // if ( _hyp && _hyp->GetToMakeGroupsOfDomains() )
    //   error( COMPERR_WARNING, "'toMakeGroupsOfDomains' is ignored since the mesh is on shape" );
  }
  else if ( OSD_File( aLogFileName ).Size() > 0 )
  {
    // get problem description from the log file
    _Ghs2smdsConvertor conv( aNodeByGhs3dId, proxyMesh );
    storeErrorDescription( aLogFileName, conv );
  }
  else
  {
    // the log file is empty
    removeFile( aLogFileName );
    INFOS( "MG-Tetra Error, command '" << cmd.ToCString() << "' failed" );
    error(COMPERR_ALGO_FAILED, "mg-tetra.exe: command not found" );
  }

  if ( !_keepFiles ) {
    if (! Ok && _compute_canceled)
      removeFile( aLogFileName );
    removeFile( aGMFFileName );
    removeFile( aRequiredVerticesFileName );
    removeFile( aSolFileName );
    removeFile( aResSolFileName );
    removeFile( aResultFileName );
    removeFile( aSmdsToGhs3dIdMapFileName );
  }
  std::cout << "<" << aResultFileName.ToCString() << "> MG-Tetra output file ";
  if ( !Ok )
    std::cout << "not ";
  std::cout << "treated !" << std::endl;
  std::cout << std::endl;

  // _nbShape = 0;    // re-initializing _nbShape for the next Compute() method call
  // delete [] tabShape;
  // delete [] tabBox;

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
  MESSAGE("GHS3DPlugin_GHS3D::Compute()");

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
//#ifdef _DEBUG_
  aGMFFileName              = aGenericName + ".mesh"; // GMF mesh file
  aResultFileName           = aGenericName + "Vol.mesh"; // GMF mesh file
  aResSolFileName           = aGenericName + "Vol.sol"; // GMF mesh file
  aRequiredVerticesFileName = aGenericNameRequired + ".mesh"; // GMF required vertices mesh file
  aSolFileName              = aGenericNameRequired + ".sol"; // GMF solution file
//#else
//  aGMFFileName    = aGenericName + ".meshb"; // GMF mesh file
//  aResultFileName = aGenericName + "Vol.meshb"; // GMF mesh file
//  aRequiredVerticesFileName    = aGenericNameRequired + ".meshb"; // GMF required vertices mesh file
//  aSolFileName    = aGenericNameRequired + ".solb"; // GMF solution file
//#endif

  std::map <int, int> nodeID2nodeIndexMap;
  std::map<std::vector<double>, std::string> enfVerticesWithGroup;
  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexCoordsValues coordsSizeMap;
  TopoDS_Shape GeomShape;
//   TopAbs_ShapeEnum GeomType;
  std::vector<double> coords;
  gp_Pnt aPnt;
  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertex* enfVertex;

  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexList enfVertices = GHS3DPlugin_Hypothesis::GetEnforcedVertices(_hyp);
  GHS3DPlugin_Hypothesis::TGHS3DEnforcedVertexList::const_iterator enfVerIt = enfVertices.begin();

  for ( ; enfVerIt != enfVertices.end() ; ++enfVerIt)
  {
    enfVertex = (*enfVerIt);
//     if (enfVertex->geomEntry.empty() && enfVertex->coords.size()) {
    if (enfVertex->coords.size()) {
      coordsSizeMap.insert(make_pair(enfVertex->coords,enfVertex->size));
      enfVerticesWithGroup.insert(make_pair(enfVertex->coords,enfVertex->groupName));
//       MESSAGE("enfVerticesWithGroup.insert(make_pair(("<<enfVertex->coords[0]<<","<<enfVertex->coords[1]<<","<<enfVertex->coords[2]<<"),\""<<enfVertex->groupName<<"\"))");
    }
    else {
//     if (!enfVertex->geomEntry.empty()) {
      GeomShape = entryToShape(enfVertex->geomEntry);
//       GeomType = GeomShape.ShapeType();

//       if (!enfVertex->isCompound) {
// //       if (GeomType == TopAbs_VERTEX) {
//         coords.clear();
//         aPnt = BRep_Tool::Pnt(TopoDS::Vertex(GeomShape));
//         coords.push_back(aPnt.X());
//         coords.push_back(aPnt.Y());
//         coords.push_back(aPnt.Z());
//         if (coordsSizeMap.find(coords) == coordsSizeMap.end()) {
//           coordsSizeMap.insert(make_pair(coords,enfVertex->size));
//           enfVerticesWithGroup.insert(make_pair(coords,enfVertex->groupName));
//         }
//       }
//
//       // Group Management
//       else {
//       if (GeomType == TopAbs_COMPOUND){
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
//               MESSAGE("enfVerticesWithGroup.insert(make_pair(("<<coords[0]<<","<<coords[1]<<","<<coords[2]<<"),\""<<enfVertex->groupName<<"\"))");
            }
          }
        }
//       }
    }
  }

//   const SMDS_MeshNode* enfNode;
  GHS3DPlugin_Hypothesis::TIDSortedNodeGroupMap enforcedNodes = GHS3DPlugin_Hypothesis::GetEnforcedNodes(_hyp);
//   GHS3DPlugin_Hypothesis::TIDSortedNodeGroupMap::const_iterator enfNodeIt = enforcedNodes.begin();
//   for ( ; enfNodeIt != enforcedNodes.end() ; ++enfNodeIt)
//   {
//     enfNode = enfNodeIt->first;
//     coords.clear();
//     coords.push_back(enfNode->X());
//     coords.push_back(enfNode->Y());
//     coords.push_back(enfNode->Z());
//     if (enfVerticesWithGro
//       enfVerticesWithGroup.insert(make_pair(coords,enfNodeIt->second));
//   }


  GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap enforcedEdges = GHS3DPlugin_Hypothesis::GetEnforcedEdges(_hyp);
  GHS3DPlugin_Hypothesis::TIDSortedElemGroupMap enforcedTriangles = GHS3DPlugin_Hypothesis::GetEnforcedTriangles(_hyp);
//   TIDSortedElemSet enforcedQuadrangles = GHS3DPlugin_Hypothesis::GetEnforcedQuadrangles(_hyp);
  GHS3DPlugin_Hypothesis::TID2SizeMap nodeIDToSizeMap = GHS3DPlugin_Hypothesis::GetNodeIDToSizeMap(_hyp);

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

  // proxyMesh must live till readGMFFile() as a proxy face can be used by
  // MG-Tetra for domain indication
  //{
    SMESH_ProxyMesh::Ptr proxyMesh( new SMESH_ProxyMesh( theMesh ));
    if ( theMesh.NbQuadrangles() > 0 )
    {
      StdMeshers_QuadToTriaAdaptor* aQuad2Trias = new StdMeshers_QuadToTriaAdaptor;
      aQuad2Trias->Compute( theMesh );
      proxyMesh.reset( aQuad2Trias );
    }

    Ok = writeGMFFile(aGMFFileName.ToCString(), aRequiredVerticesFileName.ToCString(), aSolFileName.ToCString(),
                      *proxyMesh, *theHelper,
                      aNodeByGhs3dId, aFaceByGhs3dId, aNodeToGhs3dIdMap,
                      aNodeGroupByGhs3dId, anEdgeGroupByGhs3dId, aFaceGroupByGhs3dId,
                      enforcedNodes, enforcedEdges, enforcedTriangles,
                      enfVerticesWithGroup, coordsSizeMap);
    //}

  // -----------------
  // run MG-Tetra mesher
  // -----------------

  TCollection_AsciiString cmd = TCollection_AsciiString((char*)GHS3DPlugin_Hypothesis::CommandToRun( _hyp, false ).c_str());

  cmd += TCollection_AsciiString(" --in ") + aGMFFileName;
  if ( nbEnforcedVertices + nbEnforcedNodes)
    cmd += TCollection_AsciiString(" --required_vertices ") + aGenericNameRequired;
  cmd += TCollection_AsciiString(" --out ") + aResultFileName;
  if ( !_logInStandardOutput )
    cmd += TCollection_AsciiString(" 1>" ) + aLogFileName;  // dump into file

  std::cout << std::endl;
  std::cout << "MG-Tetra execution..." << std::endl;
  std::cout << cmd << std::endl;

  _compute_canceled = false;

  system( cmd.ToCString() ); // run

  std::cout << std::endl;
  std::cout << "End of MG-Tetra execution !" << std::endl;

  // --------------
  // read a result
  // --------------
  GHS3DPlugin_Hypothesis::TSetStrings groupsToRemove = GHS3DPlugin_Hypothesis::GetGroupsToRemove(_hyp);
  const bool toMakeGroupsOfDomains = GHS3DPlugin_Hypothesis::GetToMakeGroupsOfDomains( _hyp );

  Ok = readGMFFile(aResultFileName.ToCString(),
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
    if ( _removeLogOnSuccess )
      removeFile( aLogFileName );

    //if ( !toMakeGroupsOfDomains && _hyp && _hyp->GetToMakeGroupsOfDomains() )
    //error( COMPERR_WARNING, "'toMakeGroupsOfDomains' is ignored since 'toMeshHoles' is OFF." );
  }
  else if ( OSD_File( aLogFileName ).Size() > 0 )
  {
    // get problem description from the log file
    _Ghs2smdsConvertor conv( aNodeByGhs3dId, proxyMesh );
    storeErrorDescription( aLogFileName, conv );
  }
  else {
    // the log file is empty
    removeFile( aLogFileName );
    INFOS( "MG-Tetra Error, command '" << cmd.ToCString() << "' failed" );
    error(COMPERR_ALGO_FAILED, "mg-tetra.exe: command not found" );
  }

  if ( !_keepFiles )
  {
    if (! Ok && _compute_canceled)
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
  _compute_canceled = true;
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

bool GHS3DPlugin_GHS3D::storeErrorDescription(const TCollection_AsciiString& logFile,
                                              const _Ghs2smdsConvertor &     toSmdsConvertor )
{
  if(_compute_canceled)
    return error(SMESH_Comment("interruption initiated by user"));
  // open file
#ifdef WIN32
  int file = ::_open (logFile.ToCString(), _O_RDONLY|_O_BINARY);
#else
  int file = ::open (logFile.ToCString(), O_RDONLY);
#endif
  if ( file < 0 )
    return error( SMESH_Comment("See ") << logFile << " for problem description");

  // get file size
  off_t length = lseek( file, 0, SEEK_END);
  lseek( file, 0, SEEK_SET);

  // read file
  vector< char > buf( length );
  int nBytesRead = ::read (file, & buf[0], length);
  ::close (file);
  char* ptr = & buf[0];
  char* bufEnd = ptr + nBytesRead;

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

    list<const SMDS_MeshElement*> badElems;
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

    // store bad elements
    //if ( allElemsOk ) {
      list<const SMDS_MeshElement*>::iterator elem = badElems.begin();
      for ( ; elem != badElems.end(); ++elem )
        addBadInputElement( *elem );
      //}

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

  if ( errDescription.empty() )
    errDescription << "See " << logFile << " for problem description";
  else
    errDescription << "\nSee " << logFile << " for more information";

  return error( errDescription );
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
      if ( ghsNode < 1 || ghsNode > _nodeByGhsId->size() )
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
  SMESH_MesherHelper* helper  = new SMESH_MesherHelper(theMesh );
  std::vector <const SMDS_MeshNode*> dummyNodeVector;
  std::vector <const SMDS_MeshElement*> aFaceByGhs3dId;
  std::map<const SMDS_MeshNode*,int> dummyNodeMap;
  std::map<std::vector<double>, std::string> dummyEnfVertGroup;
  std::vector<std::string> dummyElemGroup;
  std::set<std::string> dummyGroupsToRemove;

  bool ok = readGMFFile(theGMFFileName,
                        this,
                        helper, dummyNodeVector, aFaceByGhs3dId, dummyNodeMap, dummyElemGroup, dummyElemGroup, dummyElemGroup, dummyGroupsToRemove);
  theMesh.GetMeshDS()->Modified();
  return ok;
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
