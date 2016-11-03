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

//=============================================================================
// File      : GHS3DPlugin_GHS3D.hxx
// Author    : Edward AGAPOV, modified by Lioka RAZAFINDRAZAKA (CEA) 09/02/2007
// Project   : SALOME
//=============================================================================
//
#ifndef _GHS3DPlugin_GHS3D_HXX_
#define _GHS3DPlugin_GHS3D_HXX_

#include <SMESH_Algo.hxx>
#include <SMESH_Gen.hxx>
#include <SMESH_Gen_i.hxx>
#include <SMESH_ProxyMesh.hxx>

#include <map>
#include <vector>

#ifndef GMFVERSION
#define GMFVERSION GmfDouble
#endif
#define GMFDIMENSION 3

class GHS3DPlugin_Hypothesis;
class SMDS_MeshNode;
class SMESH_Mesh;
class StdMeshers_ViscousLayers;
class TCollection_AsciiString;
class _Ghs2smdsConvertor;
class TopoDS_Shape;

class GHS3DPlugin_GHS3D: public SMESH_3D_Algo
{
public:

  GHS3DPlugin_GHS3D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~GHS3DPlugin_GHS3D();

  virtual bool CheckHypothesis(SMESH_Mesh&         aMesh,
                               const TopoDS_Shape& aShape,
                               Hypothesis_Status&  aStatus);

  virtual bool Compute(SMESH_Mesh&         aMesh,
                       const TopoDS_Shape& aShape);

  virtual void CancelCompute();
  bool         computeCanceled() { return _computeCanceled; }

  virtual bool Evaluate(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape,
                        MapShapeNbElems& aResMap);

  virtual bool Compute(SMESH_Mesh&         theMesh,
                       SMESH_MesherHelper* aHelper);

  virtual void SubmeshRestored(SMESH_subMesh* subMesh);

  virtual void SetEventListener(SMESH_subMesh* subMesh);

  bool         importGMFMesh(const char* aGMFFileName, SMESH_Mesh& aMesh);

  virtual double GetProgress() const;


  static const char* Name() { return "MG-Tetra"; }

  static SMESH_ComputeErrorPtr getErrorDescription(const char*                logFile,
                                                   const std::string&         log,
                                                   const _Ghs2smdsConvertor & toSmdsConvertor,
                                                   const bool                 isOK = false);

protected:
  const GHS3DPlugin_Hypothesis*   _hyp;
  const StdMeshers_ViscousLayers* _viscousLayersHyp;
  std::string                     _genericName;

private:

  TopoDS_Shape entryToShape(std::string entry);

  int                 _iShape;
  int                 _nbShape;
  bool                _keepFiles;
  bool                _removeLogOnSuccess;
  bool                _logInStandardOutput;
  SALOMEDS::Study_var _study;
  SMESH_Gen_i*        _smeshGen_i;

  bool                _isLibUsed;
  double              _progressAdvance;
};

/*!
 * \brief Convertor of MG-Tetra elements to SMDS ones
 */
class _Ghs2smdsConvertor
{
  const std::map <int,const SMDS_MeshNode*> * _ghs2NodeMap;
  const std::vector <const SMDS_MeshNode*> *  _nodeByGhsId;
  SMESH_ProxyMesh::Ptr                        _mesh;

public:
  _Ghs2smdsConvertor( const std::map <int,const SMDS_MeshNode*> & ghs2NodeMap,
                      SMESH_ProxyMesh::Ptr                        mesh);

  _Ghs2smdsConvertor( const std::vector <const SMDS_MeshNode*> &  nodeByGhsId,
                      SMESH_ProxyMesh::Ptr                        mesh);

  const SMDS_MeshElement* getElement(const std::vector<int>& ghsNodes) const;
};

#endif
