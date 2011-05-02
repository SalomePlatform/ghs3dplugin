

/*----------------------------------------------------------*/
/*															*/
/*						LIBMESH V 5.45						*/
/*															*/
/*----------------------------------------------------------*/
/*															*/
/*	Description:		handle .meshb file format I/O		*/
/*	Author:				Loic MARECHAL						*/
/*	Creation date:		feb 16 2007							*/
/*	Last modification:	sep 27 2010							*/
/*															*/
/*----------------------------------------------------------*/


/*----------------------------------------------------------*/
/* Defines													*/
/*----------------------------------------------------------*/

#define GmfStrSiz 1024
#define GmfMaxTyp 1000
#define GmfMaxKwd 79
#define GmfMshVer 1
#define GmfRead 1
#define GmfWrite 2
#define GmfSca 1
#define GmfVec 2
#define GmfSymMat 3
#define GmfMat 4
#define GmfFloat 1
#define GmfDouble 2

enum GmfKwdCod
{
	GmfReserved1, \
	GmfVersionFormatted, \
	GmfReserved2, \
	GmfDimension, \
	GmfVertices, \
	GmfEdges, \
	GmfTriangles, \
	GmfQuadrilaterals, \
	GmfTetrahedra, \
	GmfPrisms, \
	GmfHexahedra, \
	GmfIterationsAll, \
	GmfTimesAll, \
	GmfCorners, \
	GmfRidges, \
	GmfRequiredVertices, \
	GmfRequiredEdges, \
	GmfRequiredTriangles, \
	GmfRequiredQuadrilaterals, \
	GmfTangentAtEdgeVertices, \
	GmfNormalAtVertices, \
	GmfNormalAtTriangleVertices, \
	GmfNormalAtQuadrilateralVertices, \
	GmfAngleOfCornerBound, \
	GmfTrianglesP2, \
	GmfEdgesP2, \
	GmfSolAtPyramids, \
	GmfQuadrilateralsQ2, \
	GmfISolAtPyramids, \
	GmfReserved6, \
	GmfTetrahedraP2, \
	GmfReserved7, \
	GmfReserved8, \
	GmfHexahedraQ2, \
	GmfReserved9, \
	GmfReserved10, \
	GmfReserved17, \
	GmfReserved18, \
	GmfReserved19, \
	GmfReserved20, \
	GmfReserved21, \
	GmfReserved22, \
	GmfReserved23, \
	GmfReserved24, \
	GmfReserved25, \
	GmfReserved26, \
	GmfPolyhedra, \
	GmfPolygons, \
	GmfReserved29, \
	GmfPyramids, \
	GmfBoundingBox, \
	GmfBody, \
	GmfPrivateTable, \
	GmfReserved33, \
	GmfEnd, \
	GmfReserved34, \
	GmfReserved35, \
	GmfReserved36, \
	GmfReserved37, \
	GmfTangents, \
	GmfNormals, \
	GmfTangentAtVertices, \
	GmfSolAtVertices, \
	GmfSolAtEdges, \
	GmfSolAtTriangles, \
	GmfSolAtQuadrilaterals, \
	GmfSolAtTetrahedra, \
	GmfSolAtPrisms, \
	GmfSolAtHexahedra, \
	GmfDSolAtVertices, \
	GmfISolAtVertices, \
	GmfISolAtEdges, \
	GmfISolAtTriangles, \
	GmfISolAtQuadrilaterals, \
	GmfISolAtTetrahedra, \
	GmfISolAtPrisms, \
	GmfISolAtHexahedra, \
	GmfIterations, \
	GmfTime, \
	GmfReserved38
};


/*----------------------------------------------------------*/
/* External procedures										*/
/*----------------------------------------------------------*/

extern int GmfOpenMesh(const char *, int, ...);
extern int GmfCloseMesh(int);
extern int GmfStatKwd(int, int, ...);
extern int GmfGotoKwd(int, int);
extern int GmfSetKwd(int, int, ...);
extern void GmfGetLin(int, int, ...);
extern void GmfSetLin(int, int, ...);


/*----------------------------------------------------------*/
/* Fortran 77 API											*/
/*----------------------------------------------------------*/

#if defined(F77_NO_UNDER_SCORE)
#define call(x) x
#else
#define call(x) x ## _
#endif


/*----------------------------------------------------------*/
/* Transmesh private API									*/
/*----------------------------------------------------------*/

#ifdef TRANSMESH

extern char *GmfKwdFmt[ GmfMaxKwd + 1 ][4];
extern int GmfCpyLin(int, int, int);

#endif
