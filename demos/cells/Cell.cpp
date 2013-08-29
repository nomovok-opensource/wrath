/*! 
 * \file Cell.cpp
 * \brief file Cell.cpp
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
#include "WRATHDefaultRectAttributePacker.hpp"
#include "WRATHLayerItemNodeTranslate.hpp"
#include "WRATHLayerItemDrawerFactory.hpp"
#include "Cell.hpp"
#include "Table.hpp"

#define POP_TIME 1000



namespace
{
  /*
    Convention for z's. 
    Non-popped up cells have their z's as...
    and popped up cells have their z's decreased by 1000..
  */
  const GLshort text_z=100;
  const GLshort image_z=200;
  const GLshort rect_z=300;
    
  vec2
  compute_new_position(vec2 old_position,
                       vec2 &velocity,
                       const vecN<vec2,2> &rect,
                       float ticks)
  {
    vec2 vv(velocity*ticks);

    old_position+=vv;
    if(old_position.x()>=rect[1].x())
      {
        old_position.x()=rect[1].x();
        velocity.x()=-velocity.x();
      }

    if(old_position.x()<=rect[0].x())
      {
        old_position.x()=rect[0].x();
        velocity.x()=-velocity.x();
      }

    if(old_position.y()>=rect[1].y())
      {
        old_position.y()=rect[1].y();
        velocity.y()=-velocity.y();
      }

    if(old_position.y()<=rect[0].y())
      {
        old_position.y()=rect[0].y();
        velocity.y()=-velocity.y();
      }
    return old_position;
  }

  void
  animate_node(WRATHLayerItemNodeTranslate *node,
               vec2 &velocity,
               const vecN<vec2,2> &rect,
               float ticks)
  {
    vec2 pt;

    pt=compute_new_position(node->translation(), velocity, rect, ticks);
    node->translation(pt);
  }

  float
  random_value(float pmin, float pmax)
  {
    int v;
    float fv;

    v=rand();
    
    fv=static_cast<float>(v)/static_cast<float>(RAND_MAX);
    return pmin + (pmax-pmin)*fv;
  }

  vec2
  random_value(vec2 pmin, vec2 pmax)
  {
    return vec2(random_value(pmin.x(), pmax.x()), 
                random_value(pmin.y(), pmax.y()));
  }

}

Cell::
Cell(Table *ptable, int x, int y, vec2 psize):
  m_size(psize),
  m_corner(static_cast<float>(x)*psize.x(), static_cast<float>(y)*psize.y()),
  m_background_color(1.0f, 1.0f, 1.0f, 1.0f), 

  m_parent_node(NULL),
  m_clip_node(NULL),
  m_text_node(NULL),
  m_final_text_node(NULL),
  m_image_node(NULL),
  m_final_image_node(NULL),
  m_rect_node(NULL),

  m_table(ptable),

  m_backgroud_rect(NULL),
  m_image_item(NULL),
  m_text_item(NULL),
  m_state(popped_down)
{
  m_parent_node=WRATHNew WRATHLayerItemNodeTranslate(&ptable->root_node());
  m_clip_node=WRATHNew WRATHLayerItemNodeTranslate(m_parent_node);

  m_text_node=WRATHNew WRATHLayerItemNodeTranslate(m_clip_node);
  m_final_text_node=WRATHNew WRATHLayerItemNodeTranslate(m_text_node);
  m_image_node=WRATHNew WRATHLayerItemNodeTranslate(m_clip_node);
  m_final_image_node=WRATHNew ImageNode(m_image_node);
  m_rect_node=WRATHNew RectNode(m_clip_node);

  /*
    set the z's:
   */
  m_final_text_node->z_order(text_z);
  m_final_image_node->z_order(image_z);
  m_rect_node->z_order(rect_z);
  

  /*
    now set m_parent_node so that 
    box of [m_corner, m_corner+m_size] is mapped to
    [0, m_size]
   */
  m_parent_node->translation(m_corner);

  /*
    now set clipping of m_clip_node to 
    [-m_size/2.0f, m_size/2.0f]
    and translate to m_size/2.
   */
  WRATHBBox<2> B(-m_size/2.0f, m_size/2.0f);
  m_clip_node->translation(m_size/2.0f);
  m_clip_node->clipping_active(true);
  m_clip_node->clip_rect(B);  

  /*
    make the text item
   */
  m_text_item=WRATHNew WRATHTextItem(TextFactory(), 0, //transformation magic
                                     &m_table->layer(), WRATHLayer::SubKey(m_final_text_node), 
                                     WRATHTextItemTypes::text_transparent);

  /*
    now make the background rect:
   */
  WRATHBrush brush;

  RectNode::set_shader_brush(brush);
  m_backgroud_rect=WRATHNew WRATHRectItem(RectFactory(), 0, //transformation magic
                                          &m_table->layer(), WRATHLayer::SubKey(m_rect_node), 
                                          brush);

  set_background_rect_params();

  m_animation_rect_bds[0]=-m_size/2.0f;
  m_animation_rect_bds[1]=m_size/2.0f;

  vec2 v(m_size/2000.0f); //across the cell every 2 seconds.

  m_image_velocity=random_value(-v, v);
  m_text_velocity=random_value(-v, v);

  m_text_node->translation( random_value(-m_size/2.5f, m_size/2.5f));
  m_image_node->translation( random_value(-m_size/2.5f, m_size/2.5f));
}

Cell::
~Cell()
{
  if(m_backgroud_rect!=NULL)
    {
      WRATHDelete(m_backgroud_rect);
    }
  if(m_image_item!=NULL)
    {
      WRATHDelete(m_image_item);
    }
  if(m_text_item!=NULL)
    {
      WRATHDelete(m_text_item);
    }
  /*
    no need to delete any of our nodes
    since they are child nodes of root
    node of the constructing table.
   */
  m_parent_node->parent(&m_table->root_node());
}

void
Cell::
set_background_rect_params(void)
{
  WRATHDefaultRectAttributePacker::Rect::handle rect;
  rect=WRATHNew WRATHDefaultRectAttributePacker::Rect(m_size);

  m_backgroud_rect->set_parameters(rect);
  m_rect_node->color(m_background_color);
  m_rect_node->position(-0.5*m_size);
}

void
Cell::
pop_up(float w, float h)
{
  /*;
    set the z's:
   */
  m_final_text_node->z_order(text_z-1000);
  m_final_image_node->z_order(image_z-1000);
  m_rect_node->z_order(rect_z-1000);
  
  /*
    futz with m_parent_node.
    We want to make the cell centered taking up
    middle 2/3
   */
  vec2 vv(w,h);
  /*
    scale factor that we would apply
    in each dimension..
   */
  vv=2.0f*vv/(3.0f*m_size);

  m_new_scale_factor=std::min(vv.x(), vv.y());
  m_new_tr.x() = (w - m_new_scale_factor*m_size.x())/2.0f;
  m_new_tr.y() = (h - m_new_scale_factor*m_size.y())/2.0f;

  m_old_tr=m_parent_node->global_values().m_transformation.translation();

  m_parent_node->parent(NULL);
  m_state=popping_up;
  m_pop_time.restart();
}

void
Cell::
on_window_resize(float w, float h)
{
  if(m_state==popping_up or m_state==popped_up)
    {
      vec2 vv(w,h);

      vv=2.0f*vv/(3.0f*m_size);      
      m_new_scale_factor=std::min(vv.x(), vv.y());
      m_new_tr.x() = (w - m_new_scale_factor*m_size.x())/2.0f;
      m_new_tr.y() = (h - m_new_scale_factor*m_size.y())/2.0f;

      if(m_state==popped_up)
        {
          m_parent_node->scaling_factor(m_new_scale_factor);
          m_parent_node->translation(m_new_tr);
        }
    }
}



void
Cell::
pop_down(void)
{
  /*
    set the z's:
   */
  m_final_text_node->z_order(text_z);
  m_final_image_node->z_order(image_z);
  m_rect_node->z_order(rect_z);
  
  /*
    futz with m_parent_node:
   */
  m_parent_node->parent(&m_table->root_node());
  m_parent_node->translation(m_corner);
  m_parent_node->scaling_factor(1.0f);

  m_state=popped_down;
}

void
Cell::
set_image(WRATHImage *pimage)
{
  if(m_image_item!=NULL)
    {
      WRATHDelete(m_image_item);
      m_image_item=NULL;
    }

  if(pimage==NULL or pimage->size().x()==0
     or pimage->size().y()==0)
    {
      return;
    }

  /*
    m_image_node scales the image to be about 2/3*size
   */
  vec2 scale_xy;
  scale_xy=2.0/3.0 * m_size / vec2(pimage->size());
  m_image_node->scaling_factor(std::max(scale_xy.x(), scale_xy.y()) );


  WRATHBrush brush(pimage);
  brush.flip_image_y(true);

  ImageNode::set_shader_brush(brush);
  m_image_item=WRATHNew WRATHRectItem(ImageFactory(), 0, //transformation magic
                                      &m_table->layer(), WRATHLayer::SubKey(m_final_image_node),
                                      brush);

  WRATHDefaultRectAttributePacker::Rect::handle rect;
  rect=WRATHNew WRATHDefaultRectAttributePacker::Rect(vec2(pimage->size()));
  m_image_item->set_parameters(rect);
  m_final_image_node->set_from_brush(brush);
}


void
Cell::
set_text(const WRATHTextDataStream &ptext)
{
  WRATHTextAttributePacker::BBox pbox;

  m_text_item->clear();
  m_text_item->add_text(ptext);
  pbox.set_or(m_text_item->bounding_box());
  

  if(!pbox.empty())
    {
      vec2 c( pbox.min_corner()+pbox.max_corner());
      m_final_text_node->translation(-c*0.5f);
    }
}

void
Cell::
animate(float time_delta)
{
 
  animate_node(m_text_node, m_text_velocity, m_animation_rect_bds, time_delta);
  animate_node(m_image_node, m_image_velocity, m_animation_rect_bds, time_delta);
    

  if(m_state==popping_up or m_state==popped_up)
    {
      int32_t ela;
      float r;

      ela=m_pop_time.elapsed();
      if(ela>POP_TIME)
        {
          m_state=popped_up;
          ela=POP_TIME;
        }
      r=static_cast<float>(ela)/static_cast<float>(POP_TIME);

      m_parent_node->scaling_factor(r*m_new_scale_factor);
      m_parent_node->translation(m_old_tr + r*(m_new_tr-m_old_tr));
    }
}
