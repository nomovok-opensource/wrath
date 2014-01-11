/*! 
 * \file WRATHLayerClipDrawerMesh.hpp
 * \brief file WRATHLayerClipDrawerMesh.hpp
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




#ifndef __WRATH_UI_CLIP_DRAWER_MESH_HPP__
#define __WRATH_UI_CLIP_DRAWER_MESH_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerClipDrawer.hpp"
#include "WRATHBufferObject.hpp"
#include "WRATHGLProgram.hpp"
#include "c_array.hpp"

/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerClipDrawerMesh
  A WRATHLayerClipDrawerMesh is an implementation of
  WRATHLayerClipDrawer to use a mesh (i.e. a soup
  of triangles) to specify the clipping area.
  Each method of WRATHLayerClipDrawerMesh except
  draw_region() may be called from any thread.
  Each method is thread safe.
 */
class WRATHLayerClipDrawerMesh:public WRATHLayerClipDrawer
{
public:
  /*!\typedef handle
    handle typedef
   */ 
  typedef handle_t<WRATHLayerClipDrawerMesh> handle;

  /*!\typedef attribute_type
    Attribute type for vertex of the triangle
    soup.
   */
  typedef vec3 attribute_type;

  /*!\typedef index_type
    Index type for the triangle soup.
   */
  typedef GLushort index_type;

  /*!\fn explicit WRATHLayerClipDrawerMesh
    Ctor. 
    \param prog WRATHGLProgram program uses to draw the mesh

    \param h handle to a WRATHTripleBufferEnabler that the created
             WRATHBufferObject will use for coordinating deleting
             the underlying GL buffer object.

    \param zdepthvalue_uniform_name uniform name to that sets
                                    the depth buffer value.

    \param matrix_uniform_name uniform name to that sets
                               the projection-modelview matrix value.
                                    
    \param attr_name attribute name in prog
                     for the vertices of the triangle soup.
   */
  WRATHLayerClipDrawerMesh(WRATHGLProgram *prog,
                           const WRATHTripleBufferEnabler::handle &h,
                           const std::string &zdepthvalue_uniform_name,
                           const std::string &matrix_uniform_name,
                           const std::string &attr_name);

  virtual
  ~WRATHLayerClipDrawerMesh();

  virtual
  void
  draw_region(bool clear_z, 
              const DrawStateElement &layer,
              const_c_array<DrawStateElement> draw_stack) const;
  

  virtual
  DrawStateElementClipping
  clip_mode(WRATHLayer *layer,
            const DrawStateElementTransformations &layer_transformations,
            const_c_array<DrawStateElement> draw_state_stack) const;

  /*!\fn int number_vertices(void) const
    Returns the number of vertices of the triangle soup.
   */
  int
  number_vertices(void) const;

  /*!\fn void number_vertices(int)
    Set the number of vertices of the triangle soup.
    \param v value to which to set the number of vertices.
   */
  void
  number_vertices(int v);

  /*!\fn const attribute_type& vertex(int) const
    return the value of a specified vertex
    \param i index of vertex to query
   */
  const attribute_type&
  vertex(int i) const
  {
    return vertices()[i];
  }

  /*!\fn void vertex(int, const attribute_type&)
    Set the value of a specified vertex
    \param i index of vertex to set
    \param v value to which to set the vertex
   */
  void
  vertex(int i, const attribute_type &v)
  {
    write_vertices()[i]=v;
    flush_vertices(i, i+1);    
  }

  /*!\fn c_array<attribute_type> write_vertices
    Returns a c_array of the vertices.
   */
  c_array<attribute_type>
  write_vertices(void);

  /*!\fn const_c_array<attribute_type> vertices
    Returns a const_c_array of the vertices.
   */
  const_c_array<attribute_type>
  vertices(void) const;

  /*!\fn void flush_vertices(int, int)
    If vertices have been changed, by writing to
    write_vertices(), those vertices that have been
    changed beed to flushed to GL.
    \param begin first vertex to flush changes to GL
    \param end one past the last vertex to flush changes to GL
   */
  void 
  flush_vertices(int begin, int end);

  /*!\fn void flush_vertices(void)
    Flush all vertex data to GL, equivalent to
    flush_indices(0, number_draw_vertices());
   */
  void 
  flush_vertices(void)
  {
    flush_vertices(0, number_vertices());
  }

  /*!\fn int number_draw_indices(void) const
    Returns the number of indices of the triangle soup,
    note that 3 consecutive indices make 1 triangle.
   */
  int
  number_draw_indices(void) const;

  /*!\fn void number_draw_indices(int)
    Set the number of indices of the triangle soup,
    note that 3 consecutive indices make 1 triangle.
    \param v value to which to set the number of indices.
   */
  void
  number_draw_indices(int v);

  /*!\fn index_type draw_index(int) const
    Returns the index of the named index.
    \param i index to query
   */
  index_type
  draw_index(int i) const
  {
    return indices()[i];
  }

  /*!\fn void draw_index(int, index_type)
    Sets the index of the named index.
    \param i index to set
    \param v value to which to set the index
   */
  void
  draw_index(int i, index_type v)
  {
    write_indices()[i]=v;
    flush_indices(i, i+1);
  }
  /*!\fn c_array<index_type> write_indices
    Returns a c_array of the indices.
   */
  c_array<index_type>
  write_indices(void);

  /*!\fn const_c_array<index_type> indices
    Returns a const_c_array of the indices.
   */
  const_c_array<index_type>
  indices(void) const;

  /*!\fn void flush_indices(int, int)
    If indices have been changed, by writing to
    write_indices(), those indices that have been
    changed need to flushed to GL.
    \param begin first index to flush changes to GL
    \param end one past the last index to flush changes to GL
   */
  void
  flush_indices(int begin, int end);

  /*!\fn void flush_indices(void)
    Flush all indices data to GL, equivalent to
    flush_indices(0, number_draw_indices());
   */
  void
  flush_indices(void)
  {
    flush_indices(0, number_draw_indices());
  }

  /*!\fn void flush
    Flush all vertices and indices to GL, equivalent to
    calling both flush_vertices() and flush_indices(). 
   */
  void
  flush(void)
  {
    flush_vertices();
    flush_indices();
  }

  /*!\var m_z_depth_value
    Value to which to write to depth buffer (and to
    perform depth testing with) for the mesh,
    intial value is 0. Note that the value is unnormalized,
    and the value sent to GL is 
    \code m_z_depth_value/std::numeric_limits<GLshort>::max()
    \endcode
   */
  GLshort m_z_depth_value;

private:

  void
  init_locations(void) const;

  WRATHBufferObject *m_vertex_data;
  WRATHBufferObject *m_index_data;

  WRATHGLProgram *m_program;

  std::string m_z_depth_value_name;
  std::string m_matrix_name;
  std::string m_attr_name;
  mutable bool m_inited;

  mutable GLint m_z_depth_value_location;
  mutable GLint m_matrix_location;
  mutable GLint m_attr_location;
};
/*! @} */


#endif
