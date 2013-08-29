/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */
/*
** Author: Eric Veach, July 1994.
**
*/
#include "WRATHConfig.hpp"
#include "WRATHMutex.hpp"
#include "WRATHStaticInit.hpp"
#include "gluos.hpp"
#include <stddef.h>
#include <assert.h>
#include <map>
#include <setjmp.h>
#include "memalloc.hpp"
#include "tess.hpp"
#include "mesh.hpp"
#include "normal.hpp"
#include "sweep.hpp"
#include "tessmono.hpp"
#include "render.hpp"

#define WRATH_GLU_TESS_DEFAULT_TOLERANCE 0.0
#define WRATH_GLU_TESS_MESH		100112	/* void (*)(GLUmesh *mesh)	    */

#define IGNORE(X) do { (void)(X); } while (0)


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#if defined(WRATH_MALLOC_DEBUG) or defined(WRATH_NEW_DEBUG)
namespace
{
  typedef std::pair<const char*,int> file_list_str;

  class tess_tracker:public std::map<wrath_GLUtesselator*, file_list_str>
  {
  public:
    WRATHMutex m_mutex;

    ~tess_tracker()
    {
      if(!empty())
        {
          std::cerr << "\n\nTracked allocated GLU-Tessellator objects remaining:\n"; 
          for(std::map<wrath_GLUtesselator*, file_list_str>::const_iterator
                i=begin(), e=end(); i!=e; ++i)
            {
               std::cerr << i->first << "[" << i->second.first
                         << "," << i->second.second << "]\n";
            }
        }
      
    }
  };

  tess_tracker&
  tracker(void)
  {
    WRATHStaticInit();
    static tess_tracker v;
    return v;
  }
}
#endif


/*ARGSUSED*/ static void REGALWRATH_GLU_CALL noBegin( WRATH_GLUenum type, int winding_number ) 
{ 
  IGNORE(type); 
  IGNORE(winding_number); 
}

/*ARGSUSED*/ static void REGALWRATH_GLU_CALL noEdgeFlag( WRATH_GLUboolean boundaryEdge )
{ 
  IGNORE(boundaryEdge); 
}

/*ARGSUSED*/ static void REGALWRATH_GLU_CALL noVertex( void *data ) 
{ 
  IGNORE(data); 
}


/*ARGSUSED*/ static void REGALWRATH_GLU_CALL noEnd( void ) {}

/*ARGSUSED*/ static void REGALWRATH_GLU_CALL noError( WRATH_GLUenum errnum ) 
{ 
  IGNORE(errnum); 
}

/*ARGSUSED*/ static void REGALWRATH_GLU_CALL noCombine( double coords[3], void *data[4],
                                                        float weight[4], void **dataOut ) 
{ 
  IGNORE(coords); 
  IGNORE(data); 
  IGNORE(weight); 
  IGNORE(dataOut); 
}

/*ARGSUSED*/ static void REGALWRATH_GLU_CALL noMesh( GLUmesh *mesh ) 
{ 
  IGNORE(mesh); 
}


/*ARGSUSED*/ static WRATH_GLUboolean REGALWRATH_GLU_CALL noWinding(int winding_rule) 
{ 
  IGNORE(winding_rule); 
  return winding_rule&1; 
}


/*ARGSUSED*/ void REGALWRATH_GLU_CALL __wrath__gl_noBeginData( WRATH_GLUenum type, int winding_number, 
                                                               void *polygonData ) 
{
  IGNORE(polygonData);
  IGNORE(type); 
  IGNORE(winding_number); 
}

/*ARGSUSED*/ void REGALWRATH_GLU_CALL __wrath__gl_noEdgeFlagData( WRATH_GLUboolean boundaryEdge,
				       void *polygonData )
{
  IGNORE(polygonData);
  IGNORE(boundaryEdge); 
}

/*ARGSUSED*/ void REGALWRATH_GLU_CALL __wrath__gl_noVertexData( void *data,
					      void *polygonData ) 
{
  IGNORE(polygonData);
  IGNORE(data); 
}

/*ARGSUSED*/ void REGALWRATH_GLU_CALL __wrath__gl_noEndData( void *polygonData ) 
{
  IGNORE(polygonData);
}

/*ARGSUSED*/ void REGALWRATH_GLU_CALL __wrath__gl_noErrorData( WRATH_GLUenum errnum,
					     void *polygonData ) 
{
  IGNORE(polygonData);
  IGNORE(errnum); 
}

/*ARGSUSED*/ void REGALWRATH_GLU_CALL __wrath__gl_noCombineData( double coords[3],
					       void *data[4],
					       float weight[4],
					       void **outData,
					       void *polygonData ) 
{
  IGNORE(polygonData);
  IGNORE(coords); 
  IGNORE(data); 
  IGNORE(weight); 
  IGNORE(outData); 
}

/*ARGSUSED*/ WRATH_GLUboolean REGALWRATH_GLU_CALL __wrath__gl_noWindingData(int winding_rule,
                                                                            void *polygonData) 
{ 
  IGNORE(polygonData);
  return winding_rule&1; 
}



/* Half-edges are allocated in pairs (see mesh.c) */
typedef struct { GLUhalfEdge e, eSym; } EdgePair;

#undef	MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#define MAX_FAST_ALLOC	(MAX(sizeof(EdgePair), \
                         MAX(sizeof(GLUvertex),sizeof(GLUface))))


wrath_GLUtesselator * REGALWRATH_GLU_CALL
#if defined(WRATH_MALLOC_DEBUG) or defined(WRATH_NEW_DEBUG)
wrath_gluNewTessTracked(const char *file, int line)
#else
wrath_gluNewTess( void )
#endif
{
  wrath_GLUtesselator *tess;

  /* Only initialize fields which can be changed by the api.  Other fields
   * are initialized where they are used.
   */

  if (memInit( MAX_FAST_ALLOC ) == 0) {
     return 0;			/* out of memory */
  }
  tess = (wrath_GLUtesselator *)memAlloc( sizeof( wrath_GLUtesselator ));
  if (tess == NULL) {
     return 0;			/* out of memory */
  }

  #if defined(WRATH_MALLOC_DEBUG) or defined(WRATH_NEW_DEBUG)
  {
    WRATHAutoLockMutex(tracker().m_mutex);
    tracker()[tess]=file_list_str(file, line);
  }
  #endif


  tess->state = T_DORMANT;

  tess->normal[0] = 0;
  tess->normal[1] = 0;
  tess->normal[2] = 0;

  tess->relTolerance = WRATH_GLU_TESS_DEFAULT_TOLERANCE;
  //tess->windingRule = WRATH_GLU_TESS_WINDING_ODD;
  tess->flagBoundary = FALSE;
  tess->boundaryOnly = FALSE;

  tess->callBegin = &noBegin;
  tess->callEdgeFlag = &noEdgeFlag;
  tess->callVertex = &noVertex;
  tess->callEnd = &noEnd;

  tess->callError = &noError;
  tess->callCombine = &noCombine;
  tess->callMesh = &noMesh;

  tess->callWinding= &noWinding;

  tess->callBeginData= &__wrath__gl_noBeginData;
  tess->callEdgeFlagData= &__wrath__gl_noEdgeFlagData;
  tess->callVertexData= &__wrath__gl_noVertexData;
  tess->callEndData= &__wrath__gl_noEndData;
  tess->callErrorData= &__wrath__gl_noErrorData;
  tess->callCombineData= &__wrath__gl_noCombineData;

  tess->callWindingData= &__wrath__gl_noWindingData;

  tess->polygonData= NULL;

  return tess;
}

static void MakeDormant( wrath_GLUtesselator *tess )
{
  /* Return the tessellator to its original dormant state. */

  if( tess->mesh != NULL ) {
    __wrath__gl_meshDeleteMesh( tess->mesh );
  }
  tess->state = T_DORMANT;
  tess->lastEdge = NULL;
  tess->mesh = NULL;
}

#define RequireState( tess, s )   if( tess->state != s ) GotoState(tess,s)

static void GotoState( wrath_GLUtesselator *tess, enum TessState newState )
{
  while( tess->state != newState ) {
    /* We change the current state one level at a time, to get to
     * the desired state.
     */
    if( tess->state < newState ) {
      switch( tess->state ) {
      case T_DORMANT:
	CALL_ERROR_OR_ERROR_DATA( WRATH_GLU_TESS_MISSING_BEGIN_POLYGON );
	wrath_gluTessBeginPolygon( tess, NULL );
	break;
      case T_IN_POLYGON:
	CALL_ERROR_OR_ERROR_DATA( WRATH_GLU_TESS_MISSING_BEGIN_CONTOUR );
	wrath_gluTessBeginContour( tess );
	break;
      default:
	 ;
      }
    } else {
      switch( tess->state ) {
      case T_IN_CONTOUR:
	CALL_ERROR_OR_ERROR_DATA( WRATH_GLU_TESS_MISSING_END_CONTOUR );
	wrath_gluTessEndContour( tess );
	break;
      case T_IN_POLYGON:
	CALL_ERROR_OR_ERROR_DATA( WRATH_GLU_TESS_MISSING_END_POLYGON );
	/* wrath_gluTessEndPolygon( tess ) is too much work! */
	MakeDormant( tess );
	break;
      default:
	 ;
      }
    }
  }
}


void REGALWRATH_GLU_CALL
#if  defined(WRATH_MALLOC_DEBUG) or defined(WRATH_NEW_DEBUG)
wrath_gluDeleteTessTracked(wrath_GLUtesselator* tess, const char *file, int line)
#else
wrath_gluDeleteTess( wrath_GLUtesselator *tess )
#endif
{
  RequireState( tess, T_DORMANT );
  memFree( tess );

  #if defined(WRATH_MALLOC_DEBUG) or defined(WRATH_NEW_DEBUG)
  {
    WRATHAutoLockMutex(tracker().m_mutex);

    tess_tracker::iterator iter;
    iter=tracker().find(tess);
    if(iter==tracker().end())
      {
         std::cerr << "Deletion from [" << file << ", " 
                   << line << "] of untracked GLU-Tessellator" 
                   << tess << "\n";
      }
    else
      {
        tracker().erase(iter);
      }

  }
  #endif

}

void REGALWRATH_GLU_CALL
wrath_gluTessPropertyTolerance(wrath_GLUtesselator *tess, double value)
{
  if( value < 0.0 || value > 1.0 ) return;

  tess->relTolerance = value;
}

float REGALWRATH_GLU_CALL
wrath_gluGetTessPropertyTolerance(wrath_GLUtesselator *tess)
{
  return tess->relTolerance;
}


void REGALWRATH_GLU_CALL
wrath_gluTessPropertyBoundaryOnly(wrath_GLUtesselator *tess, int value)
{
  tess->boundaryOnly = (value != 0);
}

int REGALWRATH_GLU_CALL
wrath_gluGetTessPropertyBoundaryOnly(wrath_GLUtesselator *tess)
{
  return tess->boundaryOnly;
}

WRATH_GLUboolean
call_tess_winding_or_winding_data_implement(wrath_GLUtesselator *tess, int a)
{
  if (tess->callWindingData != &__wrath__gl_noWindingData) 
    return (*tess->callWindingData)(a, tess->polygonData); 
   else 
     return (*tess->callWinding)(a);
}




void REGALWRATH_GLU_CALL
wrath_gluTessNormal( wrath_GLUtesselator *tess, double x, double y, double z )
{
  tess->normal[0] = x;
  tess->normal[1] = y;
  tess->normal[2] = z;
}

void REGALWRATH_GLU_CALL
wrath_gluTessCallback( wrath_GLUtesselator *tess, WRATH_GLUenum which, WRATH_GLUfuncptr fn)
{
  switch( which ) {
  case WRATH_GLU_TESS_BEGIN:
    tess->callBegin = (fn == NULL) ? &noBegin : (void (REGALWRATH_GLU_CALL *)(WRATH_GLUenum, int)) fn;
    return;
  case WRATH_GLU_TESS_BEGIN_DATA:
    tess->callBeginData = (fn == NULL) ?
	&__wrath__gl_noBeginData : (void (REGALWRATH_GLU_CALL *)(WRATH_GLUenum, int, void *)) fn;
    return;
  case WRATH_GLU_TESS_EDGE_FLAG:
    tess->callEdgeFlag = (fn == NULL) ? &noEdgeFlag :
					(void (REGALWRATH_GLU_CALL *)(WRATH_GLUboolean)) fn;
    /* If the client wants boundary edges to be flagged,
     * we render everything as separate triangles (no strips or fans).
     */
    tess->flagBoundary = (fn != NULL);
    return;
  case WRATH_GLU_TESS_EDGE_FLAG_DATA:
    tess->callEdgeFlagData= (fn == NULL) ?
	&__wrath__gl_noEdgeFlagData : (void (REGALWRATH_GLU_CALL *)(WRATH_GLUboolean, void *)) fn;
    /* If the client wants boundary edges to be flagged,
     * we render everything as separate triangles (no strips or fans).
     */
    tess->flagBoundary = (fn != NULL);
    return;
  case WRATH_GLU_TESS_VERTEX:
    tess->callVertex = (fn == NULL) ? &noVertex :
				      (void (REGALWRATH_GLU_CALL *)(void *)) fn;
    return;
  case WRATH_GLU_TESS_VERTEX_DATA:
    tess->callVertexData = (fn == NULL) ?
	&__wrath__gl_noVertexData : (void (REGALWRATH_GLU_CALL *)(void *, void *)) fn;
    return;
  case WRATH_GLU_TESS_END:
    tess->callEnd = (fn == NULL) ? &noEnd : (void (REGALWRATH_GLU_CALL *)(void)) fn;
    return;
  case WRATH_GLU_TESS_END_DATA:
    tess->callEndData = (fn == NULL) ? &__wrath__gl_noEndData :
				       (void (REGALWRATH_GLU_CALL *)(void *)) fn;
    return;
  case WRATH_GLU_TESS_ERROR:
    tess->callError = (fn == NULL) ? &noError : (void (REGALWRATH_GLU_CALL *)(WRATH_GLUenum)) fn;
    return;
  case WRATH_GLU_TESS_ERROR_DATA:
    tess->callErrorData = (fn == NULL) ?
	&__wrath__gl_noErrorData : (void (REGALWRATH_GLU_CALL *)(WRATH_GLUenum, void *)) fn;
    return;
  case WRATH_GLU_TESS_COMBINE:
    tess->callCombine = (fn == NULL) ? &noCombine :
	(void (REGALWRATH_GLU_CALL *)(double [3],void *[4], float [4], void ** )) fn;
    return;
  case WRATH_GLU_TESS_COMBINE_DATA:
    tess->callCombineData = (fn == NULL) ? &__wrath__gl_noCombineData :
					   (void (REGALWRATH_GLU_CALL *)(double [3],
						     void *[4],
						     float [4],
						     void **,
						     void *)) fn;
    return;
  case WRATH_GLU_TESS_MESH:
    tess->callMesh = (fn == NULL) ? &noMesh : (void (REGALWRATH_GLU_CALL *)(GLUmesh *)) fn;
    return;

  case WRATH_GLU_TESS_WINDING_CALLBACK:
    tess->callWinding=(fn==NULL)? &noWinding: (WRATH_GLUboolean (REGALWRATH_GLU_CALL *)(int)) fn;
    return;

  case WRATH_GLU_TESS_WINDING_CALLBACK_DATA:
    tess->callWindingData=(fn==NULL)? &__wrath__gl_noWindingData: (WRATH_GLUboolean (REGALWRATH_GLU_CALL *)(int, void*)) fn;
    return;

  default:
    CALL_ERROR_OR_ERROR_DATA( WRATH_GLU_INVALID_ENUM );
    return;
  }
}

static int AddVertex( wrath_GLUtesselator *tess, double coords[3], void *data )
{
  GLUhalfEdge *e;

  e = tess->lastEdge;
  if( e == NULL ) {
    /* Make a self-loop (one vertex, one edge). */

    e = __wrath__gl_meshMakeEdge( tess->mesh );
    if (e == NULL) return 0;
    if ( !__wrath__gl_meshSplice( e, e->Sym ) ) return 0;
  } else {
    /* Create a new vertex and edge which immediately follow e
     * in the ordering around the left face.
     */
    if (__wrath__gl_meshSplitEdge( e ) == NULL) return 0;
    e = e->Lnext;
  }

  /* The new vertex is now e->Org. */
  e->Org->data = data;
  e->Org->coords[0] = coords[0];
  e->Org->coords[1] = coords[1];
  e->Org->coords[2] = coords[2];

  /* The winding of an edge says how the winding number changes as we
   * cross from the edge''s right face to its left face.  We add the
   * vertices in such an order that a CCW contour will add +1 to
   * the winding number of the region inside the contour.
   */
  e->winding = 1;
  e->Sym->winding = -1;

  tess->lastEdge = e;

  return 1;
}


static void CacheVertex( wrath_GLUtesselator *tess, double coords[3], void *data )
{
  CachedVertex *v = &tess->cache[tess->cacheCount];

  v->data = data;
  v->coords[0] = coords[0];
  v->coords[1] = coords[1];
  v->coords[2] = coords[2];
  ++tess->cacheCount;
}


static int EmptyCache( wrath_GLUtesselator *tess )
{
  CachedVertex *v = tess->cache;
  CachedVertex *vLast;

  tess->mesh = __wrath__gl_meshNewMesh();
  if (tess->mesh == NULL) return 0;

  for( vLast = v + tess->cacheCount; v < vLast; ++v ) {
    if ( !AddVertex( tess, v->coords, v->data ) ) return 0;
  }
  tess->cacheCount = 0;
  tess->emptyCache = FALSE;

  return 1;
}


void REGALWRATH_GLU_CALL
wrath_gluTessVertex( wrath_GLUtesselator *tess, const double coords[3], void *data )
{
  int i, tooLarge = FALSE;
  double x, clamped[3];

  RequireState( tess, T_IN_CONTOUR );

  if( tess->emptyCache ) {
    if ( !EmptyCache( tess ) ) {
       CALL_ERROR_OR_ERROR_DATA( WRATH_GLU_OUT_OF_MEMORY );
       return;
    }
    tess->lastEdge = NULL;
  }
  for( i = 0; i < 3; ++i ) {
    x = coords[i];
    if( x < - WRATH_GLU_TESS_MAX_COORD ) {
      x = - WRATH_GLU_TESS_MAX_COORD;
      tooLarge = TRUE;
    }
    if( x > WRATH_GLU_TESS_MAX_COORD ) {
      x = WRATH_GLU_TESS_MAX_COORD;
      tooLarge = TRUE;
    }
    clamped[i] = x;
  }
  if( tooLarge ) {
    CALL_ERROR_OR_ERROR_DATA( WRATH_GLU_TESS_COORD_TOO_LARGE );
  }

  if( tess->mesh == NULL ) {
    if( tess->cacheCount < TESS_MAX_CACHE ) {
      CacheVertex( tess, clamped, data );
      return;
    }
    if ( !EmptyCache( tess ) ) {
       CALL_ERROR_OR_ERROR_DATA( WRATH_GLU_OUT_OF_MEMORY );
       return;
    }
  }
  if ( !AddVertex( tess, clamped, data ) ) {
       CALL_ERROR_OR_ERROR_DATA( WRATH_GLU_OUT_OF_MEMORY );
  }
}


void REGALWRATH_GLU_CALL
wrath_gluTessBeginPolygon( wrath_GLUtesselator *tess, void *data )
{
  RequireState( tess, T_DORMANT );

  tess->state = T_IN_POLYGON;
  tess->cacheCount = 0;
  tess->emptyCache = FALSE;
  tess->mesh = NULL;

  tess->polygonData= data;
}


void REGALWRATH_GLU_CALL
wrath_gluTessBeginContour( wrath_GLUtesselator *tess )
{
  RequireState( tess, T_IN_POLYGON );

  tess->state = T_IN_CONTOUR;
  tess->lastEdge = NULL;
  if( tess->cacheCount > 0 ) {
    /* Just set a flag so we don't get confused by empty contours
     * -- these can be generated accidentally with the obsolete
     * NextContour() interface.
     */
    tess->emptyCache = TRUE;
  }
}


void REGALWRATH_GLU_CALL
wrath_gluTessEndContour( wrath_GLUtesselator *tess )
{
  RequireState( tess, T_IN_CONTOUR );
  tess->state = T_IN_POLYGON;
}

void REGALWRATH_GLU_CALL
wrath_gluTessEndPolygon( wrath_GLUtesselator *tess )
{
  GLUmesh *mesh;

  if (setjmp(tess->env) != 0) { 
     /* come back here if out of memory */
     CALL_ERROR_OR_ERROR_DATA( WRATH_GLU_OUT_OF_MEMORY );
     return;
  }

  RequireState( tess, T_IN_POLYGON );
  tess->state = T_DORMANT;

  if( tess->mesh == NULL ) {
    if( ! tess->flagBoundary && tess->callMesh == &noMesh ) {

      /* Try some special code to make the easy cases go quickly
       * (eg. convex polygons).  This code does NOT handle multiple contours,
       * intersections, edge flags, and of course it does not generate
       * an explicit mesh either.
       */
      if( __wrath__gl_renderCache( tess )) {
	tess->polygonData= NULL;
	return;
      }
    }
    if ( !EmptyCache( tess ) ) longjmp(tess->env,1); /* could've used a label*/
  }

  /* Determine the polygon normal and project vertices onto the plane
   * of the polygon.
   */
  __wrath__gl_projectPolygon( tess );

  /* __wrath__gl_computeInterior( tess ) computes the planar arrangement specified
   * by the given contours, and further subdivides this arrangement
   * into regions.  Each region is marked "inside" if it belongs
   * to the polygon, according to the rule given by tess->windingRule.
   * Each interior region is guaranteed be monotone.
   */
  if ( !__wrath__gl_computeInterior( tess ) ) {
     longjmp(tess->env,1);	/* could've used a label */
  }

  mesh = tess->mesh;
  if( ! tess->fatalError ) {
    int rc = 1;

    /* If the user wants only the boundary contours, we throw away all edges
     * except those which separate the interior from the exterior.
     * Otherwise we tessellate all the regions marked "inside".
     */
    if( tess->boundaryOnly ) {
      rc = __wrath__gl_meshSetWindingNumber( mesh, 1, TRUE );
    } else {
      rc = __wrath__gl_meshTessellateInterior( mesh );
    }
    if (rc == 0) longjmp(tess->env,1);	/* could've used a label */

    __wrath__gl_meshCheckMesh( mesh );

    if( tess->callBegin != &noBegin || tess->callEnd != &noEnd
       || tess->callVertex != &noVertex || tess->callEdgeFlag != &noEdgeFlag
       || tess->callBeginData != &__wrath__gl_noBeginData
       || tess->callEndData != &__wrath__gl_noEndData
       || tess->callVertexData != &__wrath__gl_noVertexData
       || tess->callEdgeFlagData != &__wrath__gl_noEdgeFlagData )
    {
      if( tess->boundaryOnly ) {
	__wrath__gl_renderBoundary( tess, mesh );  /* output boundary contours */
      } else {
	__wrath__gl_renderMesh( tess, mesh );	   /* output strips and fans */
      }
    }
    if( tess->callMesh != &noMesh ) {

      /* Throw away the exterior faces, so that all faces are interior.
       * This way the user doesn't have to check the "inside" flag,
       * and we don't need to even reveal its existence.  It also leaves
       * the freedom for an implementation to not generate the exterior
       * faces in the first place.
       */
      __wrath__gl_meshDiscardExterior( mesh );
      (*tess->callMesh)( mesh );		/* user wants the mesh itself */
      tess->mesh = NULL;
      tess->polygonData= NULL;
      return;
    }
  }
  __wrath__gl_meshDeleteMesh( mesh );
  tess->polygonData= NULL;
  tess->mesh = NULL;
}


/*XXXblythe unused function*/
#if 0
void REGALWRATH_GLU_CALL
gluDeleteMesh( GLUmesh *mesh )
{
  __wrath__gl_meshDeleteMesh( mesh );
}
#endif



/*******************************************************/




