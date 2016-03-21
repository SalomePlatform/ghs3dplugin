# Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#

# An enforced vertex can be added via:
# - the coordinates x,y,z
# - a GEOM vertex or compound (No geometry, TUI only)
#
# The created enforced nodes can also be stored in
# a group.
#
# This feature is available only on meshes without geometry.

# Ex1: Add one enforced vertex with coordinates (50,50,100) 
#      and physical size 2.

import salome
salome.salome_init()

from salome.geom import geomBuilder
geompy = geomBuilder.New(salome.myStudy)

import SMESH
from salome.smesh import smeshBuilder
smesh =  smeshBuilder.New(salome.myStudy)

# create a box
box = geompy.MakeBoxDXDYDZ(200., 200., 200.)
geompy.addToStudy(box, "box")
# create a mesh on the box
mgtetraMesh = smesh.Mesh(box,"box: MG-Tetra and MG-CADSurf mesh")
# create a MG-CADSurf algorithm for faces
mgtetraMesh.Triangle(algo=smeshBuilder.MG_CADSurf)
# compute the mesh
mgtetraMesh.Compute()

# Make a copy of the 2D mesh
mgtetraMesh_wo_geometry = smesh.CopyMesh( mgtetraMesh, 'MG-Tetra w/o geometry', 0, 0)

# create a MG_Tetra algorithm and hypothesis and assign them to the mesh
MG_Tetra = mgtetraMesh.Tetrahedron( smeshBuilder.MG_Tetra )
MG_Tetra_Parameters = MG_Tetra.Parameters()
# Create the enforced vertex
MG_Tetra_Parameters.SetEnforcedVertex( 50, 50, 100, 2) # no group
# Compute the mesh
mgtetraMesh.Compute()


# Ex2: Add one vertex enforced by a GEOM vertex at (50,50,100) 
#      with physical size 5 and add it to a group called "My special nodes"

# Create another MG_Tetra hypothesis and assign it to the mesh without geometry
MG_Tetra_Parameters_wo_geometry = smesh.CreateHypothesis('MG-Tetra Parameters', 'GHS3DEngine')
mgtetraMesh_wo_geometry.AddHypothesis( MG_Tetra )
mgtetraMesh_wo_geometry.AddHypothesis( MG_Tetra_Parameters_wo_geometry )

# Create the enforced vertex
p1 = geompy.MakeVertex(150, 150, 100)
geompy.addToStudy(p1, "p1")
MG_Tetra_Parameters_wo_geometry.SetEnforcedVertexGeomWithGroup( p1, 5 , "My special nodes")
#MG_Tetra_Parameters.SetEnforcedVertexGeom( p1, 5 ) # no group

# compute the mesh
mgtetraMesh_wo_geometry.Compute()

# Erase all enforced vertices
MG_Tetra_Parameters.ClearEnforcedVertices()

# End of script
