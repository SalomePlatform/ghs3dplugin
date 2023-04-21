import salome
salome.salome_init()

from salome.geom import geomBuilder
geompy = geomBuilder.New()

import SMESH
from salome.smesh import smeshBuilder
smesh =  smeshBuilder.New()

# create a disk
disk = geompy.MakeDiskR(100., 1, theName="disk")

# triangulate the disk
mesh = smesh.Mesh( disk )
cadsurf = mesh.Triangle( smeshBuilder.MG_CADSurf )
cadsurf.SetQuadAllowed( True )
ok = mesh.Compute()
if not ok:
  raise Exception("Error when computing mesh")

# extrude the 2D mesh into a prismatic mesh
mesh.ExtrusionSweepObject( mesh, [0,0,10], 7 )

# split prisms into tetrahedra
mesh.SplitVolumesIntoTetra( mesh )

# copy the mesh into a new mesh, since only a mesh not based of geometry
# can be optimized using MG-Tetra Optimization
optMesh = smesh.CopyMesh( mesh, "optimization" )

# add MG-Tetra Optimization
mg_opt = optMesh.Tetrahedron( smeshBuilder.MG_Tetra_Optimization )
mg_opt.SetSmoothOffSlivers( True )
mg_opt.SetOptimizationLevel( smeshBuilder.Strong_Optimization )

# run optimization
ok = optMesh.Compute()
if not ok:
  raise Exception("Error when computing optimization mesh")

print("Nb tetra before optimization", mesh.NbTetras())
print("Nb tetra after  optimization", optMesh.NbTetras())

# Check that aspect ratio 3D of optimized mesh is better than original mesh
min_aspectratio_orig, max_aspectratio_orig = mesh.GetMinMax(SMESH.FT_AspectRatio3D)
min_aspectratio_optim, max_aspectratio_optim = optMesh.GetMinMax(SMESH.FT_AspectRatio3D)

assert (min_aspectratio_orig - min_aspectratio_optim)/min_aspectratio_orig < 0.5
assert (max_aspectratio_orig - max_aspectratio_optim)/max_aspectratio_orig < 0.5

# End of script

