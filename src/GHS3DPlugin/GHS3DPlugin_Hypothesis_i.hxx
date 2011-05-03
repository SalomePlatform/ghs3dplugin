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

//  GHS3DPlugin : C++ implementation
// File      : GHS3DPlugin_Hypothesis_i.hxx
// Date      : 03/04/2006
// Project   : SALOME
//
#ifndef _GHS3DPlugin_Hypothesis_i_HXX_
#define _GHS3DPlugin_Hypothesis_i_HXX_

#include "GHS3DPlugin_Defs.hxx"

#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(GHS3DPlugin_Algorithm)

#include "SMESH_Hypothesis_i.hxx"
#include "SMESH_Mesh_i.hxx"
#include "GHS3DPlugin_Hypothesis.hxx"

class SMESH_Gen;

// GHS3DPlugin parameters hypothesis

class GHS3DPLUGIN_EXPORT GHS3DPlugin_Hypothesis_i:
  public virtual POA_GHS3DPlugin::GHS3DPlugin_Hypothesis,
  public virtual SMESH_Hypothesis_i
{
 public:
  // Constructor
  GHS3DPlugin_Hypothesis_i (PortableServer::POA_ptr thePOA,
                            int                     theStudyId,
                            ::SMESH_Gen*            theGenImpl);
  // Destructor
  virtual ~GHS3DPlugin_Hypothesis_i();

  /*!
   * To mesh "holes" in a solid or not. Default is to mesh.
   */
  void SetToMeshHoles(CORBA::Boolean toMesh);
  CORBA::Boolean GetToMeshHoles();
  /*!
   * Maximal size of memory to be used by the algorithm (in Megabytes)
   */
  void SetMaximumMemory(CORBA::Short MB) throw ( SALOME::SALOME_Exception );
  CORBA::Short GetMaximumMemory();
  /*!
   * Initial size of memory to be used by the algorithm (in Megabytes) in
   * automatic memory adjustment mode. Default is zero
   */
  void SetInitialMemory(CORBA::Short MB) throw ( SALOME::SALOME_Exception );
  CORBA::Short GetInitialMemory();
  /*!
   * Optimization level: 0-none, 1-light, 2-medium, 3-strong. Default is medium
   */
  void SetOptimizationLevel(CORBA::Short level) throw ( SALOME::SALOME_Exception );
  CORBA::Short GetOptimizationLevel();
  /*!
   * Path to working directory
   */
  void SetWorkingDirectory(const char* path) throw ( SALOME::SALOME_Exception );
  char* GetWorkingDirectory();
  /*!
   * To keep working files or remove them. Log file remains in case of errors anyway.
   */
  void SetKeepFiles(CORBA::Boolean toKeep);
  CORBA::Boolean GetKeepFiles();
  /*!
   * Verbose level [0-10]
   *  0 - no standard output,
   *  2 - prints the data, quality statistics of the skin and final meshes and
   *     indicates when the final mesh is being saved. In addition the software
   *     gives indication regarding the CPU time.
   * 10 - same as 2 plus the main steps in the computation, quality statistics
   *     histogram of the skin mesh, quality statistics histogram together with
   *     the characteristics of the final mesh.
   */
  void SetVerboseLevel(CORBA::Short level) throw ( SALOME::SALOME_Exception );
  CORBA::Short GetVerboseLevel();
  /*!
   * To create new nodes
   */
  void SetToCreateNewNodes(CORBA::Boolean toCreate);
  CORBA::Boolean GetToCreateNewNodes();
  /*!
   * To use boundary recovery version which tries to create mesh on a very poor
   * quality surface mesh
   */
  void SetToUseBoundaryRecoveryVersion(CORBA::Boolean toUse);
  CORBA::Boolean GetToUseBoundaryRecoveryVersion();
  /*!
   * Applies ﬁnite-element correction by replacing overconstrained elements where
   * it is possible. The process is cutting ﬁrst the overconstrained edges and
   * second the overconstrained facets. This insure that no edges have two boundary
   * vertices and that no facets have three boundary vertices.
   */
  void SetFEMCorrection(CORBA::Boolean toUseFem);
  CORBA::Boolean GetFEMCorrection();
  /*!
   * To removes initial central point.
   */
  void SetToRemoveCentralPoint(CORBA::Boolean toRemove);
  CORBA::Boolean GetToRemoveCentralPoint();
  /*!
   * To set hiden/undocumented/advanced options
   */
  void SetTextOption(const char* option);
  char* GetTextOption();
  /*!
   * To set an enforced vertex
   */
  void SetEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size);
  CORBA::Double GetEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z) throw (SALOME::SALOME_Exception);
  void RemoveEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z) throw (SALOME::SALOME_Exception);
  GHS3DPlugin::GHS3DEnforcedVertexList* GetEnforcedVertices();
  void ClearEnforcedVertices();
  /*!
   * To set an enforced mesh
   */
  void SetEnforcedMesh(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType) throw (SALOME::SALOME_Exception);
  void SetEnforcedMeshSize(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType, double size) throw (SALOME::SALOME_Exception);
  void _SetEnforcedMesh(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType, double size) throw (SALOME::SALOME_Exception);
  void ClearEnforcedMeshes();

  // Get implementation
  ::GHS3DPlugin_Hypothesis* GetImpl();
  
  // Verify whether hypothesis supports given entity type 
  CORBA::Boolean IsDimSupported( SMESH::Dimension type );
};

#endif