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
// File      : GHS3DPlugin_OptimizerHypothesis.hxx
// Created   : Tue Nov  1 17:03:17 2016

#ifndef __GHS3DPlugin_OptimizerHypothesis_HXX__
#define __GHS3DPlugin_OptimizerHypothesis_HXX__

#include "GHS3DPlugin_Hypothesis.hxx"


class GHS3DPlugin_OptimizerHypothesis : public GHS3DPlugin_Hypothesis
{
public:
  GHS3DPlugin_OptimizerHypothesis(int hypId, int studyId, SMESH_Gen * gen);

  // inherited params:
  // 1 - create new nodes
  // 2 - optimization level
  // 3 - init and max memory
  // 4 - work dir
  // 5 - verbosity
  // 6 - log to file
  // 7 - keep work files
  // 8 - remove log file
  // 9 - advanced options

  enum PThreadsMode { SAFE, AGGRESSIVE, NONE };
  enum Mode { NO, YES, ONLY };

  void SetOptimization( Mode isOnly );
  Mode GetOptimization() const;

  void SetSplitOverConstrained( Mode mode );
  Mode GetSplitOverConstrained() const;

  void SetSmoothOffSlivers( bool toSmooth );
  bool GetSmoothOffSlivers() const;

  void SetPThreadsMode( PThreadsMode mode );
  PThreadsMode GetPThreadsMode() const;

  void SetMaximalNumberOfThreads( int nb );
  int  GetMaximalNumberOfThreads() const;

  static const char* GetHypType() { return "MG-Tetra Optimization Parameters"; }

  // Persistence
  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);

  /*!
   * \brief Does nothing
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);

  static std::string CommandToRun(const GHS3DPlugin_OptimizerHypothesis* hyp);

private:

  Mode myOptimization; // no, yes, only
  Mode mySplitOverConstrained; // no, yes, only
  bool mySmoothOffSlivers;
  int  myMaximalNumberOfThreads;
  PThreadsMode myPThreadsMode; // safe, aggressive, none

};

#endif
