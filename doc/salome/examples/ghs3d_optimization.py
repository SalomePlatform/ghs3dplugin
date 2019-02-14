import salome
salome.salome_init()

from salome.geom import geomBuilder
geompy = geomBuilder.New(salome.myStudy)

from salome.smesh import smeshBuilder
smesh =  smeshBuilder.New(salome.myStudy)

# create a disk
disk = geompy.MakeDiskR(100., 1, theName="disk")

# triangulate the disk
mesh = smesh.Mesh( disk )
cadsurf = mesh.Triangle( smeshBuilder.MG_CADSurf )
cadsurf.SetQuadAllowed( True )
mesh.Compute()

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
optMesh.Compute()

print("Nb tetra before optimization", mesh.NbTetras())
print("Nb tetra after  optimization", optMesh.NbTetras())

# End of script

