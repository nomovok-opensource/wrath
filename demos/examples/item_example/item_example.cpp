/*! 
 * \file item_example.cpp
 * \brief file item_example.cpp
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
  In this example we deomonstrate a bare bones implementation
  of drawing a regular polygon. In this example we create
  a type derived from \ref WRATHBaseItem using built in
  helper types from \ref WRATHItemTypes to specify how
  to draw the item. In addition, the item type we make,
  item, is compatible with \ref WRATHWidgetGenerator
  so we use it with elements from the \ref Layer
  module to create a widget. 

  See the follow up example, \ref item_example2_example
  for expanding the polygon to draw in multiple
  passes and for allocating and setting the attributes
  when the attributes are allocated fragmented.
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

class ItemExample:public DemoKernel
{
public:
  ItemExample(cmd_line_type *cmd_line);
  ~ItemExample();
  
  void 
  resize(int width, int height);
  
  virtual
  void
  handle_event(FURYEvent::handle ev);
  
  virtual
  void
  paint(void);

private:
  typedef WRATHLayerTranslateFamilySet::CPlainFamily Family;
  typedef WRATHGenericWidget<item, Family::WidgetBase> Widget;

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  Widget *m_widget;
};



DemoKernel* 
cmd_line_type::
make_demo(void)
{
  return WRATHNew ItemExample(this);
}
  

ItemExample::
ItemExample(cmd_line_type *cmd_line):
  DemoKernel(cmd_line)
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

  //shader
  WRATHShaderSpecifier *sp;
  WRATHGLShader::shader_source vs, fs;
  vs.add_source("item.vert.glsl", WRATHGLShader::from_resource);
  fs.add_source("item.frag.glsl", WRATHGLShader::from_resource);
  sp=WRATHNew WRATHShaderSpecifier("item_shader", vs, fs);
  
  //make our widget
  Widget::parameters params;
  
  //geometry properties
  params.m_number_sides=30;
  params.m_center=vec2(300.0f, 300.0f);
  params.m_radius=150.0f;

  params.m_drawer.m_shader=sp;

  m_widget=WRATHNew Widget(m_layer, params);
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

ItemExample::
~ItemExample()
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
ItemExample::
resize(int width, int height)
{
  float_orthogonal_projection_params proj_params(0, width, height, 0);
  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));
  glViewport(0, 0, width, height);
}

void 
ItemExample::
paint(void)
{
  m_tr->signal_complete_simulation_frame();
  m_tr->signal_begin_presentation_frame();
  m_layer->clear_and_draw();
}

void 
ItemExample::
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
