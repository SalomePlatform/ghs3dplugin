//=============================================================================
// File      : GHS3DPlugin_GHS3D.cxx
// Created   : 
// Author    : Edward AGAPOV
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

#include "utilities.h"

#include <qfile.h>

#ifdef _DEBUG_
#define DUMP(txt) \
//  cout << txt
#else
#define DUMP(txt)
#endif

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

bool GHS3DPlugin_GHS3D::CheckHypothesis
                         (SMESH_Mesh& aMesh,
                          const TopoDS_Shape& aShape,
                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
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
  //   NB_NODES NODE_NB_1 NODE_NB_2 ... (NB_NODES + 1) times: DUMMY_INT

  int nbFaces = theMesh->NbFaces();
  if ( nbFaces == 0 )
    return false;

  const char* space    = "  ";
  const int   dummyint = 0;

  // NB_ELEMS DUMMY_INT
  theFile << space << nbFaces << space << dummyint << endl;

  // Loop from 1 to NB_ELEMS
  SMDS_FaceIteratorPtr it = theMesh->facesIterator();
  while ( it->more() )
  {
    // NB_NODES
    const SMDS_MeshElement* elem = it->next();
    const int nbNodes = elem->NbNodes();
    theFile << space << nbNodes;

    // NODE_NB_1 NODE_NB_2 ...
    SMDS_ElemIteratorPtr nodeIt = elem->nodesIterator();
    while ( nodeIt->more() )
    {
      // find GHS3D ID
      int aSmdsID = nodeIt->next()->GetID();
      map<int,int>::const_iterator it = theSmdsToGhs3dIdMap.find( aSmdsID );
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

  // NB_NODES
  theFile << space << nbNodes << endl;

  // Loop from 1 to NB_NODES
  int aGhs3dID = 1;
  SMDS_NodeIteratorPtr it = theMesh->nodesIterator();
  while ( it->more() )
  {
    const SMDS_MeshNode* node = it->next();
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
//function : getInt
//purpose  : 
//=======================================================================

static bool getInt( int & theValue, char * & theLine )
{
  char *ptr;
  theValue = strtol( theLine, &ptr, 10 );
  if ( ptr == theLine ||
      // there must not be neither '.' nor ',' nor 'E' ...
      (*ptr != ' ' && *ptr != '\n' && *ptr != '\0'))
    return false;

  DUMP( "  " << theValue );
  theLine = ptr;
  return true;
}

//=======================================================================
//function : getDouble
//purpose  : 
//=======================================================================

static bool getDouble( double & theValue, char * & theLine )
{
  char *ptr;
  theValue = strtod( theLine, &ptr );
  if ( ptr == theLine )
    return false;

  DUMP( "   " << theValue );
  theLine = ptr;
  return true;
}
  
//=======================================================================
//function : readLine
//purpose  : 
//=======================================================================

#define GHS3DPlugin_BUFLENGTH 256
#define GHS3DPlugin_ReadLine(aPtr,aBuf,aFile,aLineNb) \
{  aPtr = fgets( aBuf, GHS3DPlugin_BUFLENGTH - 2, aFile ); aLineNb++; DUMP(endl); }

//=======================================================================
//function : readResult
//purpose  : 
//=======================================================================

static bool readResult(FILE *                          theFile,
                       SMESHDS_Mesh *                  theMesh,
                       const TopoDS_Shape&             theShape,
                       map <int,const SMDS_MeshNode*>& theGhs3dIdToNodeMap)
{
  // structure:

  // record 1:
  //  NB_ELEMENTS NB_NODES NB_INPUT_NODES (14 DUMMY_INT)
  // record 2:
  //  (NB_ELEMENTS * 4) node nbs
  // record 3:
  //  (NB_NODES) node XYZ

  char aBuffer[ GHS3DPlugin_BUFLENGTH ];
  char * aPtr;
  int aLineNb = 0;

  // get shell to set nodes in
  TopExp_Explorer exp( theShape, TopAbs_SHELL );
  TopoDS_Shell aShell = TopoDS::Shell( exp.Current() );
  if ( aShell.IsNull() )
    return false;

  // ----------------------------------------
  // record 1:
  // read nb of generated elements and nodes
  // ----------------------------------------
  int nbElems = 0 , nbNodes = 0, nbInputNodes = 0;
  GHS3DPlugin_ReadLine( aPtr, aBuffer, theFile, aLineNb );
  if (!aPtr ||
      !getInt( nbElems, aPtr ) ||
      !getInt( nbNodes, aPtr ) ||
      !getInt( nbInputNodes, aPtr))
    return false;

  // -------------------------------------------
  // record 2:
  // read element nodes and create tetrahedrons
  // -------------------------------------------
  GHS3DPlugin_ReadLine( aPtr, aBuffer, theFile, aLineNb );
  for (int iElem = 0; iElem < nbElems; iElem++)
  {
    // read 4 nodes
    const SMDS_MeshNode * node[4];
    for (int iNode = 0; iNode < 4; iNode++)
    {
      // read Ghs3d node ID
      int ID = 0;
      if (!aPtr || ! getInt ( ID, aPtr ))
      {
        GHS3DPlugin_ReadLine( aPtr, aBuffer, theFile, aLineNb );
        if (!aPtr || ! getInt ( ID, aPtr ))
        {
          MESSAGE( "Cant read " << (iNode+1) << "-th node on line " << aLineNb );
          return false;
        }
      }
      // find/create a node with ID
      map <int,const SMDS_MeshNode*>::iterator IdNode = theGhs3dIdToNodeMap.find( ID );
      if ( IdNode == theGhs3dIdToNodeMap.end())
      {
        // ID is not yet in theGhs3dIdToNodeMap
        ASSERT ( ID > nbInputNodes ); // it should be a new one
        SMDS_MeshNode * aNewNode = theMesh->AddNode( 0.,0.,0. ); // read XYZ later
        theMesh->SetNodeInVolume( aNewNode, aShell );
        theGhs3dIdToNodeMap.insert
          ( map <int,const SMDS_MeshNode*>::value_type( ID, aNewNode ));
        node[ iNode ] = aNewNode;
      }
      else
      {
        node[ iNode ] = (*IdNode).second;
      }
    }
    // create a tetrahedron
    SMDS_MeshElement* aTet = theMesh->AddVolume( node[0], node[1], node[2], node[3] );
    theMesh->SetMeshElementOnShape( aTet, theShape );
  }

  // ------------------------
  // record 3:
  // read and set nodes' XYZ
  // ------------------------
  GHS3DPlugin_ReadLine( aPtr, aBuffer, theFile, aLineNb );
  for (int iNode = 0; iNode < nbNodes; iNode++)
  {
    // read 3 coordinates
    double coord [3];
    for (int iCoord = 0; iCoord < 3; iCoord++)
    {
      if (!aPtr || ! getDouble ( coord[ iCoord ], aPtr ))
      {
        GHS3DPlugin_ReadLine( aPtr, aBuffer, theFile, aLineNb );
        if (!aPtr || ! getDouble ( coord[ iCoord ], aPtr ))
        {
          MESSAGE( "Cant read " << (iCoord+1) << "-th node coord on line " << aLineNb );
          return false;
        }
      }
    }
    // do not move old nodes
    int ID = iNode + 1;
    if (ID <= nbInputNodes)
      continue;
    // find a node
    map <int,const SMDS_MeshNode*>::iterator IdNode = theGhs3dIdToNodeMap.find( ID );
    ASSERT ( IdNode != theGhs3dIdToNodeMap.end());
    SMDS_MeshNode* node = const_cast<SMDS_MeshNode*> ( (*IdNode).second );

    // set XYZ
    theMesh->MoveNode( node, coord[0], coord[1], coord[2] );
  }

  return nbElems;
}

//=============================================================================
/*!
 *Here we are going to use the GHS3D mesher
 */
//=============================================================================

bool GHS3DPlugin_GHS3D::Compute(SMESH_Mesh&         theMesh,
                                const TopoDS_Shape& theShape)
{
  MESSAGE("GHS3DPlugin_GHS3D::Compute");

  // working dir
  QString aTmpDir ( getenv("SALOME_TMP_DIR") );
  if ( !aTmpDir.isEmpty() ) {
#ifdef WIN32
    if(aTmpDir.at(aTmpDir.length()-1) != '\\') aTmpDir+='\\';
#else
    if(aTmpDir.at(aTmpDir.length()-1) != '/') aTmpDir+='/';
#endif      
  }
  else {
#ifdef WIN32
    aTmpDir = "C:\\";
#else
    aTmpDir = "/tmp/";
#endif
  }
  // a unique name helps to avoid access to the same files by eg different users
  int aUniqueNb;
#ifdef WIN32
  aUniqueNb = GetCurrentProcessId();
#else
  aUniqueNb = getpid();
#endif

  const QString aGenericName    = (aTmpDir + ( "GHS3D_%1" )).arg( aUniqueNb );
  const QString aFacesFileName  = aGenericName + ".faces";  // in faces
  const QString aPointsFileName = aGenericName + ".points"; // in points
  const QString aResultFileName = aGenericName + ".noboite";// out points and volumes
  const QString aBadResFileName = aGenericName + ".boite";  // out bad result
  const QString aBbResFileName  = aGenericName + ".bb";     // out vertex stepsize
  const QString aErrorFileName  = aGenericName + ".log";    // log

  // remove possible old files
  QFile( aFacesFileName ).remove();
  QFile( aPointsFileName ).remove();
  QFile( aResultFileName ).remove();
  QFile( aErrorFileName ).remove();


  // -----------------
  // make input files
  // -----------------

  ofstream aFacesFile  ( aFacesFileName.latin1()  , ios::out);
  ofstream aPointsFile ( aPointsFileName.latin1() , ios::out);
  bool Ok =
#ifdef WIN32
    aFacesFile->is_open() && aPointsFile->is_open();
#else
    aFacesFile.rdbuf()->is_open() && aPointsFile.rdbuf()->is_open();
#endif
  if (!Ok)
  {
    MESSAGE( "Can't write into " << aTmpDir << " directory");
    return false;
  }
  SMESHDS_Mesh* meshDS = theMesh.GetMeshDS();
  map <int,int> aSmdsToGhs3dIdMap;
  map <int,const SMDS_MeshNode*> aGhs3dIdToNodeMap;

  Ok =
    (writePoints( aPointsFile, meshDS, aSmdsToGhs3dIdMap, aGhs3dIdToNodeMap ) &&
     writeFaces ( aFacesFile, meshDS, aSmdsToGhs3dIdMap ));

  aFacesFile.close();
  aPointsFile.close();

  if ( ! Ok )
    return false;

  // -----------------
  // run ghs3d mesher              WIN32???
  // -----------------

  QString cmd = "ghs3d "
    "-m 1000 "                 // memory: 1000 M
      "-f " + aGenericName +   // file to read
        " 1>" + aErrorFileName; // dump into file
  if (system(cmd.latin1()))
  {
    MESSAGE ("command failed: " << cmd.latin1() );
    return false;
  }

  // --------------
  // read a result
  // --------------

  FILE * aResultFile = fopen( aResultFileName.latin1(), "r" );
  if (!aResultFile)
  {
    MESSAGE( "GHS3D ERROR: see " << aErrorFileName.latin1() );
    return false;
  }
  
  Ok = readResult( aResultFile, meshDS, theShape, aGhs3dIdToNodeMap );
  fclose(aResultFile);

  if ( Ok ) {
    QFile( aFacesFileName ).remove();
    QFile( aPointsFileName ).remove();
    QFile( aResultFileName ).remove();
    QFile( aErrorFileName ).remove();
  }
  // remove other possible files
  QFile( aBadResFileName ).remove();
  QFile( aBbResFileName ).remove();
  
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
