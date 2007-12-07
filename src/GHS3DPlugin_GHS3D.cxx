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

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"

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

  cout << "    " << theMesh->NbFaces() << " triangles" << endl;
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
  int compoundID = theMeshDS->ShapeToIndex( theMeshDS->ShapeToMesh() );

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
    if ( (iElem + 1) == nbElems )
      cout << nbElems << " tetrahedrons have been associated to " << nbShape << " shapes" << endl;
  }
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

  cout << endl;
  cout << "Ghs3d execution..." << endl;

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

  Ok =
#ifdef WIN32
    aFacesFile->is_open() && aPointsFile->is_open();
#else
    aFacesFile.rdbuf()->is_open() && aPointsFile.rdbuf()->is_open();
#endif
  if (!Ok) {
    INFOS( "Can't write into " << aTmpDir.ToCString());
    return false;
  }
  map <int,int> aSmdsToGhs3dIdMap;
  map <int,const SMDS_MeshNode*> aGhs3dIdToNodeMap;

  Ok = writePoints( aPointsFile, meshDS, aSmdsToGhs3dIdMap, aGhs3dIdToNodeMap ) &&
       writeFaces ( aFacesFile,  meshDS, aSmdsToGhs3dIdMap );

  aFacesFile.close();
  aPointsFile.close();

  if ( ! Ok ) {
    if ( !getenv("GHS3D_KEEP_FILES") ) {
      OSD_File( aFacesFileName ).Remove();
      OSD_File( aPointsFileName ).Remove();
    }
    return false;
  }

  // -----------------
  // run ghs3d mesher              WIN32???
  // -----------------

  // ghs3d need to know amount of memory it may use (MB).
  // Default memory is defined at ghs3d installation but it may be not enough,
  // so allow to use about all available memory

  TCollection_AsciiString memory;
#ifndef WIN32
  struct sysinfo si;
  int err = sysinfo( &si );
  if ( err == 0 ) {
    int freeMem = si.totalram * si.mem_unit / 1024 / 1024;
    memory = "-m ";
    memory += int( 0.7 * freeMem );
  }
#endif

  MESSAGE("GHS3DPlugin_GHS3D::Compute");
  TCollection_AsciiString cmd( "ghs3d " ); // command to run
  cmd +=
    memory +                     // memory
    " -c0 -f " + aGenericName +  // file to read
         " 1>" + aLogFileName;   // dump into file

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
    OSD_File( aLogFileName ).Remove();
  else if ( OSD_File( aLogFileName ).Size() > 0 ) {
    INFOS( "GHS3D Error, see the " << aLogFileName.ToCString() << " file" );
  }
  else {
    OSD_File( aLogFileName ).Remove();
    INFOS( "GHS3D Error, command '" << cmd.ToCString() << "' failed" );
  }

  if ( !getenv("GHS3D_KEEP_FILES") ) {
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
  return Ok;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & GHS3DPlugin_GHS3D::SaveTo(ostream & save)
{
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & GHS3DPlugin_GHS3D::LoadFrom(istream & load)
{
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator << (ostream & save, GHS3DPlugin_GHS3D & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >> (istream & load, GHS3DPlugin_GHS3D & hyp)
{
  return hyp.LoadFrom( load );
}
