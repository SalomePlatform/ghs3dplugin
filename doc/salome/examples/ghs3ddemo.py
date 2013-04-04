
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
BLSURF = ghs3dMesh.Triangle(algo=smeshBuilder.BLSURF)
GHS3D = ghs3dMesh.Tetrahedron(algo=smeshBuilder.GHS3D)

# compute the mesh
ghs3dMesh.Compute()

# End of script

