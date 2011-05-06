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
// File      : GHS3DPlugin_Hypothesis.hxx
// Created   : Wed Apr  2 12:21:17 2008
// Author    : Edward AGAPOV (eap)
//
#ifndef GHS3DPlugin_Hypothesis_HeaderFile
#define GHS3DPlugin_Hypothesis_HeaderFile

#include "GHS3DPlugin_Defs.hxx"

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

  GHS3DPlugin_Hypothesis(int hypId, int studyId, SMESH_Gen * gen);

  /*!
   * To mesh "holes" in a solid or not. Default is to mesh.
   */
  void SetToMeshHoles(bool toMesh);
  bool GetToMeshHoles(bool checkFreeOption = false) const;
  /*!
   * Maximal size of memory to be used by the algorithm (in Megabytes)
   */
  void SetMaximumMemory(short MB);
  short GetMaximumMemory() const;
  /*!
   * Initial size of memory to be used by the algorithm (in Megabytes) in
   * automatic memory adjustment mode. Default is zero
   */
  void SetInitialMemory(short MB);
  short GetInitialMemory() const;
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
  void SetTextOption(const std::string& option);
  std::string GetTextOption() const;
  /*!
   * To set an enforced vertex
   */
//   struct TEnforcedNode {
//     std::vector<double> coords;
//     double size;
//     std::string geomEntry;
//     std::string groupName;
//   };
//   
//   struct CompareEnfNodes {
//     bool operator () (const TEnforcedNode* e1, const TEnforcedNode* e2) const {
//       if (e1 && e2) {
//         if (e1->coords.size() && e2->coords.size())
//           return (e1->coords < e2->coords);
//         else
//           return (e1->geomEntry < e2->geomEntry);
//       }
//       return false;
//     }
//   };
//   typedef std::set< TEnforcedNode*, CompareEnfNodes > TEnforcedNodeList;
//   // Map Coords / Enforced node
//   typedef std::map< std::vector<double>, TEnforcedNode* > TCoordsEnfNodeMap;
//   // Map geom entry / Enforced ndoe
//   typedef std::map< std::string, TEnforcedNode* > TGeomEntryEnfNodeMap;
//   
//   
//   struct TEnforcedEdge {
//     long ID;
//     long node1;
//     long node2;
//     std::string groupName;
//   };
  
  
  typedef std::map<std::vector<double>,double> TEnforcedVertexValues;
  void SetEnforcedVertex(double x, double y, double z, double size);
  double GetEnforcedVertex(double x, double y, double z) throw (std::invalid_argument);
  void RemoveEnforcedVertex(double x, double y, double z) throw (std::invalid_argument);
  const TEnforcedVertexValues _GetEnforcedVertices() const { return myEnforcedVertices; }
  void ClearEnforcedVertices();

  /*!
   * To set enforced elements
   */
  void SetEnforcedMesh(SMESH_Mesh& theMesh, SMESH::ElementType elementType, double size);
  void SetEnforcedElements(TIDSortedElemSet theElemSet, SMESH::ElementType elementType, double size);
  void ClearEnforcedMeshes();
  const TIDSortedNodeSet _GetEnforcedNodes() const { return _enfNodes; }
  const TIDSortedElemSet _GetEnforcedEdges() const { return _enfEdges; }
  const TIDSortedElemSet _GetEnforcedTriangles() const { return _enfTriangles; }
//   const TIDSortedElemSet _GetEnforcedQuadrangles() const { return _enfQuadrangles; }
//   typedef std::map<int,std::vector<int> > TElemID2NodeIDMap;
//   const TElemID2NodeIDMap _GetEdgeID2NodeIDMap() const {return _edgeID2nodeIDMap; }
//   const TElemID2NodeIDMap _GetTri2NodeMap() const {return _triID2nodeIDMap; }
//   const TElemID2NodeIDMap _GetQuad2NodeMap() const {return _quadID2nodeIDMap; }
  typedef std::map<int,double> TID2SizeMap;
  const TID2SizeMap _GetNodeIDToSizeMap() const {return _nodeIDToSizeMap; }
  const TID2SizeMap _GetElementIDToSizeMap() const {return _elementIDToSizeMap; }

  static bool   DefaultMeshHoles();
  static short  DefaultMaximumMemory();
  static short  DefaultInitialMemory();
  static short  DefaultOptimizationLevel();
  static std::string DefaultWorkingDirectory();
  static bool   DefaultKeepFiles();
  static short  DefaultVerboseLevel();
  static bool   DefaultToCreateNewNodes();
  static bool   DefaultToUseBoundaryRecoveryVersion();
  static bool   DefaultToUseFEMCorrection();
  static bool   DefaultToRemoveCentralPoint();
  static TEnforcedVertexValues DefaultEnforcedVertices();
  static TIDSortedNodeSet DefaultIDSortedNodeSet();
  static TIDSortedElemSet DefaultIDSortedElemSet();
  static TID2SizeMap DefaultID2SizeMap();

  /*!
   * \brief Return command to run ghs3d mesher excluding file prefix (-f)
   */
  static std::string CommandToRun(const GHS3DPlugin_Hypothesis* hyp,
                                  const bool                    hasShapeToMesh=true);
  /*!
   * \brief Return a unique file name
   */
  static std::string GetFileName(const GHS3DPlugin_Hypothesis* hyp);
  /*!
   * \brief Return the enforced vertices
   */
  static TEnforcedVertexValues GetEnforcedVertices(const GHS3DPlugin_Hypothesis* hyp);
  static TIDSortedNodeSet GetEnforcedNodes(const GHS3DPlugin_Hypothesis* hyp);
  static TIDSortedElemSet GetEnforcedEdges(const GHS3DPlugin_Hypothesis* hyp);
  static TIDSortedElemSet GetEnforcedTriangles(const GHS3DPlugin_Hypothesis* hyp);
//   static TIDSortedElemSet GetEnforcedQuadrangles(const GHS3DPlugin_Hypothesis* hyp);
//   static TElemID2NodeIDMap GetEdgeID2NodeIDMap(const GHS3DPlugin_Hypothesis* hyp);
//   static TElemID2NodeIDMap GetTri2NodeMap(const GHS3DPlugin_Hypothesis* hyp);
//   static TElemID2NodeIDMap GetQuad2NodeMap(const GHS3DPlugin_Hypothesis* hyp);
  static TID2SizeMap GetNodeIDToSizeMap(const GHS3DPlugin_Hypothesis* hyp);
  static TID2SizeMap GetElementIDToSizeMap(const GHS3DPlugin_Hypothesis* hyp);
  
  // Persistence
  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);
  friend GHS3DPLUGIN_EXPORT std::ostream & operator <<(std::ostream & save, GHS3DPlugin_Hypothesis & hyp);
  friend GHS3DPLUGIN_EXPORT std::istream & operator >>(std::istream & load, GHS3DPlugin_Hypothesis & hyp);

  /*!
   * \brief Does nothing
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);

  /*!
   * \brief Does nothing
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);

private:

  bool   myToMeshHoles;
  short  myMaximumMemory;
  short  myInitialMemory;
  short  myOptimizationLevel;
  bool   myKeepFiles;
  std::string myWorkingDirectory;
  short  myVerboseLevel;
  bool   myToCreateNewNodes;
  bool   myToUseBoundaryRecoveryVersion;
  bool   myToUseFemCorrection;
  bool   myToRemoveCentralPoint;
  std::string myTextOption;
  TEnforcedVertexValues myEnforcedVertices;
  TIDSortedNodeSet _enfNodes;
  TIDSortedElemSet _enfEdges;
  TIDSortedElemSet _enfTriangles;
//   TIDSortedElemSet _enfQuadrangles;
//   TElemID2NodeIDMap _edgeID2nodeIDMap;
//   TElemID2NodeIDMap _triID2nodeIDMap;
//   TElemID2NodeIDMap _quadID2nodeIDMap;
  TID2SizeMap _nodeIDToSizeMap;
  TID2SizeMap _elementIDToSizeMap;
};


#endif
