/*! 
 * \file WRATHTessGLU.hpp
 * \brief file WRATHTessGLU.hpp
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



#ifndef __WRATH_TESS_GLU_HPP__
#define __WRATH_TESS_GLU_HPP__

#include "WRATHConfig.hpp"
#include "WRATHgl.hpp"
#include "vectorGL.hpp"
#include "c_array.hpp"
#include "WRATHTessGLUPrivate.tcc"
#include <boost/utility.hpp>


/*! \addtogroup Shape
 * @{
 */

/*!\class WRATHTessGLU
  WRATHTessGLU prodives a C++ interface for tessellation
  via GLU's tessellator. The typical use pattern is:
  \code
  // class MyVertexObjectType has a position() method giving position of vertex
  class MyVertexObjectType { ... };

  // class MyPolyonObjectType is a class defining a polygon
  class MyPolyonObjectType { ... };

  // class MyTesser writes to MyPolyonObjectType 
  // the tessellation/triangulation data it generates
  // those writes are implemented via the implementation 
  // of WRATHTessGLU::begin_primitive(), WRATHTessGLU::end_primitive(), 
  // WRATHTessGLU::on_emit_vertex() and WRATHTessGLU::on_combine_vertex(). 
  class MyTesser:public WRATHTessGLU { ... };
  .
  .
  .
  
  void tessellate_polygons(const_c_array<MyPolyonObjectType*> polygons)
  {
    MyTesser tesser;
    for(int i=0; i<polygons.size(); ++i)
      {
        tesser.begin_polygon(polygons[i]);
        for(each outline O of polygon[i])
          {
            for(each MyVertexObjectType* v of O)
              {
                tesser.add_vertex(v->position(), v);
              }
          }
        tesser.end_polygon();        
      }
  }
  \endcode
 */
class WRATHTessGLU:boost::noncopyable
{
public:

  /*!\enum primitive_type
    Enumerations value passed to the virtual
    function begin_primitive()
   */
  enum primitive_type
    {
      /*!
        analogous to GL_TRIANGLES,
        i.e. every 3 vertices determines a triangle
       */
      triangles,
      
      /*!
        analogous to GL_TRIANGLE_FAN, i.e.
        primitive is one triangle fan with the
        1st vertex representing the center of
        the triangle fan
      */
      triangle_fan,

      /*!
        analogous to GL_TRIANGLE_STRIP, i.e.
        primitive is a strip of triangles
      */
      triangle_strip,

      /*!
        analogous to GL_LINE_LOOP, i.e.
        primitive is a sequence of points
        defining a line loop (i.e. essentially
        the vertices of a simple polygon)
       */
      line_loop
    };

  /*!\enum edge_type
    Enumeration to describe the edge of a
    primitive
   */
  enum edge_type
    {
      /*!
        edge on the interior between triangles
       */
      interior_edge,

      /*!
        edge between interior and boundary
       */
      exterior_edge
    };
  

  /*!\enum error_type
    Enumeration to describe error conditions
   */
  enum error_type
    {
      /*!
        Triangulation/tessellation failed error
       */
      tessellation_error,

      /*!
        Coordinate with too large value given/generated
        error
       */
      coordinate_too_large
    };
 
  /*!\enum tessellation_type
    Enumeration to specify how to tessellate/triangule
   */
  enum tessellation_type
    {
      /*!
        Tessellate to triangles only, i.e.
        no triangle fans or strips. In this 
        mode edge flags are supported thus,
        \ref edge_flag() will be called.
       */
      tessellate_triangles_only,

      /*!
        Tessellate to triangles, triangle fans
        and strips. In this mode edge flags
        are not supported, thus the function
        \ref edge_flag() will not be called.
       */
      tessellate_any_triangles_type,

      /*!
        Rather than tessellate the interior of 
        the fill, provide a set of closed contours 
        that separate the interior and exterior
        as a set of line loops. The interior
        contours are oriented clockwise and the
        exterior counter clockwise.
       */
      tessellate_boundary_only
    };

  /*!\fn WRATHTessGLU
    Ctor.
    \param ptype specifies what and how the created object will tessellate.
   */
  explicit
  WRATHTessGLU(enum tessellation_type ptype);

  virtual
  ~WRATHTessGLU();

  /*!\fn void on_begin_primitive
    To be implemented by a derived class to
    note that a primitive is to start
    \param p primitive type of primitive
    \param winding_number winding number of the primitive
    \param polygon_data pointer as recieve by begin_polygon()
                        for the polygon that generated the primitive
  */
  virtual
  void 
  on_begin_primitive(enum primitive_type p, int winding_number, 
                     void *polygon_data)=0;
  
  /*!\fn on_emit_vertex
    To be implemented by a derived class to
    record vertices of a primitive. Function
    is called between on_begin_primitive()/on_end_primitive()
    call pairs to specify the vertices on a primitive.
    \param vertex_data to vertex data, pointer 
           is the exact same value of that which produced
           the vertex from on_combine_vertex() or add_vertex()
    \param polygon_data pointer as recieve by begin_polygon()
                        for the polygon that generated the primitive
   */
  virtual
  void
  on_emit_vertex(void *vertex_data, void *polygon_data)=0;

  /*!\fn void edge_flag
    To be implemented by a derived clas to note when
    the edge type of edges changes. Function is called 
    between on_begin_primitive()/on_end_primitive() call
    pairs.
    \param e new edge type
    \param polygon_data pointer as recieve by begin_polygon()
                        for the polygon that generated the primitive
   */
  virtual
  void
  edge_flag(enum edge_type e, void *polygon_data)=0;

  /*!\fn void* on_combine_vertex
    To be implemented by a derived class to
    create a vertex which is a convex combination
    of up to 4 pre-existing vertices. The values
    of the entries of vertex_source_datums come
    from add_vertex(vec2, void*) and/or return 
    values from previous calls to on_combine_vertex().
    The return value is the vertex data associated
    to the new vertex. 
    \param vertex_position position of the new vertex
    \param vertex_source_datums the vertex dataum of each source vertex
    \param vertex_weights non-negative values which sum to 1.0, the new vertex
                          is the convex combination of the weights against
                          the vertices named in vertex_source_datums. The size
                          of vertex_weights is guarnanteed to be the same 
                          as vertex_source_datums
    \param polygon_data the polygon object as passed to begin_polygon()
   */
  virtual
  void*
  on_combine_vertex(vec2 vertex_position,
                    const_c_array<void*> vertex_source_datums,
                    const_c_array<float> vertex_weights, 
                    void *polygon_data)=0;
                    

  /*!\fn void on_end_primitive
    To be implemented by a derived class to
    note that a primitive has just ended.
    \param polygon_data the polygon object as passed to begin_polygon()
   */
  virtual
  void 
  on_end_primitive(void *polygon_data)=0;

  /*!\fn void on_error
    To be implemented by a derived class to
    note when a tessellation error occurs.
    \param error error value
    \param polygon_data the polygon object as passed to begin_polygon()
   */
  virtual
  void 
  on_error(error_type error, void *polygon_data)=0;

  /*!\fn bool fill_region
    To be implemented by a derived class to dictate
    if a region is to be filled based soley upon
    it's winding number. To return true if the region
    should be filled and return false if the region
    is to not be filled.
    \param winding_number winding number of region
    \param polygon_data the polygon object as passed to begin_polygon()
   */
  virtual
  bool
  fill_region(int winding_number, void *polygon_data)=0;

  /*!\fn begin_polygon 
    Call this function to start the beginning
    of a new polygon element (convex, concave 
    or self-intersecting). Within each 
    begin_polygon() / end_polygon() pairs there
    must be one or more pair calls to  
    begin_contour() / end_contour(). Within
    each begin_contour() / end_contour() there
    must be zero or more calls to add_vertex().
    The vertices added by add_vertex() specify
    a _closed_ polygon, thus the first vertex
    of a contour added with add_vertex() is
    _automatically_ added to the contour's end.

    The tessellation of the polygon is done
    once end_polygon() is called. That tessellation
    data is retrieved by implementing the virtual
    functions:
    - on_begin_primitive()
    - on_emit_vertex()
    - edge_flag()
    - on_combine_vertex()
    - on_end_primitive()
    - on_error()

    Note that this means the tessellation is done
    independently for each polygon.
    \param polygon_data user defiend point value which is echoed
                        back in on_begin_primitive(), on_emit_vertex(),
                        edge_flag(), on_combine_vertex(), on_error()
                        and on_end_primitive()
   */
  void
  begin_polygon(void *polygon_data=NULL);

  /*!\fn void begin_contour
    Call this function to start a new contour 
    within a polygon. Within each 
    begin_contour() / end_contour() pair there
    must be zero or more calls to add_vertex().
    The vertices added by add_vertex() specify
    a _closed_ polygon, thus the first vertex
    of a contour added with add_vertex() is
    _automatically_ added to the contour's end.
   */
  void
  begin_contour(void);

  /*!\fn void add_vertex
    Call this function to add a vertex to the
    current contour being defined within
    a begin_contour() / end_contour() pair.
    \param position position of the vertex
    \param vertex_data user defined data of vertex, this
                       pointer value is a echoed back in
                       on_emit_vertex() and on_combine_vertex()
   */
  void
  add_vertex(vec2 position, void *vertex_data);

  /*!\fn void end_contour
    Call this function to end the current 
    contour being defined. Within each 
    begin_contour() / end_contour() pair there
    must be zero or more calls to add_vertex().
    The vertices added by add_vertex() specify
    a _closed_ polygon, thus the first vertex
    of a contour added with add_vertex() is
    _automatically_ added to the contour's end.    
   */
  void
  end_contour(void);

  /*!\fn void end_polygon
    Call this function to end the current polygon
    being defined. Calling this function triggers
    the tessellation of the polygon which is 
    described by calling the virtual functions:
    - on_begin_primitive()
    - on_emit_vertex()
    - edge_flag()
    - on_combine_vertex()
    - on_end_primitive()
    - on_error()

    Note that this means the tessellation is done
    independently for each polygon.
   */
  void
  end_polygon(void);

private:  
  std::list<WRATHTessGLUPrivate::polygon_element> m_polygons;
  void *m_private_data;
};


/*! @} */


#endif 
