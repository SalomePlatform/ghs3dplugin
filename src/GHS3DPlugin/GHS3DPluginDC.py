# Copyright (C) 2007-2011  CEA/DEN, EDF R&D, OPEN CASCADE
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License.
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
#

from smesh import Mesh_Algorithm, AssureGeomPublished

# import GHS3DPlugin module if possible
noGHS3DPlugin = 0
try:
    import GHS3DPlugin
except ImportError:
    noGHS3DPlugin = 1
    pass

# Optimization level of GHS3D
# V3.1
None_Optimization, Light_Optimization, Medium_Optimization, Strong_Optimization = 0,1,2,3
# V4.1 (partialy redefines V3.1). Issue 0020574
None_Optimization, Light_Optimization, Standard_Optimization, StandardPlus_Optimization, Strong_Optimization = 0,1,2,3,4

GHS3D = "GHS3D_3D"

## Tetrahedron GHS3D 3D algorithm
#  It is created by calling Mesh.Tetrahedron( GHS3D, geom=0 )
#
#  @ingroup l3_algos_basic
class GHS3D_Algorithm(Mesh_Algorithm):

    meshMethod = "Tetrahedron"
    algoType   = GHS3D

    ## Private constructor.
    def __init__(self, mesh, geom=0):
        Mesh_Algorithm.__init__(self)
        if noGHS3DPlugin: print "Warning: GHS3DPlugin module unavailable"
        self.Create(mesh, geom, self.algoType, "libGHS3DEngine.so")
        self.params = None

    ## Defines hypothesis having several parameters
    #
    #  @ingroup l3_hypos_ghs3dh
    def Parameters(self):
        if not self.params:
            self.params = self.Hypothesis("GHS3D_Parameters", [],
                                          "libGHS3DEngine.so", UseExisting=0)
        return self.params

    ## To mesh "holes" in a solid or not. Default is to mesh.
    #  @ingroup l3_hypos_ghs3dh
    def SetToMeshHoles(self, toMesh):
        self.Parameters().SetToMeshHoles(toMesh)

    ## Set Optimization level:
    #   None_Optimization, Light_Optimization, Standard_Optimization, StandardPlus_Optimization,
    #   Strong_Optimization.
    # Default is Standard_Optimization
    #  @ingroup l3_hypos_ghs3dh
    def SetOptimizationLevel(self, level):
        self.Parameters().SetOptimizationLevel(level)

    ## Maximal size of memory to be used by the algorithm (in Megabytes).
    #  @ingroup l3_hypos_ghs3dh
    def SetMaximumMemory(self, MB):
        self.Parameters().SetMaximumMemory(MB)

    ## Initial size of memory to be used by the algorithm (in Megabytes) in
    #  automatic memory adjustment mode.
    #  @ingroup l3_hypos_ghs3dh
    def SetInitialMemory(self, MB):
        self.Parameters().SetInitialMemory(MB)

    ## Path to working directory.
    #  @ingroup l3_hypos_ghs3dh
    def SetWorkingDirectory(self, path):
        self.Parameters().SetWorkingDirectory(path)

    ## To keep working files or remove them. Log file remains in case of errors anyway.
    #  @ingroup l3_hypos_ghs3dh
    def SetKeepFiles(self, toKeep):
        self.Parameters().SetKeepFiles(toKeep)

    ## To set verbose level [0-10]. <ul>
    #<li> 0 - no standard output,
    #<li> 2 - prints the data, quality statistics of the skin and final meshes and
    #     indicates when the final mesh is being saved. In addition the software
    #     gives indication regarding the CPU time.
    #<li>10 - same as 2 plus the main steps in the computation, quality statistics
    #     histogram of the skin mesh, quality statistics histogram together with
    #     the characteristics of the final mesh.</ul>
    #  @ingroup l3_hypos_ghs3dh
    def SetVerboseLevel(self, level):
        self.Parameters().SetVerboseLevel(level)

    ## To create new nodes.
    #  @ingroup l3_hypos_ghs3dh
    def SetToCreateNewNodes(self, toCreate):
        self.Parameters().SetToCreateNewNodes(toCreate)

    ## To use boundary recovery version which tries to create mesh on a very poor
    #  quality surface mesh.
    #  @ingroup l3_hypos_ghs3dh
    def SetToUseBoundaryRecoveryVersion(self, toUse):
        self.Parameters().SetToUseBoundaryRecoveryVersion(toUse)

    ## Applies finite-element correction by replacing overconstrained elements where
    #  it is possible. The process is cutting first the overconstrained edges and
    #  second the overconstrained facets. This insure that no edges have two boundary
    #  vertices and that no facets have three boundary vertices.
    #  @ingroup l3_hypos_ghs3dh
    def SetFEMCorrection(self, toUseFem):
        self.Parameters().SetFEMCorrection(toUseFem)

    ## To removes initial central point.
    #  @ingroup l3_hypos_ghs3dh
    def SetToRemoveCentralPoint(self, toRemove):
        self.Parameters().SetToRemoveCentralPoint(toRemove)

    ## To set an enforced vertex.
    #  @param x            : x coordinate
    #  @param y            : y coordinate
    #  @param z            : z coordinate
    #  @param size         : size of 1D element around enforced vertex
    #  @param vertexName   : name of the enforced vertex
    #  @param groupName    : name of the group
    #  @ingroup l3_hypos_ghs3dh
    def SetEnforcedVertex(self, x, y, z, size, vertexName = "", groupName = ""):
        if vertexName == "":
            if groupName == "":
                return self.Parameters().SetEnforcedVertex(x, y, z, size)
            else:
                return self.Parameters().SetEnforcedVertexWithGroup(x, y, z, size, groupName)
        else:
            if groupName == "":
                return self.Parameters().SetEnforcedVertexNamed(x, y, z, size, vertexName)
            else:
                return self.Parameters().SetEnforcedVertexNamedWithGroup(x, y, z, size, vertexName, groupName)

    ## To set an enforced vertex given a GEOM vertex, group or compound.
    #  @param theVertex    : GEOM vertex (or group, compound) to be projected on theFace.
    #  @param size         : size of 1D element around enforced vertex
    #  @param groupName    : name of the group
    #  @ingroup l3_hypos_ghs3dh
    def SetEnforcedVertexGeom(self, theVertex, size, groupName = ""):
        AssureGeomPublished( self.mesh, theVertex )
        if groupName == "":
            return self.Parameters().SetEnforcedVertexGeom(theVertex, size)
        else:
            return self.Parameters().SetEnforcedVertexGeomWithGroup(theVertex, size, groupName)

    ## To remove an enforced vertex.
    #  @param x            : x coordinate
    #  @param y            : y coordinate
    #  @param z            : z coordinate
    #  @ingroup l3_hypos_ghs3dh
    def RemoveEnforcedVertex(self, x, y, z):
        return self.Parameters().RemoveEnforcedVertex(x, y, z)

    ## To remove an enforced vertex given a GEOM vertex, group or compound.
    #  @param theVertex    : GEOM vertex (or group, compound) to be projected on theFace.
    #  @ingroup l3_hypos_ghs3dh
    def RemoveEnforcedVertexGeom(self, theVertex):
        AssureGeomPublished( self.mesh, theVertex )
        return self.Parameters().RemoveEnforcedVertexGeom(theVertex)

    ## To set an enforced mesh with given size and add the enforced elements in the group "groupName".
    #  @param theSource    : source mesh which provides constraint elements/nodes
    #  @param elementType  : SMESH.ElementType (NODE, EDGE or FACE)
    #  @param size         : size of elements around enforced elements. Unused if -1.
    #  @param groupName    : group in which enforced elements will be added. Unused if "".
    #  @ingroup l3_hypos_ghs3dh
    def SetEnforcedMesh(self, theSource, elementType, size = -1, groupName = ""):
        if size >= 0:
            if groupName != "":
                return self.Parameters().SetEnforcedMesh(theSource, elementType)
            else:
                return self.Parameters().SetEnforcedMeshWithGroup(theSource, elementType, groupName)
        else:
            if groupName != "":
                return self.Parameters().SetEnforcedMeshSize(theSource, elementType, size)
            else:
                return self.Parameters().SetEnforcedMeshSizeWithGroup(theSource, elementType, size, groupName)

    ## Sets command line option as text.
    #  @ingroup l3_hypos_ghs3dh
    def SetTextOption(self, option):
        self.Parameters().SetTextOption(option)
    
