//  Copyright (C) 2004-2008  CEA/DEN, EDF R&D
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
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//=============================================================================
// File      : GHS3DPlugin_GHS3D.hxx
// Author    : Edward AGAPOV, modified by Lioka RAZAFINDRAZAKA (CEA) 09/02/2007
// Project   : SALOME
// $Header$
//=============================================================================
//
#ifndef _GHS3DPlugin_GHS3D_HXX_
#define _GHS3DPlugin_GHS3D_HXX_

#include "SMESH_3D_Algo.hxx"

#include <map>
#include <vector>

class SMESH_Mesh;
class GHS3DPlugin_Hypothesis;
class SMDS_MeshNode;
class TCollection_AsciiString;
class _Ghs2smdsConvertor;

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

  virtual bool Evaluate(SMESH_Mesh& aMesh, const TopoDS_Shape& aShape,
                        MapShapeNbElems& aResMap);

  virtual bool Compute(SMESH_Mesh&         theMesh,
                       SMESH_MesherHelper* aHelper);

private:

  bool storeErrorDescription(const TCollection_AsciiString& logFile,
                             const _Ghs2smdsConvertor &     toSmdsConvertor );

  int  _iShape;
  int  _nbShape;
  bool _keepFiles;
  const GHS3DPlugin_Hypothesis* _hyp;
};

/*!
 * \brief Convertor of GHS3D elements to SMDS ones
 */
class _Ghs2smdsConvertor
{
  const std::map <int,const SMDS_MeshNode*> * _ghs2NodeMap;
  const std::vector <const SMDS_MeshNode*> *  _nodeByGhsId;

public:
  _Ghs2smdsConvertor( const std::map <int,const SMDS_MeshNode*> & ghs2NodeMap);

  _Ghs2smdsConvertor( const std::vector <const SMDS_MeshNode*> &  nodeByGhsId);

  const SMDS_MeshElement* getElement(const std::vector<int>& ghsNodes) const;
};

#endif
