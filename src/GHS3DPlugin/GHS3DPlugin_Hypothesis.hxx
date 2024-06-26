// Copyright (C) 2004-2024  CEA, EDF
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
// File      : GHS3DPlugin_Hypothesis.hxx
// Created   : Wed Apr  2 12:21:17 2008
// Author    : Edward AGAPOV (eap)
//
#ifndef GHS3DPlugin_Hypothesis_HeaderFile
#define GHS3DPlugin_Hypothesis_HeaderFile

#include "GHS3DPlugin_Defs.hxx"

#include <SMDS_MeshNode.hxx>

#include "SMESH_Hypothesis.hxx"
#include "SMESH_Mesh_i.hxx"
#include "SMESH_Gen_i.hxx"
#include "SMESH_TypeDefs.hxx"
#include "utilities.h"

#include <stdexcept>
#include <map>
#include <vector>
#include <cstdio>

class GHS3DPLUGIN_EXPORT GHS3DPlugin_Hypothesis: public SMESH_Hypothesis
{
public:

  GHS3DPlugin_Hypothesis(int hypId, SMESH_Gen * gen);

  typedef std::map<std::vector<double>,double> TGHS3DEnforcedVertexCoordsValues;
  typedef std::map<std::string,double> TGHS3DEnforcedVertexEntryValues;
  
  struct TGHS3DEnforcedVertex {
    std::string name;
    std::string geomEntry;
    bool isCompound;
    std::vector<double> coords;
    std::string groupName;
    double size;
  };
  
  struct CompareGHS3DEnforcedVertex {
    bool operator () (const TGHS3DEnforcedVertex* e1, const TGHS3DEnforcedVertex* e2) const {
      if (e1 && e2) {
        if (e1->coords.size() && e2->coords.size())
          return (e1->coords < e2->coords);
        else
          return (e1->geomEntry < e2->geomEntry);
      }
      return false;
    }
  };
  typedef std::set< TGHS3DEnforcedVertex*, CompareGHS3DEnforcedVertex > TGHS3DEnforcedVertexList;
  // Map Coords / Enforced node
  typedef std::map< std::vector<double>, TGHS3DEnforcedVertex* > TCoordsGHS3DEnforcedVertexMap;
  // Map geom entry / Enforced node
  typedef std::map< std::string, TGHS3DEnforcedVertex* > TGeomEntryGHS3DEnforcedVertexMap;
  // Map groupName / Enforced node
  typedef std::map< std::string, TGHS3DEnforcedVertexList > TGroupNameGHS3DEnforcedVertexMap;
  
  ////////////////////
  // Enforced meshes
  ////////////////////
  
  struct TGHS3DEnforcedMesh {
    int         persistID;
    std::string name;
    std::string entry;
    std::string groupName;
    SMESH::ElementType elementType;
  };
  
  struct CompareGHS3DEnforcedMesh {
    bool operator () (const TGHS3DEnforcedMesh* e1, const TGHS3DEnforcedMesh* e2) const {
      if (e1 && e2) {
        if (e1->entry == e2->entry)
          return (e1->elementType < e2->elementType);
        else
          return (e1->entry < e2->entry);
      }
      else
        return false;
    }
  };
  typedef std::set< TGHS3DEnforcedMesh*, CompareGHS3DEnforcedMesh > TGHS3DEnforcedMeshList;
  // Map mesh entry / Enforced mesh list
  // ex: 0:1:2:1 -> [ ("Mesh_1", "0:1:2:1", TopAbs_NODE, ""),
  //                  ("Mesh_1", "0:1:2:1", TopAbs_EDGE, "edge group")]
  typedef std::map< std::string, TGHS3DEnforcedMeshList > TEntryGHS3DEnforcedMeshListMap;
  
  typedef std::map<int,double> TID2SizeMap;

  struct TIDMeshIDCompare {
    bool operator () (const SMDS_MeshElement* e1, const SMDS_MeshElement* e2) const
    { return e1->GetMesh() == e2->GetMesh() ? e1->GetID() < e2->GetID() : e1->GetMesh() < e2->GetMesh(); }
  };
  
  typedef std::map<const SMDS_MeshElement*, std::string, TIDMeshIDCompare > TIDSortedElemGroupMap;
  typedef std::map<const SMDS_MeshNode*, std::string, TIDMeshIDCompare > TIDSortedNodeGroupMap;
  typedef std::set<std::string> TSetStrings;

  static const char* GetHypType() { return "MG-Tetra Parameters"; }

  void SetMinSize(double theMinSize);
  double GetMinSize() const { return myMinSize; }

  void SetMaxSize(double theMaxSize);
  double GetMaxSize() const { return myMaxSize; }

  void SetUseVolumeProximity( bool toUse );
  bool GetUseVolumeProximity() const { return myUseVolumeProximity; }

  void SetNbVolumeProximityLayers( int nbLayers );
  int GetNbVolumeProximityLayers() const { return myNbVolumeProximityLayers; }

  /*!
   * To mesh "holes" in a solid or not. Default is to mesh.
   */
  void SetToMeshHoles(bool toMesh);
  bool GetToMeshHoles(bool checkFreeOption = false) const;
  /*!
   * To make groups of volumes of different domains when mesh is generated from skin.
   * Default is to make groups.
   * This option works only (1) for the mesh w/o shape and (2) if GetToMeshHoles() == true
   */
  void SetToMakeGroupsOfDomains(bool toMakeGroups);
  bool GetToMakeGroupsOfDomains() const;
  /*!
   * Maximal size of memory to be used by the algorithm (in Megabytes)
   */
  void SetMaximumMemory(float MB);
  float GetMaximumMemory() const;
  /*!
   * Initial size of memory to be used by the algorithm (in Megabytes) in
   * automatic memory adjustment mode. Default is zero
   */
  void SetInitialMemory(float MB);
  float GetInitialMemory() const;
  /*!
   * Optimization level: 0-none, 1-light, 2-medium, 3-standard+, 4-strong. Default is medium
   */
  enum OptimizationLevel { None = 0, Light, Medium, StandardPlus, Strong };
  void SetOptimizationLevel(OptimizationLevel level);
  OptimizationLevel GetOptimizationLevel() const;
  /*!
   * Path to working directory
   */
  void SetWorkingDirectory(const std::string& path);
  std::string GetWorkingDirectory() const;
  /*!
   * To keep working files or remove them. Log file remains in case of errors anyway.
   */
  void SetKeepFiles(bool toKeep);
  bool GetKeepFiles() const;
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
  void SetVerboseLevel(short level);
  short GetVerboseLevel() const;
  /*!
   * Implemented algorithms to be executed [0,1]
   *  0 - MGTetra-HPC
   *  1 - MGTetra
   */
  enum ImplementedAlgorithms { MGTetraHPC = 0,  MGTetra };
  void SetAlgorithm(ImplementedAlgorithms algoId);
  ImplementedAlgorithms GetAlgorithm() const;
  /*!
   * Set Get for flag to use pthread parallel version of the algorithm
   */
  void SetUseNumOfThreads(bool setUseOfThreads);
  bool GetUseNumOfThreads() const;
   /*!
   * Set Get num of threads to be used by MGTetra algorithms
   */
  void SetNumOfThreads(short numOfThreads);
  short GetNumOfThreads() const;
  /*!
  * Set Get pthread mode used for MGTetra
  */
  enum PThreadMode { PThreadNone = 0, PThreadAggressive,  Safe };
  void SetPthreadMode(PThreadMode pthreadMode );
  PThreadMode GetPthreadMode() const;
  /*!
  * Set Get paralle mode used for MGTetra HPC
  */
  enum ParallelMode { ParallelNone = 0, ReproducibleGivenMaxNumThreads, Reproducible, ParallelAggressive };
  void SetParallelMode(ParallelMode parallelMode );
  ParallelMode GetParallelMode() const;
  /*!
   * To create new nodes
   */
  void SetToCreateNewNodes(bool toCreate);
  bool GetToCreateNewNodes() const;
  /*!
   * To use boundary recovery version which tries to create mesh on a very poor
   * quality surface mesh
   */
  void SetToUseBoundaryRecoveryVersion(bool toUse);
  bool GetToUseBoundaryRecoveryVersion() const;
  /*!
   * Applies ﬁnite-element correction by replacing overconstrained elements where
   * it is possible. The process is cutting ﬁrst the overconstrained edges and
   * second the overconstrained facets. This insure that no edges have two boundary
   * vertices and that no facets have three boundary vertices.
   */
  void SetFEMCorrection(bool toUseFem);
  bool GetFEMCorrection() const;
  /*!
   * To removes initial central point.
   */
  void SetToRemoveCentralPoint(bool toRemove);
  bool GetToRemoveCentralPoint() const;
  /*!
   * To set hiden/undocumented/advanced options
   */
  void SetAdvancedOption(const std::string& option);
  std::string GetAdvancedOption() const;  
  /*!
  * To define the volumic gradation
  */
  void SetGradation(double gradation);
  double GetGradation() const;
  /*!
  * Print log in standard output
  */
  void SetStandardOutputLog(bool logInStandardOutput);
  bool GetStandardOutputLog() const;
  /*!
  * Remove log file on success
  */
  void SetRemoveLogOnSuccess(bool removeLogOnSuccess);
  bool GetRemoveLogOnSuccess() const;
    

  typedef std::map< std::string, std::string > TOptionValues;
  typedef std::set< std::string >              TOptionNames;

  void SetOptionValue(const std::string& optionName,
                      const std::string& optionValue);
  std::string GetOptionValue(const std::string& optionName,
                             bool*              isDefault=0) const;
  bool HasOptionDefined( const std::string& optionName ) const;
  void ClearOption(const std::string& optionName);
  TOptionValues        GetOptionValues()       const;
  const TOptionValues& GetCustomOptionValues() const { return _customOption2value; }

  // bool isMGTetraHPCOptions( std::string & option ) const;
  /*!
   * \brief To set the advanced options in the execution command line
   */
  void SetAdvancedOptionsInCommandLine( std::string & cmd ) const;
  
  //static inline const char* NoValue() { return "_"; }  
//   struct TEnforcedEdge {
//     long ID;
//     long node1;
//     long node2;
//     std::string groupName;
//   };
  /*!
   * \brief Return command to run MG-Tetra mesher excluding file prefix (-f)
   */
  static std::string CommandToRun(const GHS3DPlugin_Hypothesis* hyp,
                                  const bool                    hasShapeToMesh,
                                  const bool                    forExucutable);
  /*!
   * \brief Return a unique file name
   */
  static std::string GetFileName(const GHS3DPlugin_Hypothesis* hyp);
  /*!
   * \brief Return a unique file name for MGTetraHPC will have a GHS3D prefix
   */
  static std::string GetFileNameHPC(const GHS3DPlugin_Hypothesis* hyp);
  /*!
   * \brief Return the name of executable
   */
  static std::string GetExeName( ImplementedAlgorithms algoId );

  /*!
   * To set an enforced vertex
   */
  bool SetEnforcedVertex(std::string aName, std::string anEntry, std::string aGroupName,
                         double size, double x=0.0, double y=0.0, double z=0.0, bool isCompound = false);
  TGHS3DEnforcedVertex* GetEnforcedVertex(double x, double y, double z);
  TGHS3DEnforcedVertex* GetEnforcedVertex(const std::string anEntry);
  bool RemoveEnforcedVertex(double x=0.0, double y=0.0, double z=0.0, const std::string anEntry="" );
  const TGHS3DEnforcedVertexCoordsValues& _GetEnforcedVerticesCoordsSize() const {return _enfVertexCoordsSizeList; }
  const TGHS3DEnforcedVertexEntryValues & _GetEnforcedVerticesEntrySize() const {return _enfVertexEntrySizeList; }
  const TGHS3DEnforcedVertexList        & _GetEnforcedVertices() const { return _enfVertexList; }
  const TCoordsGHS3DEnforcedVertexMap   & _GetEnforcedVerticesByCoords() const { return _coordsEnfVertexMap; }
  const TGeomEntryGHS3DEnforcedVertexMap& _GetEnforcedVerticesByEntry() const { return _geomEntryEnfVertexMap; }
  void ClearEnforcedVertices();

  /*!
   * To set enforced elements
   */
  bool SetEnforcedMesh(SMESH_Mesh& theMesh, SMESH::ElementType elementType, std::string name, std::string entry, std::string groupName = "");
  bool SetEnforcedGroup(const SMESHDS_Mesh* theMeshDS, SMESH::smIdType_array_var theIDs, SMESH::ElementType elementType, std::string name, std::string entry, std::string groupName = "");
  bool SetEnforcedElements(TIDSortedElemSet theElemSet, SMESH::ElementType elementType, std::string groupName = "");
  const TGHS3DEnforcedMeshList _GetEnforcedMeshes() const { return _enfMeshList; }
  const TEntryGHS3DEnforcedMeshListMap _GetEnforcedMeshesByEntry() const { return _entryEnfMeshMap; }
  void ClearEnforcedMeshes();
  const TIDSortedNodeGroupMap _GetEnforcedNodes() const { return _enfNodes; }
  const TIDSortedElemGroupMap _GetEnforcedEdges() const { return _enfEdges; }
  const TIDSortedElemGroupMap _GetEnforcedTriangles() const { return _enfTriangles; }
  const TID2SizeMap _GetNodeIDToSizeMap() const {return _nodeIDToSizeMap; }
  const TSetStrings _GetGroupsToRemove() const {return _groupsToRemove; }
  void RestoreEnfElemsByMeshes(); // persistence
  /*!
   * \brief Return the enforced vertices
   */
  static TGHS3DEnforcedVertexList GetEnforcedVertices(const GHS3DPlugin_Hypothesis* hyp);
  static TGHS3DEnforcedVertexCoordsValues GetEnforcedVerticesCoordsSize(const GHS3DPlugin_Hypothesis* hyp);
  static TGHS3DEnforcedVertexEntryValues  GetEnforcedVerticesEntrySize(const GHS3DPlugin_Hypothesis* hyp);
  static TCoordsGHS3DEnforcedVertexMap GetEnforcedVerticesByCoords(const GHS3DPlugin_Hypothesis* hyp);
  static TGeomEntryGHS3DEnforcedVertexMap GetEnforcedVerticesByEntry(const GHS3DPlugin_Hypothesis* hyp);
  
  static TGHS3DEnforcedMeshList GetEnforcedMeshes(const GHS3DPlugin_Hypothesis* hyp);
  static TEntryGHS3DEnforcedMeshListMap GetEnforcedMeshesByEntry(const GHS3DPlugin_Hypothesis* hyp);
  static TIDSortedNodeGroupMap GetEnforcedNodes(const GHS3DPlugin_Hypothesis* hyp);
  static TIDSortedElemGroupMap GetEnforcedEdges(const GHS3DPlugin_Hypothesis* hyp);
  static TIDSortedElemGroupMap GetEnforcedTriangles(const GHS3DPlugin_Hypothesis* hyp);
  static TID2SizeMap GetNodeIDToSizeMap(const GHS3DPlugin_Hypothesis* hyp);
  static TSetStrings GetGroupsToRemove(const GHS3DPlugin_Hypothesis* hyp);
  static bool GetToMakeGroupsOfDomains(const GHS3DPlugin_Hypothesis* hyp);
  void ClearGroupsToRemove();
  
  static bool   DefaultMeshHoles();
  static bool   DefaultToMakeGroupsOfDomains();
  static float  DefaultMaximumMemory();
  static float  DefaultInitialMemory();
  static short  DefaultOptimizationLevel();
  static std::string DefaultWorkingDirectory();
  static bool   DefaultKeepFiles();
  static short  DefaultVerboseLevel();
  static bool   DefaultToCreateNewNodes();
  static bool   DefaultToUseBoundaryRecoveryVersion();
  static bool   DefaultToUseFEMCorrection();
  static bool   DefaultToRemoveCentralPoint();
  static bool   DefaultStandardOutputLog();
  static bool   DefaultRemoveLogOnSuccess();
  static inline double DefaultGradation() { return 1.05; }
  static bool   DefaultUseVolumeProximity() { return false; }
  static int    DefaultNbVolumeProximityLayers() { return 2; }
  static short  DefaultAlgorithm() { return MGTetra; }
  static short  DefaultNumOfThreads() { return 4; }
  static bool   DefaultUseNumOfThreads() { return true; }
  static short  DefaultMyPthreadMode() { return 2; }    //reproducible_given_max_of_threads
  static short  DefaultMyPthreadModeHPC() { return 1; } // safe
   
  void SetMinMaxSizeDefault( double theMinSize, double theMaxSize )
  { myMinSizeDefault = theMinSize; myMaxSizeDefault = theMaxSize; }
  double GetMinSizeDefault() const { return myMinSizeDefault; }
  double GetMaxSizeDefault() const { return myMaxSizeDefault; }

  // Persistence
  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);

  /*!
   * \brief Does nothing
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);

  /*!
   * \brief Sets myToMakeGroupsOfDomains depending on whether theMesh is on shape or not
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);

  static bool  ToBool(const std::string& str, bool* isOk=0);
  static double ToDbl(const std::string& str, bool* isOk=0);
  static int    ToInt(const std::string& str, bool* isOk=0);

protected:

  bool        myToMeshHoles;
  bool        myToMakeGroupsOfDomains;
  float       myMaximumMemory;
  float       myInitialMemory;
  short       myOptimizationLevel;
  bool        myKeepFiles;
  std::string myWorkingDirectory;
  short       myVerboseLevel;
  bool        myToCreateNewNodes;
  bool        myToUseBoundaryRecoveryVersion;
  bool        myToUseFemCorrection;
  bool        myToRemoveCentralPoint;
  bool        myLogInStandardOutput;
  bool        myRemoveLogOnSuccess;
  double      myGradation;
  bool        myUseVolumeProximity;
  int         myNbVolumeProximityLayers;
  short       myAlgorithm;  //member used to pivot between MG-Tetra and MG-Tetra
  short       myNumOfThreads;
  bool        myUseNumOfThreads;
  short       myPthreadModeMG;
  short       myPthreadModeMGHPC;
  double      myMinSize, myMinSizeDefault;
  double      myMaxSize, myMaxSizeDefault;
  //std::string myTextOption;

  
  TOptionValues _option2value, _customOption2value;         // user defined values
  TOptionValues _defaultOptionValues;                       // default values
  TOptionNames  _doubleOptions, _charOptions, _boolOptions; // to find a type of option  

  TGHS3DEnforcedVertexList         _enfVertexList;
  TGHS3DEnforcedVertexCoordsValues _enfVertexCoordsSizeList;
  TGHS3DEnforcedVertexEntryValues  _enfVertexEntrySizeList;
  // map to get "manual" enf vertex (through the coordinates)
  TCoordsGHS3DEnforcedVertexMap    _coordsEnfVertexMap;
  // map to get "geom" enf vertex (through the geom entries)
  TGeomEntryGHS3DEnforcedVertexMap _geomEntryEnfVertexMap;
  
  
  TGHS3DEnforcedMeshList                   _enfMeshList;
  // map to get enf meshes through the entries
  TEntryGHS3DEnforcedMeshListMap           _entryEnfMeshMap;
  TIDSortedNodeGroupMap                    _enfNodes;
  TIDSortedElemGroupMap                    _enfEdges;
  TIDSortedElemGroupMap                    _enfTriangles;
  TID2SizeMap                              _nodeIDToSizeMap;
  
  TSetStrings _groupsToRemove;
};


#endif
