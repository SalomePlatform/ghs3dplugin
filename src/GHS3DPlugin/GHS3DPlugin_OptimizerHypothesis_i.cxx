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
// File      : GHS3DPlugin_OptimizerHypothesis_i.cxx
// Created   : Tue Nov  1 17:46:39 2016

#include "GHS3DPlugin_OptimizerHypothesis_i.hxx"

#include <SMESH_Gen.hxx>
#include <SMESH_PythonDump.hxx>

#include <Utils_CorbaException.hxx>

GHS3DPlugin_OptimizerHypothesis_i::
GHS3DPlugin_OptimizerHypothesis_i (PortableServer::POA_ptr thePOA,
                                   int                     theStudyId,
                                   ::SMESH_Gen*            theGenImpl)
  : SALOME::GenericObj_i( thePOA ), 
    SMESH_Hypothesis_i( thePOA ),
    GHS3DPlugin_Hypothesis_i( thePOA, theStudyId, theGenImpl )
{
  int id = myBaseImpl ? myBaseImpl->GetID() : theGenImpl->GetANewId();
  if ( myBaseImpl )
    delete myBaseImpl;

  myBaseImpl = new ::GHS3DPlugin_OptimizerHypothesis( id, theStudyId, theGenImpl );
}

namespace
{
  const char* dumpMode( GHS3DPlugin::Mode mode )
  {
    switch( mode ) {
    case GHS3DPlugin::NO:   return "GHS3DPlugin.NO";
    case GHS3DPlugin::YES:  return "GHS3DPlugin.YES";
    case GHS3DPlugin::ONLY: return "GHS3DPlugin.ONLY";
    default:                return "GHS3DPlugin.NO";
    }
    return 0;
  }
}

void GHS3DPlugin_OptimizerHypothesis_i::SetOptimization( GHS3DPlugin::Mode mode )
{
  if ((int) GetImpl()->GetOptimization() != (int) mode )
  {
    GetImpl()->SetOptimization( ::GHS3DPlugin_OptimizerHypothesis::Mode( mode ));
    SMESH::TPythonDump() << _this() << ".SetOptimization( " << dumpMode( mode ) << " )";
  }
}

GHS3DPlugin::Mode GHS3DPlugin_OptimizerHypothesis_i::GetOptimization()
{
  return GHS3DPlugin::Mode( GetImpl()->GetOptimization() );
}

void GHS3DPlugin_OptimizerHypothesis_i::SetSplitOverConstrained( GHS3DPlugin::Mode mode )
{
  if ((int) GetImpl()->GetSplitOverConstrained() != (int) mode )
  {
    GetImpl()->SetSplitOverConstrained((::GHS3DPlugin_OptimizerHypothesis::Mode) mode );
    SMESH::TPythonDump() << _this() << ".SetSplitOverConstrained( " << dumpMode( mode ) << " )";
  }
}

GHS3DPlugin::Mode GHS3DPlugin_OptimizerHypothesis_i::GetSplitOverConstrained()
{
  return (GHS3DPlugin::Mode) GetImpl()->GetSplitOverConstrained();
}

void GHS3DPlugin_OptimizerHypothesis_i::SetSmoothOffSlivers( CORBA::Boolean toSmooth )
{
  if ( GetImpl()->GetSmoothOffSlivers() != (bool) toSmooth )
  {
    GetImpl()->SetSmoothOffSlivers((bool) toSmooth );
    SMESH::TPythonDump() << _this() << ".SetSmoothOffSlivers( " << toSmooth << " )";
  }
}

CORBA::Boolean GHS3DPlugin_OptimizerHypothesis_i::GetSmoothOffSlivers()
{
  return GetImpl()->GetSmoothOffSlivers();
}

void GHS3DPlugin_OptimizerHypothesis_i::SetPThreadsMode( GHS3DPlugin::PThreadsMode mode )
{
  if ((int) GetImpl()->GetPThreadsMode() != (int) mode )
  {
    GetImpl()->SetPThreadsMode((::GHS3DPlugin_OptimizerHypothesis::PThreadsMode) mode );

    std::string modeStr;
    switch( mode ) {
    case ::GHS3DPlugin_OptimizerHypothesis::SAFE:       modeStr = "SAFE"; break;
    case ::GHS3DPlugin_OptimizerHypothesis::AGGRESSIVE: modeStr = "AGGRESSIVE"; break;
    default:                                            modeStr = "NONE";
    }
    SMESH::TPythonDump() << _this() << ".SetPThreadsMode( GHS3DPlugin." << modeStr << " )";
  }
}

GHS3DPlugin::PThreadsMode GHS3DPlugin_OptimizerHypothesis_i::GetPThreadsMode()
{
  return (GHS3DPlugin::PThreadsMode) GetImpl()->GetPThreadsMode();
}

void GHS3DPlugin_OptimizerHypothesis_i::SetMaximalNumberOfThreads( CORBA::Short nb )
{
  if ( GetImpl()->GetMaximalNumberOfThreads() != nb )
  {
    GetImpl()->SetMaximalNumberOfThreads( nb );
    SMESH::TPythonDump() << _this() << ".SetMaximalNumberOfThreads( " << nb << " )";
  }
}

CORBA::Short GHS3DPlugin_OptimizerHypothesis_i::GetMaximalNumberOfThreads()
{
  return (CORBA::Short) GetImpl()->GetMaximalNumberOfThreads();
}

::GHS3DPlugin_OptimizerHypothesis* GHS3DPlugin_OptimizerHypothesis_i::GetImpl()
{
  return (::GHS3DPlugin_OptimizerHypothesis*)myBaseImpl;
}

CORBA::Boolean GHS3DPlugin_OptimizerHypothesis_i::IsDimSupported( SMESH::Dimension type )
{
  return type == SMESH::DIM_3D;
}


