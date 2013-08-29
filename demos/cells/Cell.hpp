/*! 
 * \file Cell.hpp
 * \brief file Cell.hpp
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


#ifndef __CELLS_HPP__
#define __CELLS_HPP__


#include "WRATHConfig.hpp"
#include "WRATHNew.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerItemNodeTranslate.hpp"
#include "WRATHRectItem.hpp"
#include "WRATHTextItem.hpp"
#include "WRATHTime.hpp"
#include "NodePacker.hpp"

class Table;
class Cell:boost::noncopyable
{
public:
  
  
  Cell(Table *ptable, int x, int y, vec2 psize);

  ~Cell();
  
  WRATHLayerItemNodeTranslate&
  image_node(void)
  {
    return *m_image_node;
  }
  
  WRATHLayerItemNodeTranslate&
  text_node(void)
  {
    return *m_text_node;
  }

  void
  set_image(WRATHImage *pimage);

  void
  set_text(const WRATHTextDataStream &ptext);

  void
  pop_up(float window_width, float window_height);

  void
  pop_down(void);

  const vec4&
  background_color(void) const
  {
    return m_background_color;
  }

  void
  background_color(const vec4 &v)
  {
    m_background_color=v;
    set_background_rect_params();
  }

  void
  animate(float ticks);

  void
  on_window_resize(float x, float y);
  
private:
  typedef WRATHLayerItemDrawerFactory<WRATHLayerItemNodeTranslate,
                                    NodePacker> TextFactory;

  typedef WRATHLayerItemNodeTexture<WRATHLayerItemNodeTranslate,
                                    WRATHTextureCoordinate::simple,
                                    WRATHTextureCoordinate::simple> ImageNode;
  typedef WRATHLayerItemDrawerFactory<ImageNode, NodePacker> ImageFactory;
  
  typedef WRATHLayerItemNodeColorValue<WRATHLayerItemNodeTranslate> RectNode;
  typedef WRATHLayerItemDrawerFactory<RectNode, NodePacker> RectFactory;

  enum state_type
    {
      popped_down,
      popping_up,
      popped_up,
      popping_down,
    };

  void
  set_background_rect_params(void);

  vec2 m_size;
  vec2 m_corner;
  vec4 m_background_color;

  /*
    parent node that does coordinate transformation
    from location of cell to [0, m_size.x()]x[0, m_size.y()].
   */
  WRATHLayerItemNodeTranslate *m_parent_node;

  /*
    node that does the clipping
   */
  WRATHLayerItemNodeTranslate *m_clip_node;

  /*
    parent is m_clip_node, location of text
   */
  WRATHLayerItemNodeTranslate *m_text_node, *m_final_text_node;

  /*
    parent is also m_clip_node location of image
   */
  WRATHLayerItemNodeTranslate *m_image_node;
  ImageNode *m_final_image_node;

  /*
    parent is also m_clip_node location of rect
   */
  RectNode *m_rect_node;

  
  Table *m_table;

  WRATHRectItem *m_backgroud_rect;
  WRATHRectItem *m_image_item;
  WRATHTextItem *m_text_item;

  vec2 m_image_velocity, m_text_velocity;
  vecN<vec2,2> m_animation_rect_bds;

  WRATHTime m_pop_time;
  enum state_type m_state;
  float m_new_scale_factor;
  vec2 m_old_tr, m_new_tr;
};

#endif
