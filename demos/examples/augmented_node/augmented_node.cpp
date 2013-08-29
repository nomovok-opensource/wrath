/*! 
 * \file augmented_node.cpp
 * \brief file augmented_node.cpp
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
#include "augmented_node.hpp"

/*!\details
  In this example we build from \ref item_example2_example
  and create an augmented node type so that our polygon
  draws a polygon ring with the inner radius and outer
  radius stored in the node.

  In addition we will apply an image brush to the item
  so that an image is always scale to the items local
  size.
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
  command_line_argument_value<std::string> m_image;
  

  cmd_line_type(void):
    m_image("images/eye.jpg", "image", "Image to use for demo", *this)
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
  typedef WRATHLayerItemNodeTranslate BaseNode;
  typedef RingNode<BaseNode> Node;
  typedef WRATHLayerItemWidget<Node>::FamilySet FamilySet;
  typedef FamilySet::CColorLinearGradientSimpleXSimpleYImageFamily Family;
  typedef WRATHGenericWidget<item, Family::WidgetBase> Widget;

  WRATHShaderBrushSourceHoard m_shader_hoard;
  WRATHGradient *m_gradient;
  WRATHImage *m_image;

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  Widget *m_widget;
  WRATHTime m_time;
  
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

  
  //load the image
  m_image=WRATHDemo::fetch_image(cmd_line->m_image.m_value,
                                 WRATHImage::ImageFormat()
                                 .internal_format(GL_RGBA)
                                 .pixel_data_format(GL_RGBA)
                                 .pixel_type(GL_UNSIGNED_BYTE)
                                 .magnification_filter(GL_LINEAR)
                                 .minification_filter(GL_LINEAR)
                                 .automatic_mipmap_generation(false));

  //make the color stops for the gradient
  m_gradient=WRATHNew WRATHGradient("my gradient");
  m_gradient->set_color(0.00f, WRATHGradient::color(1.0f, 0.0f, 0.0f, 1.0f));
  m_gradient->set_color(0.25f, WRATHGradient::color(0.0f, 1.0f, 0.0f, 1.0f));
  m_gradient->set_color(0.50f, WRATHGradient::color(0.0f, 0.0f, 1.0f, 1.0f));
  m_gradient->set_color(0.75f, WRATHGradient::color(1.0f, 1.0f, 1.0f, 1.0f));

  Widget::parameters params;
  WRATHBrush brush(m_gradient, m_image);

  //the class Widget::Node has the static method set_shader_brush 
  //to set the shader code associated for the brush
  //for details, see \ref WRATHLayerItemNodeBase::set_shader_brush
  Widget::Node::set_shader_brush(brush);

  //geometry properties
  params.m_polygon_spec.m_number_sides=30;

  //use the WRATHShaderBrushSourceHoard, m_shader_hoard,
  //to fetch/get the shader for the brush.
  const WRATHShaderSpecifier *sp;
  sp=&m_shader_hoard.fetch(brush, WRATHBaseSource::mediump_precision);
  params.m_drawer=item::Drawer(sp, //shader how to draw item 
                               ItemAttributePacker::example_packer(), //how to pack attributes for shader 
                               WRATHDrawType::opaque_pass()); //item draw type is opaque 
                               

  //we need to add the brush draw state (i.e. what gradient) 
  //to the draw state of the item.
  m_shader_hoard.add_state(brush, params.m_drawer.m_draw_passes[0].m_draw_state);

  //make the widget
  m_widget=WRATHNew Widget(m_layer, params);

  //some properties of the brush need to be transmitted
  //to the node of the widget, set_from_brush() does that
  m_widget->set_from_brush(brush);

  //set the color and linear gradient position of the widget.
  m_widget->set_gradient(vec2(400.0f, 300.0f), vec2(0.0f, 0.0f));
  m_widget->color(vec4(1.0f, 1.0f, 1.0f, 1.0f));

  //set the velocities to animate the ring
  m_widget->position( vec2(width(), height())/2.0f);
  m_widget->m_position_velocity=vec2(200.0f, 300.0f);
  m_widget->m_inner_radius=0.0f;
  m_widget->m_inner_radius_speed=15.0f;
  m_widget->m_outer_radius=300.0f;
  m_widget->m_outer_radius_speed=165.0f;
  
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

CustomNodeExample::
~CustomNodeExample()
{
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
    move and animate the ring
   */
  vec2 newp=m_widget->position();
  bound_and_v(newp.x(), 
              m_widget->m_position_velocity.x(), 
              delta_t, 
              range_type<float>(0.0f, static_cast<float>(width()) ) );

  bound_and_v(newp.y(), 
              m_widget->m_position_velocity.y(), 
              delta_t, 
              range_type<float>(0.0f, static_cast<float>(height()) ) );

  m_widget->position(newp);

  bound_and_v(m_widget->m_outer_radius,
              m_widget->m_outer_radius_speed,
              delta_t,
              range_type<float>(30.0f, 400.0f));

  bound_and_v(m_widget->m_inner_radius,
              m_widget->m_inner_radius_speed,
              delta_t,
              range_type<float>(0.0f, m_widget->m_outer_radius/2.0f));


  m_tr->signal_complete_simulation_frame();
  m_tr->signal_begin_presentation_frame();
  m_layer->clear_and_draw();
  m_first_frame=false;

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
