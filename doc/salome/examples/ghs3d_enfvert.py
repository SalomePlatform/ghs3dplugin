# Copyright (C) 2007-2014  CEA/DEN, EDF R&D, OPEN CASCADE
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
import GEOM
from salome.geom import geomBuilder
geompy = geomBuilder.New(salome.myStudy)

import SMESH, SALOMEDS
from salome.smesh import smeshBuilder
smesh =  smeshBuilder.New(salome.myStudy)

# create a box
box = geompy.MakeBoxDXDYDZ(200., 200., 200.)
geompy.addToStudy(box, "box")
# create a mesh on the box
ghs3dMesh = smesh.Mesh(box,"box: Ghs3D and BLSurf mesh")
# create a BLSurf algorithm for faces
ghs3dMesh.Triangle(algo=smeshBuilder.BLSURF)
# compute the mesh
ghs3dMesh.Compute()

# Make a copy of the 2D mesh
ghs3dMesh_wo_geometry = smesh.CopyMesh( ghs3dMesh, 'Ghs3D wo geometry', 0, 0)

# create a Ghs3D algorithm and hypothesis and assign them to the mesh
GHS3D = smesh.CreateHypothesis('GHS3D_3D', 'GHS3DEngine')
GHS3D_Parameters = smesh.CreateHypothesis('GHS3D_Parameters', 'GHS3DEngine')
ghs3dMesh.AddHypothesis( GHS3D )
ghs3dMesh.AddHypothesis( GHS3D_Parameters )
# Create the enforced vertex
GHS3D_Parameters.SetEnforcedVertex( 50, 50, 100, 2) # no group
# Compute the mesh
ghs3dMesh.Compute()


# Ex2: Add one vertex enforced by a GEOM vertex at (50,50,100) 
#      with physical size 5 and add it to a group called "My special nodes"

# Create another GHS3D hypothesis and assign it to the mesh without geometry
GHS3D_Parameters_wo_geometry = smesh.CreateHypothesis('GHS3D_Parameters', 'GHS3DEngine')
ghs3dMesh_wo_geometry.AddHypothesis( GHS3D )
ghs3dMesh_wo_geometry.AddHypothesis( GHS3D_Parameters_wo_geometry )

# Create the enforced vertex
p1 = geompy.MakeVertex(150, 150, 100)
geompy.addToStudy(p1, "p1")
GHS3D_Parameters_wo_geometry.SetEnforcedVertexGeomWithGroup( p1, 5 , "My special nodes")
#GHS3D_Parameters.SetEnforcedVertexGeom( p1, 5 ) # no group

# compute the mesh
ghs3dMesh_wo_geometry.Compute()

# Erase all enforced vertices
GHS3D_Parameters.ClearEnforcedVertices()

# End of script
