// Copyright (C) 2004-2023  CEA, EDF
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
                            ::SMESH_Gen*            theGenImpl);
  // Destructor
  virtual ~GHS3DPlugin_Hypothesis_i();

  /*!
   * To mesh "holes" in a solid or not. Default is to mesh.
   */
  void SetToMeshHoles(CORBA::Boolean toMesh);
  CORBA::Boolean GetToMeshHoles();
  /*!
   *  Activate/deactivate volume proximity computation
   */
  void SetVolumeProximity( CORBA::Boolean toUse );
  CORBA::Boolean GetVolumeProximity();
  /*!
   * Set number of surface element layers to be generated due to volume proximity
   */
  void SetNbVolumeProximityLayers( CORBA::Short nbLayers );
  CORBA::Short GetNbVolumeProximityLayers();

  void SetMaxSize(CORBA::Double theMaxSize);
  CORBA::Double GetMaxSize();

  void SetMinSize(CORBA::Double theMinSize);
  CORBA::Double GetMinSize();
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
  void SetMaximumMemory(CORBA::Float MB);
  CORBA::Float GetMaximumMemory();
  /*!
   * Initial size of memory to be used by the algorithm (in Megabytes) in
   * automatic memory adjustment mode. Default is zero
   */
  void SetInitialMemory(CORBA::Float MB);
  CORBA::Float GetInitialMemory();
  /*!
   * Optimization level: 0-none, 1-light, 2-medium, 3-strong. Default is medium
   */
  void SetOptimizationLevel(CORBA::Short level);
  CORBA::Short GetOptimizationLevel();
  /*!
   * Algorithm Id: 0-MGTetra HPC, 1-MGTetra
   */
  void SetAlgorithm(CORBA::Short algoId);
  CORBA::Short GetAlgorithm();
   /*!
   * Flag for set get optional use parallelism in MGTetra
   */
  void SetUseNumOfThreads(CORBA::Boolean useThreads);
  CORBA::Boolean GetUseNumOfThreads();
   /*!
   * Get set number of threads to use on the parallel MGTetra algorithm
   */
  void SetNumOfThreads(CORBA::Short numThreads);
  CORBA::Short GetNumOfThreads();
  /*!
   * For MGTetra
   * PthreadMode: 0-PThreadNone, 1-PThreadAggressive, 2-Safe
   */
  void SetPthreadMode(CORBA::Short pThreadMode);
  CORBA::Short GetPthreadMode();
  /*!
   * For MGTetra HPC 
   * SetParallelMode level: 0-ParallelNone, 1-ReproducibleGivenMaxNumThreads, 2-Reproducible, 3-ParallelAggressive   
   */
  void SetParallelMode(CORBA::Short parallelMode);
  CORBA::Short GetParallelMode();
  /*!
   * Path to working directory
   */
  void SetWorkingDirectory(const char* path);
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
  void SetVerboseLevel(CORBA::Short level);
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

  void SetOptionValue(const char* optionName, const char* optionValue);
  char* GetOptionValue(const char* optionName);
  void UnsetOption(const char* optionName);

  GHS3DPlugin::string_array* GetOptionValues();
  GHS3DPlugin::string_array* GetAdvancedOptionValues();

  void SetOptionValues(const GHS3DPlugin::string_array& options);
  void SetAdvancedOptionValues(const GHS3DPlugin::string_array& options);

  void AddOption(const char* optionName, const char* optionValue);
  char* GetOption(const char* optionName);
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
                           CORBA::Boolean isCompound = false);
  bool SetEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size);
  bool SetEnforcedVertexNamed(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size, const char* theVertexName);
  bool SetEnforcedVertexWithGroup(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size, const char* theGroupName);
  bool SetEnforcedVertexNamedWithGroup(CORBA::Double x, CORBA::Double y, CORBA::Double z, CORBA::Double size, const char* theVertexName, const char* theGroupName);
  bool SetEnforcedVertexGeom(GEOM::GEOM_Object_ptr theVertex, CORBA::Double size);
  bool SetEnforcedVertexGeomWithGroup(GEOM::GEOM_Object_ptr theVertex, CORBA::Double size, const char* theGroupName);
  CORBA::Double GetEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z);
  CORBA::Double GetEnforcedVertexGeom(GEOM::GEOM_Object_ptr theVertex);
  bool RemoveEnforcedVertex(CORBA::Double x, CORBA::Double y, CORBA::Double z);
  bool RemoveEnforcedVertexGeom(GEOM::GEOM_Object_ptr theVertex);
  GHS3DPlugin::GHS3DEnforcedVertexList* GetEnforcedVertices();
  void ClearEnforcedVertices();
  /*!
   * To set an enforced mesh
   */  
  bool p_SetEnforcedMesh(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType, const char* theName="", const char* theGroupName="");
  bool SetEnforcedMesh(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType);
  bool SetEnforcedMeshWithGroup(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType, const char* theGroupName);

  /* OBSOLETE FUNCTIONS */
  bool SetEnforcedMeshSize(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType, double /*size*/);
  bool SetEnforcedMeshSizeWithGroup(SMESH::SMESH_IDSource_ptr theSource, SMESH::ElementType elementType, double /*size*/, const char* theGroupName);
  /* OBSOLETE FUNCTIONS */

  GHS3DPlugin::GHS3DEnforcedMeshList* GetEnforcedMeshes();
  void ClearEnforcedMeshes();

  // 3 GUI methods
  CORBA::Double GetMaxSizeDefault();
  CORBA::Double GetMinSizeDefault();
  void SetMinMaxSizeDefault( CORBA::Double theMinSize, CORBA::Double theMaxSize );

  // Get implementation
  ::GHS3DPlugin_Hypothesis* GetImpl();
  
  // Verify whether hypothesis supports given entity type 
  CORBA::Boolean IsDimSupported( SMESH::Dimension type );
  

  // Methods for copying mesh definition to other geometry

  // Return geometry this hypothesis depends on. Return false if there is no geometry parameter
  virtual bool getObjectsDependOn( std::vector< std::string > & entryArray,
                                   std::vector< int >         & subIDArray ) const;

  // Set new geometry instead of that returned by getObjectsDependOn()
  virtual bool setObjectsDependOn( std::vector< std::string > & entryArray,
                                   std::vector< int >         & subIDArray );
};

#endif
