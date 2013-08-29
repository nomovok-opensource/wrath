/*! 
 * \file Table.hpp
 * \brief file Table.hpp
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


#ifndef __TABLE_DEMO_HPP__
#define __TABLE_DEMO_HPP__

#include "WRATHConfig.hpp"
#include <boost/multi_array.hpp>
#include <boost/utility.hpp>
#include "WRATHNew.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerItemNodeTranslate.hpp"
#include "WRATHLayerItemDrawerFactory.hpp"
#include "WRATHShapeItem.hpp"
#include "WRATHRectItem.hpp"
#include "WRATHTextItem.hpp"

#include "NodePacker.hpp"

class Cell;

class Table:boost::noncopyable
{
public:

  
  class ExtraDrawState
  {
  public:
    WRATHSubItemDrawState m_stroked_shape_extra_state;
  };

  class Drawer
  {
  public:
    Drawer(void):
      m_stroked_shape_drawer(NULL)
    {}

    WRATHShaderSpecifier *m_stroked_shape_drawer;   
  };

  Table(WRATHTripleBufferEnabler::handle h,
        const vec2 &pbox_size,
        const Drawer &drawer,
        const ExtraDrawState &extra_draw_state,
        const ivec2 &pcell_count);

  ~Table();

  const Drawer&
  drawer(void) const
  {
    return m_drawer;
  }

  const ExtraDrawState&
  extra_draw_state(void) const
  {
    return m_extra_draw_state;
  }

  WRATHLayer&
  layer(void)
  {
    return *m_layer;
  }

  WRATHLayerItemNodeTranslate&
  root_node(void)
  {
    return *m_root_node;
  }

  const ivec2&
  cell_count(void) const
  {
    return m_cell_count;
  }

  const vec2&
  box_size(void) const
  {
    return m_box_size;
  }

  Cell*
  named_cell(int x, int y) const
  {
    return m_cells[x][y];
  }

  //coordinates relative to _root_, not screen
  Cell*
  cell_at(vec2 pt) const;

  vec4& //color in .xyz, width at .w
  stroke_color_and_width_internal_lines(void)
  {
    return m_table_lines_node->color_and_stroke_width();
  }

  float&
  stroke_width_internal_lines(void)
  {
    return stroke_color_and_width_internal_lines().w();
  }

  void
  stroke_color_internal_lines(const vec3 &pcolor)
  {
    stroke_color_and_width_internal_lines().x()=pcolor.x();
    stroke_color_and_width_internal_lines().y()=pcolor.y();
    stroke_color_and_width_internal_lines().z()=pcolor.z();
  }


  vec4& //color in .xyz, width at .w
  stroke_color_and_width_external_lines(void)
  {
    return m_table_boundary_node->color_and_stroke_width();
  }

  float& 
  stroke_width_external_lines(void)
  {
    return stroke_color_and_width_external_lines().w();
  }

  void
  stroke_color_external_lines(const vec3 &pcolor)
  {
    stroke_color_and_width_external_lines().x()=pcolor.x();
    stroke_color_and_width_external_lines().y()=pcolor.y();
    stroke_color_and_width_external_lines().z()=pcolor.z();
  }

  


private:

  class ItemNodeTranslateWithColor:public WRATHLayerItemNodeTranslate
  {
  public:
    typedef WRATHLayerItemDrawerFactory<ItemNodeTranslateWithColor,
                                        NodePacker> Factory;

    ItemNodeTranslateWithColor(const WRATHTripleBufferEnabler::handle &r):
      WRATHLayerItemNodeTranslate(r),
      m_color_and_stroke_width(0.0f)
    {
    }

    ItemNodeTranslateWithColor(WRATHLayerItemNodeTranslate *pparent):
      WRATHLayerItemNodeTranslate(pparent),
      m_color_and_stroke_width(0.0f)
    {
    }

    vec4&
    color_and_stroke_width(void)
    {
      return m_color_and_stroke_width;
    }

    const vec4&
    color_and_stroke_width(void) const
    {
      return m_color_and_stroke_width;
    }

    virtual
    const node_function_packet&
    node_functions(void) const
    {
      return functions();
    }
    
    static
    const node_function_packet&
    functions(void);
    
    virtual
    void
    extract_values(reorder_c_array<float> out_value);

  private:
    vec4 m_color_and_stroke_width;

  };

  void
  build_shapes(void);
  
  WRATHLayer *m_layer;
  WRATHLayerItemNodeTranslate *m_root_node;
  Drawer m_drawer;
  ExtraDrawState m_extra_draw_state;
  
  ivec2 m_cell_count;
  vec2 m_box_size, m_cell_size;

  boost::multi_array<Cell*, 2> m_cells;

  ItemNodeTranslateWithColor *m_table_lines_node;
  ItemNodeTranslateWithColor *m_table_boundary_node;

  WRATHShapeItem *m_table_lines;
  WRATHShapeItem *m_table_boundary;
};

#endif
