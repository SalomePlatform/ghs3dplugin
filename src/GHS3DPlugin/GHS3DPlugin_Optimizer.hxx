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
// File      : GHS3DPlugin_Optimizer.hxx
// Created   : Mon Oct 31 19:58:02 2016


#ifndef __GHS3DPlugin_Optimizer_HXX__
#define __GHS3DPlugin_Optimizer_HXX__

#include <SMESH_Algo.hxx>

class GHS3DPlugin_OptimizerHypothesis;

class GHS3DPlugin_Optimizer: public SMESH_3D_Algo
{
public:
  GHS3DPlugin_Optimizer(int hypId, int studyId, SMESH_Gen* gen);

  virtual bool CheckHypothesis(SMESH_Mesh&         aMesh,
                               const TopoDS_Shape& aShape,
                               Hypothesis_Status&  aStatus);

  virtual void CancelCompute();
  bool         computeCanceled() { return _computeCanceled; }

  virtual bool Evaluate(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape,
                        MapShapeNbElems& aResMap);

  virtual bool Compute(SMESH_Mesh&         theMesh,
                       SMESH_MesherHelper* theHelper);
  virtual bool Compute(SMESH_Mesh &         aMesh,
                       const TopoDS_Shape & aShape);

  static const char* Name() { return "MG-Tetra Optimization"; }

  //virtual double GetProgress() const;

private:

  const GHS3DPlugin_OptimizerHypothesis* _hyp;
};


#endif
