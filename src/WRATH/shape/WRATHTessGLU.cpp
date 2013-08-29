/*! 
 * \file WRATHTessGLU.cpp
 * \brief file WRATHTessGLU.cpp
 * 
 * Copyright 2013 by Nomovok Ltd.
 * 
 * Contact: info@nomovok.com
 * 
 * This Source Code Form is subject to the
 * terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with
 * this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 * 
 * \author Kevin Rogovin <kevin.rogovin@nomovok.com>
 * 
 */



#include "WRATHConfig.hpp"
#include "WRATHTessGLU.hpp"

#include "wrath-glu-tess.hpp"

namespace
{
  enum WRATHTessGLU::primitive_type
  primitive_type_from_glu_enum(WRATH_GLUenum type)
  {
    switch(type)
      {
      default:
        WRATHwarning("Unreconized primitive type: 0x" << std::hex 
                     << static_cast<unsigned int>(type)
                     << std::dec << ", making it triangles");

      case WRATH_GLU_TRIANGLES:
        return WRATHTessGLU::triangles;
        break;

      case WRATH_GLU_TRIANGLE_FAN:
        return WRATHTessGLU::triangle_fan;
        break;

      case WRATH_GLU_TRIANGLE_STRIP:
        return WRATHTessGLU::triangle_strip;
        break;

      case WRATH_GLU_LINE_LOOP:
        return WRATHTessGLU::line_loop;
        break;
      }
  }

  enum WRATHTessGLU::error_type
  error_type_from_glu_enum(WRATH_GLUenum v)
  {
    return v==WRATH_GLU_TESS_COORD_TOO_LARGE?
      WRATHTessGLU::coordinate_too_large:
      WRATHTessGLU::tessellation_error;
  }

  void 
  begin_callBack(WRATH_GLUenum type, int winding_number, void *polygonData)
  {
    WRATHTessGLUPrivate::polygon_element *p;
    p=static_cast<WRATHTessGLUPrivate::polygon_element*>(polygonData);
    p->m_tess->on_begin_primitive(primitive_type_from_glu_enum(type), winding_number, p->m_polygon);
  }

  void
  edgeflag_callBack(WRATH_GLUboolean boundaryEdge, void *polygonData)
  {
    enum WRATHTessGLU::edge_type tp;
    WRATHTessGLUPrivate::polygon_element *p;
    p=static_cast<WRATHTessGLUPrivate::polygon_element*>(polygonData);

    tp=(boundaryEdge)?
      WRATHTessGLU::exterior_edge:
      WRATHTessGLU::interior_edge;

    p->m_tess->edge_flag(tp, p->m_polygon);
  }

  void 
  vertex_callBack(void *data, void *polygonData)
  {
    
    WRATHTessGLUPrivate::polygon_element *p;
    p=static_cast<WRATHTessGLUPrivate::polygon_element*>(polygonData);

    p->m_tess->on_emit_vertex(data, p->m_polygon);
  }

  void
  end_callBack(void *polygonData)
  {
    WRATHTessGLUPrivate::polygon_element *p;
    p=static_cast<WRATHTessGLUPrivate::polygon_element*>(polygonData);

    p->m_tess->on_end_primitive(p->m_polygon);
  }

  void
  error_callBack(WRATH_GLUenum errnum, void *polygonData)
  {
    WRATHTessGLUPrivate::polygon_element *p;
    p=static_cast<WRATHTessGLUPrivate::polygon_element*>(polygonData);

    p->m_tess->on_error(error_type_from_glu_enum(errnum), p->m_polygon);
  }

  void
  combine_callBack(double coords[3], void *data[4],
                   float weight[4], void **outData,
                   void *polygonData)
  {
    WRATHTessGLUPrivate::polygon_element *p;
    
    int count(0);
    vecN<void*, 4> vdata(NULL, NULL, NULL, NULL);
    vec4 fdata(0.0f, 0.0f, 0.0f, 0.0f);
    
    for(int i=0; i<4; ++i)
      {
        if(data[i]!=NULL)
          {
            vdata[count]=data[i];
            fdata[count]=weight[i];
            ++count;
          }
      }


    p=static_cast<WRATHTessGLUPrivate::polygon_element*>(polygonData);
    *outData=p->m_tess->on_combine_vertex(vec2(coords[0], coords[1]),
                                          const_c_array<void*>(vdata).sub_array(0, count),
                                          const_c_array<float>(fdata).sub_array(0, count),
                                          p->m_polygon);
  }

  WRATH_GLUboolean
  winding_callBack(int winding_number, void *polygonData)
  {
    WRATHTessGLUPrivate::polygon_element *p;
    bool v;

    p=static_cast<WRATHTessGLUPrivate::polygon_element*>(polygonData);

    v=p->m_tess->fill_region(winding_number, p->m_polygon);
    return (v)?
      WRATH_GLU_TRUE:
      WRATH_GLU_FALSE;
      
  }


}

WRATHTessGLU::
WRATHTessGLU(enum tessellation_type ptype)
{
  m_private_data=wrath_gluNewTess();

  /*
    register call backs:
   */
  wrath_GLUtesselator *tess(static_cast<wrath_GLUtesselator*>(m_private_data));

  wrath_gluTessCallbackBegin(tess, &begin_callBack); //WRATH_GLU_TESS_BEGIN_DATA
  wrath_gluTessCallbackVertex(tess, &vertex_callBack); //WRATH_GLU_TESS_VERTEX_DATA
  wrath_gluTessCallbackEnd(tess, &end_callBack); //WRATH_GLU_TESS_END_DATA
  wrath_gluTessCallbackError(tess, &error_callBack); //WRATH_GLU_TESS_ERROR_DATA
  wrath_gluTessCallbackCombine(tess, &combine_callBack); //WRATH_GLU_TESS_COMBINE_DATA
  wrath_gluTessCallbackFillRule(tess, &winding_callBack); //WRATH_GLU_TESS_COMBINE_DATA

 
  switch(ptype)
    {
    default:
      WRATHwarning("\nBad tessellation_type: " << static_cast<unsigned int>(ptype)
                   << " reevaluated as tessellate_triangles_only\n");

    case tessellate_triangles_only:
      wrath_gluTessCallbackEdgeFlag(tess, &edgeflag_callBack); //WRATH_GLU_TESS_EDGE_FLAG_DATA
      wrath_gluTessPropertyBoundaryOnly(tess, WRATH_GLU_FALSE);
      break;

    case tessellate_any_triangles_type:
      {
        wrath_glu_tess_function_edge_data no_edge_function(NULL);
        wrath_gluTessCallbackEdgeFlag(tess, no_edge_function);
        wrath_gluTessPropertyBoundaryOnly(tess, WRATH_GLU_FALSE);
      }
      break;

    case tessellate_boundary_only:
      wrath_gluTessCallbackEdgeFlag(tess, &edgeflag_callBack); //WRATH_GLU_TESS_EDGE_FLAG_DATA
      wrath_gluTessPropertyBoundaryOnly(tess, WRATH_GLU_TRUE);
      break;
    }
}

WRATHTessGLU::
~WRATHTessGLU(void)
{
  WRATHassert(m_private_data!=NULL);

  wrath_gluDeleteTess(static_cast<wrath_GLUtesselator*>(m_private_data));
  m_private_data=NULL;
}

void
WRATHTessGLU::
begin_polygon(void *polygon_data)
{
  WRATHassert(m_private_data!=NULL);
  wrath_GLUtesselator *tess(static_cast<wrath_GLUtesselator*>(m_private_data));

  /*
    create and save a polygon object.
   */
  m_polygons.push_back( WRATHTessGLUPrivate::polygon_element(this, polygon_data));

  wrath_gluTessBeginPolygon(tess, &m_polygons.back());
}

void
WRATHTessGLU::
begin_contour(void)
{
  WRATHassert(m_private_data!=NULL);
  wrath_GLUtesselator *tess(static_cast<wrath_GLUtesselator*>(m_private_data));
  
  WRATHassert(!m_polygons.empty());
  wrath_gluTessBeginContour(tess);
}

void
WRATHTessGLU::
add_vertex(vec2 position, void *vertex_data)
{
  WRATHassert(m_private_data!=NULL);

  double values[3];
  wrath_GLUtesselator *tess(static_cast<wrath_GLUtesselator*>(m_private_data));

  values[0]=position.x();
  values[1]=position.y();
  values[2]=0.0;

  wrath_gluTessVertex(tess, values, vertex_data);
}

void
WRATHTessGLU::
end_contour(void)
{
  WRATHassert(m_private_data!=NULL);

  wrath_GLUtesselator *tess(static_cast<wrath_GLUtesselator*>(m_private_data));
  wrath_gluTessEndContour(tess);
}

void
WRATHTessGLU::
end_polygon(void)
{
  WRATHassert(m_private_data!=NULL);
  wrath_GLUtesselator *tess(static_cast<wrath_GLUtesselator*>(m_private_data));
  wrath_gluTessEndPolygon(tess);
}
