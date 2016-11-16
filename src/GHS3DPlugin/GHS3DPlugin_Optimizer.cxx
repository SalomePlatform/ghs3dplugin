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
// File      : GHS3DPlugin_Optimizer.cxx
// Created   : Mon Oct 31 20:05:28 2016

#include "GHS3DPlugin_Optimizer.hxx"

#include "GHS3DPlugin_GHS3D.hxx"
#include "GHS3DPlugin_OptimizerHypothesis.hxx"
#include "MG_Tetra_API.hxx"

#include <SMDS_VolumeTool.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMESH_File.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESH_MesherHelper.hxx>

#include <TopoDS_Shape.hxx>

//================================================================================
/*!
 * \brief Constructor
 */
//================================================================================

GHS3DPlugin_Optimizer::GHS3DPlugin_Optimizer(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_3D_Algo(hypId, studyId, gen)
{
  _name = Name();
  _compatibleHypothesis.push_back( GHS3DPlugin_OptimizerHypothesis::GetHypType());
  _requireShape = false; // work without shape only
}

//================================================================================
/*!
 * \brief Get a hypothesis
 */
//================================================================================

bool GHS3DPlugin_Optimizer::CheckHypothesis(SMESH_Mesh&         aMesh,
                                            const TopoDS_Shape& aShape,
                                            Hypothesis_Status&  aStatus)
{
  _hyp = NULL;

  const std::list <const SMESHDS_Hypothesis * >& hyps = GetUsedHypothesis(aMesh, aShape);
  if ( !hyps.empty() )
    _hyp = dynamic_cast< const GHS3DPlugin_OptimizerHypothesis* >( hyps.front() );

  aStatus = HYP_OK;
  return true;
}

//================================================================================
/*!
 * \brief Terminate a process
 */
//================================================================================

void GHS3DPlugin_Optimizer::CancelCompute()
{
  _computeCanceled = true;
}

//================================================================================
/*!
 * \brief Evaluate nb of elements
 */
//================================================================================

bool GHS3DPlugin_Optimizer::Evaluate(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape,
                                     MapShapeNbElems& aResMap)
{
  return false;
}
//================================================================================
/*!
 * \brief Does nothing 
 */
//================================================================================

bool GHS3DPlugin_Optimizer::Compute(SMESH_Mesh & aMesh, const TopoDS_Shape & aShape)
{
  return false;
}

namespace
{
  //================================================================================
  /*!
   * \brief Compute average size of tetrahedra araound a node
   */
  //================================================================================

  double getSizeAtNode( const SMDS_MeshNode* node )
  {
    double size = 0;
    int nbTet = 0;

    SMDS_VolumeTool vt;
    vt.SetExternalNormal();

    SMDS_ElemIteratorPtr volIt = node->GetInverseElementIterator(SMDSAbs_Volume);
    while ( volIt->more() )
    {
      const SMDS_MeshElement* vol = volIt->next();
      if ( vol->GetGeomType() != SMDSGeom_TETRA )
        continue;
      double volSize = 0.;
      int    nbLinks = 0;
      int dN = vol->IsQuadratic() ? 2 : 1;
      vt.Set( vol );
      for ( int iF = 0, nbF = vt.NbFaces(); iF < nbF; ++iF )
      {
        const SMDS_MeshNode** nodes = vt. GetFaceNodes( iF );
        for ( int iN = 0, nbN = vt.NbFaceNodes( iF ); iN < nbN; iN += dN )
          if ( nodes[ iN ] < nodes[ iN + dN ]) // use a link once
          {
            volSize += SMESH_TNodeXYZ( nodes[ iN ]).Distance( nodes[ iN + dN ]);
            ++nbLinks;
          }
      }
      size += volSize / nbLinks;
      ++nbTet;
    }
    return nbTet > 0 ? size/nbTet : 0.;
  }

  //================================================================================
  /*!
   * \brief Write a mesh file and a solution file
   */
  //================================================================================

  bool writeGMFFile( MG_Tetra_API*       theMGInput,
                     SMESH_MesherHelper* theHelper,
                     const std::string&  theMeshFileName,
                     const std::string&  theSolFileName )
  {
    const int tag = 0;

    int mfile = theMGInput->GmfOpenMesh( theMeshFileName.c_str(), GmfWrite, GMFVERSION,GMFDIMENSION );
    int sfile = theMGInput->GmfOpenMesh( theSolFileName.c_str(), GmfWrite, GMFVERSION,GMFDIMENSION );
    if ( !mfile || !sfile )
      return false;

    // write all nodes and volume size at them

    SMESHDS_Mesh* meshDS = theHelper->GetMeshDS();
    if ( meshDS->NbNodes() != meshDS->MaxNodeID() )
      meshDS->compactMesh();

    theMGInput->GmfSetKwd( mfile, GmfVertices, meshDS->NbNodes() );
    int TypTab[] = { GmfSca };
    theMGInput->GmfSetKwd( sfile, GmfSolAtVertices, meshDS->NbNodes(), 1, TypTab);

    SMDS_NodeIteratorPtr nodeIt = theHelper->GetMeshDS()->nodesIterator();
    while ( nodeIt->more() )
    {
      const SMDS_MeshNode* node = nodeIt->next();
      theMGInput->GmfSetLin( mfile, GmfVertices, node->X(), node->Y(), node->Z(), tag );

      double size = getSizeAtNode( node );
      theMGInput->GmfSetLin( sfile, GmfSolAtVertices, &size );
    }

    // write all triangles

    theMGInput->GmfSetKwd( mfile, GmfTriangles, meshDS->GetMeshInfo().NbTriangles() );
    SMDS_ElemIteratorPtr triaIt = meshDS->elementGeomIterator( SMDSGeom_TRIANGLE );
    while ( triaIt->more() )
    {
      const SMDS_MeshElement* tria = triaIt->next();
      theMGInput->GmfSetLin( mfile, GmfTriangles,
                             tria->GetNode(0)->GetID(),
                             tria->GetNode(1)->GetID(),
                             tria->GetNode(2)->GetID(),
                             tag );
    }

    // write all tetra

    theMGInput->GmfSetKwd( mfile, GmfTetrahedra, meshDS->GetMeshInfo().NbTetras() );
    SMDS_ElemIteratorPtr tetIt = meshDS->elementGeomIterator( SMDSGeom_TETRA );
    while ( tetIt->more() )
    {
      const SMDS_MeshElement* tet = tetIt->next();
      theMGInput->GmfSetLin( mfile, GmfTetrahedra,
                             tet->GetNode(0)->GetID(),
                             tet->GetNode(2)->GetID(),
                             tet->GetNode(1)->GetID(),
                             tet->GetNode(3)->GetID(),
                             tag );
    }

    theMGInput->GmfCloseMesh( mfile );
    theMGInput->GmfCloseMesh( sfile );

    return true;
  }

  //================================================================================
  /*!
   * \brief Read optimized tetrahedra
   */
  //================================================================================

  bool readGMFFile( MG_Tetra_API*       theMGOutput,
                    SMESH_MesherHelper* theHelper,
                    const std::string&  theMeshFileName )
  {
    int ver, dim, tag;
    int inFile = theMGOutput->GmfOpenMesh( theMeshFileName.c_str(), GmfRead, &ver, &dim);
    if ( !inFile )
      return false;

    SMESHDS_Mesh* meshDS = theHelper->GetMeshDS();

    int nbNodes = theMGOutput->GmfStatKwd( inFile, GmfVertices );
    int   nbTet = theMGOutput->GmfStatKwd( inFile, GmfTetrahedra );
    int nbNodesOld = meshDS->NbNodes();
    int   nbTetOld = meshDS->GetMeshInfo().NbTetras();
    std::cout << "Optimization input:  "
              << nbNodesOld << " nodes, \t" << nbTetOld << " tetra" << std::endl;
    std::cout << "Optimization output: "
              << nbNodes << " nodes, \t" << nbTet << " tetra" << std::endl;

    double x,y,z;
    int i1, i2, i3, i4;
    theMGOutput->GmfGotoKwd( inFile, GmfVertices );

    if ( nbNodes == nbNodesOld && nbTet == nbTetOld )
    {
      // move nodes
      SMDS_NodeIteratorPtr nodeIt = theHelper->GetMeshDS()->nodesIterator();
      while ( nodeIt->more() )
      {
        const SMDS_MeshNode* node = nodeIt->next();
        theMGOutput->GmfGetLin( inFile, GmfVertices, &x, &y, &z, &tag );
        meshDS->MoveNode( node, x,y,z );
      }

      // update tetra
      const SMDS_MeshNode* nodes[ 4 ];
      theMGOutput->GmfGotoKwd( inFile, GmfTetrahedra );
      SMDS_ElemIteratorPtr tetIt = meshDS->elementGeomIterator( SMDSGeom_TETRA );
      for ( int i = 0; i < nbTet; ++i )
      {
        const SMDS_MeshElement* tet = tetIt->next();
        theMGOutput->GmfGetLin( inFile, GmfTetrahedra, &i1, &i2, &i3, &i4, &tag);
        nodes[ 0 ] = meshDS->FindNode( i1 );
        nodes[ 1 ] = meshDS->FindNode( i3 );
        nodes[ 2 ] = meshDS->FindNode( i2 );
        nodes[ 3 ] = meshDS->FindNode( i4 );
        meshDS->ChangeElementNodes( tet, &nodes[0], 4 );
      }
    }
    else if ( nbNodes >= nbNodesOld ) // tetra added/removed
    {
      // move or add nodes
      for ( int iN = 1; iN <= nbNodes; ++iN )
      {
        theMGOutput->GmfGetLin( inFile, GmfVertices, &x, &y, &z, &tag );
        const SMDS_MeshNode* node = meshDS->FindNode( iN );
        if ( !node )
          node = meshDS->AddNode( x,y,z );
        else
          meshDS->MoveNode( node, x,y,z );
      }

      // remove tetrahedra
      SMDS_ElemIteratorPtr tetIt = meshDS->elementGeomIterator( SMDSGeom_TETRA );
      while ( tetIt->more() )
        meshDS->RemoveFreeElement( tetIt->next(), /*sm=*/0 );

      // add tetrahedra
      theMGOutput->GmfGotoKwd( inFile, GmfTetrahedra );
      for ( int i = 0; i < nbTet; ++i )
      {
        theMGOutput->GmfGetLin( inFile, GmfTetrahedra, &i1, &i2, &i3, &i4, &tag);
        meshDS->AddVolume( meshDS->FindNode( i1 ),
                           meshDS->FindNode( i3 ),
                           meshDS->FindNode( i2 ),
                           meshDS->FindNode( i4 ));
      }
    }
    else if ( nbNodes < nbNodesOld ) // nodes and tetra removed
    {
      // unmark nodes
      SMDS_NodeIteratorPtr nIt = meshDS->nodesIterator();
      while ( nIt->more() )
        nIt->next()->setIsMarked( false );

      // remove tetrahedra and mark nodes used by them
      SMDS_ElemIteratorPtr tetIt = meshDS->elementGeomIterator( SMDSGeom_TETRA );
      while ( tetIt->more() )
      {
        const SMDS_MeshElement* tet = tetIt->next();
        SMDS_ElemIteratorPtr nIter = tet->nodesIterator();
        while ( nIter->more() )
          nIter->next()->setIsMarked( true );

        meshDS->RemoveFreeElement( tet, /*sm=*/0 );
      }

      // move or add nodes
      for ( int iN = 1; iN <= nbNodes; ++iN )
      {
        theMGOutput->GmfGetLin( inFile, GmfVertices, &x, &y, &z, &tag );
        const SMDS_MeshNode* node = meshDS->FindNode( iN );
        if ( !node )
          node = meshDS->AddNode( x,y,z );
        else
          meshDS->MoveNode( node, x,y,z );
      }

      // add tetrahedra
      theMGOutput->GmfGotoKwd( inFile, GmfTetrahedra );
      for ( int i = 0; i < nbTet; ++i )
      {
        theMGOutput->GmfGetLin( inFile, GmfTetrahedra, &i1, &i2, &i3, &i4, &tag);
        meshDS->AddVolume( meshDS->FindNode( i1 ),
                           meshDS->FindNode( i3 ),
                           meshDS->FindNode( i2 ),
                           meshDS->FindNode( i4 ));
      }

      // remove free marked nodes
      for ( nIt = meshDS->nodesIterator(); nIt->more(); )
      {
        const SMDS_MeshNode* node = nIt->next();
        if ( node->NbInverseElements() == 0 && node->isMarked() )
          meshDS->RemoveFreeNode( node, 0 );
      }
    }

    // avoid "No mesh elements assigned to a sub-shape" error
    theHelper->GetMesh()->GetSubMesh( theHelper->GetSubShape() )->SetIsAlwaysComputed( true );

    return true;
  }

  void getNodeByGhsId( SMESH_Mesh& mesh, std::vector <const SMDS_MeshNode*> & nodeByGhsId )
  {
    SMESHDS_Mesh* meshDS = mesh.GetMeshDS();
    const int nbNodes = meshDS->NbNodes();
    nodeByGhsId.resize( nbNodes + 1 );
    SMDS_NodeIteratorPtr nodeIt = meshDS->nodesIterator();
    while ( nodeIt->more() )
    {
      const SMDS_MeshNode* node = nodeIt->next();
      if ( node->GetID() <= nbNodes )
        nodeByGhsId[ node->GetID() ] = node;
#ifdef _DEBUG_
      else
        throw SALOME_Exception(LOCALIZED ("bad ID -- not compacted mesh"));
#endif
    }
  }

  void removeFile(const std::string& name)
  {
    SMESH_File( name ).remove();
  }
}

//================================================================================
/*!
 * \brief Optimize an existing tetrahedral mesh
 */
//================================================================================

bool GHS3DPlugin_Optimizer::Compute(SMESH_Mesh&         theMesh,
                                    SMESH_MesherHelper* theHelper)
{
  if ( theMesh.NbTetras() == 0 )
    return error( COMPERR_BAD_INPUT_MESH, "No tetrahedra" );
  if ( theMesh.NbTetras( ORDER_QUADRATIC ) > 0 )
    return error( COMPERR_BAD_INPUT_MESH, "Quadratic mesh can't be optimized" );
  if ( theMesh.NbTriangles() == 0 )
    return error( COMPERR_BAD_INPUT_MESH, "2D mesh must exist around tetrahedra" );

  std::string aGenericName    = GHS3DPlugin_Hypothesis::GetFileName(_hyp);
  std::string aLogFileName    = aGenericName + ".log";  // log
  std::string aGMFFileName    = aGenericName + ".mesh"; // input GMF mesh file
  std::string aSolFileName    = aGenericName + ".sol";  // input size map file
  std::string aResultFileName = aGenericName + "_Opt.mesh";  // out GMF mesh file
  std::string aResSolFileName = aGenericName + "_Opt.sol";   // out size map file

  MG_Tetra_API mgTetra( _computeCanceled, _progress );

  bool Ok = writeGMFFile( &mgTetra, theHelper, aGMFFileName, aSolFileName );

  // -----------------
  // run MG-Tetra mesher
  // -----------------

  bool logInStandardOutput = _hyp ? _hyp->GetStandardOutputLog() : false;
  bool removeLogOnSuccess  = _hyp ? _hyp->GetRemoveLogOnSuccess() : true;
  bool keepFiles           = _hyp ? _hyp->GetKeepFiles() : false;

  std::string cmd = GHS3DPlugin_OptimizerHypothesis::CommandToRun( _hyp );

  if ( mgTetra.IsExecutable() )
  {
    cmd += " --in " + aGMFFileName;
    cmd += " --out " + aResultFileName;
  }
  if ( !logInStandardOutput )
  {
    mgTetra.SetLogFile( aLogFileName.c_str() );
    cmd += " 1>" + aLogFileName;  // dump into file
  }
  std::cout << std::endl;
  std::cout << "MG-Tetra execution..." << std::endl;
  std::cout << cmd << std::endl;

  _computeCanceled = false;

  std::string errStr;
  Ok = mgTetra.Compute( cmd, errStr ); // run

  if ( logInStandardOutput && mgTetra.IsLibrary() )
    std::cout << std::endl << mgTetra.GetLog() << std::endl;
  if ( Ok )
    std::cout << std::endl << "End of MG-Tetra execution !" << std::endl;

  // --------------
  // read a result
  // --------------
  Ok = Ok && readGMFFile( &mgTetra, theHelper, aResultFileName );

  // ---------------------
  // remove working files
  // ---------------------
  if ( mgTetra.HasLog() )
  {
    if( _computeCanceled )
      error( "interruption initiated by user" );
    else
    {
      // get problem description from the log file
      std::vector <const SMDS_MeshNode*> nodeByGhsId;
      getNodeByGhsId( theMesh, nodeByGhsId );
      _Ghs2smdsConvertor conv( nodeByGhsId, SMESH_ProxyMesh::Ptr( new SMESH_ProxyMesh( theMesh )));
      error( GHS3DPlugin_GHS3D::getErrorDescription( logInStandardOutput ? 0 : aLogFileName.c_str(),
                                                     mgTetra.GetLog(), conv, Ok ));
    }
  }
  else {
    // the log file is empty
    removeFile( aLogFileName );
    INFOS( "MG-Tetra Error, " << errStr);
    error(COMPERR_ALGO_FAILED, errStr);
  }

  if ( Ok && removeLogOnSuccess )
  {
    removeFile( aLogFileName );
  }
  if ( !keepFiles )
  {
    if ( !Ok && _computeCanceled )
      removeFile( aLogFileName );
    removeFile( aGMFFileName );
    removeFile( aSolFileName );
    removeFile( aResultFileName );
    removeFile( aResSolFileName );
  }
  return Ok;
}


// double GHS3DPlugin_Optimizer::GetProgress() const
// {
// }
