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
   * To make groups of volumes of different domains when mesh is generated from skin.
   * Default is to make groups.
   * This option works only (1) for the mesh w/o shape and (2) if GetToMeshHoles() == true
   */
  void SetToMakeGroupsOfDomains(CORBA::Boolean toMakeGroups);
  CORBA::Boolean GetToMakeGroupsOfDomains();
  /*!
   * Maximal size of memory to be used by the algorithm (in Megabytes)
   */
  void SetMaximumMemory(CORBA::Long MB) throw ( SALOME::SALOME_Exception );
  CORBA::Long GetMaximumMemory();
  /*!
   * Initial size of memory to be used by the algorithm (in Megabytes) in
   * automatic memory adjustment mode. Default is zero
   */
  void SetInitialMemory(CORBA::Long MB) throw ( SALOME::SALOME_Exception );
  CORBA::Long GetInitialMemory();
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
  void SetAdvancedOption(const char* option);
  char* GetAdvancedOption();
  void SetTextOption(const char* option); // obsolete
  char* GetTextOption();
  /*!
  * To define the volumic gradation
  */
  void SetGradation(CORBA::Double gradation);
  CORBA::Double GetGradation();
  /*!
  * Print log in standard output
  */
  void SetStandardOutputLog(CORBA::Boolean logInStandardOutput);
  CORBA::Boolean GetStandardOutputLog();
  /*!
  * Remove log file on success
  */
  void SetRemoveLogOnSuccess(CORBA::Boolean removeLogOnSuccess);
  CORBA::Boolean GetRemoveLogOnSuccess();
  /*!
   * To set an enforced vertex
   */
  bool p_SetEnforcedVertex(CORBA::Double size, CORBA::Double x = 0, CORBA::Double y = 0, CORBA::Double z = 0,
                           const char* theVertexName = "", const char* theVertexEntry = "", const char* theGroupName = "",
                           CORBA::Boolean isCompound = false) 
    throw (SALOME::SALOME_Exception);
  bool SetEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size) throw (SALOME::SALOME_Exception);
  bool SetEnforcedVertexNamed(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size, const char* theVertexName) throw (SALOME::SALOME_Exception);
  bool SetEnforcedVertexWithGroup(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size, const char* theGroupName) throw (SALOME::SALOME_Exception);
  bool SetEnforcedVertexNamedWithGroup(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size, const char* theVertexName, const char* theGroupName) throw (SALOME::SALOME_Exception);
  bool SetEnforcedVertexGeom(GEOM::GEOM_Object_ptr theVertex, CORBA::Double size) throw (SALOME::SALOME_Exception);
  bool SetEnforcedVertexGeomWithGroup(GEOM::GEOM_Object_ptr theVertex, CORBA::Double size, const char* theGroupName) throw (SALOME::SALOME_Exception);
  CORBA::Double GetEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z) throw (SALOME::SALOME_Exception);
  CORBA::Double GetEnforcedVertexGeom(GEOM::GEOM_Object_ptr theVertex) throw (SALOME::SALOME_Exception);
  bool RemoveEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z) throw (SALOME::SALOME_Exception);
  bool RemoveEnforcedVertexGeom(GEOM::GEOM_Object_ptr theVertex) throw (SALOME::SALOME_Exception);
  GHS3DPlugin::GHS3DEnforcedVertexList* GetEnforcedVertices();
  void ClearEnforcedVertices();
  /*!
   * To set an enforced mesh
   */  
  bool p_SetEnforcedMesh(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType, const char* theName="", const char* theGroupName="") throw (SALOME::SALOME_Exception);
  bool SetEnforcedMesh(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType) throw (SALOME::SALOME_Exception);
  bool SetEnforcedMeshWithGroup(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType, const char* theGroupName) throw (SALOME::SALOME_Exception);

  /* OBSOLETE FUNCTIONS */
  bool SetEnforcedMeshSize(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType, double size) throw (SALOME::SALOME_Exception);
  bool SetEnforcedMeshSizeWithGroup(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType, double size, const char* theGroupName) throw (SALOME::SALOME_Exception);
  /* OBSOLETE FUNCTIONS */

  GHS3DPlugin::GHS3DEnforcedMeshList* GetEnforcedMeshes();
  void ClearEnforcedMeshes();

  // Get implementation
  ::GHS3DPlugin_Hypothesis* GetImpl();
  
  // Verify whether hypothesis supports given entity type 
  CORBA::Boolean IsDimSupported( SMESH::Dimension type );
  
};

#endif
