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
// File      : GHS3DPlugin_GHS3D.hxx
// Project   : SALOME
// Copyright : CEA 2003
// $Header$
//=============================================================================

#ifndef _GHS3DPlugin_GHS3D_HXX_
#define _GHS3DPlugin_GHS3D_HXX_

#include "SMESH_3D_Algo.hxx"

class SMESH_Mesh;

class GHS3DPlugin_GHS3D: public SMESH_3D_Algo
{
public:
  GHS3DPlugin_GHS3D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~GHS3DPlugin_GHS3D();

  virtual bool CheckHypothesis(SMESH_Mesh&                          aMesh,
                               const TopoDS_Shape&                  aShape,
                               SMESH_Hypothesis::Hypothesis_Status& aStatus);

  virtual bool Compute(SMESH_Mesh&         aMesh,
		       const TopoDS_Shape& aShape);

private:
  int _iShape;
  int _nbShape;
};

#endif
