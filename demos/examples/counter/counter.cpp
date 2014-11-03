/*! 
 * \file text.cpp
 * \brief file text.cpp
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
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "WRATHmalloc.hpp"
#include "WRATHUtil.hpp"
#include "WRATHTextureFontFreeType_Mix.hpp"
#include "WRATHFontFetch.hpp"
#include "WRATHLayerItemWidgetsTranslate.hpp"

#include "ngl_backend.hpp"
#include "wrath_demo.hpp"

/*!
\details
This example demonstrates the basic usage of the \ref Text
Module API for the typical "hello world" example.
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

class CounterExample:public DemoKernel
{
public:
  CounterExample(cmd_line_type *cmd_line);
  ~CounterExample();
  
  void resize(int width, int height);
  virtual void handle_event(FURYEvent::handle ev);
  virtual void paint(void);

private:

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  unsigned int m_frame;

  /*
    High level widget that takes care of rendering text
    */
  typedef WRATHLayerTranslateFamilySet::PlainFamily::TextWidget TextWidget;
  TextWidget *m_text_widget;
};



DemoKernel* 
cmd_line_type::
make_demo(void)
{
  return WRATHNew CounterExample(this);
}
  

CounterExample::
CounterExample(cmd_line_type *cmd_line):
  DemoKernel(cmd_line),
  m_layer(NULL),
  m_frame(0),
  m_text_widget(0)
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

  /*
    create the text widget
   */
  m_text_widget=WRATHNew TextWidget(m_layer, WRATHTextItemTypes::text_opaque);
  

  /*
    add the text to the text widget
   */
  m_text_widget->position(vec2(0.0f, 0.0f));
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

CounterExample::~CounterExample()
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

void CounterExample::resize(int width, int height)
{
  float_orthogonal_projection_params proj_params(0, width, height, 0);
  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));
  glViewport(0, 0, width, height);
}

void CounterExample::paint(void)
{
  WRATHTextDataStream stream;
  
  stream.stream() << WRATHText::set_pixel_size(64)
                  << WRATHText::set_color(0, 0, 0)
                  << WRATHText::set_font(WRATHFontDatabase::FontProperties()
                                         .bold(true)
                                         .family_name("DejaVuSans"),
                                         type_tag<WRATHTextureFontFreeType_Distance>())
                  << m_frame;

  m_text_widget->clear();
  m_text_widget->add_text(stream);
  m_tr->signal_complete_simulation_frame();
  m_tr->signal_begin_presentation_frame();
  m_layer->clear_and_draw();

  ++m_frame;
  update_widget();
}

void CounterExample::handle_event(FURYEvent::handle ev)
{
  if(ev->type()==FURYEvent::Resize)
    {
      FURYResizeEvent::handle rev(ev.static_cast_handle<FURYResizeEvent>());
      resize(rev->new_size().x(), rev->new_size().y());
    }
  else if(ev->type()==FURYEvent::KeyUp)
    {
      FURYKeyEvent::handle qe(ev.static_cast_handle<FURYKeyEvent>());
      if(qe->key().m_value == FURYKey_Escape || qe->key().m_value == FURYKey_Q)
        {
          end_demo();
        }
    }
}

int main(int argc, char **argv)
{
    cmd_line_type cmd_line;
    return cmd_line.main(argc, argv);
}
