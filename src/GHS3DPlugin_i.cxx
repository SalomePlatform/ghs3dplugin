//  SMESH GHS3DPlugin : implementaion of SMESH idl descriptions
//
//  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
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
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//
//
//  File   : GHS3DPlugin.cxx
//  Author : Julia DOROVSKIKH
//  Module : SMESH
//  $Header$

using namespace std;
#include "SMESH_Hypothesis_i.hxx"

#include "utilities.h"

#include "GHS3DPlugin_GHS3D_i.hxx"

template <class T> class GHS3DPlugin_Creator_i:public HypothesisCreator_i<T>
{
  // as we have 'module GHS3DPlugin' in GHS3DPlugin_Algorithm.idl
  virtual std::string GetModuleName() { return "GHS3DPlugin"; }
};

//=============================================================================
/*!
 *
 */
//=============================================================================

extern "C"
{
  GenericHypothesisCreator_i* GetHypothesisCreator (const char* aHypName)
  {
    MESSAGE("GetHypothesisCreator " << aHypName);

    GenericHypothesisCreator_i* aCreator = 0;

    // Hypotheses

    // Algorithms
    if (strcmp(aHypName, "GHS3D_3D") == 0)
      aCreator = new GHS3DPlugin_Creator_i<GHS3DPlugin_GHS3D_i>;
    else ;

    return aCreator;
  }
}
