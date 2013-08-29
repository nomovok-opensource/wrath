/*! 
 * \file brush_example.cpp
 * \brief file brush_example.cpp
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
#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "WRATHmalloc.hpp"
#include "WRATHUtil.hpp"
#include "WRATHLayerItemWidgetsTranslate.hpp"
#include "WRATHGenericWidget.hpp"
#include <dirent.h>
#include "ngl_backend.hpp"
#include <stdio.h>
#include "wrath_demo.hpp"
#include "item.hpp"


/*!\details
  In this example we build from \ref item_example2_example
  and use \ref WRATHShaderBrushSourceHoard to draw a
  polygon with a gradient brush applied.

  Exercises:
  - adjust the shader code to handle when the brush is non-linear
    (check the documentation of \ref WRATHShaderBrushSourceHoard
  - do something "interesting" and non-linear to the brush coordinate
    in the fragment shader
 */

class cmd_line_type:public DemoKernelMaker
{
public:
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

class BrushExample:public DemoKernel
{
public:
  BrushExample(cmd_line_type *cmd_line);
  ~BrushExample();
  
  void 
  resize(int width, int height);
  
  virtual
  void
  handle_event(FURYEvent::handle ev);
  
  virtual
  void
  paint(void);

private:
  typedef WRATHLayerTranslateFamilySet::CColorLinearGradientFamily Family;
  typedef WRATHGenericWidget<item, Family::WidgetBase> Widget;

  WRATHShaderBrushSourceHoard m_shader_hoard;
  WRATHGradient *m_gradient;

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  Widget *m_widget;
  
};



DemoKernel* 
cmd_line_type::
make_demo(void)
{
  return WRATHNew BrushExample(this);
}
  

BrushExample::
BrushExample(cmd_line_type *cmd_line):
  DemoKernel(cmd_line),
  m_shader_hoard(WRATHGLShader::shader_source()
                 .add_source("item.vert.glsl", WRATHGLShader::from_resource),
                 WRATHGLShader::shader_source()
                 .add_source("item.frag.glsl", WRATHGLShader::from_resource))
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

  
  //make our widget
  Widget::parameters params;
  
  //geometry properties
  params.m_polygon_spec.m_number_sides=30;
  params.m_polygon_spec.m_center=vec2(300.0f, 300.0f);
  params.m_polygon_spec.m_radius=150.0f;

  //set the brush and shader
  m_gradient=WRATHNew WRATHGradient("my gradient");
  m_gradient->set_color(0.00f, WRATHGradient::color(1.0f, 0.0f, 0.0f, 1.0f));
  m_gradient->set_color(0.25f, WRATHGradient::color(0.0f, 1.0f, 0.0f, 1.0f));
  m_gradient->set_color(0.50f, WRATHGradient::color(0.0f, 0.0f, 1.0f, 1.0f));
  m_gradient->set_color(0.75f, WRATHGradient::color(1.0f, 1.0f, 1.0f, 1.0f));

  WRATHBrush brush(m_gradient);

  //the class Widget has the static method set_shader_brush 
  //to set the shader code associated for the brush
  //for details, see \ref WRATHLayerItemNodeBase::set_shader_brush
  Widget::set_shader_brush(brush);

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
  m_widget->set_from_brush(brush);

  //set the color and linear gradient position of the widget.
  m_widget->set_gradient(vec2(400.0f, 300.0f), vec2(0.0f, 0.0f));
  m_widget->color(vec4(1.0f, 1.0f, 1.0f, 1.0f));

  glClearColor(1.0, 1.0, 1.0, 1.0);
}

BrushExample::
~BrushExample()
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
BrushExample::
resize(int width, int height)
{
  float_orthogonal_projection_params proj_params(0, width, height, 0);
  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));
  glViewport(0, 0, width, height);
}

void 
BrushExample::
paint(void)
{
  m_tr->signal_complete_simulation_frame();
  m_tr->signal_begin_presentation_frame();
  m_layer->clear_and_draw();
}

void 
BrushExample::
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
