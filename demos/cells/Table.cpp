/*! 
 * \file Table.cpp
 * \brief file Table.cpp
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
#include "WRATHDynamicStrokeAttributePacker.hpp"
#include "WRATHShapePreStroker.hpp"
#include "WRATHRawDrawData.hpp"
#include "Table.hpp"
#include "Cell.hpp"
#include "WRATHLayerItemNodeTranslate.hpp"
#include "WRATHLayerItemDrawerFactory.hpp"
#include "NodePacker.hpp"

namespace
{
  

  class PathOnTop:public WRATHDrawOrderComparer
  {
  public:
    virtual
    enum draw_sort_order_type
    compare_objects(WRATHDrawOrder::const_handle lhs, 
                    WRATHDrawOrder::const_handle rhs) const
    {
      if(lhs.valid() and !rhs.valid())
        {
          return greater_draw_sort_order;
        }
      
      if(rhs.valid() and !lhs.valid())
        {
          return less_draw_sort_order;
        }
      return equal_draw_sort_order;
    }
  };

  class functions_of_ItemNodeTranslateWithColor:public WRATHLayerItemNodeBase::node_function_packet
  {
  public:
    
    enum
      {
        base_number_per_node_values=WRATHLayerItemNodeTranslate::number_per_node_values,
        number_per_node_values=4+base_number_per_node_values,
      };

    virtual
    WRATHLayerItemNodeBase*
    create_completely_clipped_node(const WRATHTripleBufferEnabler::handle &tr) const
    {
      return WRATHLayerItemNodeTranslate::functions().create_completely_clipped_node(tr);
    }

    virtual
    void
    add_per_node_values(WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                          const WRATHLayerNodeValuePackerBase::function_packet &available) const
    {
      WRATHLayerItemNodeTranslate::functions().add_per_node_values(spec, available);
      spec
        .add_source(base_number_per_node_values+0, "stroke_color_red", GL_VERTEX_SHADER)
        .add_source(base_number_per_node_values+1, "stroke_color_green", GL_VERTEX_SHADER)
        .add_source(base_number_per_node_values+2, "stroke_color_blue", GL_VERTEX_SHADER)
        .add_source(base_number_per_node_values+3, "stroke_width", GL_VERTEX_SHADER);
    }

    virtual
    void
    append_shader_source(std::map<GLenum, WRATHGLShader::shader_source> &src,
                         const WRATHLayerNodeValuePackerBase::function_packet &available) const
    {
      WRATHLayerItemNodeTranslate::functions().append_shader_source(src, available);
    }
  };


    
  

  WRATHShapeItem*
  generate_stroked_shape(const WRATHShapeF &pshape,
                         Table *ptable, 
                         bool between_cells,
                         WRATHLayerItemNodeTranslate *n,
                         const WRATHItemDrawerFactory &fact)
  {
    static WRATHDrawOrder::handle dh(WRATHNew WRATHDrawOrder());
    WRATHShapeProcessorPayload payload;
    WRATHDynamicStrokeAttributePacker::StrokingParameters stroke_params;
    WRATHShapeItemTypes::ShapeDrawerF drawer(ptable->drawer().m_stroked_shape_drawer,
                                             WRATHDynamicStrokeAttributePackerF::fetch());

    stroke_params
      .close_outline(!between_cells)
      .width(between_cells?4.0f:10.0f)
      .join_style(WRATHDefaultStrokeAttributePacker::round_join)
      .cap_style(WRATHDefaultStrokeAttributePacker::flat_cap);

    payload=pshape.fetch_matching_payload<WRATHShapePreStrokerPayload>();

    drawer.m_draw_passes[0].m_draw_state=ptable->extra_draw_state().m_stroked_shape_extra_state;
    drawer.m_draw_passes[0].m_draw_type=WRATHDrawType::transparent_pass();
    drawer.m_draw_passes[0].m_force_draw_order=dh; 

    return WRATHNew WRATHShapeItem(fact, 0,
                                   &ptable->layer(), WRATHLayer::SubKey(n),
                                   WRATHShapeItemTypes::shape_value(pshape, payload),
                                   drawer, 
                                   stroke_params);
  }

}

///////////////////////
// Table::ItemNodeTranslateWithColor methods
const WRATHLayerItemNodeBase::node_function_packet&
Table::ItemNodeTranslateWithColor::
functions(void) 
{
  static functions_of_ItemNodeTranslateWithColor R;
  return R;
}

void
Table::ItemNodeTranslateWithColor::
extract_values(reorder_c_array<float> out_value)
{
  reorder_c_array<float> sub_array(out_value.sub_array(0, WRATHLayerItemNodeTranslate::number_per_node_values));
  
  WRATHLayerItemNodeTranslate::extract_values(sub_array);
  out_value[WRATHLayerItemNodeTranslate::number_per_node_values+0]=m_color_and_stroke_width.x();
  out_value[WRATHLayerItemNodeTranslate::number_per_node_values+1]=m_color_and_stroke_width.y();
  out_value[WRATHLayerItemNodeTranslate::number_per_node_values+2]=m_color_and_stroke_width.z();
  out_value[WRATHLayerItemNodeTranslate::number_per_node_values+3]=m_color_and_stroke_width.w();
  
}

////////////////////////////////////
// Table methods
Table::
Table(WRATHTripleBufferEnabler::handle h,
      const vec2 &pbox_size,
      const Drawer &pdrawer,
      const ExtraDrawState &pextra_draw_state,
      const ivec2 &pcell_count):
  m_drawer(pdrawer),
  m_extra_draw_state(pextra_draw_state),
  m_cell_count(pcell_count),
  m_box_size(pbox_size),
  m_table_lines(NULL),
  m_table_boundary(NULL)
{
  m_layer=WRATHNew WRATHLayer(h, 
                              WRATHLayerClipDrawer::handle(),
                              WRATHNew PathOnTop());
  m_root_node=WRATHNew WRATHLayerItemNodeTranslate(h);

  m_cell_size=m_box_size/vec2(m_cell_count.x(), m_cell_count.y());

  boost::multi_array<Cell*, 2>::extent_gen extents;

  m_cells.resize(extents[m_cell_count.x()][m_cell_count.y()]);
  for(int x=0;x<m_cell_count.x(); ++x)
    {
      for(int y=0;y<m_cell_count.y(); ++y)
        {
          m_cells[x][y]=WRATHNew Cell(this, x, y, m_cell_size);
        }
    }
  build_shapes();
}

Cell*
Table::
cell_at(vec2 pt) const
{
  int x, y;

  pt/=m_cell_size;

  x=static_cast<int>(pt.x());
  y=static_cast<int>(pt.y());

  if(y>=m_cell_count.y() 
     or x>=m_cell_count.x() 
     or x<0 
     or y<0
     or pt.x()<0.0f
     or pt.y()<0.0f)
    {
      return NULL;
    }
  
  
  return named_cell(x,y);

}

Table::
~Table()
{
  WRATHDelete(m_table_lines);
  WRATHDelete(m_table_boundary);
  for(int x=0;x<m_cell_count.x(); ++x)
    {
      for(int y=0;y<m_cell_count.y(); ++y)
        {
          WRATHDelete(m_cells[x][y]);
        }
    }
  WRATHPhasedDelete(m_root_node);
  WRATHPhasedDelete(m_layer);
}

  
void
Table::
build_shapes(void)
{
  /*
    Create m_table_lines: 
     the horizontal and vertical lines
     between shapes
   */
  WRATHShapeF vert_horz_lines;

  {
    float float_x;
    int x;
    for(x=1, float_x=m_cell_size.x(); x<m_cell_count.x(); ++x, float_x+=m_cell_size.x())
      {
        vert_horz_lines.new_outline(); 
        vert_horz_lines.current_outline() 
          << vec2(float_x, 0.0f)
          << vec2(float_x, m_box_size.y());
      }
  }
  
  {
    float float_y;
    int y;
    for(y=1, float_y=m_cell_size.y(); y<m_cell_count.y(); ++y, float_y+=m_cell_size.y())
      {
        vert_horz_lines.new_outline(); 
        vert_horz_lines.current_outline() 
          << vec2(0.0f, float_y)
          << vec2(m_box_size.x(), float_y);
      }
    
  }
  m_table_lines_node=WRATHNew ItemNodeTranslateWithColor(m_root_node);
  m_table_lines_node->color_and_stroke_width()=vec4(1.0f, 1.0f, 1.0f, 1.0f);
  m_table_lines=generate_stroked_shape(vert_horz_lines, this, true, m_table_lines_node, 
                                       ItemNodeTranslateWithColor::Factory());

  /*
    Create m_table_boundary:
     the bounding rect of the table
   */
  WRATHShapeF bounding_square;
  bounding_square.current_outline() 
    << vec2(0.0f, 0.0f)
    << vec2(m_box_size.x(), 0.0f)
    << m_box_size
    << vec2(0.0f, m_box_size.y());
  m_table_boundary_node=WRATHNew ItemNodeTranslateWithColor(m_root_node);
  m_table_boundary_node->color_and_stroke_width()=vec4(0.0f, 0.0f, 0.0f, 5.0f);
  m_table_boundary=generate_stroked_shape(bounding_square, this, false, m_table_boundary_node, 
                                          ItemNodeTranslateWithColor::Factory());
}
