// Copyright (C) 2004-2016  CEA/DEN, EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

#include "MG_Tetra_API.hxx"

#ifdef WIN32
#define NOMINMAX
#endif
#include <SMESH_Comment.hxx>
#include <SMESH_File.hxx>
#include <Utils_SALOME_Exception.hxx>

#include <vector>
#include <iterator>
#include <cstring>

#ifdef USE_MG_LIBS

extern "C"{
#include <meshgems/meshgems.h>
#include <meshgems/tetra.h>
}

struct MG_Tetra_API::LibData
{
  // MG objects
  context_t *       _context;
  tetra_session_t * _session;
  mesh_t *          _tria_mesh;
  sizemap_t *       _sizemap;
  mesh_t *          _tetra_mesh;

  // data to pass to MG
  std::vector<double> _xyz;
  std::vector<double> _nodeSize; // required nodes
  std::vector<int>    _edgeNodes;
  int                 _nbRequiredEdges;
  std::vector<int>    _triaNodes;
  int                 _nbRequiredTria;
  std::vector<int>    _tetraNodes;

  int                 _count;
  volatile bool&      _cancelled_flag;
  std::string         _errorStr;
  double&             _progress;
  bool                _progressInCallBack;

  LibData( volatile bool & cancelled_flag, double& progress )
    : _context(0), _session(0), _tria_mesh(0), _sizemap(0), _tetra_mesh(0),
      _nbRequiredEdges(0), _nbRequiredTria(0),
      _cancelled_flag( cancelled_flag ), _progress( progress ), _progressInCallBack( false )
  {
  }
  // methods setting callbacks implemented after callback definitions
  void Init();
  bool Compute();

  ~LibData()
  {
    if ( _tetra_mesh )
      tetra_regain_mesh( _session, _tetra_mesh );
    if ( _session )
      tetra_session_delete( _session );
    if ( _tria_mesh )
      mesh_delete( _tria_mesh );
    if ( _sizemap )
      sizemap_delete( _sizemap );
    if ( _context )
      context_delete( _context );

    _tetra_mesh = 0;
    _session = 0;
    _tria_mesh = 0;
    _sizemap = 0;
    _context = 0;
  }

  void AddError( const char *txt )
  {
    if ( txt )
      _errorStr += txt;
  }

  const std::string& GetErrors()
  {
    return _errorStr;
  }

  void MG_Error(const char* txt="")
  {
    SMESH_Comment msg("\nMeshGems error. ");
    msg << txt << "\n";
    AddError( msg.c_str() );
  }

  bool SetParam( const std::string& param, const std::string& value )
  {
    status_t ret = tetra_set_param( _session, param.c_str(), value.c_str() );
#ifdef _DEBUG_
    //std::cout << param << " = " << value << std::endl;
#endif
    return ( ret == STATUS_OK );
  }

  bool Cancelled()
  {
    return _cancelled_flag;
  }

  int ReadNbSubDomains()
  {
    integer nb = 0;
    status_t ret = mesh_get_subdomain_count( _tetra_mesh, & nb );

    if ( ret != STATUS_OK ) MG_Error("mesh_get_subdomain_count problem");
    return nb;
  }

  int ReadNbNodes()
  {
    integer nb = 0;
    status_t ret = mesh_get_vertex_count( _tetra_mesh, & nb );

    if ( ret != STATUS_OK ) MG_Error("mesh_get_vertex_count problem");
    return nb;
  }

  int ReadNbEdges()
  {
    integer nb = 0;
    status_t ret = mesh_get_edge_count( _tetra_mesh, & nb );

    if ( ret != STATUS_OK ) MG_Error("mesh_get_edge_count problem");
    return nb;
  }

  int ReadNbTria()
  {
    integer nb = 0;
    status_t ret = mesh_get_triangle_count( _tetra_mesh, & nb );

    if ( ret != STATUS_OK ) MG_Error("mesh_get_triangle_count problem");
    return nb;
  }

  int ReadNbQuads()
  {
    integer nb = 0;
    status_t ret = mesh_get_quadrangle_count( _tetra_mesh, & nb );

    if ( ret != STATUS_OK ) MG_Error("mesh_get_quadrangle_count problem");
    return nb;
  }

  int ReadNbTetra()
  {
    integer nb = 0;
    status_t ret = mesh_get_tetrahedron_count( _tetra_mesh, & nb );

    if ( ret != STATUS_OK ) MG_Error("mesh_get_tetrahedron_count problem");
    return nb;
  }

  int ReadNbHexa()
  {
    integer nb = 0;
    status_t ret = mesh_get_hexahedron_count( _tetra_mesh, & nb );

    if ( ret != STATUS_OK ) MG_Error("mesh_get_hexahedron_count problem");
    return nb;
  }

  void ResetCounter()
  {
    _count = 1;
  }

  void ReadSubDomain( int* nbNodes, int* faceInd, int* ori, int* domain )
  {
    integer tag, seed_type, seed_idx, seed_orientation;
    status_t ret = mesh_get_subdomain_description( _tetra_mesh, _count,
                                                   &tag, &seed_type, &seed_idx, &seed_orientation);

    if ( ret != STATUS_OK ) MG_Error( "unable to get a sub-domain description");

    *nbNodes = 3;
    *faceInd = seed_idx;
    *domain  = tag;
    *ori     = seed_orientation;

    ++_count;
  }
  void ReadNodeXYZ( double* x, double* y, double *z, int* /*domain*/  )
  {
    real coo[3];
    status_t ret = mesh_get_vertex_coordinates( _tetra_mesh, _count, coo );
    if ( ret != STATUS_OK ) MG_Error( "unable to get resulting vertices" );

    *x = coo[0];
    *y = coo[1];
    *z = coo[2];

    ++_count;
  }

  void ReadEdgeNodes( int* node1, int* node2, int* domain )
  {
    integer vtx[2], tag;
    status_t ret = mesh_get_edge_vertices( _tetra_mesh, _count, vtx);
    if (ret != STATUS_OK) MG_Error( "unable to get resulting edge" );

    *node1 = vtx[0];
    *node2 = vtx[1];

    ret = mesh_get_edge_tag( _tetra_mesh, _count, &tag );
    if (ret != STATUS_OK) MG_Error( "unable to get resulting edge tag" );

    *domain = tag;

    ++_count;
  }

  void ReadTriaNodes( int* node1, int* node2, int* node3, int* domain )
  {
    integer vtx[3], tag;
    status_t ret = mesh_get_triangle_vertices( _tetra_mesh, _count, vtx);
    if (ret != STATUS_OK) MG_Error( "unable to get resulting triangle" );

    *node1 = vtx[0];
    *node2 = vtx[1];
    *node3 = vtx[2];

    ret = mesh_get_triangle_tag( _tetra_mesh, _count, &tag );
    if (ret != STATUS_OK) MG_Error( "unable to get resulting triangle tag" );

    *domain = tag;

    ++_count;
  }

  void ReadQuadNodes( int* node1, int* node2, int* node3, int* node4, int* domain )
  {
    integer vtx[4], tag;
    status_t ret = mesh_get_quadrangle_vertices( _tetra_mesh, _count, vtx);
    if (ret != STATUS_OK) MG_Error( "unable to get resulting quadrangle" );

    *node1 = vtx[0];
    *node2 = vtx[1];
    *node3 = vtx[2];
    *node4 = vtx[3];

    ret = mesh_get_quadrangle_tag( _tetra_mesh, _count, &tag );
    if (ret != STATUS_OK) MG_Error( "unable to get resulting quadrangle tag" );

    *domain = tag;

    ++_count;
  }

  void ReadTetraNodes( int* node1, int* node2, int* node3, int* node4, int* domain )
  {
    integer vtx[4], tag;
    status_t ret = mesh_get_tetrahedron_vertices( _tetra_mesh, _count, vtx);
    if (ret != STATUS_OK) MG_Error( "unable to get resulting tetrahedron" );

    *node1 = vtx[0];
    *node2 = vtx[1];
    *node3 = vtx[2];
    *node4 = vtx[3];

    ret = mesh_get_tetrahedron_tag( _tetra_mesh, _count, &tag );
    if (ret != STATUS_OK) MG_Error( "unable to get resulting tetrahedron tag" );

    *domain = tag;

    ++_count;
  }

  void ReadHexaNodes( int* node1, int* node2, int* node3, int* node4,
                     int* node5, int* node6, int* node7, int* node8, int* domain )
  {
    integer vtx[8], tag;
    status_t ret = mesh_get_hexahedron_vertices( _tetra_mesh, _count, vtx);
    if (ret != STATUS_OK) MG_Error( "unable to get resulting hexahedron" );

    *node1 = vtx[0];
    *node2 = vtx[1];
    *node3 = vtx[2];
    *node4 = vtx[3];
    *node5 = vtx[4];
    *node6 = vtx[5];
    *node7 = vtx[6];
    *node8 = vtx[7];

    ret = mesh_get_hexahedron_tag( _tetra_mesh, _count, &tag );
    if (ret != STATUS_OK) MG_Error( "unable to get resulting hexahedron tag" );

    *domain = tag;

    ++_count;
  }

  void SetNbVertices( int nb )
  {
    _xyz.reserve( _xyz.capacity() + nb );
  }

  void SetNbEdges( int nb )
  {
    _edgeNodes.reserve( nb * 2 );
  }

  void SetNbTria( int nb )
  {
    _triaNodes.reserve( nb * 3 );
  }

  void SetNbTetra( int nb )
  {
    _tetraNodes.reserve( nb * 4 );
  }

  void SetNbReqVertices( int nb )
  {
    _nodeSize.reserve( nb );
  }

  void SetNbReqEdges( int nb )
  {
    _nbRequiredEdges = nb;
  }

  void SetNbReqTria( int nb )
  {
    _nbRequiredTria = nb;
  }

  void AddNode( double x, double y, double z, int domain )
  {
    _xyz.push_back( x );
    _xyz.push_back( y );
    _xyz.push_back( z );
  }

  void AddSizeAtNode( double size )
  {
    _nodeSize.push_back( size );
  }
  
  void AddEdgeNodes( int node1, int node2, int domain )
  {
    _edgeNodes.push_back( node1 );
    _edgeNodes.push_back( node2 );
  }
  
  void AddTriaNodes( int node1, int node2, int node3, int domain )
  {
    _triaNodes.push_back( node1 );
    _triaNodes.push_back( node2 );
    _triaNodes.push_back( node3 );
  }

  void AddTetraNodes( int node1, int node2, int node3, int node4, int domain )
  {
    _tetraNodes.push_back( node1 );
    _tetraNodes.push_back( node2 );
    _tetraNodes.push_back( node3 );
    _tetraNodes.push_back( node4 );
  }

  int NbNodes()
  {
    return _xyz.size() / 3;
  }

  double* NodeCoord( int iNode )
  {
    return & _xyz[ iNode * 3 ];
  }

  int NbEdges()
  {
    return _edgeNodes.size() / 2;
  }

  int* GetEdgeNodes( int iEdge )
  {
    return & _edgeNodes[ iEdge * 2 ];
  }

  int NbTriangles()
  {
    return _triaNodes.size() / 3;
  }

  int NbTetra()
  {
    return _tetraNodes.size() / 4;
  }

  int * GetTriaNodes( int iTria )
  {
    return & _triaNodes[ iTria * 3 ];
  }

  int * GetTetraNodes( int iTet )
  {
    return & _tetraNodes[ iTet * 4 ];
  }

  int IsVertexRequired( int iNode )
  {
    return ! ( iNode < int( NbNodes() - _nodeSize.size() ));
  }

  double GetSizeAtVertex( int iNode )
  {
    return IsVertexRequired( iNode ) ? _nodeSize[ iNode - NbNodes() + _nodeSize.size() ] : 0.;
  }
};

namespace // functions called by MG library to exchange with the application
{
  status_t get_vertex_count(integer * nbvtx, void *user_data)
  {
    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    *nbvtx = data->NbNodes();

    return STATUS_OK;
  }

  status_t get_vertex_coordinates(integer ivtx, real * xyz, void *user_data)
  {
    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    double* coord = data->NodeCoord( ivtx-1 );
    for (int j = 0; j < 3; j++)
      xyz[j] = coord[j];

    return STATUS_OK;
  }
  status_t get_edge_count(integer * nbedge, void *user_data)
  {
    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    *nbedge = data->NbEdges();

    return STATUS_OK;
  }

  status_t get_edge_vertices(integer iedge, integer * vedge, void *user_data)
  {
    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    int* nodes = data->GetEdgeNodes( iedge-1 );
    vedge[0] = nodes[0];
    vedge[1] = nodes[1];

    return STATUS_OK;
  }

  status_t get_triangle_count(integer * nbtri, void *user_data)
  {
    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    *nbtri = data->NbTriangles();

    return STATUS_OK;
  }

  status_t get_triangle_vertices(integer itri, integer * vtri, void *user_data)
  {
    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    int* nodes = data->GetTriaNodes( itri-1 );
    vtri[0] = nodes[0];
    vtri[1] = nodes[1];
    vtri[2] = nodes[2];

    return STATUS_OK;
  }

  // status_t get_triangle_extra_vertices(integer itri, integer * typetri,
  //                                      integer * evtri, void *user_data)
  // {
  //   int j;
  //   MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;

  //   if (1) {
  //     /* We want to describe a linear "3 nodes" triangle */
  //     *typetri = MESHGEMS_MESH_ELEMENT_TYPE_TRIA3;
  //   } else {
  //     /* We want to describe a quadratic "6 nodes" triangle */
  //     *typetri = MESHGEMS_MESH_ELEMENT_TYPE_TRIA6;
  //     for (j = 0; j < 3; j++)
  //       evtri[j] = 0;             /* the j'th quadratic vertex index of the itri'th triangle */
  //   }

  //   return STATUS_OK;
  // }

  status_t get_tetrahedron_count(integer * nbtetra, void *user_data)
  {
    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    *nbtetra = data->NbTetra();

    return STATUS_OK;
  }

  status_t get_tetrahedron_vertices(integer itetra, integer * vtetra,
                                    void *user_data)
  {
    int j;
    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    int* nodes = data->GetTetraNodes( itetra-1 );
    for (j = 0; j < 4; j++)
      vtetra[j] = nodes[j];

    return STATUS_OK;
  }

  status_t get_vertex_required_property(integer ivtx, integer * rvtx, void *user_data)
  {
    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    *rvtx = data->IsVertexRequired( ivtx - 1 );

    return STATUS_OK;
  }

  status_t get_vertex_weight(integer ivtx, real * wvtx, void *user_data)
  {
    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    *wvtx = data->GetSizeAtVertex( ivtx - 1 );

    return STATUS_OK;
  }

  status_t my_message_cb(message_t * msg, void *user_data)
  {
    char *desc;
    message_get_description(msg, &desc);

    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    data->AddError( desc );

#ifdef _DEBUG_
    //std::cout << desc << std::endl;
#endif

    if ( strncmp( "MGMESSAGE  1009001 ", desc, 19 ) == 0 )
    {
      // progress message (10%): "MGMESSAGE  1009001  0 1 1.000000e+01"
      data->_progress = atof( desc + 24 );

      _progressInCallBack = true;
    }

    if ( !_progressInCallBack )
    {
      // Compute progress
      // corresponding messages are:
      // "  -- PHASE 1 COMPLETED"               => 10 %
      // "  -- PHASE 2 COMPLETED"               => 25 %
      // "     ** ITERATION   1"                => 25.* %
      // "     ** ITERATION   2"                => 25.* %
      // "  -- PHASE 3 COMPLETED"               => 70 %
      // "  -- PHASE 4 COMPLETED"               => 98 %

      if ( strncmp( "-- PHASE ", desc + 2, 9 ) == 0 && desc[ 13 ] == 'C' )
      {
        const double progress[] = { 10., 25., 70., 98., 100., 100., 100. };
        int       phase = atoi( desc + 11 );
        data->_progress = std::max( data->_progress, progress[ phase - 1 ] / 100. );
      }
      else if ( strncmp( "** ITERATION ", desc + 5, 13 ) == 0 )
      {
        int         iter = atoi( desc + 20 );
        double   percent = 25. + iter * ( 70 - 25 ) / 20.;
        data->_progress = std::max( data->_progress, ( percent / 100. ));
      }
    }

    return STATUS_OK;
  }

  status_t my_interrupt_callback(integer *interrupt_status, void *user_data)
  {
    MG_Tetra_API::LibData* data = (MG_Tetra_API::LibData *) user_data;
    *interrupt_status = ( data->Cancelled() ? INTERRUPT_STOP : INTERRUPT_CONTINUE );


    return STATUS_OK;
  }

} // end namespace


void MG_Tetra_API::LibData::Init()
{
  status_t ret;

  // Create the meshgems working context
  _context = context_new();
  if ( !_context ) MG_Error( "unable to create a new context" );

  // Set the message callback for the _context.
  ret = context_set_message_callback( _context, my_message_cb, this );
  if ( ret != STATUS_OK ) MG_Error("in context_set_message_callback");

  // Create the structure holding the callbacks giving access to triangle mesh
  _tria_mesh = mesh_new( _context );
  if ( !_tria_mesh ) MG_Error("unable to create a new mesh");

  // Set callbacks to provide triangle mesh data
  mesh_set_get_vertex_count( _tria_mesh, get_vertex_count, this );
  mesh_set_get_vertex_coordinates( _tria_mesh, get_vertex_coordinates, this );
  mesh_set_get_vertex_required_property( _tria_mesh, get_vertex_required_property, this );
  mesh_set_get_edge_count( _tria_mesh, get_edge_count, this);
  mesh_set_get_edge_vertices( _tria_mesh, get_edge_vertices, this );
  mesh_set_get_triangle_count( _tria_mesh, get_triangle_count, this );
  mesh_set_get_triangle_vertices( _tria_mesh, get_triangle_vertices, this );
  mesh_set_get_tetrahedron_count( _tria_mesh, get_tetrahedron_count, this );
  mesh_set_get_tetrahedron_vertices( _tria_mesh, get_tetrahedron_vertices, this );

  // Create a tetra session
  _session = tetra_session_new( _context );
  if ( !_session ) MG_Error( "unable to create a new tetra session");

  ret = tetra_set_interrupt_callback( _session, my_interrupt_callback, this );
  if ( ret != STATUS_OK ) MG_Error("in tetra_set_interrupt_callback");

}

bool MG_Tetra_API::LibData::Compute()
{
  status_t ret;

  if ( _tetraNodes.empty() )
  {
    // Set surface mesh
    ret = tetra_set_surface_mesh( _session, _tria_mesh );
    if ( ret != STATUS_OK ) MG_Error( "unable to set surface mesh");
  }
  else
  {
    ret = tetra_set_volume_mesh( _session, _tria_mesh );
    if ( ret != STATUS_OK ) MG_Error( "unable to set volume mesh");
  }

  // Set a sizemap
  if ( !_nodeSize.empty() )
  {
    _sizemap = meshgems_sizemap_new( _tria_mesh, meshgems_sizemap_type_iso_mesh_vertex,
                                     (void*) &get_vertex_weight, this );
    if ( !_sizemap ) MG_Error("unable to create a new sizemap");

    ret = tetra_set_sizemap( _session, _sizemap );
    if ( ret != STATUS_OK ) MG_Error( "unable to set sizemap");
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // const char* file  =  "/tmp/ghs3d_IN.mesh";
  // mesh_write_mesh( _tria_mesh,file);
  // std::cout << std::endl << std::endl << "Write " << file << std::endl << std::endl << std::endl;

  ret = tetra_compute_mesh( _session );
  if ( ret != STATUS_OK ) return false;

  ret = tetra_get_mesh( _session, &_tetra_mesh);
  if (ret != STATUS_OK) MG_Error( "unable to get resulting mesh");

  //////////////////////////////////////////////////////////////////////////////////////////
  // {
  //   const char* file  =  "/tmp/ghs3d_OUT.mesh";
  //   mesh_write_mesh( _tetra_mesh,file);
  //   std::cout << std::endl << std::endl << "Write " << file << std::endl << std::endl << std::endl;
  // }
  return true;
}


#endif // ifdef USE_MG_LIBS


//================================================================================
/*!
 * \brief Constructor
 */
//================================================================================

MG_Tetra_API::MG_Tetra_API(volatile bool& cancelled_flag, double& progress)
{
  _useLib = false;
#ifdef USE_MG_LIBS
  _useLib = true;
  _libData = new LibData( cancelled_flag, progress );
  _libData->Init();
  if ( getenv("MG_TETRA_USE_EXE"))
    _useLib = false;
#endif
}

//================================================================================
/*!
 * \brief Destructor
 */
//================================================================================

MG_Tetra_API::~MG_Tetra_API()
{
#ifdef USE_MG_LIBS
  delete _libData;
  _libData = 0;
#endif
  std::set<int>::iterator id = _openFiles.begin();
  for ( ; id != _openFiles.end(); ++id )
    ::GmfCloseMesh( *id );
  _openFiles.clear();
}

//================================================================================
/*!
 * \brief Return the way of MG usage
 */
//================================================================================

bool MG_Tetra_API::IsLibrary()
{
  return _useLib;
}

//================================================================================
/*!
 * \brief Switch to usage of MG-Tetra executable
 */
//================================================================================

void MG_Tetra_API::SetUseExecutable()
{
  _useLib = false;
}

//================================================================================
/*!
 * \brief Compute the tetra mesh
 *  \param [in] cmdLine - a command to run mg_tetra.exe
 *  \return bool - Ok or not
 */
//================================================================================

bool MG_Tetra_API::Compute( const std::string& cmdLine, std::string& errStr )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS

    // split cmdLine
    std::istringstream strm( cmdLine );
    std::istream_iterator<std::string> sIt( strm ), sEnd;
    std::vector< std::string > args( sIt, sEnd );

    // set parameters
    std::string param, value;
    for ( size_t i = 1; i < args.size(); ++i )
    {
      // look for a param name; it starts from "-"
      param = args[i];
      if ( param.size() < 2 || param[0] != '-')
        continue;
      while ( param[0] == '-')
        param = param.substr( 1 );

      value = "";
      while ( i+1 < args.size() && args[i+1][0] != '-' )
      {
        if ( strncmp( "1>", args[i+1].c_str(), 2 ) == 0 )
          break;
        if ( !value.empty() ) value += " ";
        value += args[++i];
      }
      if ( !_libData->SetParam( param, value ))
        std::cout << "Warning: wrong param: '" << param <<"' = '" << value << "'" << std::endl;
    }

    // compute
    bool ok =  _libData->Compute();

    GetLog(); // write a log file
    _logFile = ""; // not to write it again

    return ok;
#endif
  }

  int err = system( cmdLine.c_str() ); // run

  if ( err )
    errStr = SMESH_Comment("system(mg-tetra.exe ...) command failed with error: ")
      << strerror( errno );

  return !err;

}

//================================================================================
/*!
 * \brief Prepare for reading a mesh data
 */
//================================================================================

int  MG_Tetra_API::GmfOpenMesh(const char* theFile, int rdOrWr, int * ver, int * dim)
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    return 1;
#endif
  }
  int id = ::GmfOpenMesh(theFile, rdOrWr, ver, dim );
  _openFiles.insert( id );
  return id;
}

//================================================================================
/*!
 * \brief Return nb of entities
 */
//================================================================================

int MG_Tetra_API::GmfStatKwd( int iMesh, GmfKwdCod what )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    switch ( what )
    {
    case GmfSubDomainFromGeom: return _libData->ReadNbSubDomains();
    case GmfVertices:          return _libData->ReadNbNodes();
    case GmfEdges:             return _libData->ReadNbEdges();
    case GmfTriangles:         return _libData->ReadNbTria();
    case GmfQuadrilaterals:    return _libData->ReadNbQuads();
    case GmfTetrahedra:        return _libData->ReadNbTetra();
    case GmfHexahedra:         return _libData->ReadNbHexa();
    default:                   return 0;
    }
    return 0;
#endif
  }
  return ::GmfStatKwd( iMesh, what );
}

//================================================================================
/*!
 * \brief Prepare for reading some data
 */
//================================================================================

void MG_Tetra_API::GmfGotoKwd( int iMesh, GmfKwdCod what )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    _libData->ResetCounter();
    return;
#endif
  }
  ::GmfGotoKwd( iMesh, what );
}

//================================================================================
/*!
 * \brief Return index of a domain identified by a triangle normal
 *  \param [in] iMesh - mesh file index
 *  \param [in] what - must be GmfSubDomainFromGeom
 *  \param [out] nbNodes - nb nodes in a face
 *  \param [out] faceInd - face index
 *  \param [out] ori - face orientation
 *  \param [out] domain - domain index
 *  \param [in] dummy - an artificial param used to discriminate from GmfGetLin() reading
 *              a triangle
 */
//================================================================================

void MG_Tetra_API::GmfGetLin( int iMesh, GmfKwdCod what, int* nbNodes, int* faceInd, int* ori, int* domain, int dummy )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    _libData->ReadSubDomain( nbNodes, faceInd, ori, domain );
    return;
#endif
  }
  ::GmfGetLin( iMesh, what, nbNodes, faceInd, ori, domain );
}

//================================================================================
/*!
 * \brief Return coordinates of a next node
 */
//================================================================================

void MG_Tetra_API::GmfGetLin(int iMesh, GmfKwdCod what,
                             double* x, double* y, double *z, int* domain )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    _libData->ReadNodeXYZ( x, y, z, domain );
    return;
#endif
  }
  ::GmfGetLin(iMesh, what, x, y, z, domain );
}

//================================================================================
/*!
 * \brief Return coordinates of a next node
 */
//================================================================================

void MG_Tetra_API::GmfGetLin(int iMesh, GmfKwdCod what,
                             float* x, float* y, float *z, int* domain )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    double X,Y,Z;
    _libData->ReadNodeXYZ( &X, &Y, &Z, domain );
    *x = X;
    *y = Y;
    *z = Z;
    return;
#endif
  }
  ::GmfGetLin(iMesh, what, x, y, z, domain );
}

//================================================================================
/*!
 * \brief Return node index of a next corner
 */
//================================================================================

void MG_Tetra_API::GmfGetLin(int iMesh, GmfKwdCod what, int* node )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    node = 0;
    return;
#endif
  }
  ::GmfGetLin(iMesh, what, node );
}

//================================================================================
/*!
 * \brief Return node indices of a next edge
 */
//================================================================================

void MG_Tetra_API::GmfGetLin(int iMesh, GmfKwdCod what, int* node1, int* node2, int* domain )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    _libData->ReadEdgeNodes( node1, node2, domain );
    return;
#endif
  }
  ::GmfGetLin( iMesh, what, node1, node2, domain );
}

//================================================================================
/*!
 * \brief Return node indices of a next triangle
 */
//================================================================================

void MG_Tetra_API::GmfGetLin(int iMesh, GmfKwdCod what,
                             int* node1, int* node2, int* node3, int* domain )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    _libData->ReadTriaNodes( node1, node2, node3, domain );
    return;
#endif
  }
  ::GmfGetLin(iMesh, what, node1, node2, node3, domain );
}

//================================================================================
/*!
 * \brief Return node indices of a next tetrahedron or quadrangle
 */
//================================================================================

void MG_Tetra_API::GmfGetLin(int iMesh, GmfKwdCod what,
                             int* node1, int* node2, int* node3, int* node4, int* domain )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    if ( what == GmfQuadrilaterals )
      _libData->ReadQuadNodes( node1, node2, node3, node4, domain );
    else
      _libData->ReadTetraNodes( node1, node2, node3, node4, domain );
    return;
#endif
  }
  ::GmfGetLin(iMesh, what, node1, node2, node3, node4, domain );
}

//================================================================================
/*!
 * \brief Return node indices of a next hexahedron
 */
//================================================================================

void MG_Tetra_API::GmfGetLin(int iMesh, GmfKwdCod what,
                             int* node1, int* node2, int* node3, int* node4,
                             int* node5, int* node6, int* node7, int* node8,
                             int* domain )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    _libData->ReadHexaNodes( node1, node2, node3, node4,
                             node5, node6, node7, node8, domain );
    return;
#endif
  }
  ::GmfGetLin(iMesh, what, node1, node2, node3, node4, node5, node6, node7, node8, domain );
}

//================================================================================
/*!
 * \brief Prepare for passing data to MeshGems
 */
//================================================================================

int  MG_Tetra_API::GmfOpenMesh(const char* theFile, int rdOrWr, int ver, int dim)
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    return 1;
#endif
  }
  int id = ::GmfOpenMesh(theFile, rdOrWr, ver, dim);
  _openFiles.insert( id );
  return id;
}

//================================================================================
/*!
 * \brief Set number of entities
 */
//================================================================================

void MG_Tetra_API::GmfSetKwd(int iMesh, GmfKwdCod what, int nb )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    switch ( what ) {
    case GmfVertices:          _libData->SetNbVertices( nb ); break;
    case GmfEdges:             _libData->SetNbEdges   ( nb ); break;
    case GmfRequiredEdges:     _libData->SetNbReqEdges( nb ); break;
    case GmfTriangles:         _libData->SetNbTria    ( nb ); break;
    case GmfRequiredTriangles: _libData->SetNbReqTria ( nb ); break;
    default:;
    }
    return;
#endif
  }
  ::GmfSetKwd(iMesh, what, nb );
}

//================================================================================
/*!
 * \brief Add coordinates of a node
 */
//================================================================================

void MG_Tetra_API::GmfSetLin(int iMesh, GmfKwdCod what, double x, double y, double z, int domain)
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    _libData->AddNode( x, y, z, domain );
    return;
#endif
  }
  ::GmfSetLin(iMesh, what, x, y, z, domain);
}

//================================================================================
/*!
 * \brief Set number of field entities
 *  \param [in] iMesh - solution file index
 *  \param [in] what - solution type
 *  \param [in] nbNodes - nb of entities
 *  \param [in] nbTypes - nb of data entries in each entity
 *  \param [in] type - types of the data entries
 *
 * Used to prepare to storing size at nodes
 */
//================================================================================

void MG_Tetra_API::GmfSetKwd(int iMesh, GmfKwdCod what, int nbNodes, int dummy, int type[] )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    if ( what == GmfSolAtVertices ) _libData->SetNbReqVertices( nbNodes );
    return;
#endif
  }
  ::GmfSetKwd(iMesh, what, nbNodes, dummy, type );
}

//================================================================================
/*!
 * \brief Add solution data
 */
//================================================================================

void MG_Tetra_API::GmfSetLin(int iMesh, GmfKwdCod what, double vals[])
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    _libData->AddSizeAtNode( vals[0] );
    return;
#endif
  }
  ::GmfSetLin(iMesh, what, vals);
}

//================================================================================
/*!
 * \brief Add edge nodes
 */
//================================================================================

void MG_Tetra_API::GmfSetLin(int iMesh, GmfKwdCod what, int node1, int node2, int domain )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    _libData->AddEdgeNodes( node1, node2, domain );
    return;
#endif
  }
  ::GmfSetLin(iMesh, what, node1, node2, domain );
}

//================================================================================
/*!
 * \brief Add a 'required' flag
 */
//================================================================================

void MG_Tetra_API::GmfSetLin(int iMesh, GmfKwdCod what, int id )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    return;
#endif
  }
  ::GmfSetLin(iMesh, what, id );
}

//================================================================================
/*!
 * \brief Add triangle nodes
 */
//================================================================================

void MG_Tetra_API::GmfSetLin(int iMesh, GmfKwdCod what, int node1, int node2, int node3, int domain )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    _libData->AddTriaNodes( node1, node2, node3, domain );
    return;
#endif
  }
  ::GmfSetLin(iMesh, what, node1, node2, node3, domain );
}

//================================================================================
/*!
 * \brief Add tetra nodes
 */
//================================================================================

void MG_Tetra_API::GmfSetLin(int iMesh, GmfKwdCod what,
                             int node1, int node2, int node3, int node4, int domain )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    _libData->AddTetraNodes( node1, node2, node3, node4, domain );
    return;
#endif
  }
  ::GmfSetLin(iMesh, what, node1, node2, node3, node4, domain );
}

//================================================================================
/*!
 * \brief Close a file
 */
//================================================================================

void MG_Tetra_API::GmfCloseMesh( int iMesh )
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    return;
#endif
  }
  ::GmfCloseMesh( iMesh );
  _openFiles.erase( iMesh );
}

//================================================================================
/*!
 * \brief Return true if the log is not empty
 */
//================================================================================

bool MG_Tetra_API::HasLog()
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    return !_libData->GetErrors().empty();
#endif
  }
  SMESH_File file( _logFile );
  return file.size() > 0;
}

//================================================================================
/*!
 * \brief Return log contents
 */
//================================================================================

std::string MG_Tetra_API::GetLog()
{
  if ( _useLib ) {
#ifdef USE_MG_LIBS
    const std::string& err = _libData->GetErrors();
    if ( !_logFile.empty() && !err.empty() )
    {
      SMESH_File file( _logFile, /*openForReading=*/false );
      file.openForWriting();
      file.write( err.c_str(), err.size() );
    }
    return err;
#endif
  }
  SMESH_File file( _logFile );
  return file.getPos();
}
