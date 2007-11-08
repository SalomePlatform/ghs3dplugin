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
//   _iShape=0;
//   _nbShape=0;
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

bool GHS3DPlugin_GHS3D::CheckHypothesis ( SMESH_Mesh&                          aMesh,
                                          const TopoDS_Shape&                  aShape,
                                          SMESH_Hypothesis::Hypothesis_Status& aStatus )
{
//  MESSAGE("GHS3DPlugin_GHS3D::CheckHypothesis");
  aStatus = SMESH_Hypothesis::HYP_OK;
  return true;
}

//================================================================================
/*!
 * \brief Write faces bounding theShape to file
 */
//================================================================================

static bool writeFaces (ofstream &                      theFile,
                        SMESHDS_Mesh *                  theMesh,
                        const TopoDS_Shape&             theShape,
                        vector <const SMDS_MeshNode*> & theNodeByGhs3dId)
{
  // record structure:
  //
  // NB_ELEMS DUMMY_INT
  // Loop from 1 to NB_ELEMS
  // NB_NODES NODE_NB_1 NODE_NB_2 ... (NB_NODES + 1) times: DUMMY_INT

  // get all faces bound to theShape

  // Solids in the ShapeToMesh() can be meshed by different meshers,
  // so we take faces only from the given shape
  //TopoDS_Shape theShape = theMesh->ShapeToMesh();
  int nbFaces = 0;
  list< const SMDS_MeshElement* > faces;
  // Use TopTools_IndexedMapOfShape in order not to take twice mesh faces from
  // a geom face shared by two solids
  TopTools_IndexedMapOfShape faceMap;
  TopExp::MapShapes( theShape, TopAbs_FACE, faceMap );
  SMESHDS_SubMesh* sm;
  SMDS_ElemIteratorPtr eIt;

  const char* space    = "  ";
  const int   dummyint = 0;

  list< const SMDS_MeshElement* >::iterator f;
  map< const SMDS_MeshNode*,int >::iterator it;
  SMDS_ElemIteratorPtr nodeIt;
  const SMDS_MeshElement* elem;
  int nbNodes;
  //int aSmdsID;

  for ( int i = 0; i < faceMap.Extent(); ++i ) {
    sm = theMesh->MeshElements( faceMap( i+1 ) );
    if ( sm ) {
      eIt = sm->GetElements();
      while ( eIt->more() ) {
        faces.push_back( eIt->next() );
        nbFaces++;
      }
    }
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
//function : findSolid
//purpose  : 
//=======================================================================

static TopoDS_Shape findSolid(const SMDS_MeshNode *aNode[],
                              TopoDS_Shape        aSolid,
                              const TopoDS_Shape  shape[],
                              const double        box[][6],
                              const int           nShape) {

  Standard_Real PX, PY, PZ;
  int iShape;

  PX = ( aNode[0]->X() + aNode[1]->X() + aNode[2]->X() + aNode[3]->X() ) / 4.0;
  PY = ( aNode[0]->Y() + aNode[1]->Y() + aNode[2]->Y() + aNode[3]->Y() ) / 4.0;
  PZ = ( aNode[0]->Z() + aNode[1]->Z() + aNode[2]->Z() + aNode[3]->Z() ) / 4.0;
  gp_Pnt aPnt(PX, PY, PZ);

  BRepClass3d_SolidClassifier SC (aSolid, aPnt, Precision::Confusion());
  if ( not(SC.State() == TopAbs_IN) ) {
    for (iShape = 0; iShape < nShape; iShape++) {
      aSolid = shape[iShape];
      if ( not( PX < box[iShape][0] || box[iShape][1] < PX ||
                PY < box[iShape][2] || box[iShape][3] < PY ||
                PZ < box[iShape][4] || box[iShape][5] < PZ) ) {
        BRepClass3d_SolidClassifier SC (aSolid, aPnt, Precision::Confusion());
        if (SC.State() == TopAbs_IN)
          break;
      }
    }
  }
  return aSolid;
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

#include <list>
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
  int tab[3];
  int nbTriangle;
  int ID, shapeID;

  double coord [3];

  SMDS_MeshNode * aNewNode;
  const SMDS_MeshNode * node[4];
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
  return true;
}

static bool readResultFile(const int                      fileOpen,
                           SMESHDS_Mesh*                  theMeshDS,
                           TopoDS_Shape                   tabShape[],
                           double                         tabBox[][6],
                           const int                      nShape,
                           vector <const SMDS_MeshNode*>& theNodeByGhs3dId) {

  struct stat  status;
  size_t       length;

  char *ptr, *mapPtr;
  char *tetraPtr;
  char *shapePtr;

  int fileStat;
  int nbElems, nbNodes, nbInputNodes;
  int nodeId, triangleId;
  int tab[3]/*, tabID[nShape]*/;
  int nbTriangle;
  int ID, shapeID, ghs3dShapeID;

  double coord [3];
  vector< int > tabID (nShape, 0);

  TopoDS_Shape aSolid;
  SMDS_MeshNode * aNewNode;
  const SMDS_MeshNode * node[4];
  map <int,const SMDS_MeshNode*>::iterator IdNode;
  SMDS_MeshElement* aTet;

//   for (int i=0; i<nShape; i++)
//     tabID[i] = 0;

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
  for (int iNode=0; iNode < nbNodes; iNode++) {
    for (int iCoor=0; iCoor < 3; iCoor++)
      coord[ iCoor ] = strtod(ptr, &ptr);
    if ((iNode+1) > nbInputNodes) {
      aNewNode = theMeshDS->AddNode( coord[0],coord[1],coord[2] );
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
    ghs3dShapeID = strtol(shapePtr, &shapePtr, 10);
    if ( !ghs3dShapeID ) ghs3dShapeID = 1;
    if ( tabID[ ghs3dShapeID - 1 ] == 0 ) {
      if (iElem == 0)
        aSolid = tabShape[0];
      aSolid = findSolid(node, aSolid, tabShape, tabBox, nShape /*nbTriangle*/);
      shapeID = theMeshDS->ShapeToIndex( aSolid );
      tabID[ ghs3dShapeID - 1] = shapeID;
    }
    else {
      shapeID = tabID[ ghs3dShapeID - 1];
    }
    theMeshDS->SetMeshElementOnShape( aTet, shapeID );
    // set new nodes on to the shape
    SMDS_ElemIteratorPtr nodeIt = aTet->nodesIterator();
    while ( nodeIt->more() ) {
      const SMDS_MeshNode * n = castToNode( nodeIt->next() );
      if ( !n->GetPosition()->GetShapeId() )
        theMeshDS->SetNodeInVolume( n, shapeID );
    }
  }
  if ( nbElems )
    cout << nbElems << " tetrahedrons have been associated to " << nbTriangle << " shapes" << endl;
  munmap(mapPtr, length);
  close(fileOpen);
  return true;
}



//=======================================================================
//function : getTmpDir
//purpose  : 
//=======================================================================

static TCollection_AsciiString getTmpDir()
{
  TCollection_AsciiString aTmpDir;

  char *Tmp_dir = getenv("SALOME_TMP_DIR");
  if(Tmp_dir != NULL) {
    aTmpDir = Tmp_dir;
#ifdef WIN32
    if(aTmpDir.Value(aTmpDir.Length()) != '\\') aTmpDir+='\\';
#else
    if(aTmpDir.Value(aTmpDir.Length()) != '/') aTmpDir+='/';
#endif      
  }
  else {
#ifdef WIN32
    aTmpDir = TCollection_AsciiString("C:\\");
#else
    aTmpDir = TCollection_AsciiString("/tmp/");
#endif
  }
  return aTmpDir;
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

//=============================================================================
/*!
 *Here we are going to use the GHS3D mesher
 */
//=============================================================================

bool GHS3DPlugin_GHS3D::Compute(SMESH_Mesh&         theMesh,
                                const TopoDS_Shape& theShape)
{
  // theShape is a compound of solids as _onlyUnaryInput = false
  bool Ok(false);
  SMESHDS_Mesh* meshDS = theMesh.GetMeshDS();

  int _nbShape = 0;
  /*if (_iShape == 0 && _nbShape == 0)*/ {
    cout << endl;
    cout << "Ghs3d execution..." << endl;
    cout << endl;
    
    //TopExp_Explorer exp (meshDS->ShapeToMesh(), TopAbs_SOLID);
    TopExp_Explorer exp (theShape, TopAbs_SOLID);
    for (; exp.More(); exp.Next())
      _nbShape++;
  }
  
  //_iShape++;

  /*if ( _iShape == _nbShape )*/ {

    // create bounding box for every shape

    int iShape = 0;
    TopoDS_Shape tabShape[_nbShape];
    double tabBox[_nbShape][6];
    Standard_Real Xmin, Ymin, Zmin, Xmax, Ymax, Zmax;

    TopExp_Explorer expBox (theShape, TopAbs_SOLID);
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

    // make a unique working file name
    // to avoid access to the same files by eg different users
  
    TCollection_AsciiString aGenericName, aTmpDir = getTmpDir();
    aGenericName = aTmpDir + "GHS3D_";
#ifdef WIN32
    aGenericName += GetCurrentProcessId();
#else
    aGenericName += getpid();
#endif
    aGenericName += "_";
    aGenericName += meshDS->ShapeToIndex( theShape );

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
    // bool Ok =
    Ok =
#ifdef WIN32
      aFacesFile->is_open() && aPointsFile->is_open();
#else
      aFacesFile.rdbuf()->is_open() && aPointsFile.rdbuf()->is_open();
#endif
    if (!Ok)
	return error(SMESH_Comment("Can't write into ") << aTmpDir);

    vector <const SMDS_MeshNode*> aNodeByGhs3dId;

    Ok = ( writeFaces ( aFacesFile,  meshDS, theShape, aNodeByGhs3dId ) &&
           writePoints( aPointsFile, meshDS, aNodeByGhs3dId ));

    aFacesFile.close();
    aPointsFile.close();

    if ( ! Ok ) {
      if ( !getenv("GHS3D_KEEP_FILES") ) {
        OSD_File( aFacesFileName ).Remove();
        OSD_File( aPointsFileName ).Remove();
      }
      return error(COMPERR_BAD_INPUT_MESH);
    }

    // -----------------
    // run ghs3d mesher              WIN32???
    // -----------------

    // ghs3d need to know amount of memory it may use (MB).
    // Default memory is defined at ghs3d installation but it may be not enough,
    // so allow to use about all available memory

    TCollection_AsciiString memory;
#ifdef WIN32
    // ????
#else
    struct sysinfo si;
    int err = sysinfo( &si );
    if ( !err ) {
      int freeMem = si.totalram * si.mem_unit / 1024 / 1024;
      memory = "-m ";
      memory += int( 0.7 * freeMem );
    }
#endif

    OSD_File( aResultFileName ).Remove(); // old file prevents writing a new one

    TCollection_AsciiString cmd( "ghs3d " ); // command to run
    cmd +=
      memory +                // memory
      " -c 0"                 // 0 - mesh all components, 1 - keep only the main component
      " -f " + aGenericName + // file to read
      " 1>" + aLogFileName;   // dump into file

    MESSAGE("GHS3DPlugin_GHS3D::Compute() " << cmd );
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
      cout << "Error when opening the " << aResultFileName.ToCString() << " file" << endl;
      cout << endl;
      Ok = false;
    }
    else {
      Ok = readResultFile( fileOpen, meshDS, tabShape, tabBox, _nbShape, aNodeByGhs3dId );
    }

  // ---------------------
  // remove working files
  // ---------------------

    if ( Ok )
    {
      OSD_File( aLogFileName ).Remove();
    }
    else if ( OSD_File( aLogFileName ).Size() > 0 )
    {
      // get problem description from the log file
      SMESH_Comment comment;
      TCollection_AsciiString foundLine;
      if ( findLineContaing( "has expired",aLogFileName,foundLine) &&
           foundLine.Search("Licence") >= 0)
      {
        foundLine.LeftAdjust();
        comment << foundLine;
      }
      if ( findLineContaing( "%% ERROR",aLogFileName,foundLine))
      {
        foundLine.LeftAdjust();
        comment << foundLine;
      }
      if ( findLineContaing( "%% NO SAVING OPERATION",aLogFileName,foundLine))
      {
        comment << "Too many elements generated for a trial version.\n";
      }
      if ( comment.empty() )
        comment << "See " << aLogFileName << " for problem description";
      else
        comment << "See " << aLogFileName << " for more information";
      error(COMPERR_ALGO_FAILED, comment);
    }
    else
    {
      // the log file is empty
      OSD_File( aLogFileName ).Remove();
      error(COMPERR_ALGO_FAILED, "ghs3d: command not found" );
    }

    if ( !getenv("GHS3D_KEEP_FILES") )
    {
      OSD_File( aFacesFileName ).Remove();
      OSD_File( aPointsFileName ).Remove();
      OSD_File( aResultFileName ).Remove();
      OSD_File( aBadResFileName ).Remove();
      OSD_File( aBbResFileName ).Remove();
    }
    /*if ( _iShape == _nbShape )*/ {
      cout << aResultFileName.ToCString() << " Output file ";
      if ( !Ok )
        cout << "not ";
      cout << "treated !" << endl;
      cout << endl;
    }
  }

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

  // make a unique working file name
  // to avoid access to the same files by eg different users
  
  TCollection_AsciiString aGenericName, aTmpDir = getTmpDir();
  aGenericName = aTmpDir + "GHS3D_";
#ifdef WIN32
  aGenericName += GetCurrentProcessId();
#else
  aGenericName += getpid();
#endif
  aGenericName += "_";
  aGenericName += meshDS->ShapeToIndex( theShape );

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
    return error( SMESH_Comment("Can't write into ") << aTmpDir.ToCString());

  vector <const SMDS_MeshNode*> aNodeByGhs3dId;

  Ok = (writeFaces ( aFacesFile, meshDS, aNodeByGhs3dId ) &&
        writePoints( aPointsFile, meshDS, aNodeByGhs3dId));
  
  aFacesFile.close();
  aPointsFile.close();
  
  if ( ! Ok ) {
    if ( !getenv("GHS3D_KEEP_FILES") ) {
      OSD_File( aFacesFileName ).Remove();
      OSD_File( aPointsFileName ).Remove();
    }
    return error(COMPERR_BAD_INPUT_MESH);
  }

  // -----------------
  // run ghs3d mesher              WIN32???
  // -----------------

  // ghs3d need to know amount of memory it may use (MB).
  // Default memory is defined at ghs3d installation but it may be not enough,
  // so allow to use about all available memory
  TCollection_AsciiString memory;
#ifdef WIN32
  // ????
#else
  struct sysinfo si;
  int err = sysinfo( &si );
  if ( !err ) {
    int freeMem = si.totalram * si.mem_unit / 1024 / 1024;
    memory = "-m ";
    memory += int( 0.7 * freeMem );
  }
#endif
  
  TCollection_AsciiString cmd( "ghs3d " ); // command to run
  cmd +=
    memory +                 // memory
    " -f " + aGenericName +  // file to read
    " 1>" + aLogFileName;    // dump into file
  
  
  
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

  if ( Ok ) {
    OSD_File( aLogFileName ).Remove();
  }
  else if ( OSD_File( aLogFileName ).Size() > 0 ) {
    Ok = error( SMESH_Comment("See ")<< aLogFileName.ToCString() );
  }
  else {
    OSD_File( aLogFileName ).Remove();
  }

  if ( !getenv("GHS3D_KEEP_FILES") )
  {
    OSD_File( aFacesFileName ).Remove();
    OSD_File( aPointsFileName ).Remove();
    OSD_File( aResultFileName ).Remove();
    OSD_File( aBadResFileName ).Remove();
    OSD_File( aBbResFileName ).Remove();
  }
  
  return Ok;
}






