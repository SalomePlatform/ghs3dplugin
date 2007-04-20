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

  // get all faces bound to theShape

  int nbFaces = 0;
  TopoDS_Shape theShape = theMesh->ShapeToMesh();
  list< const SMDS_MeshElement* > faces;
  TopExp_Explorer fExp( theShape, TopAbs_FACE );
  SMESHDS_SubMesh* sm;
  SMDS_ElemIteratorPtr eIt;

  const char* space    = "  ";
  const int   dummyint = 0;

  list< const SMDS_MeshElement* >::iterator f;
  map<int,int>::const_iterator it;
  SMDS_ElemIteratorPtr nodeIt;
  const SMDS_MeshElement* elem;
  int nbNodes;
  int aSmdsID;

  for ( ; fExp.More(); fExp.Next() ) {
    sm = theMesh->MeshElements( fExp.Current() );
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

  cout << nbFaces << " triangles" << endl;

  // NB_ELEMS DUMMY_INT
  theFile << space << nbFaces << space << dummyint << endl;

  // Loop from 1 to NB_ELEMS

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
      aSmdsID = nodeIt->next()->GetID();
      it = theSmdsToGhs3dIdMap.find( aSmdsID );
      ASSERT( it != theSmdsToGhs3dIdMap.end() );
      theFile << space << (*it).second;
    }

    // (NB_NODES + 1) times: DUMMY_INT
    for ( int i=0; i<=nbNodes; i++)
      theFile << space << dummyint;

    theFile << endl;
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
  cout << "The initial 2D mesh contains " << nbNodes << " nodes and ";

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


//=======================================================================
//function : readResultFile
//purpose  : 
//=======================================================================

static bool readResultFile(const int                       fileOpen,
                           SMESHDS_Mesh*                   theMeshDS,
                           TopoDS_Shape                    tabShape[],
                           double                          tabBox[][6],
                           const int                       nShape,
                           map <int,const SMDS_MeshNode*>& theGhs3dIdToNodeMap) {

  struct stat  status;
  size_t       length;

  char *ptr, *mapPtr;
  char *tetraPtr;
  char *shapePtr;

  int fileStat;
  int nbElems, nbNodes, nbInputNodes;
  int nodeId, triangleId;
  int tab[3], tabID[nShape];
  int nbTriangle;
  int ID, shapeID, ghs3dShapeID;

  double coord [3];

  TopoDS_Shape aSolid;
  SMDS_MeshNode * aNewNode;
  const SMDS_MeshNode * node[4];
  map <int,const SMDS_MeshNode*>::iterator IdNode;
  SMDS_MeshElement* aTet;

  for (int i=0; i<nShape; i++)
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

  // Reading the nodeId
  for (int i=0; i < 4*nbElems; i++)
    nodeId = strtol(ptr, &ptr, 10);

  // Reading the nodeCoor and update the nodeMap
  for (int iNode=0; iNode < nbNodes; iNode++) {
    for (int iCoor=0; iCoor < 3; iCoor++)
      coord[ iCoor ] = strtod(ptr, &ptr);
    if ((iNode+1) > nbInputNodes) {
      aNewNode = theMeshDS->AddNode( coord[0],coord[1],coord[2] );
      theGhs3dIdToNodeMap.insert(make_pair( (iNode+1), aNewNode ));
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
      IdNode = theGhs3dIdToNodeMap.find(ID);
      node[ iNode ] = IdNode->second;
    }
    aTet = theMeshDS->AddVolume( node[1], node[0], node[2], node[3] );
    ghs3dShapeID = strtol(shapePtr, &shapePtr, 10);
    if ( tabID[ ghs3dShapeID - 1 ] == 0 ) {
      if (iElem == 0)
        aSolid = tabShape[0];
      aSolid = findSolid(node, aSolid, tabShape, tabBox, nbTriangle);
      shapeID = theMeshDS->ShapeToIndex( aSolid );
      tabID[ ghs3dShapeID - 1] = shapeID;
    }
    else
      shapeID = tabID[ ghs3dShapeID - 1];
    theMeshDS->SetMeshElementOnShape( aTet, shapeID );
    if ( (iElem + 1) == nbElems )
      cout << nbElems << " tetrahedrons have been associated to " << nbTriangle << " shapes" << endl;
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

//================================================================================
/*!
 * \brief Decrease amount of memory GHS3D may use until it can allocate it
  * \param nbMB - memory size to adjust
  * \param aLogFileName - file for GHS3D output
  * \retval bool - false if GHS3D can not run for any reason
 */
//================================================================================

static bool adjustMemory(int & nbMB, const TCollection_AsciiString & aLogFileName)
{
  TCollection_AsciiString cmd( "ghs3d -m " );
  cmd += nbMB;
  cmd += " 1>";
  cmd += aLogFileName;

  system( cmd.ToCString() ); // run

  // analyse log file

  FILE * aLogFile = fopen( aLogFileName.ToCString(), "r" );
  if ( aLogFile )
  {
    bool memoryOK = true;
    char * aPtr;
    char aBuffer[ GHS3DPlugin_BUFLENGTH ];
    int aLineNb = 0;
    do { 
      GHS3DPlugin_ReadLine( aPtr, aBuffer, aLogFile, aLineNb );
      if ( aPtr ) {
        TCollection_AsciiString line( aPtr );
        if ( line.Search( "UNABLE TO ALLOCATE MEMORY" ) > 0 )
          memoryOK = false;
      }
    } while ( aPtr && memoryOK );

    fclose( aLogFile );

    if ( !memoryOK ) {
      nbMB *= 0.75;
      return adjustMemory( nbMB, aLogFileName );
    }
    return true;
  }
  return false;
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

  if (_iShape == 0 && _nbShape == 0) {
    cout << endl;
    cout << "Ghs3d execution..." << endl;
    cout << endl;

    TopExp_Explorer exp (meshDS->ShapeToMesh(), TopAbs_SOLID);
    for (; exp.More(); exp.Next())
      _nbShape++;
  }

  _iShape++;

  if ( _iShape == _nbShape ) {

    // create bounding box for every shape

    int iShape = 0;
    TopoDS_Shape tabShape[_nbShape];
    double tabBox[_nbShape][6];
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
	return error(dfltErr(), SMESH_Comment("Can't write into ") << aTmpDir.ToCString());

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
      return error(COMPERR_BAD_INPUT_MESH);
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
	int MB = 0.9 * ( si.freeram + si.freeswap ) * si.mem_unit / 1024 / 1024;
	adjustMemory( MB, aLogFileName );
	memory = "-m ";
	memory += MB;
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
      cout << "Error when opening the " << aResultFileName.ToCString() << " file" << endl;
      cout << endl;
      Ok = false;
    }
    else
      Ok = readResultFile( fileOpen, meshDS, tabShape, tabBox, _nbShape, aGhs3dIdToNodeMap );

  // ---------------------
  // remove working files
  // ---------------------

    if ( Ok ) {
	OSD_File( aLogFileName ).Remove();
    }
    else if ( OSD_File( aLogFileName ).Size() > 0 ) {
	Ok = error(dfltErr(), SMESH_Comment("See ")<< aLogFileName.ToCString() );
    }
    else {
	OSD_File( aLogFileName ).Remove();
    }
  }
  return Ok;
}
