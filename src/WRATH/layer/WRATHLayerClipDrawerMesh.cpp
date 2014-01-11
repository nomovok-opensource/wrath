/*! 
 * \file WRATHLayerClipDrawerMesh.cpp
 * \brief file WRATHLayerClipDrawerMesh.cpp
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
#include <limits>
#include "opengl_trait.hpp"
#include "WRATHgluniform.hpp"
#include "WRATHLayerClipDrawerMesh.hpp"
#include "WRATHLayer.hpp"

////////////////////////////////////
// WRATHLayerClipDrawerMesh methods
WRATHLayerClipDrawerMesh::
WRATHLayerClipDrawerMesh(WRATHGLProgram *prog,
                         const WRATHTripleBufferEnabler::handle &h,
                         const std::string &zdraworder_uniform_name,
                         const std::string &matrix_uniform_name,
                         const std::string &attr_name):
  m_z_depth_value(0),
  m_program(prog),
  m_z_depth_value_name(zdraworder_uniform_name),
  m_matrix_name(matrix_uniform_name),
  m_attr_name(attr_name),
  m_inited(false)
{
  m_vertex_data=WRATHNew WRATHBufferObject(h);
  m_index_data=WRATHNew WRATHBufferObject(h);
}

WRATHLayerClipDrawerMesh::
~WRATHLayerClipDrawerMesh()
{
  WRATHPhasedDelete(m_vertex_data);
  WRATHPhasedDelete(m_index_data);
}

void
WRATHLayerClipDrawerMesh::
init_locations(void) const
{
  
  WRATHassert(m_program!=NULL);
  WRATHassert(m_program->link_success());

  m_z_depth_value_location=m_program->uniform_location(m_z_depth_value_name);
  WRATHassert(m_z_depth_value_location!=-1);
  
  m_matrix_location=m_program->uniform_location(m_matrix_name);
  WRATHassert(m_matrix_location!=-1);
  
  m_attr_location=m_program->attribute_location(m_attr_name);
  WRATHassert(m_attr_location!=-1);
}

int
WRATHLayerClipDrawerMesh::
number_vertices(void) const
{
  int sz;

  sz=m_vertex_data->size();
  WRATHassert(sz%sizeof(attribute_type)==0);

  return sz/sizeof(attribute_type);
}

void
WRATHLayerClipDrawerMesh::
number_vertices(int sz)
{
  sz*=sizeof(attribute_type);
  m_vertex_data->resize(sz);
}


int
WRATHLayerClipDrawerMesh::
number_draw_indices(void) const
{
  int sz;

  sz=m_index_data->size();
  WRATHassert(sz%sizeof(index_type)==0);

  return sz/sizeof(index_type);
}

void
WRATHLayerClipDrawerMesh::
number_draw_indices(int sz)
{
  sz*=sizeof(index_type);
  m_index_data->resize(sz);
}

c_array<WRATHLayerClipDrawerMesh::attribute_type>
WRATHLayerClipDrawerMesh::
write_vertices(void)
{
  uint8_t *cptr;
  attribute_type *ptr;

  cptr=m_vertex_data->c_ptr(0);
  ptr=reinterpret_cast<attribute_type*>(cptr);
  return c_array<attribute_type>(ptr, number_vertices());
}
 
const_c_array<WRATHLayerClipDrawerMesh::attribute_type>
WRATHLayerClipDrawerMesh::
vertices(void) const
{
  const uint8_t *cptr;
  const attribute_type *ptr;

  cptr=m_vertex_data->c_ptr(0);
  ptr=reinterpret_cast<const attribute_type*>(cptr);
  return const_c_array<attribute_type>(ptr, number_vertices());
}

c_array<WRATHLayerClipDrawerMesh::index_type>
WRATHLayerClipDrawerMesh::
write_indices(void)
{
  uint8_t *cptr;
  index_type *ptr;

  cptr=m_index_data->c_ptr(0);
  ptr=reinterpret_cast<index_type*>(cptr);
  return c_array<index_type>(ptr, number_draw_indices());
}
 
const_c_array<WRATHLayerClipDrawerMesh::index_type>
WRATHLayerClipDrawerMesh::
indices(void) const
{
  const uint8_t *cptr;
  const index_type *ptr;

  cptr=m_index_data->c_ptr(0);
  ptr=reinterpret_cast<const index_type*>(cptr);
  return const_c_array<index_type>(ptr, number_draw_indices());
}


void
WRATHLayerClipDrawerMesh::
flush_indices(int begin, int end)
{
  begin*=sizeof(index_type);
  end*=sizeof(index_type);
  m_index_data->mark_bytes_dirty(begin, end);
}


void
WRATHLayerClipDrawerMesh::
flush_vertices(int begin, int end)
{
  begin*=sizeof(attribute_type);
  end*=sizeof(attribute_type);
  m_vertex_data->mark_bytes_dirty(begin, end);
}

void
WRATHLayerClipDrawerMesh::
draw_region(bool clear_z, 
            const DrawStateElement &layer,
            const_c_array<DrawStateElement> /*draw_stack*/) const
{
  float zvalue;

  if(clear_z)
    {
      zvalue=1.0f; //clear "z"
    }
  else
    {
      zvalue=static_cast<float>(m_z_depth_value)/static_cast<float>(std::numeric_limits<GLshort>::max());
    }

  m_program->use_program();

  
  if(!m_inited)
    {
      init_locations();
      m_inited=true;
    }

  WRATHglUniform(m_z_depth_value_location, zvalue);
  WRATHglUniform(m_matrix_location, layer.m_layer->current_render_transformation().m_composed_pvm);
    
  m_vertex_data->bind(GL_ARRAY_BUFFER);
  m_index_data->bind(GL_ELEMENT_ARRAY_BUFFER);

  for(int i=0; i<WRATHDrawCallSpec::attribute_count; ++i)
    {
      glDisableVertexAttribArray(i);
    }

  glEnableVertexAttribArray(m_attr_location);
  glVertexAttribPointer(m_attr_location, 
                        opengl_trait<attribute_type>::count,
                        opengl_trait<attribute_type>::type,
                        GL_FALSE,
                        opengl_trait<attribute_type>::stride,
                        m_vertex_data->offset_pointer(0));


  glDrawElements(GL_TRIANGLES, 
                 number_draw_indices(), 
                 opengl_trait<index_type>::type,
                 m_index_data->offset_pointer(0));
  
}

WRATHLayerClipDrawer::DrawStateElementClipping
WRATHLayerClipDrawerMesh::
clip_mode(WRATHLayer*,
          const DrawStateElementTransformations&,
          const_c_array<DrawStateElement>) const
{
  return WRATHLayerClipDrawer::layer_clipped_hierarchy;
}
