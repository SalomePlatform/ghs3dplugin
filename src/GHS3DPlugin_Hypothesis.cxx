// Copyright (C) 2005  CEA/DEN, EDF R&D, OPEN CASCADE, PRINCIPIA R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License.
//
// This library is distributed in the hope that it will be useful
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
//=============================================================================
// File      : GHS3DPlugin_Hypothesis.cxx
// Created   : Wed Apr  2 12:36:29 2008
// Author    : Edward AGAPOV (eap)
//=============================================================================


#include "GHS3DPlugin_Hypothesis.hxx"

#include <TCollection_AsciiString.hxx>

//=======================================================================
//function : GHS3DPlugin_Hypothesis
//=======================================================================

GHS3DPlugin_Hypothesis::GHS3DPlugin_Hypothesis(int hypId, int studyId, SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen)
{
  _name = "GHS3D_Parameters";
  _param_algo_dim = 3;

  myToMeshHoles       = DefaultMeshHoles();        
  myMaximumMemory     = -1;//DefaultMaximumMemory();    
  myInitialMemory     = -1;//DefaultInitialMemory();    
  myOptimizationLevel = DefaultOptimizationLevel();
  myWorkingDirectory  = DefaultWorkingDirectory(); 
  myKeepFiles         = DefaultKeepFiles();        
}

//=======================================================================
//function : SetToMeshHoles
//=======================================================================

void GHS3DPlugin_Hypothesis::SetToMeshHoles(bool toMesh)
{
  if ( myToMeshHoles != toMesh ) {
    myToMeshHoles = toMesh;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetToMeshHoles
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetToMeshHoles() const
{
  return myToMeshHoles;
}

//=======================================================================
//function : SetMaximumMemory
//=======================================================================

void GHS3DPlugin_Hypothesis::SetMaximumMemory(short MB)
{
  if ( myMaximumMemory != MB ) {
    myMaximumMemory = MB;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetMaximumMemory
//           * automatic memory adjustment mode. Default is zero
//=======================================================================

short GHS3DPlugin_Hypothesis::GetMaximumMemory() const
{
  return myMaximumMemory;
}

//=======================================================================
//function : SetInitialMemory
//=======================================================================

void GHS3DPlugin_Hypothesis::SetInitialMemory(short MB)
{
  if ( myInitialMemory != MB ) {
    myInitialMemory = MB;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetInitialMemory
//=======================================================================

short GHS3DPlugin_Hypothesis::GetInitialMemory() const
{
  return myInitialMemory;
}

//=======================================================================
//function : SetOptimizationLevel
//=======================================================================

void GHS3DPlugin_Hypothesis::SetOptimizationLevel(OptimizationLevel level)
{
  if ( myOptimizationLevel != level ) {
    myOptimizationLevel = level;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetOptimizationLevel
//=======================================================================

GHS3DPlugin_Hypothesis::OptimizationLevel GHS3DPlugin_Hypothesis::GetOptimizationLevel() const
{
  return (OptimizationLevel) myOptimizationLevel;
}

//=======================================================================
//function : SetWorkingDirectory
//=======================================================================

void GHS3DPlugin_Hypothesis::SetWorkingDirectory(string path)
{
  if ( myWorkingDirectory != path ) {
    myWorkingDirectory = path;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetWorkingDirectory
//=======================================================================

string GHS3DPlugin_Hypothesis::GetWorkingDirectory() const
{
  return myWorkingDirectory;
}

//=======================================================================
//function : SetKeepFiles
//=======================================================================

void GHS3DPlugin_Hypothesis::SetKeepFiles(bool toKeep)
{
  if ( myKeepFiles != toKeep ) {
    myKeepFiles = toKeep;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetKeepFiles
//=======================================================================

bool GHS3DPlugin_Hypothesis::GetKeepFiles() const
{
  return myKeepFiles;
}

//=======================================================================
//function : DefaultMeshHoles
//=======================================================================

bool   GHS3DPlugin_Hypothesis::DefaultMeshHoles()
{
  return true;
}

//=======================================================================
//function : DefaultMaximumMemory
//=======================================================================

#ifndef WIN32
#include <sys/sysinfo.h>
#endif

short  GHS3DPlugin_Hypothesis::DefaultMaximumMemory()
{
#ifndef WIN32
  struct sysinfo si;
  int err = sysinfo( &si );
  if ( err == 0 ) {
    int ramMB = si.totalram * si.mem_unit / 1024 / 1024;
    return (short) ( 0.7 * ramMB );
  }
#endif
  return -1;
}

//=======================================================================
//function : DefaultInitialMemory
//=======================================================================

short  GHS3DPlugin_Hypothesis::DefaultInitialMemory()
{
  return DefaultMaximumMemory();
}

//=======================================================================
//function : DefaultOptimizationLevel
//=======================================================================

short  GHS3DPlugin_Hypothesis::DefaultOptimizationLevel()
{
  return Medium;
}

//=======================================================================
//function : DefaultWorkingDirectory
//=======================================================================

string GHS3DPlugin_Hypothesis::DefaultWorkingDirectory()
{
  TCollection_AsciiString aTmpDir;

  char *Tmp_dir = getenv("SALOME_TMP_DIR");
  if(Tmp_dir != NULL) {
    aTmpDir = Tmp_dir;
  }
  else {
#ifdef WIN32
    aTmpDir = TCollection_AsciiString("C:\\");
#else
    aTmpDir = TCollection_AsciiString("/tmp/");
#endif
  }
  return aTmpDir.ToCString();
}

//=======================================================================
//function : DefaultKeepFiles
//=======================================================================

bool   GHS3DPlugin_Hypothesis::DefaultKeepFiles()
{
  return false;
}

//=======================================================================
//function : SaveTo
//=======================================================================

ostream & GHS3DPlugin_Hypothesis::SaveTo(ostream & save)
{
  save << (int) myToMeshHoles   << " ";
  save << myMaximumMemory     << " ";
  save << myInitialMemory     << " ";
  save << myOptimizationLevel << " ";
  save << myWorkingDirectory  << " ";
  save << (int)myKeepFiles    << " ";
  return save;
}

//=======================================================================
//function : LoadFrom
//=======================================================================

istream & GHS3DPlugin_Hypothesis::LoadFrom(istream & load)
{
  bool isOK = true;
  int i;

  isOK = (load >> i);
  if (isOK)
    myToMeshHoles = i;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = (load >> i);
  if (isOK)
    myMaximumMemory = i;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = (load >> i);
  if (isOK)
    myInitialMemory = i;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = (load >> i);
  if (isOK)
    myOptimizationLevel = i;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = (load >> myWorkingDirectory);
  if (isOK) {
    if ( myWorkingDirectory == "0") { // myWorkingDirectory was empty
      myKeepFiles = false;
      myWorkingDirectory.clear();
      return load;
    }
    else if ( myWorkingDirectory == "1" ) {
      myKeepFiles = true;
      myWorkingDirectory.clear();
      return load;
    }
  }
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = (load >> i);
  if (isOK)
    myKeepFiles = i;
  else
    load.clear(ios::badbit | load.rdstate());

  return load;
}

//=======================================================================
//function : SetParametersByMesh
//=======================================================================

bool GHS3DPlugin_Hypothesis::SetParametersByMesh(const SMESH_Mesh* ,const TopoDS_Shape&)
{
  return false;
}

//================================================================================
/*!
 * \brief Return command to run ghs3d mesher excluding file prefix (-f)
 */
//================================================================================

string GHS3DPlugin_Hypothesis::CommandToRun(const GHS3DPlugin_Hypothesis* hyp)
{
#ifndef WIN32
  TCollection_AsciiString cmd( "ghs3d" );
#else
  TCollection_AsciiString cmd( "ghs3d.exe" );
#endif

  // ghs3d needs to know amount of memory it may use (MB).
  // Default memory is defined at ghs3d installation but it may be not enough,
  // so allow to use about all available memory
  short aMaximumMemory = hyp ? hyp->myMaximumMemory : -1;
  cmd += " -m ";
  if ( aMaximumMemory < 0 )
    cmd += DefaultMaximumMemory();
  else
    cmd += aMaximumMemory;
  short aInitialMemory = hyp ? hyp->myInitialMemory : -1;
  cmd += " -M ";
  if ( aInitialMemory > 0 )
    cmd += aInitialMemory;
  else
    cmd += "100";

  // component to mesh
  // 0 , all components to be meshed
  // 1 , only the main ( outermost ) component to be meshed
  bool aToMeshHoles = hyp ? hyp->myToMeshHoles : DefaultMeshHoles();
  if ( aToMeshHoles )
    cmd += " -c 0";
  else
    cmd += " -c 1";

  // optimization level
  short aOptimizationLevel = hyp ? hyp->myOptimizationLevel : DefaultOptimizationLevel();
  if ( aOptimizationLevel >= 0 && aOptimizationLevel < 4 ) {
    char* level[] = { "none" , "light" , "standard" , "strong" };
    cmd += " -o ";
    cmd += level[ aOptimizationLevel ];
  }

  return cmd.ToCString();
}

//================================================================================
/*!
 * \brief Return a unique file name
 */
//================================================================================

string GHS3DPlugin_Hypothesis::GetFileName(const GHS3DPlugin_Hypothesis* hyp)
{
  string aTmpDir = hyp ? hyp->GetWorkingDirectory() : DefaultWorkingDirectory();
  const char lastChar = *aTmpDir.rbegin();
#ifdef WIN32
    if(lastChar != '\\') aTmpDir+='\\';
#else
    if(lastChar != '/') aTmpDir+='/';
#endif      

  TCollection_AsciiString aGenericName = (char*)aTmpDir.c_str();
  aGenericName += "GHS3D_";
#ifdef WIN32
  aGenericName += GetCurrentProcessId();
#else
  aGenericName += getpid();
#endif
  aGenericName += "_";
  aGenericName += Abs((Standard_Integer)(long) & aGenericName);

  return aGenericName.ToCString();
}
