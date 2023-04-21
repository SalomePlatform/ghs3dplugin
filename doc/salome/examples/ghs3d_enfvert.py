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

import math

import salome
salome.salome_init()

from salome.geom import geomBuilder
geompy = geomBuilder.New()

import SMESH
from salome.smesh import smeshBuilder
smesh =  smeshBuilder.New()

## check if a node is at the coords given the tolerance
def isNodeAtCoord(node_id, x, y, z, tol=1e-12):
  xn, yn, zn = mgtetraMesh.GetNodeXYZ(node_id)
  dist = math.sqrt((x-xn)**2+(y-yn)**2+(z-zn)**2)
  print("dist: ", dist)
  return dist < tol

# create a box
box = geompy.MakeBoxDXDYDZ(200., 200., 200.)
geompy.addToStudy(box, "box")
# create a mesh on the box
mgtetraMesh = smesh.Mesh(box,"box: MG-Tetra and MG-CADSurf mesh")
# create a MG-CADSurf algorithm for faces
mgtetraMesh.Triangle(algo=smeshBuilder.MG_CADSurf)
# compute the mesh
ok  = mgtetraMesh.Compute()
if not ok:
  raise Exception("Error when computing MG-CADSurf mesh")

# Make a copy of the 2D mesh
mgtetraMesh_wo_geometry = smesh.CopyMesh( mgtetraMesh, 'MG-Tetra w/o geometry', 0, 0)

# create a MG_Tetra algorithm and hypothesis and assign them to the mesh
MG_Tetra = mgtetraMesh.Tetrahedron( smeshBuilder.MG_Tetra )
MG_Tetra_Parameters = MG_Tetra.Parameters()
# Create the enforced vertex
x1 = 50
y1 = 50
z1 = 100
MG_Tetra_Parameters.SetEnforcedVertex( x1, y1, z1, 2) # no group
# Compute the mesh
ok = mgtetraMesh.Compute()
if not ok:
  raise Exception("Error when computing MG_Tetra mesh")

# Check that the enforced node is at the enforced coords
node_closest = mgtetraMesh.FindNodeClosestTo(x1, y1, z1)

assert isNodeAtCoord(node_closest, x1, y1, z1)

# Ex2: Add one vertex enforced by a GEOM vertex at (50,50,100) 
#      with physical size 5 and add it to a group called "My special nodes"

# Create another MG_Tetra hypothesis and assign it to the mesh without geometry
MG_Tetra_Parameters_wo_geometry = smesh.CreateHypothesis('MG-Tetra Parameters', 'GHS3DEngine')
mgtetraMesh_wo_geometry.AddHypothesis( MG_Tetra )
mgtetraMesh_wo_geometry.AddHypothesis( MG_Tetra_Parameters_wo_geometry )

# Create the enforced vertex
x2 = 50
y2 = 50
z2 = 100
p2 = geompy.MakeVertex(x2, y2, z2)
geompy.addToStudy(p2, "p2")
gr_enforced_name = "My special nodes"
MG_Tetra_Parameters_wo_geometry.SetEnforcedVertexGeomWithGroup( p2, 5 , gr_enforced_name)
#MG_Tetra_Parameters.SetEnforcedVertexGeom( p1, 5 ) # no group

# compute the mesh
ok = mgtetraMesh_wo_geometry.Compute()
if not ok:
  raise Exception("Error when computing MG_Tetra mesh without geometry")

# Check that the enforced node is at the enforced coords
gr_enforced_nodes = mgtetraMesh_wo_geometry.GetGroupByName(gr_enforced_name)[0]
assert (gr_enforced_nodes.Size() == 1)
node_enforced = gr_enforced_nodes.GetIDs()[0]

assert isNodeAtCoord(node_enforced, x2, y2, z2)

# Erase all enforced vertices
MG_Tetra_Parameters.ClearEnforcedVertices()

# End of script
