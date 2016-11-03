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
// File      : GHS3DPlugin_OptimizerHypothesis_i.hxx
// Created   : Tue Nov  1 17:36:05 2016

#ifndef __GHS3DPlugin_OptimizerHypothesis_i_HXX__
#define __GHS3DPlugin_OptimizerHypothesis_i_HXX__

#include "GHS3DPlugin_Defs.hxx"

#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(GHS3DPlugin_Algorithm)

#include "GHS3DPlugin_OptimizerHypothesis.hxx"
#include "GHS3DPlugin_Hypothesis_i.hxx"
#include "SMESH_Hypothesis_i.hxx"

class SMESH_Gen;

// GHS3DPlugin Optimization Parameters hypothesis

class GHS3DPLUGIN_EXPORT GHS3DPlugin_OptimizerHypothesis_i:
  public virtual POA_GHS3DPlugin::GHS3DPlugin_OptimizerHypothesis,
  public virtual GHS3DPlugin_Hypothesis_i
{
 public:
  GHS3DPlugin_OptimizerHypothesis_i (PortableServer::POA_ptr thePOA,
                                     int                     theStudyId,
                                     ::SMESH_Gen*            theGenImpl);
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

  void SetOptimization( GHS3DPlugin::Mode mode );
  GHS3DPlugin::Mode GetOptimization();

  void SetSplitOverConstrained( GHS3DPlugin::Mode mode );
  GHS3DPlugin::Mode GetSplitOverConstrained();

  void SetSmoothOffSlivers( CORBA::Boolean toSmooth );
  CORBA::Boolean GetSmoothOffSlivers();

  void SetPThreadsMode( GHS3DPlugin::PThreadsMode mode );
  GHS3DPlugin::PThreadsMode GetPThreadsMode();

  void SetMaximalNumberOfThreads( CORBA::Short nb );
  CORBA::Short GetMaximalNumberOfThreads();


  // Get implementation
  ::GHS3DPlugin_OptimizerHypothesis* GetImpl();
  
  // Verify whether hypothesis supports given entity type 
  CORBA::Boolean IsDimSupported( SMESH::Dimension type );
  
};

#endif
