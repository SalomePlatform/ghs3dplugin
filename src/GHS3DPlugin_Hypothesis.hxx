//  GHS3DPlugin : C++ implementation
//
//  Copyright (C) 2006  OPEN CASCADE, CEA/DEN, EDF R&D
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

// File      : GHS3DPlugin_Hypothesis.hxx
// Created   : Wed Apr  2 12:21:17 2008
// Author    : Edward AGAPOV (eap)

#ifndef GHS3DPlugin_Hypothesis_HeaderFile
#define GHS3DPlugin_Hypothesis_HeaderFile

#include "GHS3DPlugin_Defs.hxx"

#include <SMESH_Hypothesis.hxx>

using namespace std;

class GHS3DPLUGIN_EXPORT GHS3DPlugin_Hypothesis: public SMESH_Hypothesis
{
public:

  GHS3DPlugin_Hypothesis(int hypId, int studyId, SMESH_Gen * gen);

  /*!
   * To mesh "holes" in a solid or not. Default is to mesh.
   */
  void SetToMeshHoles(bool toMesh);
  bool GetToMeshHoles() const;
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
   * Optimization level: 0-none, 1-light, 2-medium, 3-strong. Default is medium
   */
  enum OptimizationLevel { None = 0, Light, Medium, Strong };
  void SetOptimizationLevel(OptimizationLevel level);
  OptimizationLevel GetOptimizationLevel() const;
  /*!
   * Path to working directory
   */
  void SetWorkingDirectory(string path);
  string GetWorkingDirectory() const;
  /*!
   * To keep working files or remove them. Log file remains in case of errors anyway.
   */
  void SetKeepFiles(bool toKeep);
  bool GetKeepFiles() const;

  static bool   DefaultMeshHoles();
  static short  DefaultMaximumMemory();
  static short  DefaultInitialMemory();
  static short  DefaultOptimizationLevel();
  static string DefaultWorkingDirectory();
  static bool   DefaultKeepFiles();

  /*!
   * \brief Return command to run ghs3d mesher excluding file prefix (-f)
   */
  static std::string CommandToRun(const GHS3DPlugin_Hypothesis* hyp);
  /*!
   * \brief Return a unique file name
   */
  static std::string GetFileName(const GHS3DPlugin_Hypothesis* hyp);

  // Persistence
  virtual ostream & SaveTo(ostream & save);
  virtual istream & LoadFrom(istream & load);
  friend GHS3DPLUGIN_EXPORT ostream & operator <<(ostream & save, GHS3DPlugin_Hypothesis & hyp);
  friend GHS3DPLUGIN_EXPORT istream & operator >>(istream & load, GHS3DPlugin_Hypothesis & hyp);

  /*!
   * \brief Does nothing
   * \param theMesh - the built mesh
   * \param theShape - the geometry of interest
   * \retval bool - always false
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);

private:

  bool        myToMeshHoles;
  short       myMaximumMemory;
  short       myInitialMemory;
  short       myOptimizationLevel;
  std::string myWorkingDirectory;
  bool        myKeepFiles;
};


#endif