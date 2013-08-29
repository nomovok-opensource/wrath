
/*

  Tessellation only GLU library with WRATH modifications
   - all but tessellation removed
   - only 2D points
   - arbitary winding rule
   - prefix public functions with wrath_ 
   - prefix public macros with WRATH_
   - change source and header files to cpp and hpp extension
   - so that API header allows for C++ niceties of overloading
   - TODO: use floats instead of doubles 

  whose original liscense is below

 */

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

#ifndef __wrath_tess_glu_hpp__
#define __wrath_tess_glu_hpp__

#include "WRATHConfig.hpp"


/*************************************************************/

/* Boolean */
#define WRATH_GLU_FALSE                          0
#define WRATH_GLU_TRUE                           1
typedef unsigned char	WRATH_GLUboolean;
typedef unsigned int    WRATH_GLUenum;


#define WRATH_GLU_INVALID_ENUM                   100900
#define WRATH_GLU_INVALID_VALUE                  100901
#define WRATH_GLU_OUT_OF_MEMORY                  100902
#define WRATH_GLU_INCOMPATIBLE_GL_VERSION        100903
#define WRATH_GLU_INVALID_OPERATION              100904


/* Primitive type, avoid dependence on GL headers */
#define WRATH_GLU_LINE_LOOP 0x0002
#define WRATH_GLU_TRIANGLES 0x0004
#define WRATH_GLU_TRIANGLE_STRIP 0x0005
#define WRATH_GLU_TRIANGLE_FAN 0x0006


/* TessCallback */
#define WRATH_GLU_TESS_BEGIN                     100100
#define WRATH_GLU_BEGIN                          100100
typedef void (*wrath_glu_tess_function_begin)( WRATH_GLUenum type, int winding_number);


#define WRATH_GLU_TESS_VERTEX                    100101
#define WRATH_GLU_VERTEX                         100101
typedef void (*wrath_glu_tess_function_vertex)(void *vertex_data);

#define WRATH_GLU_TESS_END                       100102
#define WRATH_GLU_END                            100102
typedef void (*wrath_glu_tess_function_end)(void);

#define WRATH_GLU_TESS_ERROR                     100103
typedef void (*wrath_glu_tess_function_error)(WRATH_GLUenum errnum);

#define WRATH_GLU_TESS_EDGE_FLAG                 100104
#define WRATH_GLU_EDGE_FLAG                      100104
typedef void (*wrath_glu_tess_function_edge)(WRATH_GLUboolean boundaryEdge);

#define WRATH_GLU_TESS_COMBINE                   100105
typedef void (*wrath_glu_tess_function_combine)(double coords[3], void *data[4],
                                                float weight[4], void **outData);

#define WRATH_GLU_TESS_BEGIN_DATA                100106
typedef void (*wrath_glu_tess_function_begin_data)(WRATH_GLUenum type, int winding_number, void *polygon_data);

#define WRATH_GLU_TESS_VERTEX_DATA               100107
typedef void (*wrath_glu_tess_function_vertex_data)(void *vertex_data, void *polygon_data);

#define WRATH_GLU_TESS_END_DATA                  100108
typedef void (*wrath_glu_tess_function_end_data)(void *polygon_data);

#define WRATH_GLU_TESS_ERROR_DATA                100109
typedef void (*wrath_glu_tess_function_error_data)(WRATH_GLUenum errnum, void *polygon_data);

#define WRATH_GLU_TESS_EDGE_FLAG_DATA            100110
typedef void (*wrath_glu_tess_function_edge_data)(WRATH_GLUboolean boundaryEdge, void *polygon_data);

#define WRATH_GLU_TESS_COMBINE_DATA              100111
typedef void (*wrath_glu_tess_function_combine_data)(double coords[3], void *data[4],
                                                     float weight[4], void **outData,
                                                     void *polygon_data);

  /*
    additions from WRATH, use a call back for the winding rule
    function signature is:
    
    WRATH_GLU_TESS_WINDING_CALLBACK--> WRATH_GLUboolean fill_region(int winding_number) 
    WRATH_GLU_TESS_WINDING_CALLBACK_DATA --> WRATH_GLUboolean fill_region(int winding_number, void *polygon_data)

    return GL_TRUE if the winding_number dictates to fill the region
    return GL_FALSE if the winding_number dictates to not fill the region
  */ 
#define WRATH_GLU_TESS_WINDING_CALLBACK 200100
typedef WRATH_GLUboolean (*wrath_glu_tess_function_winding)(int winding_number);

#define WRATH_GLU_TESS_WINDING_CALLBACK_DATA 200101
typedef WRATH_GLUboolean (*wrath_glu_tess_function_winding_data)(int winding_number, void *polygon_data);

/* TessContour */
#define WRATH_GLU_CW                             100120
#define WRATH_GLU_CCW                            100121
#define WRATH_GLU_INTERIOR                       100122
#define WRATH_GLU_EXTERIOR                       100123
#define WRATH_GLU_UNKNOWN                        100124

/* TessProperty */
#define WRATH_GLU_TESS_WINDING_RULE              100140
#define WRATH_GLU_TESS_BOUNDARY_ONLY             100141
#define WRATH_GLU_TESS_TOLERANCE                 100142

/* TessError */
#define WRATH_GLU_TESS_ERROR1                    100151
#define WRATH_GLU_TESS_ERROR2                    100152
#define WRATH_GLU_TESS_ERROR3                    100153
#define WRATH_GLU_TESS_ERROR4                    100154
#define WRATH_GLU_TESS_ERROR5                    100155
#define WRATH_GLU_TESS_ERROR6                    100156
#define WRATH_GLU_TESS_ERROR7                    100157
#define WRATH_GLU_TESS_ERROR8                    100158
#define WRATH_GLU_TESS_MISSING_BEGIN_POLYGON     100151
#define WRATH_GLU_TESS_MISSING_BEGIN_CONTOUR     100152
#define WRATH_GLU_TESS_MISSING_END_POLYGON       100153
#define WRATH_GLU_TESS_MISSING_END_CONTOUR       100154
#define WRATH_GLU_TESS_COORD_TOO_LARGE           100155
#define WRATH_GLU_TESS_NEED_COMBINE_CALLBACK     100156

/* TessWinding */

/*
#define WRATH_GLU_TESS_WINDING_ODD               100130
#define WRATH_GLU_TESS_WINDING_NONZERO           100131
#define WRATH_GLU_TESS_WINDING_POSITIVE          100132
#define WRATH_GLU_TESS_WINDING_NEGATIVE          100133
#define WRATH_GLU_TESS_WINDING_ABS_GEQ_TWO       100134

// we use a call back for the winding rule.
*/

/*************************************************************/



class wrath_GLUtesselator;

typedef wrath_GLUtesselator wrath_GLUtesselatorObj;
typedef wrath_GLUtesselator wrath_GLUtriangulatorObj;

#define WRATH_GLU_TESS_MAX_COORD 1.0e150

/* Internal convenience typedefs */
typedef void (*WRATH_GLUfuncptr)(void);


#if defined(WRATH_MALLOC_DEBUG) or defined(WRATH_NEW_DEBUG)
class wrath_GLUTessHarmlessParentesis
{
public:
  wrath_GLUTessHarmlessParentesis(wrath_GLUtesselator *p):
    m_p(p)
  {}

  wrath_GLUtesselator*
  operator()(void) const
  {
    return m_p;
  }

private:
  wrath_GLUtesselator *m_p;
};

wrath_GLUtesselator* wrath_gluNewTessTracked(const char *file, int line);
void wrath_gluDeleteTessTracked(wrath_GLUtesselator* tess, const char *file, int line);

#define wrath_gluNewTess wrath_GLUTessHarmlessParentesis(wrath_gluNewTessTracked(__FILE__, __LINE__))
#define wrath_gluDeleteTess(X) wrath_gluDeleteTessTracked(X, __FILE__, __LINE__)

#else

wrath_GLUtesselator* wrath_gluNewTess (void);
void wrath_gluDeleteTess (wrath_GLUtesselator* tess);

#endif

void wrath_gluTessBeginContour (wrath_GLUtesselator* tess);
void wrath_gluTessBeginPolygon (wrath_GLUtesselator* tess, void* data);
void wrath_gluTessEndContour (wrath_GLUtesselator* tess);
void wrath_gluTessEndPolygon (wrath_GLUtesselator* tess);

/*
  Note: if the WRATH_GLU_TESS_EDGE_FLAG_DATA or WRATH_GLU_TESS_EDGE_FLAG
  is non-NULL, then triangle fans and strips are NOT emitted.
 */
void wrath_gluTessCallback (wrath_GLUtesselator* tess, WRATH_GLUenum which, WRATH_GLUfuncptr CallBackFunc);

  /*
    unlike original GLU, the coordinates are _copied_ thus no need to keep in scope the values.
   */
void wrath_gluTessVertex (wrath_GLUtesselator* tess, const double location[3], void* data);

/*
  set and fetch the merging tolerance
 */
float wrath_gluTessPropertyTolerance(wrath_GLUtesselator* tess);
void wrath_gluTessPropertyTolerance(wrath_GLUtesselator *tess, double value);

/*
  set and fetch weather or not to only provide line loops
  decribing the boundary, using WRATH_GLU_FALSE and WRATH_GLU_TRUE.
  If value is WRATH_GLU_TRUE, then the boundary is provided
  only as a sequence of line loops. If value is WRATH_GLU_FALSE, then
  tessellates according to the fill rule.
 */
int wrath_gluGetTessPropertyBoundaryOnly(wrath_GLUtesselator *tess);
void wrath_gluTessPropertyBoundaryOnly(wrath_GLUtesselator *tess, int value);


#define WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(which, type, label)                \
  inline void wrath_gluTessCallback##label(wrath_GLUtesselator *tess, type f) { wrath_gluTessCallback(tess, which, (WRATH_GLUfuncptr)(f)); }


WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_BEGIN, wrath_glu_tess_function_begin, Begin)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_VERTEX, wrath_glu_tess_function_vertex, Vertex)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_END, wrath_glu_tess_function_end, End)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_ERROR, wrath_glu_tess_function_error, Error)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_EDGE_FLAG, wrath_glu_tess_function_edge, EdgeFlag)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_COMBINE, wrath_glu_tess_function_combine, Combine)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_WINDING_CALLBACK, wrath_glu_tess_function_winding, FillRule)

WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_BEGIN_DATA, wrath_glu_tess_function_begin_data, Begin)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_VERTEX_DATA, wrath_glu_tess_function_vertex_data, Vertex)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_END_DATA, wrath_glu_tess_function_end_data, End)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_ERROR_DATA, wrath_glu_tess_function_error_data, Error)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_EDGE_FLAG_DATA, wrath_glu_tess_function_edge_data, EdgeFlag)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_COMBINE_DATA, wrath_glu_tess_function_combine_data, Combine)
WRATH_GLU_TESS_TYPE_SAFE_CALL_BACK(WRATH_GLU_TESS_WINDING_CALLBACK_DATA, wrath_glu_tess_function_winding_data, FillRule)




#endif /* __tess_glu_h__ */
