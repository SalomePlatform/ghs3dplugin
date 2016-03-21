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

# It is possible to constrain MG-Tetra with another mesh or group.
# The constraint can refer to the nodes, edges or faces.
# This feature is available only on 2D meshes without geometry.
# The constraining elements are called enforced elements for the mesh.
# They can be recovered using groups if necessary.

# In the following examples, a box and a cylinder are meshed in 2D.
# The mesh of the cylinder will be used as a constraint for the 
# 3D mesh of the box.

import salome
salome.salome_init()
import GEOM
from salome.geom import geomBuilder
geompy = geomBuilder.New(salome.myStudy)

import SMESH, SALOMEDS
from salome.smesh import smeshBuilder
smesh =  smeshBuilder.New(salome.myStudy)

box = geompy.MakeBoxDXDYDZ(200, 200, 200)
geompy.addToStudy( box, "box" )
cylindre = geompy.MakeCylinderRH(50, 50)
geompy.TranslateDXDYDZ(cylindre, 100, 100, 30)
face_cyl = geompy.ExtractShapes(cylindre, geompy.ShapeType["FACE"], True)[1]
geompy.addToStudy( cylindre, 'cylindre' )
geompy.addToStudyInFather( cylindre, face_cyl, 'face_cyl' )
p1 = geompy.MakeVertex(20, 20, 20)
p2 = geompy.MakeVertex(180, 180, 20)
c = geompy.MakeCompound([p1,p2])
geompy.addToStudy( p1, "p1" )
geompy.addToStudy( p2, "p2" )
geompy.addToStudy( c, "c" )

# Create the 2D algorithm and hypothesis
MG_CADSurf = smesh.CreateHypothesis('MG-CADSurf', 'BLSURFEngine')
# For the box
MG_CADSurf_Parameters = smesh.CreateHypothesis('MG-CADSurf Parameters', 'BLSURFEngine')
MG_CADSurf_Parameters.SetPhysicalMesh( 1 )
MG_CADSurf_Parameters.SetPhySize( 200 )
# For the cylinder
MG_CADSurf_Parameters2 = smesh.CreateHypothesis('MG-CADSurf Parameters', 'BLSURFEngine')
MG_CADSurf_Parameters2.SetGeometricMesh( 1 )

# Create the 3D algorithm and hypothesis
MG_Tetra = smesh.CreateHypothesis('MG-Tetra', 'GHS3DEngine')
MG_Tetra_Parameters_node = smesh.CreateHypothesis('MG-Tetra Parameters', 'GHS3DEngine')
#MG_Tetra_Parameters_node.SetToMeshHoles( 1 )
MG_Tetra_Parameters_edge = smesh.CreateHypothesis('MG-Tetra Parameters', 'GHS3DEngine')
#MG_Tetra_Parameters_edge.SetToMeshHoles( 1 )
MG_Tetra_Parameters_face = smesh.CreateHypothesis('MG-Tetra Parameters', 'GHS3DEngine')
MG_Tetra_Parameters_face.SetToMeshHoles( 1 ) # to mesh inside the cylinder
MG_Tetra_Parameters_mesh = smesh.CreateHypothesis('MG-Tetra Parameters', 'GHS3DEngine')
MG_Tetra_Parameters_mesh.SetToMeshHoles( 1 ) # to mesh inside the cylinder

# Create the mesh on the cylinder
Mesh_cylindre = smesh.Mesh(cylindre)
smesh.SetName(Mesh_cylindre,"Mesh_cylindre")
Mesh_cylindre.AddHypothesis( MG_CADSurf )
Mesh_cylindre.AddHypothesis( MG_CADSurf_Parameters2 )
# Create some groups
face_cyl_faces = Mesh_cylindre.GroupOnGeom(face_cyl,'group_face_cyl', SMESH.FACE)
face_cyl_edges = Mesh_cylindre.GroupOnGeom(face_cyl,'group_edge_cyl', SMESH.EDGE)
face_cyl_nodes = Mesh_cylindre.GroupOnGeom(face_cyl,'group_node_cyl', SMESH.NODE)
Mesh_cylindre.Compute()

# Create the mesh on the cylinder
Mesh_box_tri = smesh.Mesh(box,"Mesh_box_tri")
Mesh_box_tri.AddHypothesis( MG_CADSurf )
Mesh_box_tri.AddHypothesis( MG_CADSurf_Parameters )
Mesh_box_tri.Compute()

# Create 4 copies of the 2D mesh to test the 3 types of contraints (NODE, EDGE, FACE)
# from the whole mesh and from groups of elements.
# Then the 3D algo and hypothesis are assigned to them.

mesh_mesh = smesh.CopyMesh( Mesh_box_tri, 'Enforced by faces of mesh', 0, 0)
mesh_mesh.AddHypothesis( MG_Tetra )
mesh_mesh.AddHypothesis( MG_Tetra_Parameters_mesh)

mesh_node = smesh.CopyMesh( Mesh_box_tri, 'Enforced by group of nodes', 0, 0)
mesh_node.AddHypothesis( MG_Tetra )
mesh_node.AddHypothesis( MG_Tetra_Parameters_node)

mesh_edge = smesh.CopyMesh( Mesh_box_tri, 'Enforced by group of edges', 0, 0)
mesh_edge.AddHypothesis( MG_Tetra )
mesh_edge.AddHypothesis( MG_Tetra_Parameters_edge)

mesh_face = smesh.CopyMesh( Mesh_box_tri, 'Enforced by group of faces', 0, 0)
mesh_face.AddHypothesis( MG_Tetra )
mesh_face.AddHypothesis( MG_Tetra_Parameters_face)

# Add the enforced elements
MG_Tetra_Parameters_mesh.SetEnforcedMeshWithGroup(Mesh_cylindre.GetMesh(),SMESH.FACE,"faces from cylinder")
MG_Tetra_Parameters_node.SetEnforcedMeshWithGroup(face_cyl_nodes,SMESH.NODE,"nodes from face_cyl_nodes")
MG_Tetra_Parameters_edge.SetEnforcedMeshWithGroup(face_cyl_edges,SMESH.EDGE,"edges from face_cyl_edges")
MG_Tetra_Parameters_face.SetEnforcedMeshWithGroup(face_cyl_faces,SMESH.FACE,"faces from face_cyl_faces")

#Compute the meshes
mesh_node.Compute()
mesh_edge.Compute()
mesh_face.Compute()
mesh_mesh.Compute()

# End of script
