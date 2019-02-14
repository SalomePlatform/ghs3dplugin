import salome
salome.salome_init()

from salome.geom import geomBuilder
geompy = geomBuilder.New()

from salome.smesh import smeshBuilder
smesh =  smeshBuilder.New()

# create a box
box = geompy.MakeBoxDXDYDZ(200., 200., 200.)
geompy.addToStudy(box, "box")

# create a mesh on the box
mgtetraMesh = smesh.Mesh(box,"box: MG-Tetra and MG-CADSurf mesh")

# create a MG_CADSurf algorithm for faces
MG_CADSurf = mgtetraMesh.Triangle(algo=smeshBuilder.MG_CADSurf)
MG_Tetra = mgtetraMesh.Tetrahedron(algo=smeshBuilder.MG_Tetra)

# compute the mesh
mgtetraMesh.Compute()

# End of script

