// Copyright (C) 2005  CEA/DEN, EDF R&D, OPEN CASCADE, PRINCIPIA R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License.
//
// This library is distributed in the hope that it will be useful
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
// Copyright : CEA 2003
// $Header$
//=============================================================================
using namespace std;

#include "GHS3DPlugin_GHS3D.hxx"
#include "GHS3DPlugin_Hypothesis.hxx"

#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_MesherHelper.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"

#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <OSD_File.hxx>

#include "utilities.h"

#ifndef WIN32
#include <sys/sysinfo.h>
#endif

//#include <Standard_Stream.hxx>

#include <BRepGProp.hxx>
#include <BRepBndLib.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <TopAbs.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>

#define castToNode(n) static_cast<const SMDS_MeshNode *>( n );

#ifdef _DEBUG_
#define DUMP(txt) \
//  cout << txt
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
                              const int           nShape) {
  double *pntCoor;
  int iShape, nbNode = 4;

  pntCoor = new double[3];
  for ( int i=0; i<3; i++ ) {
    pntCoor[i] = 0;
    for ( int j=0; j<nbNode; j++ ) {
      if ( i == 0) pntCoor[i] += aNode[j]->X();
      if ( i == 1) pntCoor[i] += aNode[j]->Y();
      if ( i == 2) pntCoor[i] += aNode[j]->Z();
    }
    pntCoor[i] /= nbNode;
  }

  gp_Pnt aPnt(pntCoor[0], pntCoor[1], pntCoor[2]);
  BRepClass3d_SolidClassifier SC (aShape, aPnt, Precision::Confusion());
  if ( not(SC.State() == TopAbs_IN) ) {
    for (iShape = 0; iShape < nShape; iShape++) {
      aShape = shape[iShape];
      if ( not( pntCoor[0] < box[iShape][0] || box[iShape][1] < pntCoor[0] ||
                pntCoor[1] < box[iShape][2] || box[iShape][3] < pntCoor[1] ||
                pntCoor[2] < box[iShape][4] || box[iShape][5] < pntCoor[2]) ) {
        BRepClass3d_SolidClassifier SC (aShape, aPnt, Precision::Confusion());
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
  cout << endl;

  for ( int i=0; i<17; i++ ) {
    intVal = strtol(ptr, &ptr, 10);
    if ( i < 3 )
      tab[i] = intVal;
  }
  return ptr;
}

//=======================================================================
//function : readLine
//purpose  : 
//=======================================================================

#define GHS3DPlugin_BUFLENGTH 256
#define GHS3DPlugin_ReadLine(aPtr,aBuf,aFile,aLineNb) \
{  aPtr = fgets( aBuf, GHS3DPlugin_BUFLENGTH - 2, aFile ); aLineNb++; DUMP(endl); }

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

  cout << "    " << theMesh->NbFaces() << " shapes of 2D dimension" << endl;
  cout << endl;

  theFile << space << theMesh->NbFaces() << space << dummyint << endl;

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
    if ( not idFound ) {
      tabID[i]    = shapeID;
      tabShape[i] = aShape;
    }
  }
  for ( int i =0; i < nbShape; i++ ) {
    if ( not (tabID[i] == 0) ) {
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

        theFile << endl;
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
  
  cout << "The initial 2D mesh contains " << nbFaces << " faces and ";

  // NB_ELEMS DUMMY_INT
  theFile << space << nbFaces << space << dummyint << endl;

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

    theFile << endl;
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

static bool writePoints (ofstream &                       theFile,
                         SMESHDS_Mesh *                   theMesh,
                         map <int,int> &                  theSmdsToGhs3dIdMap,
                         map <int,const SMDS_MeshNode*> & theGhs3dIdToNodeMap)
{
  // record structure:
  //
  // NB_NODES
  // Loop from 1 to NB_NODES
  //   X Y Z DUMMY_INT

  int nbNodes = theMesh->NbNodes();
  if ( nbNodes == 0 )
    return false;

  const char* space    = "  ";
  const int   dummyint = 0;

  int aGhs3dID = 1;
  SMDS_NodeIteratorPtr it = theMesh->nodesIterator();
  const SMDS_MeshNode* node;

  // NB_NODES
  theFile << space << nbNodes << endl;
  cout << endl;
  cout << "The initial 2D mesh contains :" << endl;
  cout << "    " << nbNodes << " vertices" << endl;

  // Loop from 1 to NB_NODES

  while ( it->more() )
  {
    node = it->next();
    theSmdsToGhs3dIdMap.insert( map <int,int>::value_type( node->GetID(), aGhs3dID ));
    theGhs3dIdToNodeMap.insert (map <int,const SMDS_MeshNode*>::value_type( aGhs3dID, node ));
    aGhs3dID++;

    // X Y Z DUMMY_INT
    theFile
      << space << node->X()
      << space << node->Y()
      << space << node->Z()
      << space << dummyint;

    theFile << endl;
  }

  return true;
}

//=======================================================================
//function : writePoints
//purpose  : 
//=======================================================================

static bool writePoints (ofstream &                            theFile,
                         SMESHDS_Mesh *                        theMesh,
                         const vector <const SMDS_MeshNode*> & theNodeByGhs3dId)
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

  const char* space    = "  ";
  const int   dummyint = 0;

  const SMDS_MeshNode* node;

  // NB_NODES
  theFile << space << nbNodes << endl;
  cout << nbNodes << " nodes" << endl;

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

    theFile << endl;
  }

  return true;
}

//=======================================================================
//function : readResultFile
//purpose  : 
//=======================================================================

static bool readResultFile(const int                       fileOpen,
                           SMESHDS_Mesh*                   theMeshDS,
                           TopoDS_Shape                    tabShape[],
                           double**                        tabBox,
                           const int                       nbShape,
                           map <int,const SMDS_MeshNode*>& theGhs3dIdToNodeMap) {

  struct stat status;
  size_t      length;

  char *ptr, *mapPtr;
  char *tetraPtr;
  char *shapePtr;

  int fileStat;
  int nbElems, nbNodes, nbInputNodes;
  int nodeId, triangleId;
  int nbTriangle;
  int ID, shapeID, ghs3dShapeID;
  int IdShapeRef = 1;
  int compoundID =
    nbShape ? theMeshDS->ShapeToIndex( tabShape[0] ) : theMeshDS->ShapeToIndex( theMeshDS->ShapeToMesh() );

  int *tab, *tabID, *nodeID, *nodeAssigne;
  double *coord;
  const SMDS_MeshNode **node;

  tab    = new int[3];
  tabID  = new int[nbShape];
  nodeID = new int[4];
  coord  = new double[3];
  node   = new const SMDS_MeshNode*[4];

  TopoDS_Shape aSolid;
  SMDS_MeshNode * aNewNode;
  map <int,const SMDS_MeshNode*>::iterator itOnNode;
  SMDS_MeshElement* aTet;

  for (int i=0; i<nbShape; i++)
    tabID[i] = 0;

  // Read the file state
  fileStat = fstat(fileOpen, &status);
  length   = status.st_size;

  // Mapping the result file into memory
  ptr = (char *) mmap(0,length,PROT_READ,MAP_PRIVATE,fileOpen,0);
  mapPtr = ptr;

  ptr      = readMapIntLine(ptr, tab);
  tetraPtr = ptr;

  nbElems      = tab[0];
  nbNodes      = tab[1];
  nbInputNodes = tab[2];

  nodeAssigne = new int[ nbNodes+1 ];

  // Reading the nodeId
  for (int i=0; i < 4*nbElems; i++)
    nodeId = strtol(ptr, &ptr, 10);

  // Reading the nodeCoor and update the nodeMap
  for (int iNode=0; iNode < nbNodes; iNode++) {
    for (int iCoor=0; iCoor < 3; iCoor++)
      coord[ iCoor ] = strtod(ptr, &ptr);
    nodeAssigne[ iNode+1 ] = 1;
    if ( (iNode+1) > nbInputNodes ) {
      nodeAssigne[ iNode+1 ] = 0;
      aNewNode = theMeshDS->AddNode( coord[0],coord[1],coord[2] );
      theGhs3dIdToNodeMap.insert(make_pair( (iNode+1), aNewNode ));
    }
  }

  // Reading the number of triangles which corresponds to the number of shapes
  nbTriangle = strtol(ptr, &ptr, 10);

  for (int i=0; i < 3*nbShape; i++)
    triangleId = strtol(ptr, &ptr, 10);

  shapePtr = ptr;

  // Associating the tetrahedrons to the shapes
  shapeID = compoundID;
  for (int iElem = 0; iElem < nbElems; iElem++) {
    for (int iNode = 0; iNode < 4; iNode++) {
      ID = strtol(tetraPtr, &tetraPtr, 10);
      itOnNode = theGhs3dIdToNodeMap.find(ID);
      node[ iNode ] = itOnNode->second;
      nodeID[ iNode ] = ID;
    }
    aTet = theMeshDS->AddVolume( node[1], node[0], node[2], node[3] );
    if ( nbShape > 1 ) {
      ghs3dShapeID = strtol(shapePtr, &shapePtr, 10) - IdShapeRef;
      if ( tabID[ ghs3dShapeID ] == 0 ) {
        if (iElem == 0)
          aSolid = tabShape[0];
        aSolid = findShape(node, aSolid, tabShape, tabBox, nbShape);
        shapeID = theMeshDS->ShapeToIndex( aSolid );
        tabID[ ghs3dShapeID ] = shapeID;
      }
      else
        shapeID = tabID[ ghs3dShapeID ];
    }
    // set new nodes and tetrahedron on to the shape
    for ( int i=0; i<4; i++ ) {
      if ( nodeAssigne[ nodeID[i] ] == 0 )
        theMeshDS->SetNodeInVolume( node[i], shapeID );
    }
    theMeshDS->SetMeshElementOnShape( aTet, shapeID );
  }
  if ( nbElems )
    cout << nbElems << " tetrahedrons have been associated to " << nbShape << " shapes" << endl;
  munmap(mapPtr, length);
  close(fileOpen);

  delete [] tab;
  delete [] tabID;
  delete [] nodeID;
  delete [] coord;
  delete [] node;

  return true;
}

//=======================================================================
//function : readResultFile
//purpose  : 
//=======================================================================

static bool readResultFile(const int                      fileOpen,
                           SMESHDS_Mesh*                  theMeshDS,
                           TopoDS_Shape                   aSolid,
                           vector <const SMDS_MeshNode*>& theNodeByGhs3dId) {

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
  ptr = (char *) mmap(0,length,PROT_READ,MAP_PRIVATE,fileOpen,0);
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
    if ((iNode+1) > nbInputNodes) {
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
  munmap(mapPtr, length);
  close(fileOpen);

  delete [] tab;
  delete [] coord;
  delete [] node;

  return true;
}

//================================================================================
/*!
 * \brief Look for a line containing a text in a file
  * \retval bool - true if the line is found
 */
//================================================================================

static bool findLineContaing(const TCollection_AsciiString& theText,
                             const TCollection_AsciiString& theFile,
                             TCollection_AsciiString &      theFoundLine)
{
  bool found = false;
  if ( FILE * aFile = fopen( theFile.ToCString(), "r" ))
  {
    char * aPtr;
    char aBuffer[ GHS3DPlugin_BUFLENGTH ];
    int aLineNb = 0;
    do {
      GHS3DPlugin_ReadLine( aPtr, aBuffer, aFile, aLineNb );
      if ( aPtr ) {
        theFoundLine = aPtr;
        found = theFoundLine.Search( theText ) >= 0;
      }
    } while ( aPtr && !found );

    fclose( aFile );
  }
  return found;
}

//================================================================================
/*!
 * \brief Provide human readable text by error code reported by ghs3d
 */
//================================================================================

static TCollection_AsciiString translateError(const TCollection_AsciiString& errorLine)
{
  int beg = errorLine.Location("ERR ", 1, errorLine.Length());
  if ( !beg ) return errorLine;

  TCollection_AsciiString errCodeStr = errorLine.SubString( beg + 4 , errorLine.Length());
  if ( !errCodeStr.IsIntegerValue() )
    return errorLine;
  Standard_Integer errCode = errCodeStr.IntegerValue();

  int codeEnd = beg + 7;
  while ( codeEnd < errorLine.Length() && isdigit( errorLine.Value(codeEnd+1) ))
    codeEnd++;
  TCollection_AsciiString error = errorLine.SubString( beg, codeEnd ) + ": ";

  switch ( errCode ) {
  case 0:
    return "The surface mesh includes a face of type type other than edge, "
      "triangle or quadrilateral. This face type is not supported.";
  case 1:
    return error + "Not enough memory for the face table";
  case 2:
    return error + "Not enough memory";
  case 3:
    return error + "Not enough memory";
  case 4:
    return error + "Face is ignored";
  case 5:
    return error + errorLine.SubString( beg + 5, errorLine.Length()) +
      ": End of file. Some data are missing in the file.";
  case 6:
    return error + errorLine.SubString( beg + 5, errorLine.Length()) +
      ": Read error on the file. There are wrong data in the file.";
  case 7:
    return error + "the metric file is inadequate (dimension other than 3).";
  case 8:
    return error + "the metric file is inadequate (values not per vertices).";
  case 9:
      return error + "the metric file contains more than one field.";
  case 10:
    return error + "the number of values in the \".bb\" (metric file) is incompatible with the expected"
      "value of number of mesh vertices in the \".noboite\" file.";
  case 12:
    return error + "Too many sub-domains";
  case 13:
    return error + "the number of vertices is negative or null.";
  case 14:
    return error + "the number of faces is negative or null.";
  case 15:
    return error + "A facehas a null vertex.";
  case 22:
    return error + "incompatible data";
  case 131:
    return error + "the number of vertices is negative or null.";
  case 132:
    return error + "the number of vertices is negative or null (in the \".mesh\" file).";
  case 133:
    return error + "the number of faces is negative or null.";
  case 1000:
    return error + "A face appears more than once in the input surface mesh.";
  case 1001:
    return error + "An edge appears more than once in the input surface mesh.";
  case 1002:
    return error + "A face has a vertex negative or null.";
  case 1003:
    return error + "NOT ENOUGH MEMORY";
  case 2000:
    return error + "Not enough available memory.";
  case 2002:
    return error + "Some initial points cannot be inserted. The surface mesh is probably very bad "
      "in terms of quality or the input list of points is wrong";
  case 2003:
    return error + "Some vertices are too close to one another or coincident.";
  case 2004:
    return error + "Some vertices are too close to one another or coincident.";
  case 2012:
    return error + "A vertex cannot be inserted.";
  case 2014:
    return error + "There are at least two points considered as coincident";
  case 2103:
    return error + "Some vertices are too close to one another or coincident";
  case 3000:
    return error + "The surface mesh regeneration step has failed";
  case 3009:
    return error + "Constrained edge cannot be enforced";
  case 3019:
    return error + "Constrained face cannot be enforced";
  case 3029:
    return error + "Missing faces";
  case 3100:
    return error + "No guess to start the definition of the connected component(s)";
  case 3101:
    return error + "The surface mesh includes at least one hole. The domain is not well defined";
  case 3102:
    return error + "Impossible to define a component";
  case 3103:
    return error + "The surface edge intersects another surface edge";
  case 3104:
    return error + "The surface edge intersects the surface face";
  case 3105:
    return error + "One boundary point lies within a surface face";
  case 3106:
    return error + "One surface edge intersects a surface face";
  case 3107:
    return error + "One boundary point lies within a surface edge";
  case 3108:
    return error + "Insufficient memory ressources detected due to a bad quality surface mesh leading "
      "to too many swaps";
  case 3109:
    return error + "Edge is unique (i.e., bounds a hole in the surface)";
  case 3122:
    return error + "Presumably, the surface mesh is not compatible with the domain being processed";
  case 3123:
    return error + "Too many components, too many sub-domain";
  case 3209:
    return error + "The surface mesh includes at least one hole. "
      "Therefore there is no domain properly defined";
  case 3300:
    return error + "Statistics.";
  case 3400:
    return error + "Statistics.";
  case 3500:
    return error + "Warning, it is dramatically tedious to enforce the boundary items";
  case 4000:
    return error + "Not enough memory at this time, nevertheless, the program continues. "
      "The expected mesh will be correct but not really as large as required";
  case 4002:
    return error + "see above error code, resulting quality may be poor.";
  case 4003:
    return error + "Not enough memory at this time, nevertheless, the program continues (warning)";
  case 8000:
    return error + "Unknown face type.";
  case 8005:
  case 8006:
    return error + errorLine.SubString( beg + 5, errorLine.Length()) +
      ": End of file. Some data are missing in the file.";
  case 9000:
    return error + "A too small volume element is detected";
  case 9001:
    return error + "There exists at least a null or negative volume element";
  case 9002:
    return error + "There exist null or negative volume elements";
  case 9003:
    return error + "A too small volume element is detected. A face is considered being degenerated";
  case 9100:
    return error + "Some element is suspected to be very bad shaped or wrong";
  case 9102:
    return error + "A too bad quality face is detected. This face is considered degenerated";
  case 9112:
    return error + "A too bad quality face is detected. This face is degenerated";
  case 9122:
    return error + "Presumably, the surface mesh is not compatible with the domain being processed";
  case 9999:
    return error + "Abnormal error occured, contact hotline";
  case 23600:
    return error + "Not enough memory for the face table";
  case 23601:
    return error + "The algorithm cannot run further. "
      "The surface mesh is probably very bad in terms of quality.";
  case 23602:
    return error + "Bad vertex number";
  }
  return errorLine;
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

  _nbShape = countShape( meshDS, TopAbs_SOLID ); // we count the number of shapes

  // create bounding box for every shape inside the compound

  int iShape = 0;
  TopoDS_Shape* tabShape;
  double**      tabBox;
  tabShape = new TopoDS_Shape[_nbShape];
  tabBox   = new double*[_nbShape];
  for (int i=0; i<_nbShape; i++)
    tabBox[i] = new double[6];
  Standard_Real Xmin, Ymin, Zmin, Xmax, Ymax, Zmax;

  TopExp_Explorer expBox (meshDS->ShapeToMesh(), TopAbs_SOLID);
  for (; expBox.More(); expBox.Next()) {
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
#ifdef WIN32
    aFacesFile->is_open() && aPointsFile->is_open();
#else
    aFacesFile.rdbuf()->is_open() && aPointsFile.rdbuf()->is_open();
#endif
  if (!Ok) {
    INFOS( "Can't write into " << aFacesFileName);
    return error(SMESH_Comment("Can't write into ") << aFacesFileName);
  }
  map <int,int> aSmdsToGhs3dIdMap;
  map <int,const SMDS_MeshNode*> aGhs3dIdToNodeMap;

  Ok = writePoints( aPointsFile, meshDS, aSmdsToGhs3dIdMap, aGhs3dIdToNodeMap ) &&
       writeFaces ( aFacesFile,  meshDS, aSmdsToGhs3dIdMap );

  aFacesFile.close();
  aPointsFile.close();

  if ( ! Ok ) {
    if ( !_keepFiles ) {
      OSD_File( aFacesFileName ).Remove();
      OSD_File( aPointsFileName ).Remove();
    }
    return error(COMPERR_BAD_INPUT_MESH);
  }

  // -----------------
  // run ghs3d mesher
  // -----------------

  TCollection_AsciiString cmd( (char*)GHS3DPlugin_Hypothesis::CommandToRun( _hyp ).c_str() );
  cmd += TCollection_AsciiString(" -f ") + aGenericName;  // file to read
  cmd += TCollection_AsciiString(" 1>" ) + aLogFileName;  // dump into file

  cout << endl;
  cout << "Ghs3d execution..." << endl;
  cout << cmd << endl;

  system( cmd.ToCString() ); // run

  cout << endl;
  cout << "End of Ghs3d execution !" << endl;

  // --------------
  // read a result
  // --------------

  // Mapping the result file

  int fileOpen;
  fileOpen = open( aResultFileName.ToCString(), O_RDONLY);
  if ( fileOpen < 0 ) {
    cout << endl;
    cout << "Can't open the " << aResultFileName.ToCString() << " GHS3D output file" << endl;
    Ok = false;
  }
  else
    Ok = readResultFile( fileOpen, meshDS, tabShape, tabBox, _nbShape, aGhs3dIdToNodeMap );

  // ---------------------
  // remove working files
  // ---------------------

  if ( Ok )
  {
    if ( !_keepFiles )
      OSD_File( aLogFileName ).Remove();
  }
  else if ( OSD_File( aLogFileName ).Size() > 0 )
  {
    INFOS( "GHS3D Error, see the " << aLogFileName.ToCString() << " file" );

    // get problem description from the log file
    SMESH_Comment comment;
    TCollection_AsciiString foundLine;
    if ( findLineContaing( "has expired",aLogFileName,foundLine) &&
         foundLine.Search("Licence") >= 0)
    {
      foundLine.LeftAdjust();
      comment << foundLine;
    }
    if ( findLineContaing( "%% ERROR",aLogFileName,foundLine) ||
         findLineContaing( " ERR ",aLogFileName,foundLine))
    {
      foundLine.LeftAdjust();
      comment << translateError( foundLine );
    }
    else if ( findLineContaing( "%% NO SAVING OPERATION",aLogFileName,foundLine))
    {
      comment << "Too many elements generated for a trial version.\n";
    }
    if ( comment.empty() )
      comment << "See " << aLogFileName << " for problem description";
    else
      comment << "\nSee " << aLogFileName << " for more information";
    error(COMPERR_ALGO_FAILED, comment);
  }
  else
  {
    // the log file is empty
    OSD_File( aLogFileName ).Remove();
    INFOS( "GHS3D Error, command '" << cmd.ToCString() << "' failed" );
    error(COMPERR_ALGO_FAILED, "ghs3d: command not found" );
  }

  if ( !_keepFiles ) {
    OSD_File( aFacesFileName ).Remove();
    OSD_File( aPointsFileName ).Remove();
    OSD_File( aResultFileName ).Remove();
    OSD_File( aBadResFileName ).Remove();
    OSD_File( aBbResFileName ).Remove();
  }
  cout << "<" << aResultFileName.ToCString() << "> GHS3D output file ";
  if ( !Ok )
    cout << "not ";
  cout << "treated !" << endl;
  cout << endl;

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
#ifdef WIN32
    aFacesFile->is_open() && aPointsFile->is_open();
#else
    aFacesFile.rdbuf()->is_open() && aPointsFile.rdbuf()->is_open();
#endif

  if (!Ok)
    return error( SMESH_Comment("Can't write into ") << aPointsFileName);

  vector <const SMDS_MeshNode*> aNodeByGhs3dId;

  Ok = (writeFaces ( aFacesFile, meshDS, aNodeByGhs3dId ) &&
        writePoints( aPointsFile, meshDS, aNodeByGhs3dId));
  
  aFacesFile.close();
  aPointsFile.close();
  
  if ( ! Ok ) {
    if ( !_keepFiles ) {
      OSD_File( aFacesFileName ).Remove();
      OSD_File( aPointsFileName ).Remove();
    }
    return error(COMPERR_BAD_INPUT_MESH);
  }

  // -----------------
  // run ghs3d mesher
  // -----------------

  TCollection_AsciiString cmd( (char*)GHS3DPlugin_Hypothesis::CommandToRun( _hyp ).c_str() );
  cmd += TCollection_AsciiString(" -f ") + aGenericName;  // file to read
  cmd += TCollection_AsciiString(" 1>" ) + aLogFileName;  // dump into file
  
  system( cmd.ToCString() ); // run

  // --------------
  // read a result
  // --------------
  int fileOpen;
  fileOpen = open( aResultFileName.ToCString(), O_RDONLY);
  if ( fileOpen < 0 ) {
    cout << endl;
    cout << "Error when opening the " << aResultFileName.ToCString() << " file" << endl;
    cout << endl;
    Ok = false;
  }
  else {
    Ok = readResultFile( fileOpen, meshDS, theShape ,aNodeByGhs3dId );
  }
  
  // ---------------------
  // remove working files
  // ---------------------

  if ( Ok )
  {
    if ( !_keepFiles )
      OSD_File( aLogFileName ).Remove();
  }
  else if ( OSD_File( aLogFileName ).Size() > 0 )
  {
    INFOS( "GHS3D Error, see the " << aLogFileName.ToCString() << " file" );

    // get problem description from the log file
    SMESH_Comment comment;
    TCollection_AsciiString foundLine;
    if ( findLineContaing( "has expired",aLogFileName,foundLine) &&
         foundLine.Search("Licence") >= 0)
    {
      foundLine.LeftAdjust();
      comment << foundLine;
    }
    if ( findLineContaing( "%% ERROR",aLogFileName,foundLine) ||
         findLineContaing( " ERR ",aLogFileName,foundLine))
    {
      foundLine.LeftAdjust();
      comment << translateError( foundLine );
    }
    else if ( findLineContaing( "%% NO SAVING OPERATION",aLogFileName,foundLine))
    {
      comment << "Too many elements generated for a trial version.\n";
    }
    if ( comment.empty() )
      comment << "See " << aLogFileName << " for problem description";
    else
      comment << "\nSee " << aLogFileName << " for more information";
    error(COMPERR_ALGO_FAILED, comment);
  }
  else {
    // the log file is empty
    OSD_File( aLogFileName ).Remove();
    INFOS( "GHS3D Error, command '" << cmd.ToCString() << "' failed" );
    error(COMPERR_ALGO_FAILED, "ghs3d: command not found" );
  }

  if ( !_keepFiles )
  {
    OSD_File( aFacesFileName ).Remove();
    OSD_File( aPointsFileName ).Remove();
    OSD_File( aResultFileName ).Remove();
    OSD_File( aBadResFileName ).Remove();
    OSD_File( aBbResFileName ).Remove();
  }
  
  return Ok;
}
