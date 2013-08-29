/*! 
 * \file augmented_node2.cpp
 * \brief file augmented_node2.cpp
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
 * \author Jean Fairlie <jean.fairlie@nomovok.com>
 * 
 */


#include "WRATHConfig.hpp"
#include "WRATHNew.hpp"
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>

#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "WRATHmalloc.hpp"
#include "WRATHUtil.hpp"
#include "WRATHLayerItemWidgets.hpp"
#include "WRATHLayerItemNodeTranslate.hpp"
#include "WRATHGenericWidget.hpp"
#include "WRATHTime.hpp"

#include "wrath_demo.hpp"
#include "wrath_demo_image_support.hpp"

#include "item.hpp"
#include "augmented_node2.hpp"

/*!\details
  In this example we build from \ref augmented_node2_example
  to demonstrate thay many rings, even with different images
  applied to them generate very few draw calls.
 */

void
bound_and_v(float &in, 
            float &speed, 
            float delta_t,
            range_type<float> bounds)
{
  float delta, p;

  delta=delta_t*speed;
  p=in + delta;

  if(p<bounds.m_begin)
    {
      p = bounds.m_begin + std::abs(bounds.m_begin-p);
      speed=std::abs(speed);
    }
  else if(p>bounds.m_end)
    {
      p = bounds.m_end - std::abs(p-bounds.m_end);
      speed=-std::abs(speed);
    }
  in=p;
}

class cmd_line_type:public DemoKernelMaker
{
public:
  command_line_argument_value<int> m_num_items;
  command_line_argument_value<int> m_num_sides;
  

  cmd_line_type(void):
     m_num_items(100, "num_items",
                 "number of polygons floating around", *this),
     m_num_sides(40, "num_sides",
                 "number of sides each polygon has", *this)
  {}

  virtual
  DemoKernel* 
  make_demo(void);
  
  virtual
  void
  delete_demo(DemoKernel *k)
  {
    if(k!=NULL)
      {
        WRATHDelete(k);
      }
  }
};

class CustomNodeExample:public DemoKernel
{
public:
  CustomNodeExample(cmd_line_type *cmd_line);
  ~CustomNodeExample();
  
  void 
  resize(int width, int height);
  
  virtual
  void
  handle_event(FURYEvent::handle ev);
  
  virtual
  void
  paint(void);

private:
  //typedef WRATHLayerNodeValuePackerTextureFP32 NodePacker;
  typedef WRATHLayerNodeValuePackerUniformArrays NodePacker;
  typedef WRATHLayerItemNodeTranslate BaseNode;
  typedef RingNode<BaseNode> Node;
  typedef WRATHLayerItemWidget<Node, NodePacker>::FamilySet FamilySet;
  typedef FamilySet::CColorSimpleXSimpleYImageFamily Family;
  typedef WRATHGenericWidget<item, Family::WidgetBase> Widget;

  void
  load_images(void);

  void
  animate_ring(Widget *c, float delta);

  WRATHShaderBrushSourceHoard m_shader_hoard;
  std::vector<WRATHImage*> m_images;

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  std::vector<Widget*> m_widgets;
  WRATHTime m_time, m_total_time;
  
  WRATHLayer::draw_information m_draw_stats;
  bool m_first_frame;
};



DemoKernel* 
cmd_line_type::
make_demo(void)
{
  return WRATHNew CustomNodeExample(this);
}
  

CustomNodeExample::
CustomNodeExample(cmd_line_type *cmd_line):
  DemoKernel(cmd_line),
  m_shader_hoard(WRATHGLShader::shader_source()
                 .add_source("item.vert.glsl", WRATHGLShader::from_resource),
                 WRATHGLShader::shader_source()
                 .add_source("item.frag.glsl", WRATHGLShader::from_resource)),
  m_first_frame(true)
{
  /*
    Create the WRATHTripleBufferEnabler object
    which our visual items will use to sync
   */
  m_tr=WRATHNew WRATHTripleBufferEnabler();

  /*
    Create the WRATHLayer which will contain and
    draw our shape
   */
  m_layer=WRATHNew WRATHLayer(m_tr);

  /*
    these are the transforms that will be
    applied to all elements contained in the
    WRATHLayer:

    + a 3D transform
    + a Projection transform

    For the purpose of this example the 3D transform
    will be the identity matrix. In other words no
    transform will be applied to the vertex data.

    Projection will be orthographic
    */

  float_orthogonal_projection_params proj_params(0, width(),
                                                 height(), 0);

  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));

  
  //load images
  load_images();

  //make the widgets.
  for(int i=0; i<cmd_line->m_num_items.m_value; ++i)
    {
      Widget::parameters params;
      WRATHImage *img(m_images[rand()%m_images.size()]);
      WRATHBrush brush(img);
      Widget *c;

      brush.flip_image_y(true);

      params.m_polygon_spec.m_number_sides=cmd_line->m_num_sides.m_value;

      //the class Widget::Node has the static method set_shader_brush 
      //to set the shader code associated for the brush
      //for details, see \ref WRATHLayerItemNodeBase::set_shader_brush
      Widget::Node::set_shader_brush(brush);

      //use the WRATHShaderBrushSourceHoard, m_shader_hoard,
      //to fetch/get the shader for the brush.
      const WRATHShaderSpecifier *sp;
      sp=&m_shader_hoard.fetch(brush, WRATHBaseSource::mediump_precision);

      //set the drawer for params
      params.m_drawer=item::Drawer(sp, //shader how to draw item 
                                   ItemAttributePacker::example_packer(), //how to pack attributes for shader 
                                   WRATHDrawType::opaque_pass()); //item draw type is opaque 

      //we need to add the brush draw state (i.e. what gradient) 
      //to the draw state of the item.
      m_shader_hoard.add_state(brush, params.m_drawer.m_draw_passes[0].m_draw_state);
      
      //make the widget
      c=WRATHNew Widget(m_layer, params);
      
      //some properties of the brush need to be transmitted
      //to the node of the widget, set_from_brush() does that
      c->set_from_brush(brush);

      float rand_angle, rand_magnitude, r1, r2, r3, r4;
      float r5, r6, r7;
      vec2 pos;
      
      r1=static_cast<float>(rand())/static_cast<float>(RAND_MAX);
      r2=static_cast<float>(rand())/static_cast<float>(RAND_MAX);
      r3=static_cast<float>(rand())/static_cast<float>(RAND_MAX);
      r4=static_cast<float>(rand())/static_cast<float>(RAND_MAX);
      r5=static_cast<float>(rand())/static_cast<float>(RAND_MAX);
      r6=static_cast<float>(rand())/static_cast<float>(RAND_MAX);
      r7=static_cast<float>(rand())/static_cast<float>(RAND_MAX);
      

      rand_angle=r1*2.0f*M_PI;
      rand_magnitude=100.0f*(1.0f + r2);
      c->m_position_velocity=rand_magnitude*vec2( std::cos(rand_angle), std::sin(rand_angle));
      
      pos=vec2(r3, r4);
      c->position(pos*vec2(width(), height()));
      c->color(vec4(r5, r6, r7, 1.0f));
      c->z_order(-i);

      c->m_ID=i;

      m_widgets.push_back(c);
    }
  
  glClearColor(0.0, 0.0, 0.0, 1.0);
}

CustomNodeExample::
~CustomNodeExample()
{
  /*
    the parent of each widget is m_layer, so deleting
    m_layer deletes each of the widgets too.
   */
  if(m_layer!=NULL)
    {
      WRATHPhasedDelete(m_layer);
    }

  
  /*
    Delete all resources
   */
  WRATHResourceManagerBase::clear_all_resource_managers();

  /*
    purge cleanup to perform post processing cleanup
    tasks(typically deletion of GL objects).
   */
  m_tr->purge_cleanup();
  m_tr=NULL;

  std::cout << "\n-----------------------------------\n\n#draw calls per frame: "
            << m_draw_stats.m_draw_count
            << "\n\n-----------------------------------\n\n";
}


void
CustomNodeExample::
load_images(void)
{
  WRATHImage::ImageFormat fmt;

  std::vector<std::string> name;
  name.push_back("images/eye.jpg");
  name.push_back("images/hands.jpg");
  name.push_back("images/light5.jpg");
  name.push_back("images/image1.jpg");
  name.push_back("images/image.png");
  
  for(std::vector<std::string>::const_iterator
        iter=name.begin(), end=name.end();
      iter!=end; ++iter)
    {
      WRATHImage::WRATHImageID id(*iter);
      //helper function
      WRATHImage* im = WRATHDemo::fetch_image(id, fmt);
      m_images.push_back(im);
    }
}

void
CustomNodeExample::
resize(int width, int height)
{
  float_orthogonal_projection_params proj_params(0, width, height, 0);
  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));
  glViewport(0, 0, width, height);
}


void
CustomNodeExample::
animate_ring(Widget *c, float delta_t)
{
  float cycle, radius, phase;
  uint32_t total_time, modulas;
  uint32_t period_in_ms(1500);
  float freq( 2.0f*M_PI/ static_cast<float>(period_in_ms));
  int i;

  i=c->m_ID;
  phase= static_cast<float>(i) * M_PI/12.0f;
  /*
    we want it cyclic; first we take the modulas in integer
    arithmatic.
   */
  total_time=m_total_time.elapsed();
  modulas=total_time%period_in_ms;
  cycle=std::sin( freq*static_cast<float>(modulas) + phase);
  cycle = (cycle + 1.0)/2.0;


  radius = cycle*5.0*(i%10 + 1) + 5.0*(i%10 + 1);
  c->m_outer_radius=radius + 2.0*(i%20);
  c->m_inner_radius=radius - 2.0*(i%20);

  //cycle the color too.
  vec4 new_color(cycle, cycle, cycle, 1.0f);
  new_color[i%3]=1.0f;
  c->color(new_color);

  vec2 newp=c->position();
  bound_and_v(newp.x(), 
              c->m_position_velocity.x(), 
              delta_t, 
              range_type<float>(0.0f, static_cast<float>(width()) ) );

  bound_and_v(newp.y(), 
              c->m_position_velocity.y(), 
              delta_t, 
              range_type<float>(0.0f, static_cast<float>(height()) ) );

  c->position(newp);
}

void 
CustomNodeExample::
paint(void)
{
  /*
    on the first frame, we make delta_t
    but we also need to restart m_time
    because it started at its construction.
   */
  float delta_t;
  delta_t=static_cast<float>(m_time.restart())/1000.0f;
  delta_t=m_first_frame?
    0.0f:
    delta_t;

  /*
    move and animate each ring
   */
  for(std::vector<Widget*>::iterator 
        iter=m_widgets.begin(), end=m_widgets.end();
      iter!=end; ++iter)
    {
      animate_ring(*iter, delta_t);
    }


  m_tr->signal_complete_simulation_frame();
  m_tr->signal_begin_presentation_frame();

  //reset then record the number of draw calls
  //WRATHLayer::clear_and_draw _increments_ the value.
  m_draw_stats=WRATHLayer::draw_information();
  m_layer->clear_and_draw(&m_draw_stats);
  m_first_frame=false;

  //signal to draw again.
  update_widget();
}

void 
CustomNodeExample::
handle_event(FURYEvent::handle ev)
{
  if(ev->type()==FURYEvent::Resize)
    {
      FURYResizeEvent::handle rev(ev.static_cast_handle<FURYResizeEvent>());
      resize(rev->new_size().x(), rev->new_size().y());
    }
}

int 
main(int argc, char **argv)
{
    cmd_line_type cmd_line;
    return cmd_line.main(argc, argv);
}
