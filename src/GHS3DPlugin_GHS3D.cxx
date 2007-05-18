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
// Author    : Edward AGAPOV
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
                        const TopoDS_Shape&   theShape,
                        const map <int,int> & theSmdsToGhs3dIdMap)
{
  // record structure:
  //
  // NB_ELEMS DUMMY_INT
  // Loop from 1 to NB_ELEMS
  //   NB_NODES NODE_NB_1 NODE_NB_2 ... (NB_NODES + 1) times: DUMMY_INT

  // get all faces bound to theShape
  int nbFaces = 0;
  list< const SMDS_MeshElement* > faces;
  TopExp_Explorer fExp( theShape, TopAbs_FACE );
  for ( ; fExp.More(); fExp.Next() ) {
    SMESHDS_SubMesh* sm = theMesh->MeshElements( fExp.Current() );
    if ( sm ) {
      SMDS_ElemIteratorPtr eIt = sm->GetElements();
      while ( eIt->more() ) {
        faces.push_back( eIt->next() );
        nbFaces++;
      }
    }
  }

  if ( nbFaces == 0 )
    return false;

  const char* space    = "  ";
  const int   dummyint = 0;

  // NB_ELEMS DUMMY_INT
  theFile << space << nbFaces << space << dummyint << endl;

  // Loop from 1 to NB_ELEMS
  list< const SMDS_MeshElement* >::iterator f = faces.begin();
  for ( ; f != faces.end(); ++f )
  {
    // NB_NODES
    const SMDS_MeshElement* elem = *f;
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
  int shapeID = theMesh->ShapeToIndex( theShape );

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
        theMesh->SetNodeInVolume( aNewNode, shapeID );
        theGhs3dIdToNodeMap.insert ( make_pair( ID, aNewNode ));
        node[ iNode ] = aNewNode;
      }
      else
      {
        node[ iNode ] = IdNode->second;
      }
    }
    // create a tetrahedron with orientation as for MED
    SMDS_MeshElement* aTet = theMesh->AddVolume( node[1], node[0], node[2], node[3] );
    theMesh->SetMeshElementOnShape( aTet, shapeID );
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
  MESSAGE("GHS3DPlugin_GHS3D::Compute");

  SMESHDS_Mesh* meshDS = theMesh.GetMeshDS();

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
  bool Ok =
#ifdef WIN32
    aFacesFile->is_open() && aPointsFile->is_open();
#else
    aFacesFile.rdbuf()->is_open() && aPointsFile.rdbuf()->is_open();
#endif

  if (!Ok)
    return error( SMESH_Comment("Can't write into ") << aTmpDir.ToCString());

  map <int,int> aSmdsToGhs3dIdMap;
  map <int,const SMDS_MeshNode*> aGhs3dIdToNodeMap;

  Ok =
    (writePoints( aPointsFile, meshDS, aSmdsToGhs3dIdMap, aGhs3dIdToNodeMap ) &&
     writeFaces ( aFacesFile, meshDS, theShape, aSmdsToGhs3dIdMap ));

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

  TCollection_AsciiString cmd( "ghs3d " ); // command to run
  cmd +=
    memory +                   // memory
      " -f " + aGenericName +  // file to read
        " 1>" + aLogFileName;  // dump into file

  system( cmd.ToCString() ); // run

  // --------------
  // read a result
  // --------------

  FILE * aResultFile = fopen( aResultFileName.ToCString(), "r" );
  if (aResultFile)
  {
    Ok = readResult( aResultFile, meshDS, theShape, aGhs3dIdToNodeMap );
    fclose(aResultFile);
  }
  else
  {
    Ok = error( "Problem with launching ghs3d");
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
