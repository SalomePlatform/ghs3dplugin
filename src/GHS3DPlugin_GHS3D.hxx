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

  ostream & SaveTo(ostream & save);
  istream & LoadFrom(istream & load);
  friend ostream & operator << (ostream & save, GHS3DPlugin_GHS3D & hyp);
  friend istream & operator >> (istream & load, GHS3DPlugin_GHS3D & hyp);

};

#endif
