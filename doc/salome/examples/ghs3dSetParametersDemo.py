import unittest

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
mgtetraMesh = smesh.Mesh(box,"box: MG-Tetra and NETGEN_1D_2D mesh")

# create a NETGEN1D2D algorithm for faces and vertices
NETGEN_1D_2D   = mgtetraMesh.Triangle(algo=smeshBuilder.NETGEN_1D2D)
NETGEN_2D_Parameters_1 = NETGEN_1D_2D.Parameters()
MG_Tetra       = mgtetraMesh.Tetrahedron(algo=smeshBuilder.MG_Tetra)
MG_Tetra_Parameters_1 = MG_Tetra.Parameters()
MG_Tetra_Parameters_1.SetAlgorithm( 1 )         # 1 MGTetra (Default) - 0 MGTetra HPC
MG_Tetra_Parameters_1.SetUseNumOfThreads( 1 )   # 1 true - 0 false
MG_Tetra_Parameters_1.SetNumOfThreads( 6 )      # Number of threads
MG_Tetra_Parameters_1.SetPthreadMode( 1 )       # 0 - none, 1 - aggressive, 2 - safe

# compute the mesh with MGTetra
status = mgtetraMesh.Compute()
assert( status )

mgtetraHPCMesh       = smesh.Mesh(box,"box: MG-Tetra HPC and NETGEN_1D_2D mesh")
status               = mgtetraHPCMesh.AddHypothesis(NETGEN_2D_Parameters_1)
NETGEN_1D_2D_1       = mgtetraHPCMesh.Triangle(algo=smeshBuilder.NETGEN_1D2D)
MG_Tetra_1           = mgtetraHPCMesh.Tetrahedron(algo=smeshBuilder.MG_Tetra)
MG_Tetra_Parameters_2 = MG_Tetra_1.Parameters()
MG_Tetra_Parameters_2.SetAlgorithm( 0 )         # 1 MGTetra (Default) - 0 MGTetra HPC
MG_Tetra_Parameters_2.SetUseNumOfThreads( 1 )   # 1 true - 0 false
MG_Tetra_Parameters_2.SetNumOfThreads( 6 )      # Number of threads
MG_Tetra_Parameters_2.SetParallelMode( 1 )      # 0 - none, 1 - reproducible_given_max_num_of_threads, 2 - reproducible, 3 - aggressive

# compute the mesh  with MGTetra HPC
status = mgtetraHPCMesh.Compute()
assert( status )


# End of script

