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
// File      : GHS3DPlugin_GHS3D.cxx
// Created   : 
// Author    : Edward AGAPOV, modified by Lioka RAZAFINDRAZAKA (CEA) 09/02/2007
// Project   : SALOME
//=============================================================================
//
#include "GHS3DPlugin_GHS3D.hxx"
#include "GHS3DPlugin_Hypothesis.hxx"

#include <Basics_Utils.hxx>

//#include "SMESH_Gen.hxx"
#include <SMESH_Client.hxx>
#include "SMESH_Mesh.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_MeshEditor.hxx"
#include "SMESH_OctreeNode.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_FaceOfNodes.hxx"
#include "SMDS_VolumeOfNodes.hxx"

#include <StdMeshers_QuadToTriaAdaptor.hxx>
#include <StdMeshers_ViscousLayers.hxx>

#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <OSD_File.hxx>
#include <Precision.hxx>
#include <Quantity_Parameter.hxx>
#include <Standard_ProgramError.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS.hxx>
//#include <BRepClass_FaceClassifier.hxx>
#include <TopTools_MapOfShape.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

#include "utilities.h"

#ifdef WIN32
#include <io.h>
#else
#include <sys/sysinfo.h>
#endif
#include <algorithm>

using namespace std;

//#include <Standard_Stream.hxx>


#define castToNode(n) static_cast<const SMDS_MeshNode *>( n );

#ifdef _DEBUG_
#define DUMP(txt) \
//  std::cout << txt
#else
#define DUMP(txt)
#endif

extern "C"
{
#ifndef WNT
#include <unistd.h>
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
}

#define HOLE_ID -1

#ifndef GHS3D_VERSION
#define GHS3D_VERSION 41
#endif

typedef const list<const SMDS_MeshFace*> TTriaList;

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
  _name = "GHS3D_3D";
  _shapeType = (1 << TopAbs_SHELL) | (1 << TopAbs_SOLID);// 1 bit /shape type
  _onlyUnaryInput = false; // Compute() will be called on a compound of solids
  _iShape=0;
  _nbShape=0;
  _compatibleHypothesis.push_back("GHS3D_Parameters");
  _compatibleHypothesis.push_back( StdMeshers_ViscousLayers::GetHypType() );
  _requireShape = false; // can work without shape
#ifdef WITH_SMESH_CANCEL_COMPUTE
  _compute_canceled = false;
#endif
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
    _keepFiles = _hyp->GetKeepFiles();

  return true;
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
 * \brief returns true if a triangle defined by the nodes is a temporary face on a
 * side facet of pyramid and defines sub-domian inside the pyramid
 */
//================================================================================

static bool isTmpFace(const SMDS_MeshNode* node1,
                      const SMDS_MeshNode* node2,
                      const SMDS_MeshNode* node3)
{
  // find a pyramid sharing the 3 nodes
  //const SMDS_MeshElement* pyram = 0;
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
      return ( i3base != i3 );
    }
  }
  return false;
}

//=======================================================================
//function : findShapeID
//purpose  : find the solid corresponding to GHS3D sub-domain following
//           the technique proposed in GHS3D manual (available within
//           ghs3d installation) in chapter "B.4 Subdomain (sub-region) assignment".
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
  const SMDS_MeshElement * face = meshDS->FindFace(node1,node2,node3);
  if ( !face )
    return isTmpFace(node1, node2, node3) ? HOLE_ID : invalidID;
#ifdef _DEBUG_
  std::cout << "bnd face " << face->GetID() << " - ";
#endif
  // geom face the face assigned to
  SMESH_MeshEditor editor(&mesh);
  int geomFaceID = editor.FindShape( face );
  if ( !geomFaceID )
    return isTmpFace(node1, node2, node3) ? HOLE_ID : invalidID;
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
    if ( shells(1).IsSame( BRepTools::OuterShell( solid1 )) )
    { // - No, but maybe a hole is bound by two shapes? Does shells(1) touches another shell?
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
  const SMDS_MeshNode* nodes[3] = { node1, node2, node3 };
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

//=======================================================================
//function : countShape
//purpose  :
//=======================================================================

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

//=======================================================================
//function : getShape
//purpose  :
//=======================================================================

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

// //=======================================================================
// //function : findEdgeID
// //purpose  :
// //=======================================================================
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


//=======================================================================
//function : readGMFFile
//purpose  : read GMF file with geometry associated to mesh
// TODO
//=======================================================================

// static bool readGMFFile(
//                         const int                       fileOpen,
//                         const char*                     theFileName, 
//                         SMESH_Mesh&                     theMesh,
//                         const int                       nbShape,
//                         const TopoDS_Shape*             tabShape,
//                         double**                        tabBox,
//                         map <int,const SMDS_MeshNode*>& theGhs3dIdToNodeMap,
//                         bool                            toMeshHoles,
//                         int                             nbEnforcedVertices,
//                         int                             nbEnforcedNodes,
//                         TIDSortedNodeSet &              theEnforcedNodes,
//                         TIDSortedElemSet &              theEnforcedTriangles,
//                         TIDSortedElemSet &              theEnforcedQuadrangles)
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
// #ifdef WNT
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
// //   int nbTriangle = GmfStatKwd(InpMsh, GmfSubdomain);
// //   int id_tri[3];
// 
// 
//   tabID = new int[nbTriangle];
//   for (int i=0; i < nbTriangle; i++) {
//     tabID[i] = 0;
//     int nodeId1, nodeId2, nodeId3;
//     // find the solid corresponding to GHS3D sub-domain following
//     // the technique proposed in GHS3D manual in chapter
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
//         // -- 0020330: Pb with ghs3d as a submesh
//         // check that found shape is to be meshed
//         if ( tabID[i] > 0 ) {
//           const TopoDS_Shape& foundShape = theMeshDS->IndexToShape( tabID[i] );
//           bool isToBeMeshed = false;
//           for ( int iS = 0; !isToBeMeshed && iS < nbShape; ++iS )
//             isToBeMeshed = foundShape.IsSame( tabShape[ iS ]);
//           if ( !isToBeMeshed )
//             tabID[i] = HOLE_ID;
//         }
//         // END -- 0020330: Pb with ghs3d as a submesh
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
//       SMDS_MeshNode** node;
// //       std::vector< SMDS_MeshNode* > enfNode( nbRef );
//       SMDS_MeshElement * aGMFElement;
//       
//       node    = new SMDS_MeshNode*[nbRef];
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
//           // We always run GHS3D with "to mesh holes"==TRUE but we must not create
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
// #ifdef WNT
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
//function : readGMFFile
//purpose  : read GMF file w/o geometry associated to mesh
//=======================================================================


static bool readGMFFile(const char* theFile,
#ifdef WITH_SMESH_CANCEL_COMPUTE
                        GHS3DPlugin_GHS3D*  theAlgo,
#endif 
                        SMESH_MesherHelper* theHelper,
                        TIDSortedNodeSet &  theEnforcedNodes,
                        TIDSortedElemSet &  theEnforcedTriangles,
                        TIDSortedElemSet &  theEnforcedQuadrangles)
{
  SMESHDS_Mesh* theMesh = theHelper->GetMeshDS();

  // ---------------------------------
  // Read generated elements and nodes
  // ---------------------------------

  int nbElem = 0, nbRef = 0;
  int aGMFNodeID = 0, shapeID;
  int *nodeAssigne;
  SMDS_MeshNode** GMFNode;
  std::map <GmfKwdCod,int> tabRef;

  tabRef[GmfVertices]       = 3;
  tabRef[GmfCorners]        = 1;
  tabRef[GmfEdges]          = 2;
  tabRef[GmfRidges]         = 1;
  tabRef[GmfTriangles]      = 3;
  tabRef[GmfQuadrilaterals] = 4;
  tabRef[GmfTetrahedra]     = 4;
  tabRef[GmfHexahedra]      = 8;

  theHelper->GetMesh()->Clear();

  int ver, dim;
  MESSAGE("Read " << theFile << " file");
  int InpMsh = GmfOpenMesh(theFile, GmfRead, &ver, &dim);
  if (!InpMsh)
    return false;

  int nbVertices = GmfStatKwd(InpMsh, GmfVertices);
  GMFNode = new SMDS_MeshNode*[ nbVertices + 1 ];
  nodeAssigne = new int[ nbVertices + 1 ];

  std::map <GmfKwdCod,int>::const_iterator it = tabRef.begin();
  for ( ; it != tabRef.end() ; ++it)
  {
#ifdef WITH_SMESH_CANCEL_COMPUTE
    if(theAlgo->computeCanceled()) {
      GmfCloseMesh(InpMsh);
      delete [] GMFNode;
      delete [] nodeAssigne;
      return false;
    }
#endif
    int dummy;
    GmfKwdCod token = it->first;
    nbRef    = it->second;

    nbElem = GmfStatKwd(InpMsh, token);
    if (nbElem > 0) {
      GmfGotoKwd(InpMsh, token);
      std::cout << "Read " << nbElem;
    }
    else
      continue;

    int id[nbElem*tabRef[token]];

    if (token == GmfVertices) {
      std::cout << " vertices" << std::endl;
      int aGMFID;

      float VerTab_f[nbElem][3];
      double VerTab_d[nbElem][3];
      SMDS_MeshNode * aGMFNode;

      for ( int iElem = 0; iElem < nbElem; iElem++ ) {
#ifdef WITH_SMESH_CANCEL_COMPUTE
        if(theAlgo->computeCanceled()) {
          GmfCloseMesh(InpMsh);
          delete [] GMFNode;
          delete [] nodeAssigne;
          return false;
        }
#endif
        aGMFID = iElem + 1;
        if (ver == GmfFloat) {
          GmfGetLin(InpMsh, token, &VerTab_f[nbElem][0], &VerTab_f[nbElem][1], &VerTab_f[nbElem][2], &dummy);
          aGMFNode = theMesh->AddNode(VerTab_f[nbElem][0], VerTab_f[nbElem][1], VerTab_f[nbElem][2]);
        }
        else {
          GmfGetLin(InpMsh, token, &VerTab_d[nbElem][0], &VerTab_d[nbElem][1], &VerTab_d[nbElem][2], &dummy);
          aGMFNode = theMesh->AddNode(VerTab_d[nbElem][0], VerTab_d[nbElem][1], VerTab_d[nbElem][2]);
        }
        GMFNode[ aGMFID ] = aGMFNode;
        nodeAssigne[ aGMFID ] = 0;
      }
    }
    else if (token == GmfCorners && nbElem > 0) {
      std::cout << " corners" << std::endl;
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]]);
    }
    else if (token == GmfRidges && nbElem > 0) {
      std::cout << " ridges" << std::endl;
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]]);
    }
    else if (token == GmfEdges && nbElem > 0) {
      std::cout << " edges" << std::endl;
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &dummy);
    }
    else if (token == GmfTriangles && nbElem > 0) {
      std::cout << " triangles" << std::endl;
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &dummy);
    }
    else if (token == GmfQuadrilaterals && nbElem > 0) {
      std::cout << " Quadrilaterals" << std::endl;
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &id[iElem*tabRef[token]+3], &dummy);
    }
    else if (token == GmfTetrahedra && nbElem > 0) {
      std::cout << " Tetrahedra" << std::endl;
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &id[iElem*tabRef[token]+3], &dummy);
    }
    else if (token == GmfHexahedra && nbElem > 0) {
      std::cout << " Hexahedra" << std::endl;
      for ( int iElem = 0; iElem < nbElem; iElem++ )
        GmfGetLin(InpMsh, token, &id[iElem*tabRef[token]], &id[iElem*tabRef[token]+1], &id[iElem*tabRef[token]+2], &id[iElem*tabRef[token]+3],
                  &id[iElem*tabRef[token]+4], &id[iElem*tabRef[token]+5], &id[iElem*tabRef[token]+6], &id[iElem*tabRef[token]+7], &dummy);
    }
    std::cout << std::endl;

    switch (token) {
    case GmfCorners:
    case GmfRidges:
    case GmfEdges:
    case GmfTriangles:
    case GmfQuadrilaterals:
    case GmfTetrahedra:
    case GmfHexahedra: {
      std::vector< SMDS_MeshNode* > node( nbRef );
      std::vector< int >          nodeID( nbRef );
      std::vector< SMDS_MeshNode* > enfNode( nbRef );

      for ( int iElem = 0; iElem < nbElem; iElem++ )
      {
#ifdef WITH_SMESH_CANCEL_COMPUTE
        if(theAlgo->computeCanceled()) {
          GmfCloseMesh(InpMsh);
          delete [] GMFNode;
          delete [] nodeAssigne;
          return false;
        }
#endif
        for ( int iRef = 0; iRef < nbRef; iRef++ )
        {
          aGMFNodeID = id[iElem*tabRef[token]+iRef]; // read nbRef aGMFNodeID
          node  [ iRef ] = GMFNode[ aGMFNodeID ];
          nodeID[ iRef ] = aGMFNodeID;
        }

        switch (token)
        {
        case GmfEdges:
          theHelper->AddEdge( node[0], node[1] ); break;
        case GmfTriangles: {
          theMesh->AddFace( node[0], node[1], node[2]);
          break;
        }
        case GmfQuadrilaterals: {
          theMesh->AddFace( node[0], node[1], node[2], node[3] );
          break;
        }
        case GmfTetrahedra:
          theHelper->AddVolume( node[0], node[1], node[2], node[3] ); break;
        case GmfHexahedra:
          theHelper->AddVolume( node[0], node[3], node[2], node[1],
                                node[4], node[7], node[6], node[5] ); break;
        default: continue;
        }
        if ( token == GmfTriangles || token == GmfQuadrilaterals ) // "Quadrilaterals" and "Triangles"
          for ( int iRef = 0; iRef < nbRef; iRef++ )
            nodeAssigne[ nodeID[ iRef ]] = 1;
      }
      break;
    }
    }
  }

  shapeID = theHelper->GetSubShapeID();
  for ( int i = 0; i < nbVertices; ++i ) {
#ifdef WITH_SMESH_CANCEL_COMPUTE
    if(theAlgo->computeCanceled()) {
      GmfCloseMesh(InpMsh);
      delete [] GMFNode;
      delete [] nodeAssigne;
      return false;
    }
#endif
    if ( !nodeAssigne[ i+1 ])
      theMesh->SetNodeInVolume( GMFNode[ i+1 ], shapeID );
  }

  GmfCloseMesh(InpMsh);
  delete [] GMFNode;
  delete [] nodeAssigne;
  return true;
}

static bool writeGMFFile(const char*   theMeshFileName,
                         const char*   theRequiredFileName,
                         const char*   theSolFileName,
                         const SMESH_ProxyMesh&           theProxyMesh,
                         SMESH_Mesh *                     theMesh,
                         vector <const SMDS_MeshNode*> &  theNodeByGhs3dId,
                         vector <const SMDS_MeshNode*> &  theEnforcedNodeByGhs3dId,
                         TIDSortedNodeSet &               theEnforcedNodes,
                         TIDSortedElemSet &               theEnforcedEdges,
                         TIDSortedElemSet &               theEnforcedTriangles,
                         TIDSortedElemSet &               theEnforcedQuadrangles,
                         GHS3DPlugin_Hypothesis::TEnforcedVertexValues & theEnforcedVertices)
{
  MESSAGE("writeGMFFile w/o geometry");
  int idx, idxRequired, idxSol;
  const int dummyint = 0;
  GHS3DPlugin_Hypothesis::TEnforcedVertexValues::const_iterator vertexIt;
  std::vector<double> enfVertexSizes;
  const SMDS_MeshElement* elem;
  TIDSortedElemSet anElemSet, anEnforcedEdgeSet, anEnforcedTriangleSet, anEnforcedQuadrangleSet;
  TIDSortedElemSet aQuadElemSet, aQuadEnforcedEdgeSet, aQuadEnforcedTriangleSet, aQuadEnforcedQuadrangleSet;
  SMDS_ElemIteratorPtr nodeIt;
  map<const SMDS_MeshNode*,int> aNodeToGhs3dIdMap, anEnforcedNodeToGhs3dIdMap;
  TIDSortedElemSet::iterator elemIt;
  bool isOK;
  auto_ptr< SMESH_ElementSearcher > pntCls ( SMESH_MeshEditor( theMesh ).GetElementSearcher());
  
  int nbEnforcedVertices = theEnforcedVertices.size();
//  int nbEnforcedNodes    = theEnforcedNodes.size();
//  int nbEnforcedEdges       = theEnforcedEdges.size();
//  int nbEnforcedTriangles   = theEnforcedTriangles.size();
//  int nbEnforcedQuadrangles = theEnforcedQuadrangles.size();
  
  // count faces
  int nbFaces = theProxyMesh.NbFaces();

  if ( nbFaces == 0 )
    return false;
  
  idx = GmfOpenMesh(theMeshFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
  if (!idx)
    return false;
  
  /* ========================== FACES ========================== */
  /* TRIANGLES ========================== */
  SMDS_ElemIteratorPtr eIt = theProxyMesh.GetFaces();
  while ( eIt->more() )
  {
    elem = eIt->next();
    if (elem->IsQuadratic())
      aQuadElemSet.insert(elem);
    else
      anElemSet.insert(elem);
    nodeIt = elem->nodesIterator();
    while ( nodeIt->more() )
    {
      // find GHS3D ID
      const SMDS_MeshNode* node = castToNode( nodeIt->next() );
      int newId = aNodeToGhs3dIdMap.size() + 1; // ghs3d ids count from 1
      aNodeToGhs3dIdMap.insert( make_pair( node, newId ));
    }
  }
  
  /* EDGES ========================== */
  
  // Iterate over the enforced edges
  for(elemIt = theEnforcedEdges.begin() ; elemIt != theEnforcedEdges.end() ; ++elemIt) {
    elem = (*elemIt);
    isOK = true;
    nodeIt = elem->nodesIterator();
    while ( nodeIt->more()) {
      // find GHS3D ID
      const SMDS_MeshNode* node = castToNode( nodeIt->next() );
      // Test if point is inside shape to mesh
      gp_Pnt myPoint(node->X(),node->Y(),node->Z());
      TopAbs_State result = pntCls->GetPointState( myPoint );
      if ( result != TopAbs_IN ) {
        isOK = false;
        break;
      }
    }
    if (isOK) {
      nodeIt = elem->nodesIterator();
      while ( nodeIt->more()) {
        // find GHS3D ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        int newId = aNodeToGhs3dIdMap.size() + anEnforcedNodeToGhs3dIdMap.size() + 1; // ghs3d ids count from 1
        anEnforcedNodeToGhs3dIdMap.insert( make_pair( node, newId ));
      }
      if (elem->IsQuadratic())
        aQuadEnforcedEdgeSet.insert(elem);
      else
        anEnforcedEdgeSet.insert(elem);
    }
  }
  
  /* ENFORCED TRIANGLES ========================== */
  
  // Iterate over the enforced triangles
  for(elemIt = theEnforcedTriangles.begin() ; elemIt != theEnforcedTriangles.end() ; ++elemIt) {
    elem = (*elemIt);
    isOK = true;
    nodeIt = elem->nodesIterator();
    while ( nodeIt->more() ) {
      // find GHS3D ID
      const SMDS_MeshNode* node = castToNode( nodeIt->next() );
      // Test if point is inside shape to mesh
      gp_Pnt myPoint(node->X(),node->Y(),node->Z());
      TopAbs_State result = pntCls->GetPointState( myPoint );
      if ( result != TopAbs_IN ) {
        isOK = false;
        break;
      }
    }
    if (isOK) {
      nodeIt = elem->nodesIterator();
      while ( nodeIt->more() ) {
        // find GHS3D ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        int newId = aNodeToGhs3dIdMap.size() + anEnforcedNodeToGhs3dIdMap.size() + 1; // ghs3d ids count from 1
        anEnforcedNodeToGhs3dIdMap.insert( make_pair( node, newId ));
      }
      if (elem->IsQuadratic())
        aQuadEnforcedTriangleSet.insert(elem);
      else
        anEnforcedTriangleSet.insert(elem);
    }
  }
  
  /* ENFORCED QUADRANGLES ========================== */
  
    // Iterate over the enforced quadrangles
  for(elemIt = theEnforcedQuadrangles.begin() ; elemIt != theEnforcedQuadrangles.end() ; ++elemIt) {
    elem = (*elemIt);
    isOK = true;
    nodeIt = elem->nodesIterator();
    while ( nodeIt->more() ) {
      // find GHS3D ID
      const SMDS_MeshNode* node = castToNode( nodeIt->next() );
      // Test if point is inside shape to mesh
      gp_Pnt myPoint(node->X(),node->Y(),node->Z());
      TopAbs_State result = pntCls->GetPointState( myPoint );
      if ( result != TopAbs_IN ) {
        isOK = false;
        break;
      }
    }
    if (isOK) {
      nodeIt = elem->nodesIterator();
      while ( nodeIt->more() ) {
        // find GHS3D ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        int newId = aNodeToGhs3dIdMap.size() + anEnforcedNodeToGhs3dIdMap.size() + 1; // ghs3d ids count from 1
        anEnforcedNodeToGhs3dIdMap.insert( make_pair( node, newId ));
      }
      if (elem->IsQuadratic())
        aQuadEnforcedQuadrangleSet.insert(elem);
      else
        anEnforcedQuadrangleSet.insert(elem);
    }
  }
  
  
  // put nodes to theNodeByGhs3dId vector
  std::cout << "aNodeToGhs3dIdMap.size(): "<<aNodeToGhs3dIdMap.size()<<std::endl;
  theNodeByGhs3dId.resize( aNodeToGhs3dIdMap.size() );
  map<const SMDS_MeshNode*,int>::const_iterator n2id = aNodeToGhs3dIdMap.begin();
  for ( ; n2id != aNodeToGhs3dIdMap.end(); ++ n2id)
  {
//     std::cout << "n2id->first: "<<n2id->first<<std::endl;
    theNodeByGhs3dId[ n2id->second - 1 ] = n2id->first; // ghs3d ids count from 1
  }

  // put nodes to anEnforcedNodeToGhs3dIdMap vector
  std::cout << "anEnforcedNodeToGhs3dIdMap.size(): "<<anEnforcedNodeToGhs3dIdMap.size()<<std::endl;
  theEnforcedNodeByGhs3dId.resize( anEnforcedNodeToGhs3dIdMap.size() );
  n2id = anEnforcedNodeToGhs3dIdMap.begin();
  for ( ; n2id != anEnforcedNodeToGhs3dIdMap.end(); ++ n2id)
  {
//     std::cout << "n2id->first: "<<n2id->first<<std::endl;
    theEnforcedNodeByGhs3dId[ n2id->second - aNodeToGhs3dIdMap.size() - 1 ] = n2id->first; // ghs3d ids count from 1
  }
  
  
  /* ========================== NODES ========================== */
  vector<const SMDS_MeshNode*> theOrderedNodes, theRequiredNodes;
  std::set< std::vector<double> > nodesCoords;
  vector<const SMDS_MeshNode*>::const_iterator ghs3dNodeIt = theNodeByGhs3dId.begin();
  vector<const SMDS_MeshNode*>::const_iterator after  = theNodeByGhs3dId.end();
  
  std::cout << theNodeByGhs3dId.size() << " nodes from mesh ..." << std::endl;
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
  
  // Iterate over the enforced nodes
  TIDSortedNodeSet::const_iterator enfNodeIt;
  std::cout << theEnforcedNodes.size() << " nodes from enforced nodes ..." << std::endl;
  for(enfNodeIt = theEnforcedNodes.begin() ; enfNodeIt != theEnforcedNodes.end() ; ++enfNodeIt)
  {
    const SMDS_MeshNode* node = *enfNodeIt;
    std::vector<double> coords;
    coords.push_back(node->X());
    coords.push_back(node->Y());
    coords.push_back(node->Z());
    
    if (nodesCoords.find(coords) != nodesCoords.end()) {
      std::cout << "Node at " << node->X()<<", " <<node->Y()<<", " <<node->Z() << " found" << std::endl;
      continue;
    }

    if (theEnforcedVertices.find(coords) != theEnforcedVertices.end())
      continue;
    
    // Test if point is inside shape to mesh
    gp_Pnt myPoint(node->X(),node->Y(),node->Z());
    TopAbs_State result = pntCls->GetPointState( myPoint );
    if ( result != TopAbs_IN )
      continue;
    
    nodesCoords.insert(coords);
    theOrderedNodes.push_back(node);
    theRequiredNodes.push_back(node);
  }
  // Iterate over the enforced nodes given by enforced elements
  ghs3dNodeIt = theEnforcedNodeByGhs3dId.begin();
  after  = theEnforcedNodeByGhs3dId.end();
  std::cout << theEnforcedNodeByGhs3dId.size() << " nodes from enforced elements ..." << std::endl;
  for ( ; ghs3dNodeIt != after; ++ghs3dNodeIt )
  {
    const SMDS_MeshNode* node = *ghs3dNodeIt;
    std::vector<double> coords;
    coords.push_back(node->X());
    coords.push_back(node->Y());
    coords.push_back(node->Z());
    
    if (nodesCoords.find(coords) != nodesCoords.end()) {
      std::cout << "Node at " << node->X()<<", " <<node->Y()<<", " <<node->Z() << " found" << std::endl;
      continue;
    }
    
    if (theEnforcedVertices.find(coords) != theEnforcedVertices.end())
      continue;
    
    nodesCoords.insert(coords);
    theOrderedNodes.push_back(node);
    theRequiredNodes.push_back(node);
  }
  
  int requiredNodes = theRequiredNodes.size();
  int solSize = 0;
  std::vector<std::vector<double> > ReqVerTab;
  if (nbEnforcedVertices) {
//    ReqVerTab.clear();
    std::cout << theEnforcedVertices.size() << " nodes from enforced vertices ..." << std::endl;
    // Iterate over the enforced vertices
    for(vertexIt = theEnforcedVertices.begin() ; vertexIt != theEnforcedVertices.end() ; ++vertexIt) {
      double x = vertexIt->first[0];
      double y = vertexIt->first[1];
      double z = vertexIt->first[2];
      // Test if point is inside shape to mesh
      gp_Pnt myPoint(x,y,z);
      TopAbs_State result = pntCls->GetPointState( myPoint );
      if ( result != TopAbs_IN )
        continue;
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
  GmfSetKwd(idx, GmfVertices, theOrderedNodes.size()+solSize);
  for (ghs3dNodeIt = theOrderedNodes.begin();ghs3dNodeIt != theOrderedNodes.end();++ghs3dNodeIt)
    GmfSetLin(idx, GmfVertices, (*ghs3dNodeIt)->X(), (*ghs3dNodeIt)->Y(), (*ghs3dNodeIt)->Z(), dummyint);
  for (int i=0;i<solSize;i++) {
    std::cout << ReqVerTab[i][0] <<" "<< ReqVerTab[i][1] << " "<< ReqVerTab[i][2] << std::endl;
    GmfSetLin(idx, GmfVertices, ReqVerTab[i][0], ReqVerTab[i][1], ReqVerTab[i][2], dummyint);
  }
  std::cout << "End writting required nodes in GmfVertices" << std::endl;

  if (requiredNodes + solSize) {
    GmfSetKwd(idx, GmfRequiredVertices, requiredNodes+solSize);
    if (requiredNodes) {
      std::cout << "Begin writting required nodes in GmfRequiredVertices" << std::endl;
      int startId = theOrderedNodes.size()-requiredNodes+1;
      std::cout << "startId: " << startId << std::endl;
      for (int i=0;i<requiredNodes;i++)
        GmfSetLin(idx, GmfRequiredVertices, startId+i);
      std::cout << "End writting required nodes in GmfRequiredVertices" << std::endl;
    }
    if (solSize) {
      std::cout << "Begin writting required vertices in GmfRequiredVertices" << std::endl;
      int startId = theOrderedNodes.size()+1;
      std::cout << "startId: " << startId << std::endl;
      for (int i=0;i<solSize;i++)
        GmfSetLin(idx, GmfRequiredVertices, startId+i);
      std::cout << "End writting required vertices in GmfRequiredVertices" << std::endl;

      std::cout << "Begin writting in sol file" << std::endl;
      idxSol = GmfOpenMesh(theSolFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
      if (!idxSol) {
        GmfCloseMesh(idx);
        if (idxRequired)
          GmfCloseMesh(idxRequired);
        return false;
      }
      int TypTab[] = {GmfSca};
      GmfSetKwd(idxSol, GmfSolAtVertices, solSize, 1, TypTab);
      for (int i=0;i<solSize;i++) {
        std::cout << "enfVertexSizes.at(i): " << enfVertexSizes.at(i) << std::endl;
        double solTab[] = {enfVertexSizes.at(i)};
        GmfSetLin(idxSol, GmfSolAtVertices, solTab);
      }
      std::cout << "End writting in sol file" << std::endl;
    }
  }

//  // GmfRequiredVertices + GmfSolAtVertices
////  std::cout << "theRequiredNodes.size() + solSize: " << theRequiredNodes.size()+ solSize << std::endl;
////  std::cout << "theRequiredNodes.size(): " << theRequiredNodes.size() << std::endl;
//  std::cout << "solSize: " << solSize << std::endl;
////  if (theRequiredNodes.size()+ solSize) {
////    GmfSetKwd(idx, GmfRequiredVertices, theRequiredNodes.size()+solSize);
////
////    if (theRequiredNodes.size()) {
////      std::cout << "Begin writting required nodes in GmfRequiredVertices" << std::endl;
////      int startId = theOrderedNodes.size()-theRequiredNodes.size();
////      std::cout << "startId: " << startId << std::endl;
////      for (int i=1;i<=theRequiredNodes.size();i++)
////        GmfSetLin(idx, GmfRequiredVertices, startId+i);
////      std::cout << "End writting required nodes in GmfRequiredVertices" << std::endl;
////    }
//
//    if (solSize) {
//      std::cout << "Begin writting in sol file" << std::endl;
//      GmfSetKwd(idx, GmfRequiredVertices, solSize);
//      idxSol = GmfOpenMesh(theSolFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
//      if (!idxSol) {
//        GmfCloseMesh(idx);
//        if (idxRequired)
//          GmfCloseMesh(idxRequired);
//        return false;
//      }
//
//      int TypTab[] = {GmfSca};
////      GmfSetKwd(idxRequired, GmfVertices, solSize);
//      GmfSetKwd(idxSol, GmfSolAtVertices, solSize, 1, TypTab);
//
//      for (int i=0;i<solSize;i++) {
//        double solTab[] = {enfVertexSizes.at(i)};
//        GmfSetLin(idx, GmfRequiredVertices, theOrderedNodes.size()+i+1);
//        GmfSetLin(idxSol, GmfSolAtVertices, solTab);
////      GmfSetLin(idxRequired, GmfVertices, ReqVerTab[i][0], ReqVerTab[i][1], ReqVerTab[i][2], dummyint);
//      }
//      std::cout << "End writting in sol file" << std::endl;
//    }
//
////  }

  int nedge[2], ntri[3], nquad[4], nedgeP2[3], ntriP2[6], nquadQ2[9];
  
  // GmfEdges
  int usedEnforcedEdges = 0;
  if (anEnforcedEdgeSet.size()) {
//    idxRequired = GmfOpenMesh(theRequiredFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
//    if (!idxRequired)
//      return false;
    GmfSetKwd(idx, GmfEdges, anEnforcedEdgeSet.size());
//    GmfSetKwd(idxRequired, GmfEdges, anEnforcedEdgeSet.size());
    for(elemIt = anEnforcedEdgeSet.begin() ; elemIt != anEnforcedEdgeSet.end() ; ++elemIt) {
      elem = (*elemIt);
      nodeIt = elem->nodesIterator();
      int index=0;
      while ( nodeIt->more() ) {
        // find GHS3D ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        map< const SMDS_MeshNode*,int >::iterator it = anEnforcedNodeToGhs3dIdMap.find(node);
        if (it == anEnforcedNodeToGhs3dIdMap.end())
          throw "Node not found";
        nedge[index] = it->second;
        index++;
      }
      GmfSetLin(idx, GmfEdges, nedge[0], nedge[1], dummyint);
//      GmfSetLin(idxRequired, GmfEdges, nedge[0], nedge[1], dummyint);
      usedEnforcedEdges++;
    }
//    GmfCloseMesh(idxRequired);
  }
  
  // GmfEdgesP2
  int usedEnforcedEdgesP2 = 0;
  if (aQuadEnforcedEdgeSet.size()) {
//    idxRequired = GmfOpenMesh(theRequiredFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
//    if (!idxRequired)
//      return false;
    GmfSetKwd(idx, GmfEdgesP2, aQuadEnforcedEdgeSet.size());
//    GmfSetKwd(idxRequired, GmfEdges, anEnforcedEdgeSet.size());
    for(elemIt = aQuadEnforcedEdgeSet.begin() ; elemIt != aQuadEnforcedEdgeSet.end() ; ++elemIt) {
      elem = (*elemIt);
      nodeIt = elem->nodesIterator();
      int index=0;
      while ( nodeIt->more() ) {
        // find GHS3D ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        map< const SMDS_MeshNode*,int >::iterator it = anEnforcedNodeToGhs3dIdMap.find(node);
        if (it == anEnforcedNodeToGhs3dIdMap.end())
          throw "Node not found";
        nedgeP2[index] = it->second;
        index++;
      }
      GmfSetLin(idx, GmfEdgesP2, nedgeP2[0], nedgeP2[1], nedgeP2[2], dummyint);
//      GmfSetLin(idxRequired, GmfEdges, nedge[0], nedge[1], dummyint);
      usedEnforcedEdgesP2++;
    }
//    GmfCloseMesh(idxRequired);
  }

  if (usedEnforcedEdges+usedEnforcedEdgesP2) {
    GmfSetKwd(idx, GmfRequiredEdges, usedEnforcedEdges+usedEnforcedEdgesP2);
    for (int enfID=1;enfID<=usedEnforcedEdges+usedEnforcedEdgesP2;enfID++) {
      GmfSetLin(idx, GmfRequiredEdges, enfID);
    }
  }

  // GmfTriangles
  int usedEnforcedTriangles = 0;
  if (anElemSet.size()+anEnforcedTriangleSet.size()) {
    GmfSetKwd(idx, GmfTriangles, anElemSet.size()+anEnforcedTriangleSet.size());
    for(elemIt = anElemSet.begin() ; elemIt != anElemSet.end() ; ++elemIt) {
      elem = (*elemIt);
      nodeIt = elem->nodesIterator();
      int index=0;
      for ( int j = 0; j < 3; ++j ) {
        // find GHS3D ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        map< const SMDS_MeshNode*,int >::iterator it = aNodeToGhs3dIdMap.find(node);
        if (it == aNodeToGhs3dIdMap.end())
          throw "Node not found";
        ntri[index] = it->second;
        index++;
      }
      GmfSetLin(idx, GmfTriangles, ntri[0], ntri[1], ntri[2], dummyint);
    }
    if (anEnforcedTriangleSet.size()) {
      for(elemIt = anEnforcedTriangleSet.begin() ; elemIt != anEnforcedTriangleSet.end() ; ++elemIt) {
        elem = (*elemIt);
        nodeIt = elem->nodesIterator();
        int index=0;
        for ( int j = 0; j < 3; ++j ) {
          // find GHS3D ID
          const SMDS_MeshNode* node = castToNode( nodeIt->next() );
          map< const SMDS_MeshNode*,int >::iterator it = anEnforcedNodeToGhs3dIdMap.find(node);
          if (it == anEnforcedNodeToGhs3dIdMap.end())
            throw "Node not found";
          ntri[index] = it->second;
          index++;
        }
        GmfSetLin(idx, GmfTriangles, ntri[0], ntri[1], ntri[2], dummyint);
        usedEnforcedTriangles++;
      }
    }
  }
  
  // GmfTrianglesP2
  int usedEnforcedTrianglesP2 = 0;
  if (aQuadElemSet.size()+aQuadEnforcedTriangleSet.size()) {
    GmfSetKwd(idx, GmfTrianglesP2, aQuadElemSet.size()+aQuadEnforcedTriangleSet.size());
    for(elemIt = aQuadElemSet.begin() ; elemIt != aQuadElemSet.end() ; ++elemIt) {
      elem = (*elemIt);
      nodeIt = elem->nodesIterator();
      int index=0;
      while ( nodeIt->more() ) {
        // find GHS3D ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        map< const SMDS_MeshNode*,int >::iterator it = aNodeToGhs3dIdMap.find(node);
        if (it == aNodeToGhs3dIdMap.end())
          throw "Node not found";
        ntriP2[index] = it->second;
        index++;
      }
      GmfSetLin(idx, GmfTrianglesP2, ntriP2[0], ntriP2[1], ntriP2[2], ntriP2[3], ntriP2[4], ntriP2[5], dummyint);
    }
    if (aQuadEnforcedTriangleSet.size()) {
      for(elemIt = aQuadEnforcedTriangleSet.begin() ; elemIt != aQuadEnforcedTriangleSet.end() ; ++elemIt) {
        elem = (*elemIt);
        nodeIt = elem->nodesIterator();
        int index=0;
        while ( nodeIt->more() ) {
          // find GHS3D ID
          const SMDS_MeshNode* node = castToNode( nodeIt->next() );
          map< const SMDS_MeshNode*,int >::iterator it = anEnforcedNodeToGhs3dIdMap.find(node);
          if (it == anEnforcedNodeToGhs3dIdMap.end())
            throw "Node not found";
          ntriP2[index] = it->second;
          index++;
        }
        GmfSetLin(idx, GmfTrianglesP2, ntriP2[0], ntriP2[1], ntriP2[2], ntriP2[3], ntriP2[4], ntriP2[5], dummyint);
        usedEnforcedTrianglesP2++;
      }
    }
  }
  
  if (usedEnforcedTriangles+usedEnforcedTrianglesP2) {
    GmfSetKwd(idx, GmfRequiredTriangles, usedEnforcedTriangles+usedEnforcedTrianglesP2);
    for (int enfID=1;enfID<=usedEnforcedTriangles+usedEnforcedTrianglesP2;enfID++)
      GmfSetLin(idx, GmfRequiredTriangles, anElemSet.size()+aQuadElemSet.size()+enfID);
  }

  // GmfQuadrangles
  int usedEnforcedQuadrilaterals = 0;
  if (anEnforcedQuadrangleSet.size()) {
    GmfSetKwd(idx, GmfQuadrilaterals, anEnforcedQuadrangleSet.size());
    for(elemIt = anEnforcedQuadrangleSet.begin() ; elemIt != anEnforcedQuadrangleSet.end() ; ++elemIt) {
      elem = (*elemIt);
      nodeIt = elem->nodesIterator();
      int index=0;
      for ( int j = 0; j < 4; ++j ) {
        // find GHS3D ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        map< const SMDS_MeshNode*,int >::iterator it = anEnforcedNodeToGhs3dIdMap.find(node);
        if (it == anEnforcedNodeToGhs3dIdMap.end())
          throw "Node not found";
        nquad[index] = it->second;
        index++;
      }
      GmfSetLin(idx, GmfQuadrilaterals, nquad[0], nquad[1], nquad[2], nquad[3], dummyint);
      usedEnforcedQuadrilaterals++;
    }
  }
  
  // GmfQuadranglesQ2
  int usedEnforcedQuadrilateralsQ2 = 0;
  if (aQuadEnforcedQuadrangleSet.size()) {
    GmfSetKwd(idx, GmfQuadrilateralsQ2, aQuadEnforcedQuadrangleSet.size());
    for(elemIt = aQuadEnforcedQuadrangleSet.begin() ; elemIt != aQuadEnforcedQuadrangleSet.end() ; ++elemIt) {
      elem = (*elemIt);
      nodeIt = elem->nodesIterator();
      int index=0;
      while ( nodeIt->more() ) {
        // find GHS3D ID
        const SMDS_MeshNode* node = castToNode( nodeIt->next() );
        map< const SMDS_MeshNode*,int >::iterator it = anEnforcedNodeToGhs3dIdMap.find(node);
        if (it == anEnforcedNodeToGhs3dIdMap.end())
          throw "Node not found";
        nquadQ2[index] = it->second;
        index++;
      }
      //
      // !!! The last value in nquadQ2 is missing because in Salome the quadratic quadrilaterals have only 8 points !!!
      //
      GmfSetLin(idx, GmfQuadrilateralsQ2, nquadQ2[0], nquadQ2[1], nquadQ2[2], nquadQ2[3], nquadQ2[4], nquadQ2[5], nquadQ2[6], nquadQ2[7], nquadQ2[8], dummyint);
      usedEnforcedQuadrilateralsQ2++;
    }
  }
  
  if (usedEnforcedQuadrilaterals+usedEnforcedQuadrilateralsQ2) {
    GmfSetKwd(idx, GmfRequiredQuadrilaterals, usedEnforcedQuadrilaterals+usedEnforcedQuadrilateralsQ2);
    for (int enfID=1;enfID<=usedEnforcedQuadrilaterals+usedEnforcedQuadrilateralsQ2;enfID++)
      GmfSetLin(idx, GmfRequiredQuadrilaterals, enfID);
  }

  GmfCloseMesh(idx);
  if (idxRequired)
    GmfCloseMesh(idxRequired);
  if (idxSol)
    GmfCloseMesh(idxSol);
  
  return true;
  
}

static bool writeGMFFile(const char*   theMeshFileName,
                         const char*   theRequiredFileName,
                         const char*   theSolFileName,
                         SMESH_MesherHelper&              theHelper,
                         const SMESH_ProxyMesh&           theProxyMesh,
                         map <int,int> &                  theSmdsToGhs3dIdMap,
                         map <int,const SMDS_MeshNode*> & theGhs3dIdToNodeMap,
                         TIDSortedNodeSet &               theEnforcedNodes,
                         TIDSortedElemSet &               theEnforcedEdges,
                         TIDSortedElemSet &               theEnforcedTriangles,
                         TIDSortedElemSet &               theEnforcedQuadrangles,
                         GHS3DPlugin_Hypothesis::TEnforcedVertexValues & theEnforcedVertices)
{
  MESSAGE("writeGMFFile with geometry");
  int idx, nbv, nbev, nben, aGhs3dID = 0;
  const int dummyint = 0;
  GHS3DPlugin_Hypothesis::TEnforcedVertexValues::const_iterator vertexIt;
  std::vector<double> enfVertexSizes;
  TIDSortedNodeSet::const_iterator enfNodeIt;
  const SMDS_MeshNode* node;
  SMDS_NodeIteratorPtr nodeIt;
  std::map<int,int> theNodeId2NodeIndexMap;

  idx = GmfOpenMesh(theMeshFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
  if (!idx)
    return false;
  
  SMESHDS_Mesh * theMeshDS = theHelper.GetMeshDS();
  
  /* ========================== NODES ========================== */
  // NB_NODES
  nbv = theMeshDS->NbNodes();
  if ( nbv == 0 )
    return false;
  nbev = theEnforcedVertices.size();
  nben = theEnforcedNodes.size();
  
  nodeIt = theMeshDS->nodesIterator();
  
  // Issue 020674: EDF 870 SMESH: Mesh generated by Netgen not usable by GHS3D
  // The problem is in nodes on degenerated edges, we need to skip them
  if ( theHelper.HasDegeneratedEdges() )
  {
    // here we decrease total nb of nodes by nb of nodes on degenerated edges
    set<int> checkedSM;
    for (TopExp_Explorer e(theMeshDS->ShapeToMesh(), TopAbs_EDGE ); e.More(); e.Next())
    {
      SMESH_subMesh* sm = theHelper.GetMesh()->GetSubMesh( e.Current() );
      if ( checkedSM.insert( sm->GetId() ).second && theHelper.IsDegenShape(sm->GetId() )) {
        if ( sm->GetSubMeshDS() )
          nbv -= sm->GetSubMeshDS()->NbNodes();
      }
    }
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
        nbv -= int( theHelper.IsMedium( node ));
    }
    nodeIt = theMeshDS->nodesIterator();
  }
  
  std::vector<std::vector<double> > VerTab;
  VerTab.clear();
  std::vector<double> aVerTab;
  // Loop from 1 to NB_NODES

  while ( nodeIt->more() )
  {
    node = nodeIt->next();
    if (( isQuadMesh && theHelper.IsMedium( node )) || // Issue 0021238
        theHelper.IsDegenShape( node->getshapeId() )) // Issue 0020674
      continue;

    aVerTab.clear();
    aVerTab.push_back(node->X());
    aVerTab.push_back(node->Y());
    aVerTab.push_back(node->Z());
    VerTab.push_back(aVerTab);
    aGhs3dID++;
    theSmdsToGhs3dIdMap.insert( make_pair( node->GetID(), aGhs3dID ));
    theGhs3dIdToNodeMap.insert( make_pair( aGhs3dID, node ));
  }
  
  /* ENFORCED NODES ========================== */
  if (nben) {
    for(enfNodeIt = theEnforcedNodes.begin() ; enfNodeIt != theEnforcedNodes.end() ; ++enfNodeIt) {
      double x = (*enfNodeIt)->X();
      double y = (*enfNodeIt)->Y();
      double z = (*enfNodeIt)->Z();
      // Test if point is inside shape to mesh
      gp_Pnt myPoint(x,y,z);
      BRepClass3d_SolidClassifier scl(theMeshDS->ShapeToMesh());
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
      aVerTab.clear();
      aVerTab.push_back(x);
      aVerTab.push_back(y);
      aVerTab.push_back(z);
      VerTab.push_back(aVerTab);
      aGhs3dID++;
      theNodeId2NodeIndexMap.insert( make_pair( (*enfNodeIt)->GetID(), aGhs3dID ));
    }
  }
  
  /* Write vertices number */
  MESSAGE("Number of vertices: "<<aGhs3dID);
  MESSAGE("Size of vector: "<<VerTab.size());
  GmfSetKwd(idx, GmfVertices, aGhs3dID);
  for (int i=0;i<aGhs3dID;i++)
    GmfSetLin(idx, GmfVertices, VerTab[i][0], VerTab[i][1], VerTab[i][2], dummyint);
  
  
  /* ENFORCED VERTICES ========================== */
  if (nbev) {
    std::vector<std::vector<double> > ReqVerTab;
    ReqVerTab.clear();
    std::vector<double> aReqVerTab;
    int solSize = 0;
    for(vertexIt = theEnforcedVertices.begin() ; vertexIt != theEnforcedVertices.end() ; ++vertexIt) {
      double x = vertexIt->first[0];
      double y = vertexIt->first[1];
      double z = vertexIt->first[2];
      // Test if point is inside shape to mesh
      gp_Pnt myPoint(x,y,z);
      BRepClass3d_SolidClassifier scl(theMeshDS->ShapeToMesh());
      scl.Perform(myPoint, 1e-7);
      TopAbs_State result = scl.State();
      if ( result != TopAbs_IN )
        continue;
      enfVertexSizes.push_back(vertexIt->second);
      aReqVerTab.clear();
      aReqVerTab.push_back(x);
      aReqVerTab.push_back(y);
      aReqVerTab.push_back(z);
      ReqVerTab.push_back(aReqVerTab);
      solSize++;
    }

    if (solSize) {
      int idxRequired = GmfOpenMesh(theRequiredFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
      if (!idxRequired)
        return false;
      int idxSol = GmfOpenMesh(theSolFileName, GmfWrite, GMFVERSION, GMFDIMENSION);
      if (!idxSol)
        return false;
      
      int TypTab[] = {GmfSca};
      GmfSetKwd(idxRequired, GmfVertices, solSize);
      GmfSetKwd(idxSol, GmfSolAtVertices, solSize, 1, TypTab);
      
      for (int i=0;i<solSize;i++) {
        double solTab[] = {enfVertexSizes.at(i)};
        GmfSetLin(idxRequired, GmfVertices, ReqVerTab[i][0], ReqVerTab[i][1], ReqVerTab[i][2], dummyint);
        GmfSetLin(idxSol, GmfSolAtVertices, solTab);
      }
      GmfCloseMesh(idxRequired);
      GmfCloseMesh(idxSol);
    }
  }

  
  /* ========================== FACES ========================== */
  
  int nbTriangles = 0, nbQuadrangles = 0, aSmdsID;
  TopTools_IndexedMapOfShape facesMap, trianglesMap, quadranglesMap;
  TIDSortedElemSet::const_iterator elemIt;
  const SMESHDS_SubMesh* theSubMesh;
  TopoDS_Shape aShape;
  SMDS_ElemIteratorPtr itOnSubMesh, itOnSubFace;
  const SMDS_MeshElement* aFace;
  map<int,int>::const_iterator itOnMap;
  std::vector<std::vector<int> > tt, qt,et;
  tt.clear();
  qt.clear();
  et.clear();
  std::vector<int> att, aqt, aet;
  
  TopExp::MapShapes( theMeshDS->ShapeToMesh(), TopAbs_FACE, facesMap );

  for ( int i = 1; i <= facesMap.Extent(); ++i )
    if (( theSubMesh  = theProxyMesh.GetSubMesh( facesMap(i))))
    {
      SMDS_ElemIteratorPtr it = theSubMesh->GetElements();
      while (it->more())
      {
        const SMDS_MeshElement *elem = it->next();
        if (elem->NbCornerNodes() == 3)
        {
          trianglesMap.Add(facesMap(i));
          nbTriangles ++;
        }
        else if (elem->NbCornerNodes() == 4)
        {
          quadranglesMap.Add(facesMap(i));
          nbQuadrangles ++;
        }
      }
    }
    
  /* TRIANGLES ========================== */
  if (nbTriangles) {
    for ( int i = 1; i <= trianglesMap.Extent(); i++ )
    {
      aShape = trianglesMap(i);
      theSubMesh = theProxyMesh.GetSubMesh(aShape);
      if ( !theSubMesh ) continue;
      itOnSubMesh = theSubMesh->GetElements();
      while ( itOnSubMesh->more() )
      {
        aFace = itOnSubMesh->next();
        itOnSubFace = aFace->nodesIterator();
        att.clear();
        for ( int j = 0; j < 3; ++j ) {
          // find GHS3D ID
          aSmdsID = itOnSubFace->next()->GetID();
          itOnMap = theSmdsToGhs3dIdMap.find( aSmdsID );
          ASSERT( itOnMap != theSmdsToGhs3dIdMap.end() );
          att.push_back((*itOnMap).second);
        }
        tt.push_back(att);
      }
    }
  }

  if (theEnforcedTriangles.size()) {
    // Iterate over the enforced triangles
    for(elemIt = theEnforcedTriangles.begin() ; elemIt != theEnforcedTriangles.end() ; ++elemIt) {
      aFace = (*elemIt);
      bool isOK = true;
      itOnSubFace = aFace->nodesIterator();
      att.clear();
      for ( int j = 0; j < 3; ++j ) {
        int aNodeID = itOnSubFace->next()->GetID();
        itOnMap = theNodeId2NodeIndexMap.find(aNodeID);
        if (itOnMap != theNodeId2NodeIndexMap.end())
          att.push_back((*itOnMap).second);
        else {
          isOK = false;
          theEnforcedTriangles.erase(elemIt);
          break;
        }
      }
      if (isOK)
        tt.push_back(att);
    }
  }

  if (tt.size()) {
    GmfSetKwd(idx, GmfTriangles, tt.size());
    for (int i=0;i<tt.size();i++)
      GmfSetLin(idx, GmfTriangles, tt[i][0], tt[i][1], tt[i][2], dummyint);
  }

  /* QUADRANGLES ========================== */
  if (nbQuadrangles) {
    for ( int i = 1; i <= quadranglesMap.Extent(); i++ )
    {
      aShape = quadranglesMap(i);
      theSubMesh = theProxyMesh.GetSubMesh(aShape);
      if ( !theSubMesh ) continue;
      itOnSubMesh = theSubMesh->GetElements();
      for ( int j = 0; j < 4; ++j )
      {
        aFace = itOnSubMesh->next();
        itOnSubFace = aFace->nodesIterator();
        aqt.clear();
        while ( itOnSubFace->more() ) {
          // find GHS3D ID
          aSmdsID = itOnSubFace->next()->GetID();
          itOnMap = theSmdsToGhs3dIdMap.find( aSmdsID );
          ASSERT( itOnMap != theSmdsToGhs3dIdMap.end() );
          aqt.push_back((*itOnMap).second);
        }
        qt.push_back(aqt);
      }
    }
  }

  if (theEnforcedQuadrangles.size()) {
    // Iterate over the enforced triangles
    for(elemIt = theEnforcedQuadrangles.begin() ; elemIt != theEnforcedQuadrangles.end() ; ++elemIt) {
      aFace = (*elemIt);
      bool isOK = true;
      itOnSubFace = aFace->nodesIterator();
      aqt.clear();
      for ( int j = 0; j < 4; ++j ) {
        int aNodeID = itOnSubFace->next()->GetID();
        itOnMap = theNodeId2NodeIndexMap.find(aNodeID);
        if (itOnMap != theNodeId2NodeIndexMap.end())
          aqt.push_back((*itOnMap).second);
        else {
          isOK = false;
          theEnforcedQuadrangles.erase(elemIt);
          break;
        }
      }
      if (isOK)
        qt.push_back(aqt);
    }
  }
  
  if (qt.size()) {
    GmfSetKwd(idx, GmfQuadrilaterals, qt.size());
    for (int i=0;i<qt.size();i++)
      GmfSetLin(idx, GmfQuadrilaterals, qt[i][0], qt[i][1], qt[i][2], qt[i][3], dummyint);
  }
  

  /* ========================== EDGES ========================== */

  if (theEnforcedEdges.size()) {
    // Iterate over the enforced edges
    for(elemIt = theEnforcedEdges.begin() ; elemIt != theEnforcedEdges.end() ; ++elemIt) {
      aFace = (*elemIt);
      bool isOK = true;
      itOnSubFace = aFace->nodesIterator();
      aet.clear();
      for ( int j = 0; j < 2; ++j ) {
        int aNodeID = itOnSubFace->next()->GetID();
        itOnMap = theNodeId2NodeIndexMap.find(aNodeID);
        if (itOnMap != theNodeId2NodeIndexMap.end())
          aet.push_back((*itOnMap).second);
        else {
          isOK = false;
          theEnforcedEdges.erase(elemIt);
          break;
        }
      }
      if (isOK)
        et.push_back(aet);
    }
  }

  if (et.size()) {
    GmfSetKwd(idx, GmfEdges, et.size());
    for (int i=0;i<et.size();i++)
      GmfSetLin(idx, GmfEdges, et[i][0], et[i][1], dummyint);
  }

  GmfCloseMesh(idx);
  return true;
}

//=======================================================================
//function : writeFaces
//purpose  : Write Faces in case if generate 3D mesh with geometry
//=======================================================================

// static bool writeFaces (ofstream &             theFile,
//                         const SMESH_ProxyMesh& theMesh,
//                         const TopoDS_Shape&    theShape,
//                         const map <int,int> &  theSmdsToGhs3dIdMap,
//                         const map <int,int> &  theEnforcedNodeIdToGhs3dIdMap,
//                         TIDSortedElemSet & theEnforcedEdges,
//                         TIDSortedElemSet & theEnforcedTriangles,
//                         TIDSortedElemSet & theEnforcedQuadrangles)
// {
//   // record structure:
//   //
//   // NB_ELEMS DUMMY_INT
//   // Loop from 1 to NB_ELEMS
//   // NB_NODES NODE_NB_1 NODE_NB_2 ... (NB_NODES + 1) times: DUMMY_INT
// 
//   TopoDS_Shape aShape;
//   const SMESHDS_SubMesh* theSubMesh;
//   const SMDS_MeshElement* aFace;
//   const char* space    = "  ";
//   const int   dummyint = 0;
//   map<int,int>::const_iterator itOnMap;
//   SMDS_ElemIteratorPtr itOnSubMesh, itOnSubFace;
//   int nbNodes, aSmdsID;
// 
//   TIDSortedElemSet::const_iterator elemIt;
//   int nbEnforcedEdges       = theEnforcedEdges.size();
//   int nbEnforcedTriangles   = theEnforcedTriangles.size();
//   int nbEnforcedQuadrangles = theEnforcedQuadrangles.size();
//   // count triangles bound to geometry
//   int nbTriangles = 0;
// 
//   TopTools_IndexedMapOfShape facesMap, trianglesMap, quadranglesMap;
//   TopExp::MapShapes( theShape, TopAbs_FACE, facesMap );
// 
//   for ( int i = 1; i <= facesMap.Extent(); ++i )
//     if (( theSubMesh  = theMesh.GetSubMesh( facesMap(i))))
//       nbTriangles += theSubMesh->NbElements();
// 
//   std::cout << "    " << facesMap.Extent() << " shapes of 2D dimension and" << std::endl;
//   if (nbEnforcedEdges+nbEnforcedTriangles+nbEnforcedQuadrangles)
//     std::cout << "    " << nbEnforcedEdges+nbEnforcedTriangles+nbEnforcedQuadrangles 
//                         << " enforced shapes:" << std::endl;
//   if (nbEnforcedEdges)
//     std::cout << "      " << nbEnforcedEdges << " enforced edges" << std::endl;
//   if (nbEnforcedTriangles)
//     std::cout << "      " << nbEnforcedTriangles << " enforced triangles" << std::endl;
//   if (nbEnforcedQuadrangles)
//     std::cout << "      " << nbEnforcedQuadrangles << " enforced quadrangles" << std::endl;
//   std::cout << std::endl;
// 
// //   theFile << space << nbTriangles << space << dummyint << std::endl;
//   std::ostringstream globalStream, localStream, aStream;
// 
//   //
//   //        FACES : BEGIN
//   //
//   
//   for ( int i = 1; i <= facesMap.Extent(); i++ )
//   {
//     aShape = facesMap(i);
//     theSubMesh = theMesh.GetSubMesh(aShape);
//     if ( !theSubMesh ) continue;
//     itOnSubMesh = theSubMesh->GetElements();
//     while ( itOnSubMesh->more() )
//     {
//       aFace = itOnSubMesh->next();
//       nbNodes = aFace->NbNodes();
// 
//       localStream << nbNodes << space;
// 
//       itOnSubFace = aFace->nodesIterator();
//       while ( itOnSubFace->more() ) {
//         // find GHS3D ID
//         aSmdsID = itOnSubFace->next()->GetID();
//         itOnMap = theSmdsToGhs3dIdMap.find( aSmdsID );
//         // if ( itOnMap == theSmdsToGhs3dIdMap.end() ) {
//         //   cout << "not found node: " << aSmdsID << endl;
//         //   return false;
//         // }
//         ASSERT( itOnMap != theSmdsToGhs3dIdMap.end() );
// 
//         localStream << (*itOnMap).second << space ;
//       }
// 
//       // (NB_NODES + 1) times: DUMMY_INT
//       for ( int j=0; j<=nbNodes; j++)
//         localStream << dummyint << space ;
// 
//       localStream << std::endl;
//     }
//   }
//   
//   globalStream << localStream.str();
//   localStream.str("");
// 
//   //
//   //        FACES : END
//   //
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
//     while ( itOnSubFace->more() ) {
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
//     isOK = true;
//     itOnSubFace = aFace->nodesIterator();
//     aStream.str("");
//     aStream << "3" << space ;
//     while ( itOnSubFace->more() ) {
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
// 
//   //
//   //        ENFORCED QUADRANGLES : BEGIN
//   //
//     // Iterate over the enforced quadrangles
//   int usedEnforcedQuadrangles = 0;
//   for(elemIt = theEnforcedQuadrangles.begin() ; elemIt != theEnforcedQuadrangles.end() ; ++elemIt) {
//     aFace = (*elemIt);
//     isOK = true;
//     itOnSubFace = aFace->nodesIterator();
//     aStream.str("");
//     aStream << "4" << space ;
//     while ( itOnSubFace->more() ) {
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
//       for ( int j=0; j<=4; j++)
//         aStream << dummyint << space ;
//       localStream << aStream.str() << std::endl;
//       usedEnforcedQuadrangles++;
//     }
//   }
//   
//   if (usedEnforcedQuadrangles) {
//     globalStream << localStream.str();
//     localStream.str("");
//   }
//   //
//   //        ENFORCED QUADRANGLES : END
//   //
// 
//   theFile
//   << nbTriangles+usedEnforcedQuadrangles+usedEnforcedTriangles+usedEnforcedEdges
//   << " 0" << std::endl
//   << globalStream.str();
// 
//   return true;
// }

//=======================================================================
//function : writePoints
//purpose  : 
//=======================================================================

// static bool writePoints (ofstream &                       theFile,
//                          SMESH_MesherHelper&              theHelper,
//                          map <int,int> &                  theSmdsToGhs3dIdMap,
//                          map <int,int> &                  theEnforcedNodeIdToGhs3dIdMap,
//                          map <int,const SMDS_MeshNode*> & theGhs3dIdToNodeMap,
//                          GHS3DPlugin_Hypothesis::TID2SizeMap & theNodeIDToSizeMap,
//                          GHS3DPlugin_Hypothesis::TEnforcedVertexValues & theEnforcedVertices,
//                          TIDSortedNodeSet & theEnforcedNodes)
// {
//   // record structure:
//   //
//   // NB_NODES
//   // Loop from 1 to NB_NODES
//   //   X Y Z DUMMY_INT
// 
//   SMESHDS_Mesh * theMesh = theHelper.GetMeshDS();
//   int nbNodes = theMesh->NbNodes();
//   if ( nbNodes == 0 )
//     return false;
//   int nbEnforcedVertices = theEnforcedVertices.size();
//   int nbEnforcedNodes    = theEnforcedNodes.size();
// 
//   int aGhs3dID = 1;
//   SMDS_NodeIteratorPtr it = theMesh->nodesIterator();
//   const SMDS_MeshNode* node;
// 
//   // Issue 020674: EDF 870 SMESH: Mesh generated by Netgen not usable by GHS3D
//   // The problem is in nodes on degenerated edges, we need to skip them
//   if ( theHelper.HasDegeneratedEdges() )
//   {
//     // here we decrease total nb of nodes by nb of nodes on degenerated edges
//     set<int> checkedSM;
//     for (TopExp_Explorer e(theMesh->ShapeToMesh(), TopAbs_EDGE ); e.More(); e.Next())
//     {
//       SMESH_subMesh* sm = theHelper.GetMesh()->GetSubMesh( e.Current() );
//       if ( checkedSM.insert( sm->GetId() ).second && theHelper.IsDegenShape(sm->GetId() )) {
//         if ( sm->GetSubMeshDS() )
//           nbNodes -= sm->GetSubMeshDS()->NbNodes();
//       }
//     }
//   }
// 
//   const bool isQuadMesh = 
//     theHelper.GetMesh()->NbEdges( ORDER_QUADRATIC ) ||
//     theHelper.GetMesh()->NbFaces( ORDER_QUADRATIC ) ||
//     theHelper.GetMesh()->NbVolumes( ORDER_QUADRATIC );
//   if ( isQuadMesh )
//   {
//     // descrease nbNodes by nb of medium nodes
//     while ( it->more() )
//     {
//       node = it->next();
//       if ( !theHelper.IsDegenShape( node->getshapeId() ))
//         nbNodes -= int( theHelper.IsMedium( node ));
//     }
//     it = theMesh->nodesIterator();
//   }
// 
//   const char* space    = "  ";
//   const int   dummyint = 0;
// 
//   // NB_NODES
//   std::cout << std::endl;
//   std::cout << "The initial 2D mesh contains :" << std::endl;
//   std::cout << "    " << nbNodes << " nodes" << std::endl;
//   if (nbEnforcedVertices > 0)
//     std::cout << "    " << nbEnforcedVertices << " enforced vertices" << std::endl;
//   if (nbEnforcedNodes > 0)
//     std::cout << "    " << nbEnforcedNodes << " enforced nodes" << std::endl;
// 
// //   std::cout << std::endl;
// //   std::cout << "Start writing in 'points' file ..." << std::endl;
//   theFile << nbNodes << space << std::endl;
// 
//   // Loop from 1 to NB_NODES
// 
//   while ( it->more() )
//   {
//     node = it->next();
//     if (( isQuadMesh && theHelper.IsMedium( node )) || // Issue 0021238
//         theHelper.IsDegenShape( node->getshapeId() )) // Issue 0020674
//       continue;
// 
//     theSmdsToGhs3dIdMap.insert( make_pair( node->GetID(), aGhs3dID ));
//     theGhs3dIdToNodeMap.insert( make_pair( aGhs3dID, node ));
//     aGhs3dID++;
// 
//     // X Y Z DUMMY_INT
//     theFile
//     << node->X() << space 
//     << node->Y() << space 
//     << node->Z() << space 
//     << dummyint << space ;
//     theFile << std::endl;
// 
//   }
//   
//   // Iterate over the enforced nodes
//   TIDSortedNodeSet::const_iterator nodeIt;
//   std::map<int,double> enfVertexIndexSizeMap;
//   if (nbEnforcedNodes) {
//     for(nodeIt = theEnforcedNodes.begin() ; nodeIt != theEnforcedNodes.end() ; ++nodeIt) {
//       double x = (*nodeIt)->X();
//       double y = (*nodeIt)->Y();
//       double z = (*nodeIt)->Z();
//       // Test if point is inside shape to mesh
//       gp_Pnt myPoint(x,y,z);
//       BRepClass3d_SolidClassifier scl(theMesh->ShapeToMesh());
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
//         
//       double size = theNodeIDToSizeMap.find((*nodeIt)->GetID())->second;
//   //       theGhs3dIdToNodeMap.insert( make_pair( nbNodes + i, (*nodeIt) ));
//   //       MESSAGE("Adding enforced node (" << x << "," << y <<"," << z << ")");
//       // X Y Z PHY_SIZE DUMMY_INT
//       theFile
//       << x << space 
//       << y << space 
//       << z << space
//       << size << space
//       << dummyint << space;
//       theFile << std::endl;
//       theEnforcedNodeIdToGhs3dIdMap.insert( make_pair( (*nodeIt)->GetID(), aGhs3dID ));
//       enfVertexIndexSizeMap[aGhs3dID] = -1;
//       aGhs3dID++;
//   //     else
//   //         MESSAGE("Enforced vertex (" << x << "," << y <<"," << z << ") is not inside the geometry: it was not added ");
//     }
//   }
//   
//   if (nbEnforcedVertices) {
//     // Iterate over the enforced vertices
//     GHS3DPlugin_Hypothesis::TEnforcedVertexValues::const_iterator vertexIt;
// //     int i = 1;
//     for(vertexIt = theEnforcedVertices.begin() ; vertexIt != theEnforcedVertices.end() ; ++vertexIt) {
//       double x = vertexIt->first[0];
//       double y = vertexIt->first[1];
//       double z = vertexIt->first[2];
//       // Test if point is inside shape to mesh
//       gp_Pnt myPoint(x,y,z);
//       BRepClass3d_SolidClassifier scl(theMesh->ShapeToMesh());
//       scl.Perform(myPoint, 1e-7);
//       TopAbs_State result = scl.State();
//       if ( result != TopAbs_IN )
//         continue;
//   //         MESSAGE("Adding enforced vertex (" << x << "," << y <<"," << z << ") = " << vertexIt->second);
//           // X Y Z PHY_SIZE DUMMY_INT
//       theFile
//       << x << space 
//       << y << space 
//       << z << space
//       << vertexIt->second << space 
//       << dummyint << space;
//       theFile << std::endl;
//       enfVertexIndexSizeMap[aGhs3dID] = vertexIt->second;
//       aGhs3dID++;
//       
//   //     else
//   //         MESSAGE("Enforced vertex (" << x << "," << y <<"," << z << ") is not inside the geometry: it was not added ");
//     }
//   }
//   theFile << std::endl;
//   
//   
// //   std::cout << std::endl;
// //   std::cout << "End writing in 'points' file." << std::endl;
// 
//   return true;
// }


//=======================================================================
//function : readResultFile
//purpose  : 
//=======================================================================

static bool readResultFile(const int                       fileOpen,
#ifdef WNT
                           const char*                     fileName,
#endif
#ifdef WITH_SMESH_CANCEL_COMPUTE
                           GHS3DPlugin_GHS3D*              theAlgo,
#endif
                           SMESH_MesherHelper&             theHelper,
//                            SMESH_Mesh&                     theMesh,
                           TopoDS_Shape                    tabShape[],
                           double**                        tabBox,
                           const int                       nbShape,
                           map <int,const SMDS_MeshNode*>& theGhs3dIdToNodeMap,
                           bool                            toMeshHoles,
                           int                             nbEnforcedVertices,
                           int                             nbEnforcedNodes,
                           TIDSortedElemSet &              theEnforcedEdges,
                           TIDSortedElemSet &              theEnforcedTriangles,
                           TIDSortedElemSet &              theEnforcedQuadrangles)
{
  MESSAGE("GHS3DPlugin_GHS3D::readResultFile()");
  Kernel_Utils::Localizer loc;
  struct stat status;
  size_t      length;

  char *ptr, *mapPtr;
  char *tetraPtr;
  char *shapePtr;

  SMESHDS_Mesh* theMeshDS = theHelper.GetMeshDS();

  int fileStat;
  int nbElems, nbNodes, nbInputNodes;
  int nodeId;
  int nbTriangle;
  int ID, shapeID, ghs3dShapeID;
  int IdShapeRef = 1;
  int compoundID =
    nbShape ? theMeshDS->ShapeToIndex( tabShape[0] ) : theMeshDS->ShapeToIndex( theMeshDS->ShapeToMesh() );

  int *tab, *tabID, *nodeID, *nodeAssigne;
  double *coord;
  const SMDS_MeshNode **node;

  tab    = new int[3];
  nodeID = new int[4];
  coord  = new double[3];
  node   = new const SMDS_MeshNode*[4];

  TopoDS_Shape aSolid;
  SMDS_MeshNode * aNewNode;
  map <int,const SMDS_MeshNode*>::iterator itOnNode;
  SMDS_MeshElement* aTet;
#ifdef _DEBUG_
  set<int> shapeIDs;
#endif

  // Read the file state
  fileStat = fstat(fileOpen, &status);
  length   = status.st_size;

  // Mapping the result file into memory
#ifdef WNT
  HANDLE fd = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  HANDLE hMapObject = CreateFileMapping(fd, NULL, PAGE_READONLY,
                                        0, (DWORD)length, NULL);
  ptr = ( char* ) MapViewOfFile(hMapObject, FILE_MAP_READ, 0, 0, 0 );
#else
  ptr = (char *) mmap(0,length,PROT_READ,MAP_PRIVATE,fileOpen,0);
#endif
  mapPtr = ptr;

  ptr      = readMapIntLine(ptr, tab);
  tetraPtr = ptr;

  nbElems      = tab[0];
  nbNodes      = tab[1];
  nbInputNodes = tab[2];

  nodeAssigne = new int[ nbNodes+1 ];

  if (nbShape > 0)
    aSolid = tabShape[0];

  // Reading the nodeId
  for (int i=0; i < 4*nbElems; i++)
    nodeId = strtol(ptr, &ptr, 10);

  MESSAGE("nbInputNodes: "<<nbInputNodes);
  MESSAGE("nbEnforcedVertices: "<<nbEnforcedVertices);
  // Reading the nodeCoor and update the nodeMap
  for (int iNode=1; iNode <= nbNodes; iNode++) {
#ifdef WITH_SMESH_CANCEL_COMPUTE
    if(theAlgo->computeCanceled())
      return false;
#endif
    for (int iCoor=0; iCoor < 3; iCoor++)
      coord[ iCoor ] = strtod(ptr, &ptr);
    nodeAssigne[ iNode ] = 1;
    if ( iNode > (nbInputNodes-(nbEnforcedVertices+nbEnforcedNodes)) ) {
      // Creating SMESH nodes
      // - for enforced vertices
      // - for vertices of forced edges
      // - for ghs3d nodes
      nodeAssigne[ iNode ] = 0;
      aNewNode = theMeshDS->AddNode( coord[0],coord[1],coord[2] );
      theGhs3dIdToNodeMap.insert(theGhs3dIdToNodeMap.end(), make_pair( iNode, aNewNode ));
    }
  }

  // Reading the number of triangles which corresponds to the number of sub-domains
  nbTriangle = strtol(ptr, &ptr, 10);

  tabID = new int[nbTriangle];
  for (int i=0; i < nbTriangle; i++) {
#ifdef WITH_SMESH_CANCEL_COMPUTE
    if(theAlgo->computeCanceled())
      return false;
#endif
    tabID[i] = 0;
    // find the solid corresponding to GHS3D sub-domain following
    // the technique proposed in GHS3D manual in chapter
    // "B.4 Subdomain (sub-region) assignment"
    int nodeId1 = strtol(ptr, &ptr, 10);
    int nodeId2 = strtol(ptr, &ptr, 10);
    int nodeId3 = strtol(ptr, &ptr, 10);
    if ( nbTriangle > 1 ) {
      const SMDS_MeshNode* n1 = theGhs3dIdToNodeMap[ nodeId1 ];
      const SMDS_MeshNode* n2 = theGhs3dIdToNodeMap[ nodeId2 ];
      const SMDS_MeshNode* n3 = theGhs3dIdToNodeMap[ nodeId3 ];
      try {
        OCC_CATCH_SIGNALS;
//         tabID[i] = findShapeID( theHelper, n1, n2, n3, toMeshHoles );
        tabID[i] = findShapeID( *theHelper.GetMesh(), n1, n2, n3, toMeshHoles );
        // -- 0020330: Pb with ghs3d as a submesh
        // check that found shape is to be meshed
        if ( tabID[i] > 0 ) {
          const TopoDS_Shape& foundShape = theMeshDS->IndexToShape( tabID[i] );
          bool isToBeMeshed = false;
          for ( int iS = 0; !isToBeMeshed && iS < nbShape; ++iS )
            isToBeMeshed = foundShape.IsSame( tabShape[ iS ]);
          if ( !isToBeMeshed )
            tabID[i] = HOLE_ID;
        }
        // END -- 0020330: Pb with ghs3d as a submesh
#ifdef _DEBUG_
        std::cout << i+1 << " subdomain: findShapeID() returns " << tabID[i] << std::endl;
#endif
      }
      catch ( Standard_Failure & ex)
      {
#ifdef _DEBUG_
        std::cout << i+1 << " subdomain: Exception caugt: " << ex.GetMessageString() << std::endl;
#endif
      }
      catch (...) {
#ifdef _DEBUG_
        std::cout << i+1 << " subdomain: unknown exception caught " << std::endl;
#endif
      }
    }
  }

  shapePtr = ptr;

  if ( nbTriangle <= nbShape ) // no holes
    toMeshHoles = true; // not avoid creating tetras in holes

  // Associating the tetrahedrons to the shapes
  shapeID = compoundID;
  for (int iElem = 0; iElem < nbElems; iElem++) {
#ifdef WITH_SMESH_CANCEL_COMPUTE
    if(theAlgo->computeCanceled())
      return false;
#endif
    for (int iNode = 0; iNode < 4; iNode++) {
      ID = strtol(tetraPtr, &tetraPtr, 10);
      itOnNode = theGhs3dIdToNodeMap.find(ID);
      node[ iNode ] = itOnNode->second;
      nodeID[ iNode ] = ID;
    }
    // We always run GHS3D with "to mesh holes"==TRUE but we must not create
    // tetras within holes depending on hypo option,
    // so we first check if aTet is inside a hole and then create it 
    //aTet = theMeshDS->AddVolume( node[1], node[0], node[2], node[3] );
    if ( nbTriangle > 1 ) {
      shapeID = HOLE_ID; // negative shapeID means not to create tetras if !toMeshHoles
      ghs3dShapeID = strtol(shapePtr, &shapePtr, 10) - IdShapeRef;
      if ( tabID[ ghs3dShapeID ] == 0 ) {
        TopAbs_State state;
        aSolid = findShape(node, aSolid, tabShape, tabBox, nbShape, &state);
        if ( toMeshHoles || state == TopAbs_IN )
          shapeID = theMeshDS->ShapeToIndex( aSolid );
        tabID[ ghs3dShapeID ] = shapeID;
      }
      else
        shapeID = tabID[ ghs3dShapeID ];
    }
    else if ( nbShape > 1 ) {
      // Case where nbTriangle == 1 while nbShape == 2 encountered
      // with compound of 2 boxes and "To mesh holes"==False,
      // so there are no subdomains specified for each tetrahedron.
      // Try to guess a solid by a node already bound to shape
      shapeID = 0;
      for ( int i=0; i<4 && shapeID==0; i++ ) {
        if ( nodeAssigne[ nodeID[i] ] == 1 &&
             node[i]->GetPosition()->GetTypeOfPosition() == SMDS_TOP_3DSPACE &&
             node[i]->getshapeId() > 1 )
        {
          shapeID = node[i]->getshapeId();
        }
      }
      if ( shapeID==0 ) {
        aSolid = findShape(node, aSolid, tabShape, tabBox, nbShape);
        shapeID = theMeshDS->ShapeToIndex( aSolid );
      }
    }
    // set new nodes and tetrahedron onto the shape
    for ( int i=0; i<4; i++ ) {
      if ( nodeAssigne[ nodeID[i] ] == 0 ) {
        if ( shapeID != HOLE_ID )
          theMeshDS->SetNodeInVolume( node[i], shapeID );
        nodeAssigne[ nodeID[i] ] = shapeID;
      }
    }
    if ( toMeshHoles || shapeID != HOLE_ID ) {
      aTet = theHelper.AddVolume( node[1], node[0], node[2], node[3],
                                  /*id=*/0, /*force3d=*/false);
      theMeshDS->SetMeshElementOnShape( aTet, shapeID );
    }
#ifdef _DEBUG_
    shapeIDs.insert( shapeID );
#endif
  }
  // Remove nodes of tetras inside holes if !toMeshHoles
  if ( !toMeshHoles ) {
    itOnNode = theGhs3dIdToNodeMap.find( nbInputNodes );
    for ( ; itOnNode != theGhs3dIdToNodeMap.end(); ++itOnNode) {
      ID = itOnNode->first;
      if ( nodeAssigne[ ID ] == HOLE_ID )
        theMeshDS->RemoveFreeNode( itOnNode->second, 0 );
    }
  }

  if ( nbElems )
    cout << nbElems << " tetrahedrons have been associated to " << nbShape << " shapes" << endl;
#ifdef WNT
  UnmapViewOfFile(mapPtr);
  CloseHandle(hMapObject);
  CloseHandle(fd);
#else
  munmap(mapPtr, length);
#endif
  close(fileOpen);

  delete [] tab;
  delete [] tabID;
  delete [] nodeID;
  delete [] coord;
  delete [] node;
  delete [] nodeAssigne;

#ifdef _DEBUG_
  shapeIDs.erase(-1);
  if ( shapeIDs.size() != nbShape ) {
    std::cout << "Only " << shapeIDs.size() << " solids of " << nbShape << " found" << std::endl;
    for (int i=0; i<nbShape; i++) {
      shapeID = theMeshDS->ShapeToIndex( tabShape[i] );
      if ( shapeIDs.find( shapeID ) == shapeIDs.end() )
        std::cout << "  Solid #" << shapeID << " not found" << std::endl;
    }
  }
#endif

  return true;
}


//=============================================================================
/*!
 *Here we are going to use the GHS3D mesher with geometry
 */
//=============================================================================

bool GHS3DPlugin_GHS3D::Compute(SMESH_Mesh&         theMesh,
                                const TopoDS_Shape& theShape)
{
  bool Ok(false);
  //SMESHDS_Mesh* meshDS = theMesh.GetMeshDS();

  // we count the number of shapes
  // _nbShape = countShape( meshDS, TopAbs_SOLID ); -- 0020330: Pb with ghs3d as a submesh
  _nbShape = 0;
  TopExp_Explorer expBox ( theShape, TopAbs_SOLID );
  for ( ; expBox.More(); expBox.Next() )
    _nbShape++;

  // create bounding box for every shape inside the compound

  int iShape = 0;
  TopoDS_Shape* tabShape;
  double**      tabBox;
  tabShape = new TopoDS_Shape[_nbShape];
  tabBox   = new double*[_nbShape];
  for (int i=0; i<_nbShape; i++)
    tabBox[i] = new double[6];
  Standard_Real Xmin, Ymin, Zmin, Xmax, Ymax, Zmax;

  for (expBox.ReInit(); expBox.More(); expBox.Next()) {
    tabShape[iShape] = expBox.Current();
    Bnd_Box BoundingBox;
    BRepBndLib::Add(expBox.Current(), BoundingBox);
    BoundingBox.Get(Xmin, Ymin, Zmin, Xmax, Ymax, Zmax);
    tabBox[iShape][0] = Xmin; tabBox[iShape][1] = Xmax;
    tabBox[iShape][2] = Ymin; tabBox[iShape][3] = Ymax;
    tabBox[iShape][4] = Zmin; tabBox[iShape][5] = Zmax;
    iShape++;
  }

  // a unique working file name
  // to avoid access to the same files by eg different users
  TCollection_AsciiString aGenericName
    = (char*) GHS3DPlugin_Hypothesis::GetFileName(_hyp).c_str();

  TCollection_AsciiString aResultFileName;
  TCollection_AsciiString aLogFileName    = aGenericName + ".log";    // log
// #if GHS3D_VERSION < 42
//   TCollection_AsciiString aFacesFileName, aPointsFileName;
//   TCollection_AsciiString aBadResFileName, aBbResFileName;
//   aFacesFileName  = aGenericName + ".faces";  // in faces
//   aPointsFileName = aGenericName + ".points"; // in points
//   aResultFileName = aGenericName + ".noboite";// out points and volumes
//   aBadResFileName = aGenericName + ".boite";  // out bad result
//   aBbResFileName  = aGenericName + ".bb";     // out vertex stepsize
// 
//   // -----------------
//   // make input files
//   // -----------------
// 
//   ofstream aFacesFile  ( aFacesFileName.ToCString()  , ios::out);
//   ofstream aPointsFile ( aPointsFileName.ToCString() , ios::out);
// 
//   Ok =
//     aFacesFile.rdbuf()->is_open() && aPointsFile.rdbuf()->is_open();
//   if (!Ok) {
//     INFOS( "Can't write into " << aFacesFileName);
//     return error(SMESH_Comment("Can't write into ") << aFacesFileName);
//   }
// #else
  TCollection_AsciiString aGMFFileName, aRequiredVerticesFileName, aSolFileName;
  TCollection_AsciiString aResultGMFFileName;

#ifdef _DEBUG_
  aGMFFileName    = aGenericName + ".mesh"; // GMF mesh file
  // The output .mesh file does not contain yet the subdomain-info (Ghs3D 4.2)
  aResultGMFFileName = aGenericName + "Vol.mesh"; // GMF mesh file
  aResultFileName = aGenericName + ".noboite";// out points and volumes
//   aResultFileName = aGenericName + "Vol.mesh"; // GMF mesh file
  aRequiredVerticesFileName    = aGenericName + "_required.mesh"; // GMF required vertices mesh file
  aSolFileName    = aGenericName + "_required.sol"; // GMF solution file
#else
  aGMFFileName    = aGenericName + ".mesh"; // GMF mesh file
//   aGMFFileName    = aGenericName + ".meshb"; // GMF mesh file
  // The output .mesh file does not contain yet the subdomain-info (Ghs3D 4.2)
  aResultGMFFileName = aGenericName + "Vol.meshb"; // GMF mesh file
  aResultFileName = aGenericName + ".noboite";// out points and volumes
//   aResultFileName = aGenericName + "Vol.meshb"; // GMF mesh file
  aRequiredVerticesFileName    = aGenericName + "_required.meshb"; // GMF required vertices mesh file
  aSolFileName    = aGenericName + "_required.solb"; // GMF solution file
#endif
  map <int,int> aSmdsToGhs3dIdMap, anEnforcedNodeIdToGhs3dIdMap;
  map <int,const SMDS_MeshNode*> aGhs3dIdToNodeMap;
  std::map <int, int> nodeID2nodeIndexMap;
  GHS3DPlugin_Hypothesis::TEnforcedVertexValues enforcedVertices = GHS3DPlugin_Hypothesis::GetEnforcedVertices(_hyp);
  TIDSortedNodeSet enforcedNodes = GHS3DPlugin_Hypothesis::GetEnforcedNodes(_hyp);
  TIDSortedElemSet enforcedEdges = GHS3DPlugin_Hypothesis::GetEnforcedEdges(_hyp);
  TIDSortedElemSet enforcedTriangles = GHS3DPlugin_Hypothesis::GetEnforcedTriangles(_hyp);
  TIDSortedElemSet enforcedQuadrangles = GHS3DPlugin_Hypothesis::GetEnforcedQuadrangles(_hyp);
  GHS3DPlugin_Hypothesis::TID2SizeMap nodeIDToSizeMap = GHS3DPlugin_Hypothesis::GetNodeIDToSizeMap(_hyp);
//   GHS3DPlugin_Hypothesis::TID2SizeMap elemIDToSizeMap = GHS3DPlugin_Hypothesis::GetElementIDToSizeMap(_hyp);

  int nbEnforcedVertices = enforcedVertices.size();
  int nbEnforcedNodes = enforcedNodes.size();
  
  SMESH_MesherHelper helper( theMesh );
  helper.SetSubShape( theShape );

  {
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
// #if GHS3D_VERSION < 42
//     Ok = (writePoints( aPointsFile, helper, 
//                        aSmdsToGhs3dIdMap, anEnforcedNodeIdToGhs3dIdMap, aGhs3dIdToNodeMap, 
//                        nodeIDToSizeMap,
//                        enforcedVertices, enforcedNodes)
//           &&
//           writeFaces ( aFacesFile, *proxyMesh, theShape, 
//                        aSmdsToGhs3dIdMap, anEnforcedNodeIdToGhs3dIdMap,
//                        enforcedEdges, enforcedTriangles, enforcedQuadrangles));
// #else
    Ok = writeGMFFile(aGMFFileName.ToCString(), aRequiredVerticesFileName.ToCString(), aSolFileName.ToCString(),
                      helper, *proxyMesh,
                      aSmdsToGhs3dIdMap, aGhs3dIdToNodeMap,
                      enforcedNodes, enforcedEdges, enforcedTriangles, enforcedQuadrangles,
                      enforcedVertices);
// #endif
  }

  // Write aSmdsToGhs3dIdMap to temp file
  TCollection_AsciiString aSmdsToGhs3dIdMapFileName;
  aSmdsToGhs3dIdMapFileName = aGenericName + ".ids";  // ids relation
  ofstream aIdsFile  ( aSmdsToGhs3dIdMapFileName.ToCString()  , ios::out);
  Ok = aIdsFile.rdbuf()->is_open();
  if (!Ok) {
    INFOS( "Can't write into " << aSmdsToGhs3dIdMapFileName);
    return error(SMESH_Comment("Can't write into ") << aSmdsToGhs3dIdMapFileName);
  }
  aIdsFile << "Smds Ghs3d" << std::endl;
  map <int,int>::const_iterator myit;
  for (myit=aSmdsToGhs3dIdMap.begin() ; myit != aSmdsToGhs3dIdMap.end() ; ++myit) {
    aIdsFile << myit->first << " " << myit->second << std::endl;
  }

  aIdsFile.close();
// #if GHS3D_VERSION < 42
//   aFacesFile.close();
//   aPointsFile.close();
// #endif
  
  if ( ! Ok ) {
    if ( !_keepFiles ) {
// #if GHS3D_VERSION < 42
//       removeFile( aFacesFileName );
//       removeFile( aPointsFileName );
// #else
      removeFile( aGMFFileName );
      removeFile( aRequiredVerticesFileName );
      removeFile( aSolFileName );
// #endif
      removeFile( aSmdsToGhs3dIdMapFileName );
    }
    return error(COMPERR_BAD_INPUT_MESH);
  }
  removeFile( aResultFileName ); // needed for boundary recovery module usage

  // -----------------
  // run ghs3d mesher
  // -----------------

  TCollection_AsciiString cmd = TCollection_AsciiString((char*)GHS3DPlugin_Hypothesis::CommandToRun( _hyp ).c_str() );
  // The output .mesh file does not contain yet the subdomain-info (Ghs3D 4.2)
  cmd += TCollection_AsciiString(" -f ") + aGenericName;  // file to read
  cmd += TCollection_AsciiString(" -IM ");
//   cmd += TCollection_AsciiString(" --in ") + aGenericName;
//   cmd += TCollection_AsciiString(" --required_vertices ") + aRequiredVerticesFileName;
//    cmd += TCollection_AsciiString(" --out ") + aGenericName;
  cmd += TCollection_AsciiString(" -Om 1>" ) + aLogFileName;  // dump into file

  std::cout << std::endl;
  std::cout << "Ghs3d execution..." << std::endl;
  std::cout << cmd << std::endl;

#ifdef WITH_SMESH_CANCEL_COMPUTE
  _compute_canceled = false;
#endif

  system( cmd.ToCString() ); // run

  std::cout << std::endl;
  std::cout << "End of Ghs3d execution !" << std::endl;

  // --------------
  // read a result
  // --------------

// #if GHS3D_VERSION < 42
  // Mapping the result file

  int fileOpen;
  fileOpen = open( aResultFileName.ToCString(), O_RDONLY);
  if ( fileOpen < 0 ) {
    std::cout << std::endl;
    std::cout << "Can't open the " << aResultFileName.ToCString() << " GHS3D output file" << std::endl;
    std::cout << "Log: " << aLogFileName << std::endl;
    Ok = false;
  }
  else {
    bool toMeshHoles =
      _hyp ? _hyp->GetToMeshHoles(true) : GHS3DPlugin_Hypothesis::DefaultMeshHoles();

    helper.IsQuadraticSubMesh( theShape );
    helper.SetElementsOnShape( false );

    Ok = readResultFile( fileOpen,
#ifdef WNT
                         aResultFileName.ToCString(),
#endif
#ifdef WITH_SMESH_CANCEL_COMPUTE
                         this,
#endif
                         /*theMesh, */helper, tabShape, tabBox, _nbShape, aGhs3dIdToNodeMap,
                         toMeshHoles, 
                         nbEnforcedVertices, nbEnforcedNodes, 
                         enforcedEdges, enforcedTriangles, enforcedQuadrangles );
  }
// /*/*#else
// #ifndef GMF_HAS_SUBDOMAIN_INFO
//   // The output .mesh file does not contain yet the subdomain-info (Ghs3D 4.2)
//   
//   int fileOpen = open( aResultFileName.ToCString(), O_RDONLY);
//   if ( fileOpen < 0 ) {
//     std::cout << std::endl;
//     std::cout << "Can't open the " << aResultFileName.ToCString() << " GHS3D output file" << std::endl;
//     std::cout << "Log: " << aLogFileName << std::endl;
//     Ok = false;
//   }
//   else {
// #endif
//       Ok = readGMFFile(
// #ifndef GMF_HAS_SUBDOMAIN_INFO
//                        fileOpen,
// #endif
//                        aGenericName.ToCString(), theMesh,
//                        _nbShape, tabShape, tabBox, 
//                        aGhs3dIdToNodeMap, toMeshHoles,
//                        nbEnforcedVertices, nbEnforcedNodes, 
//                        enforcedNodes, enforcedTriangles, enforcedQuadrangles);
// #ifndef GMF_HAS_SUBDOMAIN_INFO
//   }
// #endif
// #endif*/*/

  // ---------------------
  // remove working files
  // ---------------------

  if ( Ok )
  {
    if ( !_keepFiles )
      removeFile( aLogFileName );
  }
  else if ( OSD_File( aLogFileName ).Size() > 0 )
  {
    // get problem description from the log file
    _Ghs2smdsConvertor conv( aGhs3dIdToNodeMap );
    storeErrorDescription( aLogFileName, conv );
  }
  else
  {
    // the log file is empty
    removeFile( aLogFileName );
    INFOS( "GHS3D Error, command '" << cmd.ToCString() << "' failed" );
    error(COMPERR_ALGO_FAILED, "ghs3d: command not found" );
  }

  if ( !_keepFiles ) {
// #if GHS3D_VERSION < 42
//     removeFile( aFacesFileName );
//     removeFile( aPointsFileName );
//     removeFile( aResultFileName );
//     removeFile( aBadResFileName );
//     removeFile( aBbResFileName );
// #endif
    removeFile( aSmdsToGhs3dIdMapFileName );
  // The output .mesh file does not contain yet the subdomain-info (Ghs3D 4.2)

#ifdef WITH_SMESH_CANCEL_COMPUTE
    if (! Ok)
      if(_compute_canceled)
        removeFile( aLogFileName );
#endif
  }
  std::cout << "<" << aResultFileName.ToCString() << "> GHS3D output file ";
  if ( !Ok )
    std::cout << "not ";
  std::cout << "treated !" << std::endl;
  std::cout << std::endl;

  _nbShape = 0;    // re-initializing _nbShape for the next Compute() method call
  delete [] tabShape;
  delete [] tabBox;

  return Ok;
}

//=============================================================================
/*!
 *Here we are going to use the GHS3D mesher w/o geometry
 */
//=============================================================================
bool GHS3DPlugin_GHS3D::Compute(SMESH_Mesh&         theMesh,
                                SMESH_MesherHelper* theHelper)
{
  MESSAGE("GHS3DPlugin_GHS3D::Compute()");

  //SMESHDS_Mesh* meshDS = theMesh.GetMeshDS();
  TopoDS_Shape theShape = theHelper->GetSubShape();

  // a unique working file name
  // to avoid access to the same files by eg different users
  TCollection_AsciiString aGenericName
    = (char*) GHS3DPlugin_Hypothesis::GetFileName(_hyp).c_str();

  TCollection_AsciiString aLogFileName    = aGenericName + ".log";    // log
  TCollection_AsciiString aResultFileName;
  bool Ok;
// #if GHS3D_VERSION < 42
//   TCollection_AsciiString aFacesFileName, aPointsFileName;
//   TCollection_AsciiString aBadResFileName, aBbResFileName;
//   aFacesFileName  = aGenericName + ".faces";  // in faces
//   aPointsFileName = aGenericName + ".points"; // in points
//   aResultFileName = aGenericName + ".noboite";// out points and volumes
//   aBadResFileName = aGenericName + ".boite";  // out bad result
//   aBbResFileName  = aGenericName + ".bb";     // out vertex stepsize
// 
//   // -----------------
//   // make input files
//   // -----------------
// 
//   ofstream aFacesFile  ( aFacesFileName.ToCString()  , ios::out);
//   ofstream aPointsFile  ( aPointsFileName.ToCString()  , ios::out);
//   Ok = aFacesFile.rdbuf()->is_open() && aPointsFile.rdbuf()->is_open();
//   if (!Ok) {
//     INFOS( "Can't write into " << aFacesFileName);
//     return error( SMESH_Comment("Can't write into ") << aFacesFileName);
//   }
// #else
  TCollection_AsciiString aGMFFileName, aRequiredVerticesFileName, aSolFileName;
#ifdef _DEBUG_
  aGMFFileName    = aGenericName + ".mesh"; // GMF mesh file
  aResultFileName = aGenericName + "Vol.mesh"; // GMF mesh file
  aRequiredVerticesFileName    = aGenericName + "_required.mesh"; // GMF required vertices mesh file
  aSolFileName    = aGenericName + "_required.sol"; // GMF solution file
#else
  aGMFFileName    = aGenericName + ".meshb"; // GMF mesh file
  aResultFileName = aGenericName + "Vol.meshb"; // GMF mesh file
  aRequiredVerticesFileName    = aGenericName + "_required.meshb"; // GMF required vertices mesh file
  aSolFileName    = aGenericName + ".solb"; // GMF solution file
#endif
// #endif
  
  std::map <int, int> nodeID2nodeIndexMap;
  GHS3DPlugin_Hypothesis::TEnforcedVertexValues enforcedVertices = GHS3DPlugin_Hypothesis::GetEnforcedVertices(_hyp);
  TIDSortedNodeSet enforcedNodes = GHS3DPlugin_Hypothesis::GetEnforcedNodes(_hyp);
  TIDSortedElemSet enforcedEdges = GHS3DPlugin_Hypothesis::GetEnforcedEdges(_hyp);
  TIDSortedElemSet enforcedTriangles = GHS3DPlugin_Hypothesis::GetEnforcedTriangles(_hyp);
  TIDSortedElemSet enforcedQuadrangles = GHS3DPlugin_Hypothesis::GetEnforcedQuadrangles(_hyp);
  GHS3DPlugin_Hypothesis::TID2SizeMap nodeIDToSizeMap = GHS3DPlugin_Hypothesis::GetNodeIDToSizeMap(_hyp);

  vector <const SMDS_MeshNode*> aNodeByGhs3dId, anEnforcedNodeByGhs3dId;
  {
    SMESH_ProxyMesh::Ptr proxyMesh( new SMESH_ProxyMesh( theMesh ));
    if ( theMesh.NbQuadrangles() > 0 )
    {
      StdMeshers_QuadToTriaAdaptor* aQuad2Trias = new StdMeshers_QuadToTriaAdaptor;
      aQuad2Trias->Compute( theMesh );
      proxyMesh.reset( aQuad2Trias );
    }
// #if GHS3D_VERSION < 42
//     Ok = (writeFaces ( aFacesFile, *proxyMesh, &theMesh, aNodeByGhs3dId, anEnforcedNodeByGhs3dId,
//                        enforcedEdges, enforcedTriangles, enforcedQuadrangles ) &&
//           writePoints( aPointsFile, &theMesh, aNodeByGhs3dId, anEnforcedNodeByGhs3dId,
//                        nodeIDToSizeMap, enforcedVertices, enforcedNodes));
//     int nbEnforcedVertices = enforcedVertices.size();
//     int nbEnforcedNodes = enforcedNodes.size();
// #else
    Ok = writeGMFFile(aGMFFileName.ToCString(), aRequiredVerticesFileName.ToCString(), aSolFileName.ToCString(),
                      *proxyMesh, &theMesh,
                      aNodeByGhs3dId, anEnforcedNodeByGhs3dId,
                      enforcedNodes, enforcedEdges, enforcedTriangles, enforcedQuadrangles,
                      enforcedVertices);
// #endif
  }
  
  TIDSortedNodeSet enforcedNodesFromEnforcedElem;
  for (int i=0;i<anEnforcedNodeByGhs3dId.size();i++)
    enforcedNodesFromEnforcedElem.insert(anEnforcedNodeByGhs3dId[i]);

// #if GHS3D_VERSION < 42
//   aFacesFile.close();
//   aPointsFile.close();
//   
//   if ( ! Ok ) {
//     if ( !_keepFiles ) {
//       removeFile( aFacesFileName );
//       removeFile( aPointsFileName );
//     }
//     return error(COMPERR_BAD_INPUT_MESH);
//   }
//   removeFile( aResultFileName ); // needed for boundary recovery module usage
// #endif

  // -----------------
  // run ghs3d mesher
  // -----------------

  TCollection_AsciiString cmd = TCollection_AsciiString((char*)GHS3DPlugin_Hypothesis::CommandToRun( _hyp, false ).c_str());
// #if GHS3D_VERSION < 42
//   cmd += TCollection_AsciiString(" -f ") + aGenericName;  // file to read
// #else
  cmd += TCollection_AsciiString(" --in ") + aGenericName;
//   cmd += TCollection_AsciiString(" --required_vertices ") + aRequiredVerticesFileName;
  cmd += TCollection_AsciiString(" --out ") + aResultFileName;
// #endif
  cmd += TCollection_AsciiString(" 1>" ) + aLogFileName;  // dump into file

  std::cout << std::endl;
  std::cout << "Ghs3d execution..." << std::endl;
  std::cout << cmd << std::endl;
  
#ifdef WITH_SMESH_CANCEL_COMPUTE
  _compute_canceled = false;
#endif

  system( cmd.ToCString() ); // run

  std::cout << std::endl;
  std::cout << "End of Ghs3d execution !" << std::endl;

  // --------------
  // read a result
  // --------------
// #if GHS3D_VERSION < 42
//   int fileOpen = open( aResultFileName.ToCString(), O_RDONLY);
//   if ( fileOpen < 0 ) {
//     std::cout << std::endl;
//     std::cout << "Error when opening the " << aResultFileName.ToCString() << " file" << std::endl;
//     std::cout << "Log: " << aLogFileName << std::endl;
//     std::cout << std::endl;
//     Ok = false;
//   }
//   else {
//     Ok = readResultFile( fileOpen,
// #ifdef WNT
//                          aResultFileName.ToCString(),
// #endif
// #ifdef WITH_SMESH_CANCEL_COMPUTE
//                          this,
// #endif
//                          theMesh, theShape ,aNodeByGhs3dId, anEnforcedNodeByGhs3dId,
//                          nbEnforcedVertices, nbEnforcedNodes, 
//                          enforcedEdges, enforcedTriangles, enforcedQuadrangles );
//   }
// #else
  Ok = readGMFFile(aResultFileName.ToCString(),
#ifdef WITH_SMESH_CANCEL_COMPUTE
                   this,
#endif
                   theHelper, 
                   enforcedNodesFromEnforcedElem, enforcedTriangles, enforcedQuadrangles);
// #endif
  
  // ---------------------
  // remove working files
  // ---------------------

  if ( Ok )
  {
    if ( !_keepFiles )
      removeFile( aLogFileName );
  }
  else if ( OSD_File( aLogFileName ).Size() > 0 )
  {
    // get problem description from the log file
    _Ghs2smdsConvertor conv( aNodeByGhs3dId );
    storeErrorDescription( aLogFileName, conv );
  }
  else {
    // the log file is empty
    removeFile( aLogFileName );
    INFOS( "GHS3D Error, command '" << cmd.ToCString() << "' failed" );
    error(COMPERR_ALGO_FAILED, "ghs3d: command not found" );
  }
// #if GHS3D_VERSION < 42
  if ( !_keepFiles )
  {
#ifdef WITH_SMESH_CANCEL_COMPUTE
    if (! Ok)
      if(_compute_canceled)
        removeFile( aLogFileName );
#endif
//     removeFile( aFacesFileName );
//     removeFile( aPointsFileName );
//     removeFile( aResultFileName );
//     removeFile( aBadResFileName );
//     removeFile( aBbResFileName );
  }
// #endif
  return Ok;
}

#ifdef WITH_SMESH_CANCEL_COMPUTE
void GHS3DPlugin_GHS3D::CancelCompute()
{
  _compute_canceled = true;
#ifdef WNT
#else
  TCollection_AsciiString aGenericName
    = (char*) GHS3DPlugin_Hypothesis::GetFileName(_hyp).c_str();
  TCollection_AsciiString cmd =
    TCollection_AsciiString("ps ux | grep ") + aGenericName;
  cmd += TCollection_AsciiString(" | grep -v grep | awk '{print $2}' | xargs kill -9 > /dev/null 2>&1");
  system( cmd.ToCString() );
#endif
}
#endif

//================================================================================
/*!
 * \brief Provide human readable text by error code reported by ghs3d
 */
//================================================================================

static string translateError(const int errNum)
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
#ifdef WITH_SMESH_CANCEL_COMPUTE
  if(_compute_canceled)
    return error(SMESH_Comment("interruption initiated by user"));
#endif
  // open file
#ifdef WNT
  int file = ::_open (logFile.ToCString(), _O_RDONLY|_O_BINARY);
#else
  int file = ::open (logFile.ToCString(), O_RDONLY);
#endif
  if ( file < 0 )
    return error( SMESH_Comment("See ") << logFile << " for problem description");

  // get file size
//   struct stat status;
//   fstat(file, &status);
//   size_t length = status.st_size;
  off_t length = lseek( file, 0, SEEK_END);
  lseek( file, 0, SEEK_SET);

  // read file
  vector< char > buf( length );
  int nBytesRead = ::read (file, & buf[0], length);
  ::close (file);
  char* ptr = & buf[0];
  char* bufEnd = ptr + nBytesRead;

  SMESH_Comment errDescription;

  enum { NODE = 1, EDGE, TRIA, VOL, ID = 1 };

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
    int   errNum = strtol(ptr, &ptr, 10);
    switch ( errNum ) { // we treat errors enumerated in [SALOME platform 0019316] issue
    case 0015:
      // The face number (numfac) with vertices (f 1, f 2, f 3) has a null vertex.
      ptr = getIds(ptr, NODE, nodeIds);
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 1000: // ERR  1000 :  1 3 2
      // Face (f 1, f 2, f 3) appears more than once in the input surface mesh.
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 1001:
      // Edge (e1, e2) appears more than once in the input surface mesh
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 1002:
      // Face (f 1, f 2, f 3) has a vertex negative or null
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 2004:
      // Vertex v1 and vertex v2 are too close to one another or coincident (warning).
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 2012:
      // Vertex v1 cannot be inserted (warning).
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 2014:
      // There are at least two points whose distance is dist, i.e., considered as coincident
    case 2103: // ERR  2103 :  16 WITH  3
      // Vertex v1 and vertex v2 are too close to one another or coincident (warning).
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3009:
      // Constrained edge (e1, e2) cannot be enforced (warning).
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3019:
      // Constrained face (f 1, f 2, f 3) cannot be enforced
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3103: // ERR  3103 :  1 2 WITH  7 3
      // The surface edge (e1, e2) intersects another surface edge (e3, e4)
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3104: // ERR  3104 :  9 10 WITH  1 2 3
      // The surface edge (e1, e2) intersects the surface face (f 1, f 2, f 3)
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3105: // ERR  3105 :  8 IN  2 3 5
      // One boundary point (say p1) lies within a surface face (f 1, f 2, f 3)
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3106:
      // One surface edge (say e1, e2) intersects a surface face (f 1, f 2, f 3)
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, TRIA, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3107: // ERR  3107 :  2 IN  4 1
      // One boundary point (say p1) lies within a surface edge (e1, e2) (stop).
      ptr = getIds(ptr, NODE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 3109: // ERR  3109 :  EDGE  5 6 UNIQUE
      // Edge (e1, e2) is unique (i.e., bounds a hole in the surface)
      ptr = getIds(ptr, EDGE, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      break;
    case 9000: // ERR  9000 
      //  ELEMENT  261 WITH VERTICES :  7 396 -8 242 
      //  VOLUME   : -1.11325045E+11 W.R.T. EPSILON   0.
      // A too small volume element is detected. Are reported the index of the element,
      // its four vertex indices, its volume and the tolerance threshold value
      ptr = getIds(ptr, ID, nodeIds);
      ptr = getIds(ptr, VOL, nodeIds);
      badElems.push_back( toSmdsConvertor.getElement(nodeIds));
      // even if all nodes found, volume it most probably invisible,
      // add its faces to demenstrate it anyhow
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
      ptr = getIds(ptr, ID, nodeIds);
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
    char msg[] = "connection to server failed";
    if ( search( &buf[0], bufEnd, msg, msg + strlen(msg)) != bufEnd )
      errDescription << "Licence problems.";
    else
    {
      char msg2[] = "SEGMENTATION FAULT";
      if ( search( &buf[0], bufEnd, msg2, msg2 + strlen(msg2)) != bufEnd )
        errDescription << "ghs3d: SEGMENTATION FAULT. ";
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

_Ghs2smdsConvertor::_Ghs2smdsConvertor( const map <int,const SMDS_MeshNode*> & ghs2NodeMap)
  :_ghs2NodeMap( & ghs2NodeMap ), _nodeByGhsId( 0 )
{
}

//================================================================================
/*!
 * \brief Creates _Ghs2smdsConvertor
 */
//================================================================================

_Ghs2smdsConvertor::_Ghs2smdsConvertor( const vector <const SMDS_MeshNode*> &  nodeByGhsId)
  : _ghs2NodeMap( 0 ), _nodeByGhsId( &nodeByGhsId )
{
}

//================================================================================
/*!
 * \brief Return SMDS element by ids of GHS3D nodes
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
    if ( !edge )
      edge = new SMDS_LinearEdge( nodes[0], nodes[1] );
    return edge;
  }
  if ( nbNodes == 3 ) {
    const SMDS_MeshElement* face = SMDS_Mesh::FindFace( nodes );
    if ( !face )
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
  SMESH_MesherHelper* helper = new SMESH_MesherHelper(theMesh );
  TIDSortedElemSet dummyElemSet;
  TIDSortedNodeSet dummyNodeSet;
  return readGMFFile(theGMFFileName, 
#ifdef WITH_SMESH_CANCEL_COMPUTE
                   this,
#endif
                   helper, dummyNodeSet , dummyElemSet, dummyElemSet);
}
