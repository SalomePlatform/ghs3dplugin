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
// File      : GHS3DPlugin_OptimizerHypothesis.cxx
// Created   : Tue Nov  1 17:18:38 2016

#include "GHS3DPlugin_OptimizerHypothesis.hxx"

#include <SMESH_Gen.hxx>

GHS3DPlugin_OptimizerHypothesis::GHS3DPlugin_OptimizerHypothesis(int         hypId,
                                                                 int         studyId,
                                                                 SMESH_Gen * gen)
  :GHS3DPlugin_Hypothesis( hypId, studyId, gen ),
   myOptimization( YES ),
   mySplitOverConstrained( NO ),
   mySmoothOffSlivers( false ),
   myMaximalNumberOfThreads( 4 ),
   myPThreadsMode( NONE )
{
  _name = GetHypType();
  _param_algo_dim = 3;
}

void GHS3DPlugin_OptimizerHypothesis::SetOptimization( Mode mode )
{
  if ( myOptimization != mode )
  {
    myOptimization = mode;
    NotifySubMeshesHypothesisModification();
  }
}

GHS3DPlugin_OptimizerHypothesis::Mode GHS3DPlugin_OptimizerHypothesis::GetOptimization() const
{
  return myOptimization;
}

void GHS3DPlugin_OptimizerHypothesis::SetSplitOverConstrained( Mode mode )
{
  if ( mode != mySplitOverConstrained )
  {
    mySplitOverConstrained = mode;
    NotifySubMeshesHypothesisModification();
  }
}

GHS3DPlugin_OptimizerHypothesis::Mode GHS3DPlugin_OptimizerHypothesis::GetSplitOverConstrained() const
{
  return mySplitOverConstrained;
}

void GHS3DPlugin_OptimizerHypothesis::SetSmoothOffSlivers( bool toSmooth )
{
  if ( mySmoothOffSlivers != toSmooth )
  {
    mySmoothOffSlivers = toSmooth;
    NotifySubMeshesHypothesisModification();
  }
}

bool GHS3DPlugin_OptimizerHypothesis::GetSmoothOffSlivers() const
{
  return mySmoothOffSlivers;
}

void GHS3DPlugin_OptimizerHypothesis::SetPThreadsMode( PThreadsMode mode )
{
  if ( myPThreadsMode != mode )
  {
    myPThreadsMode = mode;
    NotifySubMeshesHypothesisModification();
  }
}

GHS3DPlugin_OptimizerHypothesis::PThreadsMode GHS3DPlugin_OptimizerHypothesis::GetPThreadsMode() const
{
  return myPThreadsMode;
}

void GHS3DPlugin_OptimizerHypothesis::SetMaximalNumberOfThreads( int nb )
{
  if ( myMaximalNumberOfThreads != nb )
  {
    myMaximalNumberOfThreads = nb;
    NotifySubMeshesHypothesisModification();
  }
}

int GHS3DPlugin_OptimizerHypothesis::GetMaximalNumberOfThreads() const
{
  return myMaximalNumberOfThreads;
}


std::ostream & GHS3DPlugin_OptimizerHypothesis::SaveTo(std::ostream & save)
{
  save << " " << (int)myOptimization
       << " " << (int)mySplitOverConstrained
       << " " << (int)mySmoothOffSlivers
       << " " << (int)myMaximalNumberOfThreads
       << " " << (int)myPThreadsMode << " ";

  GHS3DPlugin_Hypothesis::SaveTo( save );

  return save;
}

std::istream & GHS3DPlugin_OptimizerHypothesis::LoadFrom(std::istream & load)
{
  int i;

  if ( load >> i )
    myOptimization = (Mode) i;
  if ( load >> i )
    mySplitOverConstrained = (Mode) i;
  if ( load >> i )
    mySmoothOffSlivers = (bool) i;
  if ( load >> i )
    myMaximalNumberOfThreads = i;
  if ( load >> i )
    myPThreadsMode = (PThreadsMode) i;

  GHS3DPlugin_Hypothesis::LoadFrom( load );
  return load;
}

bool GHS3DPlugin_OptimizerHypothesis::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                          const TopoDS_Shape& theShape)
{
  return false;
}

bool GHS3DPlugin_OptimizerHypothesis::SetParametersByDefaults(const TDefaults&  theDflts,
                                                              const SMESH_Mesh* theMesh)
{
  return false;
}

std::string GHS3DPlugin_OptimizerHypothesis::CommandToRun(const GHS3DPlugin_OptimizerHypothesis* hyp)
{
  SMESH_Comment cmd( GetExeName() );

  if ( hyp )
  {
    const char* mode[3] = { "no", "yes", "only" };

    if ( hyp->GetOptimization() >= 0 && hyp->GetOptimization() < 3 )
      cmd << " --optimisation " << mode[ hyp->GetOptimization() ];

    if ( hyp->myOptimizationLevel >= 0 && hyp->myOptimizationLevel < 5 ) {
      const char* level[] = { "none" , "light" , "standard" , "standard+" , "strong" };
      cmd << " --optimisation_level " << level[ hyp->myOptimizationLevel ];
    }

    if ( hyp->GetSplitOverConstrained() >= 0 && hyp->GetSplitOverConstrained() < 3 )
      cmd << " --split_overconstrained_elements " << mode[ hyp->GetSplitOverConstrained() ];

    if ( hyp->GetSmoothOffSlivers() )
      cmd << " --smooth_off_slivers yes";

    switch ( hyp->GetPThreadsMode() ) {
    case GHS3DPlugin_OptimizerHypothesis::SAFE:
      cmd << " --pthreads_mode safe"; break;
    case GHS3DPlugin_OptimizerHypothesis::AGGRESSIVE:
      cmd << " --pthreads_mode aggressive"; break;
    default:;
    }

    if ( hyp->GetMaximalNumberOfThreads() > 0 )
      cmd << " --max_number_of_threads " << hyp->GetMaximalNumberOfThreads();

    if ( !hyp->myToCreateNewNodes )
      cmd << " --no_internal_points";

    if ( hyp->myMaximumMemory > 0 )
      cmd << " --max_memory " << hyp->myMaximumMemory;

    if ( hyp->myInitialMemory > 0 )
      cmd << " --automatic_memory " << hyp->myInitialMemory;

    cmd << " --verbose " << hyp->myVerboseLevel;
    
    cmd << " " << hyp->myTextOption;
  }

  return cmd;
}
