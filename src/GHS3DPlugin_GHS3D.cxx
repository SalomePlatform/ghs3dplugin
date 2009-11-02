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
//=============================================================================
// File      : GHS3DPlugin_GHS3D.cxx
// Created   : 
// Author    : Edward AGAPOV, modified by Lioka RAZAFINDRAZAKA (CEA) 09/02/2007
// Project   : SALOME
// $Header$
//=============================================================================
//
#include "GHS3DPlugin_GHS3D.hxx"
#include "GHS3DPlugin_Hypothesis.hxx"


#include <Basics_Utils.hxx>

#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_MeshEditor.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMDS_FaceOfNodes.hxx"
#include "SMDS_VolumeOfNodes.hxx"

#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
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
  _requireShape = false; // can work without shape
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

  // there is only one compatible Hypothesis so far
  _hyp = 0;
  _keepFiles = false;
  const list <const SMESHDS_Hypothesis * >& hyps = GetUsedHypothesis(aMesh, aShape);
  if ( !hyps.empty() )
    _hyp = static_cast<const GHS3DPlugin_Hypothesis*> ( hyps.front() );
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

//=======================================================================
//function : countShape
//purpose  :
//=======================================================================

template < class Mesh, class Shape >
static int countShape( Mesh* mesh, Shape shape ) {
  TopExp_Explorer expShape ( mesh->ShapeToMesh(), shape );
  int nbShape = 0;
  for ( ; expShape.More(); expShape.Next() ) {
      nbShape++;
  }
  return nbShape;
}

//=======================================================================
//function : writeFaces
//purpose  : 
//=======================================================================

static bool writeFaces (ofstream &            theFile,
                        SMESHDS_Mesh *        theMesh,
                        const map <int,int> & theSmdsToGhs3dIdMap)
{
  // record structure:
  //
  // NB_ELEMS DUMMY_INT
  // Loop from 1 to NB_ELEMS
  // NB_NODES NODE_NB_1 NODE_NB_2 ... (NB_NODES + 1) times: DUMMY_INT

  int nbShape = countShape( theMesh, TopAbs_FACE );

  int *tabID;             tabID    = new int[nbShape];
  TopoDS_Shape *tabShape; tabShape = new TopoDS_Shape[nbShape];
  TopoDS_Shape aShape;
  SMESHDS_SubMesh* theSubMesh;
  const SMDS_MeshElement* aFace;
  const char* space    = "  ";
  const int   dummyint = 0;
  map<int,int>::const_iterator itOnMap;
  SMDS_ElemIteratorPtr itOnSubMesh, itOnSubFace;
  int shapeID, nbNodes, aSmdsID;
  bool idFound;

  std::cout << "    " << theMesh->NbFaces() << " shapes of 2D dimension" << std::endl;
  std::cout << std::endl;

  theFile << space << theMesh->NbFaces() << space << dummyint << std::endl;

  TopExp_Explorer expface( theMesh->ShapeToMesh(), TopAbs_FACE );
  for ( int i = 0; expface.More(); expface.Next(), i++ ) {
    tabID[i] = 0;
    aShape   = expface.Current();
    shapeID  = theMesh->ShapeToIndex( aShape );
    idFound  = false;
    for ( int j=0; j<=i; j++) {
      if ( shapeID == tabID[j] ) {
        idFound = true;
        break;
      }
    }
    if ( ! idFound ) {
      tabID[i]    = shapeID;
      tabShape[i] = aShape;
    }
  }
  for ( int i =0; i < nbShape; i++ ) {
    if ( tabID[i] != 0 ) {
      aShape      = tabShape[i];
      shapeID     = tabID[i];
      theSubMesh  = theMesh->MeshElements( aShape );
      if ( !theSubMesh ) continue;
      itOnSubMesh = theSubMesh->GetElements();
      while ( itOnSubMesh->more() ) {
        aFace   = itOnSubMesh->next();
        nbNodes = aFace->NbNodes();

        theFile << space << nbNodes;

        itOnSubFace = aFace->nodesIterator();
        while ( itOnSubFace->more() ) {
          // find GHS3D ID
          aSmdsID = itOnSubFace->next()->GetID();
          itOnMap = theSmdsToGhs3dIdMap.find( aSmdsID );
          ASSERT( itOnMap != theSmdsToGhs3dIdMap.end() );

          theFile << space << (*itOnMap).second;
        }

        // (NB_NODES + 1) times: DUMMY_INT
        for ( int j=0; j<=nbNodes; j++)
          theFile << space << dummyint;

        theFile << std::endl;
      }
    }
  }
  
  
  delete [] tabID;
  delete [] tabShape;

  return true;
}

//=======================================================================
//function : writeFaces
//purpose  : Write Faces in case if generate 3D mesh w/o geometry
//=======================================================================

static bool writeFaces (ofstream &            theFile,
                        SMESHDS_Mesh *        theMesh,
                        vector <const SMDS_MeshNode*> & theNodeByGhs3dId)
{
  // record structure:
  //
  // NB_ELEMS DUMMY_INT
  // Loop from 1 to NB_ELEMS
  //   NB_NODES NODE_NB_1 NODE_NB_2 ... (NB_NODES + 1) times: DUMMY_INT


  int nbFaces = 0;
  list< const SMDS_MeshElement* > faces;
  list< const SMDS_MeshElement* >::iterator f;
  map< const SMDS_MeshNode*,int >::iterator it;
  SMDS_ElemIteratorPtr nodeIt;
  const SMDS_MeshElement* elem;
  int nbNodes;

  const char* space    = "  ";
  const int   dummyint = 0;

  //get all faces from mesh
  SMDS_FaceIteratorPtr eIt = theMesh->facesIterator();
  while ( eIt->more() ) {
    const SMDS_MeshElement* elem = eIt->next();
    if ( !elem )
      return false;
    faces.push_back( elem );
    nbFaces++;
  }

  if ( nbFaces == 0 )
    return false;
  
  std::cout << "The initial 2D mesh contains " << nbFaces << " faces and ";

  // NB_ELEMS DUMMY_INT
  theFile << space << nbFaces << space << dummyint << std::endl;

  // Loop from 1 to NB_ELEMS

  map<const SMDS_MeshNode*,int> aNodeToGhs3dIdMap;
  f = faces.begin();
  for ( ; f != faces.end(); ++f )
  {
    // NB_NODES PER FACE
    elem = *f;
    nbNodes = elem->NbNodes();
    theFile << space << nbNodes;

    // NODE_NB_1 NODE_NB_2 ...
    nodeIt = elem->nodesIterator();
    while ( nodeIt->more() )
    {
      // find GHS3D ID
      const SMDS_MeshNode* node = castToNode( nodeIt->next() );
      int newId = aNodeToGhs3dIdMap.size() + 1; // ghs3d ids count from 1
      it = aNodeToGhs3dIdMap.insert( make_pair( node, newId )).first;
      theFile << space << it->second;
    }

    // (NB_NODES + 1) times: DUMMY_INT
    for ( int i=0; i<=nbNodes; i++)
      theFile << space << dummyint;

    theFile << std::endl;
  }

  // put nodes to theNodeByGhs3dId vector
  theNodeByGhs3dId.resize( aNodeToGhs3dIdMap.size() );
  map<const SMDS_MeshNode*,int>::const_iterator n2id = aNodeToGhs3dIdMap.begin();
  for ( ; n2id != aNodeToGhs3dIdMap.end(); ++ n2id)
  {
    theNodeByGhs3dId[ n2id->second - 1 ] = n2id->first; // ghs3d ids count from 1
  }

  return true;
}

//=======================================================================
//function : writePoints
//purpose  : 
//=======================================================================

static bool writePoints (ofstream &                                     theFile,
                         SMESHDS_Mesh *                                 theMesh,
                         map <int,int> &                                theSmdsToGhs3dIdMap,
                         map <int,const SMDS_MeshNode*> &               theGhs3dIdToNodeMap,
                         map<vector<double>,double> & theEnforcedVertices)
{
  // record structure:
  //
  // NB_NODES
  // Loop from 1 to NB_NODES
  //   X Y Z DUMMY_INT

  int nbNodes = theMesh->NbNodes();
  if ( nbNodes == 0 )
    return false;
  int nbEnforcedVertices = theEnforcedVertices.size();

  const char* space    = "  ";
  const int   dummyint = 0;

  int aGhs3dID = 1;
  SMDS_NodeIteratorPtr it = theMesh->nodesIterator();
  const SMDS_MeshNode* node;

  // NB_NODES
  std::cout << std::endl;
  std::cout << "The initial 2D mesh contains :" << std::endl;
  std::cout << "    " << nbNodes << " nodes" << std::endl;
  if (nbEnforcedVertices > 0) {
    std::cout << "    " << nbEnforcedVertices << " enforced vertices" << std::endl;
  }
  std::cout << std::endl;
  std::cout << "Start writing in 'points' file ..." << std::endl;
  theFile << space << nbNodes << std::endl;

  // Loop from 1 to NB_NODES

  while ( it->more() )
  {
    node = it->next();
    theSmdsToGhs3dIdMap.insert( make_pair( node->GetID(), aGhs3dID ));
    theGhs3dIdToNodeMap.insert( make_pair( aGhs3dID, node ));
    aGhs3dID++;

    // X Y Z DUMMY_INT
    theFile
    << space << node->X()
    << space << node->Y()
    << space << node->Z()
    << space << dummyint;

    theFile << std::endl;

  }
  
  // Iterate over the enforced vertices
  GHS3DPlugin_Hypothesis::TEnforcedVertexValues::const_iterator vertexIt;
  const TopoDS_Shape shapeToMesh = theMesh->ShapeToMesh();
  for(vertexIt = theEnforcedVertices.begin() ; vertexIt != theEnforcedVertices.end() ; ++vertexIt) {
    double x = vertexIt->first[0];
    double y = vertexIt->first[1];
    double z = vertexIt->first[2];
    // Test if point is inside shape to mesh
    gp_Pnt myPoint(x,y,z);
    BRepClass3d_SolidClassifier scl(shapeToMesh);
    scl.Perform(myPoint, 1e-7);
    TopAbs_State result = scl.State();
    if ( result == TopAbs_IN ) {
        MESSAGE("Adding enforced vertex (" << x << "," << y <<"," << z << ") = " << vertexIt->second);
        // X Y Z PHY_SIZE DUMMY_INT
        theFile
        << space << x
        << space << y
        << space << z
        << space << vertexIt->second
        << space << dummyint;
    
        theFile << std::endl;
    }
    else
        MESSAGE("Enforced vertex (" << x << "," << y <<"," << z << ") is not inside the geometry: it was not added ");
  }
  
  
  std::cout << std::endl;
  std::cout << "End writing in 'points' file." << std::endl;

  return true;
}

//=======================================================================
//function : writePoints
//purpose  : 
//=======================================================================

static bool writePoints (ofstream &                                     theFile,
                         SMESHDS_Mesh *                                 theMesh,
                         const vector <const SMDS_MeshNode*> &          theNodeByGhs3dId,
                         map<vector<double>,double> & theEnforcedVertices)
{
  // record structure:
  //
  // NB_NODES
  // Loop from 1 to NB_NODES
  //   X Y Z DUMMY_INT
  
  //int nbNodes = theMesh->NbNodes();
  int nbNodes = theNodeByGhs3dId.size();
  if ( nbNodes == 0 )
    return false;

  int nbEnforcedVertices = theEnforcedVertices.size();

  const char* space    = "  ";
  const int   dummyint = 0;

  const SMDS_MeshNode* node;

  // NB_NODES
  std::cout << std::endl;
  std::cout << "The initial 2D mesh contains :" << std::endl;
  std::cout << "    " << nbNodes << " nodes" << std::endl;
  std::cout << "    " << nbEnforcedVertices << " enforced vertices" << std::endl;
  std::cout << std::endl;
  std::cout << "Start writing in 'points' file ..." << std::endl;
  theFile << space << nbNodes << std::endl;

  // Loop from 1 to NB_NODES

  vector<const SMDS_MeshNode*>::const_iterator nodeIt = theNodeByGhs3dId.begin();
  vector<const SMDS_MeshNode*>::const_iterator after  = theNodeByGhs3dId.end();
  for ( ; nodeIt != after; ++nodeIt )
  {
    node = *nodeIt;

    // X Y Z DUMMY_INT
    theFile
    << space << node->X()
    << space << node->Y()
    << space << node->Z()
    << space << dummyint;

    theFile << std::endl;

  }
  
  // Iterate over the enforced vertices
  GHS3DPlugin_Hypothesis::TEnforcedVertexValues::const_iterator vertexIt;
  const TopoDS_Shape shapeToMesh = theMesh->ShapeToMesh();
  for(vertexIt = theEnforcedVertices.begin() ; vertexIt != theEnforcedVertices.end() ; ++vertexIt) {
    double x = vertexIt->first[0];
    double y = vertexIt->first[1];
    double z = vertexIt->first[2];
    // Test if point is inside shape to mesh
    gp_Pnt myPoint(x,y,z);
    BRepClass3d_SolidClassifier scl(shapeToMesh);
    scl.Perform(myPoint, 1e-7);
    TopAbs_State result = scl.State();
    if ( result == TopAbs_IN ) {
        std::cout << "Adding enforced vertex (" << x << "," << y <<"," << z << ") = " << vertexIt->second << std::endl;

        // X Y Z PHY_SIZE DUMMY_INT
        theFile
        << space << x
        << space << y
        << space << z
        << space << vertexIt->second
        << space << dummyint;
    
        theFile << std::endl;
    }
  }
  std::cout << std::endl;
  std::cout << "End writing in 'points' file." << std::endl;

  return true;
}

//=======================================================================
//function : findShapeID
//purpose  : find the solid corresponding to GHS3D sub-domain following
//           the technique proposed in GHS3D manual in chapter
//           "B.4 Subdomain (sub-region) assignment"
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
    return invalidID;

  // geom face the face assigned to
  SMESH_MeshEditor editor(&mesh);
  int geomFaceID = editor.FindShape( face );
  if ( !geomFaceID )
    return invalidID;
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

  // find UV of node1 on geomFace
  SMESH_MesherHelper helper( mesh ); helper.SetSubShape( geomFace );
  const SMDS_MeshNode* nNotOnSeamEdge = 0;
  if ( helper.IsSeamShape( node1->GetPosition()->GetShapeId() ))
    if ( helper.IsSeamShape( node2->GetPosition()->GetShapeId() ))
      nNotOnSeamEdge = node3;
    else
      nNotOnSeamEdge = node2;
  bool uvOK;
  gp_XY uv = helper.GetNodeUV( geomFace, node1, nNotOnSeamEdge, &uvOK );
  // check that uv is correct
  double tol = 1e-6;
  TopoDS_Shape nodeShape = helper.GetSubShapeByNode( node1, meshDS );
  if ( !nodeShape.IsNull() )
    switch ( nodeShape.ShapeType() )
    {
    case TopAbs_FACE:   tol = BRep_Tool::Tolerance( TopoDS::Face( nodeShape )); break;
    case TopAbs_EDGE:   tol = BRep_Tool::Tolerance( TopoDS::Edge( nodeShape )); break;
    case TopAbs_VERTEX: tol = BRep_Tool::Tolerance( TopoDS::Vertex( nodeShape )); break;
    default:;
    }
  BRepAdaptor_Surface surface( geomFace );
  if ( !uvOK || node1Pnt.Distance( surface.Value( uv.X(), uv.Y() )) > 2 * tol )
      return invalidID;

  // normale to geomFace at UV
  gp_Vec du, dv;
  surface.D1( uv.X(), uv.Y(), node1Pnt, du, dv );
  gp_Vec geomNormal = du ^ dv;
  if ( geomNormal.SquareMagnitude() < DBL_MIN )
    return findShapeID( mesh, node2, node3, node1, toMeshHoles );
  if ( geomFace.Orientation() == TopAbs_REVERSED )
    geomNormal.Reverse();

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
//function : readResultFile
//purpose  : 
//=======================================================================

static bool readResultFile(const int                       fileOpen,
#ifdef WNT
                           const char*                     fileName,
#endif
                           SMESH_Mesh&                     theMesh,
                           TopoDS_Shape                    tabShape[],
                           double**                        tabBox,
                           const int                       nbShape,
                           map <int,const SMDS_MeshNode*>& theGhs3dIdToNodeMap,
                           bool                            toMeshHoles,
                           int                             nbEnforcedVertices)
{
  MESSAGE("GHS3DPlugin_GHS3D::readResultFile()");
  Kernel_Utils::Localizer loc;
  struct stat status;
  size_t      length;

  char *ptr, *mapPtr;
  char *tetraPtr;
  char *shapePtr;

  SMESHDS_Mesh* theMeshDS = theMesh.GetMeshDS();

  int fileStat;
  int nbElems, nbNodes, nbInputNodes;
  int nodeId/*, triangleId*/;
  int nbTriangle;
  int ID, shapeID, ghs3dShapeID;
  int IdShapeRef = 1;
  int compoundID =
    nbShape ? theMeshDS->ShapeToIndex( tabShape[0] ) : theMeshDS->ShapeToIndex( theMeshDS->ShapeToMesh() );

  int *tab, *tabID, *nodeID, *nodeAssigne;
  double *coord;
  const SMDS_MeshNode **node;

  tab    = new int[3];
  //tabID  = new int[nbShape];
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
    for (int iCoor=0; iCoor < 3; iCoor++)
      coord[ iCoor ] = strtod(ptr, &ptr);
    nodeAssigne[ iNode ] = 1;
    if ( iNode > (nbInputNodes-nbEnforcedVertices) ) {
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
        tabID[i] = findShapeID( theMesh, n1, n2, n3, toMeshHoles );
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
      } catch ( Standard_Failure ) {
      } catch (...) {}
    }
  }

  shapePtr = ptr;

  if ( nbTriangle <= nbShape ) // no holes
    toMeshHoles = true; // not avoid creating tetras in holes

  // Associating the tetrahedrons to the shapes
  shapeID = compoundID;
  for (int iElem = 0; iElem < nbElems; iElem++) {
    for (int iNode = 0; iNode < 4; iNode++) {
      ID = strtol(tetraPtr, &tetraPtr, 10);
      itOnNode = theGhs3dIdToNodeMap.find(ID);
      node[ iNode ] = itOnNode->second;
      nodeID[ iNode ] = ID;
    }
    // We always run GHS3D with "to mesh holes'==TRUE but we must not create
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
             node[i]->GetPosition()->GetShapeId() > 1 )
        {
          shapeID = node[i]->GetPosition()->GetShapeId();
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
      aTet = theMeshDS->AddVolume( node[1], node[0], node[2], node[3] );
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

//=======================================================================
//function : readResultFile
//purpose  : 
//=======================================================================

static bool readResultFile(const int                      fileOpen,
#ifdef WNT
                           const char*                    fileName,
#endif
                           SMESHDS_Mesh*                  theMeshDS,
                           TopoDS_Shape                   aSolid,
                           vector <const SMDS_MeshNode*>& theNodeByGhs3dId,
                           int                            nbEnforcedVertices) {

  Kernel_Utils::Localizer loc;
  struct stat  status;
  size_t       length;

  char *ptr, *mapPtr;
  char *tetraPtr;
  char *shapePtr;

  int fileStat;
  int nbElems, nbNodes, nbInputNodes;
  int nodeId, triangleId;
  int nbTriangle;
  int ID, shapeID;

  int *tab;
  double *coord;
  const SMDS_MeshNode **node;

  tab   = new int[3];
  coord = new double[3];
  node  = new const SMDS_MeshNode*[4];

  SMDS_MeshNode * aNewNode;
  map <int,const SMDS_MeshNode*>::iterator IdNode;
  SMDS_MeshElement* aTet;

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

  theNodeByGhs3dId.resize( nbNodes );

  // Reading the nodeId
  for (int i=0; i < 4*nbElems; i++)
    nodeId = strtol(ptr, &ptr, 10);

  // Reading the nodeCoor and update the nodeMap
  shapeID = theMeshDS->ShapeToIndex( aSolid );
  for (int iNode=0; iNode < nbNodes; iNode++) {
    for (int iCoor=0; iCoor < 3; iCoor++)
      coord[ iCoor ] = strtod(ptr, &ptr);
    if ((iNode+1) > (nbInputNodes-nbEnforcedVertices)) {
      aNewNode = theMeshDS->AddNode( coord[0],coord[1],coord[2] );
      theMeshDS->SetNodeInVolume( aNewNode, shapeID );
      theNodeByGhs3dId[ iNode ] = aNewNode;
    }
  }

  // Reading the triangles
  nbTriangle = strtol(ptr, &ptr, 10);

  for (int i=0; i < 3*nbTriangle; i++)
    triangleId = strtol(ptr, &ptr, 10);

  shapePtr = ptr;

  // Associating the tetrahedrons to the shapes
  for (int iElem = 0; iElem < nbElems; iElem++) {
    for (int iNode = 0; iNode < 4; iNode++) {
      ID = strtol(tetraPtr, &tetraPtr, 10);
      node[ iNode ] = theNodeByGhs3dId[ ID-1 ];
    }
    aTet = theMeshDS->AddVolume( node[1], node[0], node[2], node[3] );
    shapeID = theMeshDS->ShapeToIndex( aSolid );
    theMeshDS->SetMeshElementOnShape( aTet, shapeID );
  }
  if ( nbElems )
    cout << nbElems << " tetrahedrons have been associated to " << nbTriangle << " shapes" << endl;
#ifdef WNT
  UnmapViewOfFile(mapPtr);
  CloseHandle(hMapObject);
  CloseHandle(fd);
#else
  munmap(mapPtr, length);
#endif
  close(fileOpen);

  delete [] tab;
  delete [] coord;
  delete [] node;

  return true;
}

//=============================================================================
/*!
 *Here we are going to use the GHS3D mesher
 */
//=============================================================================

bool GHS3DPlugin_GHS3D::Compute(SMESH_Mesh&         theMesh,
                                const TopoDS_Shape& theShape)
{
  bool Ok(false);
  SMESHDS_Mesh* meshDS = theMesh.GetMeshDS();

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

  // TopExp_Explorer expBox (meshDS->ShapeToMesh(), TopAbs_SOLID); -- 0020330:...ghs3d as a submesh
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

  TCollection_AsciiString aFacesFileName, aPointsFileName, aResultFileName;
  TCollection_AsciiString aBadResFileName, aBbResFileName, aLogFileName;
  aFacesFileName  = aGenericName + ".faces";  // in faces
  aPointsFileName = aGenericName + ".points"; // in points
  aResultFileName = aGenericName + ".noboite";// out points and volumes
  aBadResFileName = aGenericName + ".boite";  // out bad result
  aBbResFileName  = aGenericName + ".bb";     // out vertex stepsize
  aLogFileName    = aGenericName + ".log";    // log

  // -----------------
  // make input files
  // -----------------

  ofstream aFacesFile  ( aFacesFileName.ToCString()  , ios::out);
  ofstream aPointsFile ( aPointsFileName.ToCString() , ios::out);

  Ok =
    aFacesFile.rdbuf()->is_open() && aPointsFile.rdbuf()->is_open();
  if (!Ok) {
    INFOS( "Can't write into " << aFacesFileName);
    return error(SMESH_Comment("Can't write into ") << aFacesFileName);
  }
  map <int,int> aSmdsToGhs3dIdMap;
  map <int,const SMDS_MeshNode*> aGhs3dIdToNodeMap;
  map<vector<double>,double> enforcedVertices;
  int nbEnforcedVertices = 0;
  try {
    enforcedVertices = GHS3DPlugin_Hypothesis::GetEnforcedVertices(_hyp);
    nbEnforcedVertices = enforcedVertices.size();
  }
  catch(...) {
  }
  
  Ok = writePoints( aPointsFile, meshDS, aSmdsToGhs3dIdMap, aGhs3dIdToNodeMap, enforcedVertices) &&
       writeFaces ( aFacesFile,  meshDS, aSmdsToGhs3dIdMap );

  aFacesFile.close();
  aPointsFile.close();
  
  if ( ! Ok ) {
    if ( !_keepFiles ) {
      removeFile( aFacesFileName );
      removeFile( aPointsFileName );
    }
    return error(COMPERR_BAD_INPUT_MESH);
  }
  removeFile( aResultFileName ); // needed for boundary recovery module usage

  // -----------------
  // run ghs3d mesher
  // -----------------

  TCollection_AsciiString cmd( (char*)GHS3DPlugin_Hypothesis::CommandToRun( _hyp ).c_str() );
  cmd += TCollection_AsciiString(" -f ") + aGenericName;  // file to read
  cmd += TCollection_AsciiString(" 1>" ) + aLogFileName;  // dump into file

  std::cout << std::endl;
  std::cout << "Ghs3d execution..." << std::endl;
  std::cout << cmd << std::endl;

  system( cmd.ToCString() ); // run

  std::cout << std::endl;
  std::cout << "End of Ghs3d execution !" << std::endl;

  // --------------
  // read a result
  // --------------

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
    Ok = readResultFile( fileOpen,
#ifdef WNT
                         aResultFileName.ToCString(),
#endif
                         theMesh, tabShape, tabBox, _nbShape, aGhs3dIdToNodeMap,
                         toMeshHoles, nbEnforcedVertices );
  }

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
    removeFile( aFacesFileName );
    removeFile( aPointsFileName );
    removeFile( aResultFileName );
    removeFile( aBadResFileName );
    removeFile( aBbResFileName );
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
                                SMESH_MesherHelper* aHelper)
{
  MESSAGE("GHS3DPlugin_GHS3D::Compute()");

  SMESHDS_Mesh* meshDS = theMesh.GetMeshDS();
  TopoDS_Shape theShape = aHelper->GetSubShape();

  // a unique working file name
  // to avoid access to the same files by eg different users
  TCollection_AsciiString aGenericName
    = (char*) GHS3DPlugin_Hypothesis::GetFileName(_hyp).c_str();

  TCollection_AsciiString aFacesFileName, aPointsFileName, aResultFileName;
  TCollection_AsciiString aBadResFileName, aBbResFileName, aLogFileName;
  aFacesFileName  = aGenericName + ".faces";  // in faces
  aPointsFileName = aGenericName + ".points"; // in points
  aResultFileName = aGenericName + ".noboite";// out points and volumes
  aBadResFileName = aGenericName + ".boite";  // out bad result
  aBbResFileName  = aGenericName + ".bb";     // out vertex stepsize
  aLogFileName    = aGenericName + ".log";    // log

  // -----------------
  // make input files
  // -----------------

  ofstream aFacesFile  ( aFacesFileName.ToCString()  , ios::out);
  ofstream aPointsFile  ( aPointsFileName.ToCString()  , ios::out);
  bool Ok =
    aFacesFile.rdbuf()->is_open() && aPointsFile.rdbuf()->is_open();

  if (!Ok)
    return error( SMESH_Comment("Can't write into ") << aPointsFileName);
  
  GHS3DPlugin_Hypothesis::TEnforcedVertexValues enforcedVertices;
  int nbEnforcedVertices = 0;
  try {
    enforcedVertices = GHS3DPlugin_Hypothesis::GetEnforcedVertices(_hyp);
    nbEnforcedVertices = enforcedVertices.size();
  }
  catch(...) {
  }

  vector <const SMDS_MeshNode*> aNodeByGhs3dId;

  Ok = (writeFaces ( aFacesFile, meshDS, aNodeByGhs3dId ) &&
        writePoints( aPointsFile, meshDS, aNodeByGhs3dId,enforcedVertices));
  
  aFacesFile.close();
  aPointsFile.close();
  
  if ( ! Ok ) {
    if ( !_keepFiles ) {
      removeFile( aFacesFileName );
      removeFile( aPointsFileName );
    }
    return error(COMPERR_BAD_INPUT_MESH);
  }
  removeFile( aResultFileName ); // needed for boundary recovery module usage

  // -----------------
  // run ghs3d mesher
  // -----------------

  TCollection_AsciiString cmd =
    (char*)GHS3DPlugin_Hypothesis::CommandToRun( _hyp, false ).c_str();
  cmd += TCollection_AsciiString(" -f ") + aGenericName;  // file to read
  cmd += TCollection_AsciiString(" 1>" ) + aLogFileName;  // dump into file
  
  system( cmd.ToCString() ); // run

  // --------------
  // read a result
  // --------------
  int fileOpen;
  fileOpen = open( aResultFileName.ToCString(), O_RDONLY);
  if ( fileOpen < 0 ) {
    std::cout << std::endl;
    std::cout << "Error when opening the " << aResultFileName.ToCString() << " file" << std::endl;
    std::cout << "Log: " << aLogFileName << std::endl;
    std::cout << std::endl;
    Ok = false;
  }
  else {
    Ok = readResultFile( fileOpen,
#ifdef WNT
                         aResultFileName.ToCString(),
#endif
                         meshDS, theShape ,aNodeByGhs3dId, nbEnforcedVertices );
  }
  
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

  if ( !_keepFiles )
  {
    removeFile( aFacesFileName );
    removeFile( aPointsFileName );
    removeFile( aResultFileName );
    removeFile( aBadResFileName );
    removeFile( aBbResFileName );
  }
  
  return Ok;
}

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
      edge = new SMDS_MeshEdge( nodes[0], nodes[1] );
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

