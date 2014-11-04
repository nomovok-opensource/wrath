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
#include "WRATHTime.hpp"

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
  command_line_argument_value<int> m_virtual_height, m_virtual_width, m_layer_count;

  cmd_line_type(void):
    m_virtual_height(128, "virtual_height", "Virtual height to which to scale display", *this),
    m_virtual_width(256, "virtual_width", "Virtual width to which to scale display", *this),
    m_layer_count(100, "layer_count", "# of full screen blends underneath text", *this)
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

class CounterExample:public DemoKernel
{
public:
  CounterExample(cmd_line_type *cmd_line);
  ~CounterExample();
  
  void resize(int width, int height);
  virtual void handle_event(FURYEvent::handle ev);
  virtual void paint(void);

private:
  typedef WRATHLayerTranslateFamilySet FamilySet;


  typedef FamilySet::PlainFamily::TextWidget TextWidget;
  typedef FamilySet::ColorRadialGradientFamily::RectWidget RectWidget;

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  WRATHLayer *m_child_layer;
  
  unsigned int m_frame;
  WRATHTime m_time, m_total_time;
  unsigned int m_virtual_height;

  WRATHGradient *m_gradient;

  std::vector<RectWidget*> m_rects;
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


  m_child_layer=WRATHNew WRATHLayer(m_layer);

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

  float_orthogonal_projection_params proj_params(0, cmd_line->m_virtual_width.m_value,
                                                 cmd_line->m_virtual_height.m_value, 0);

  m_virtual_height=cmd_line->m_virtual_height.m_value;
  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));

  /*
    create the text widget
   */
  m_text_widget=WRATHNew TextWidget(m_layer, WRATHTextItemTypes::text_opaque);
  

  /*
    add the text to the text widget
   */
  m_text_widget->position(vec2(0.0f, 0.0f));
  
  
  m_gradient=WRATHNew WRATHGradient("my little gradient");
  m_gradient->set_color(0.00f, WRATHGradient::color(1.0f, 0.0f, 0.0f, 1.0f));
  m_gradient->set_color(0.25f, WRATHGradient::color(0.0f, 1.0f, 0.0f, 1.0f));
  m_gradient->set_color(0.50f, WRATHGradient::color(0.0f, 0.0f, 1.0f, 1.0f));
  m_gradient->set_color(0.75f, WRATHGradient::color(1.0f, 1.0f, 1.0f, 1.0f));


  //create the brush, the node type specifies the shader
  WRATHBrush brush(type_tag<RectWidget::Node>(), m_gradient);
  RectWidget::Drawer drawer(brush, WRATHDrawType::transparent_pass());

  m_rects.resize(std::max(0, cmd_line->m_layer_count.m_value));
  for(unsigned int i=0, endi=m_rects.size(); i<endi ; ++i)
    {
      m_rects[i]=WRATHNew RectWidget(m_child_layer, drawer);
      m_rects[i]->color(WRATHGradient::color(1.0f, 1.0f, 1.0f, 0.2f));
      m_rects[i]->properties()->set_parameters(WRATHDefaultRectAttributePacker::rect_properties(cmd_line->m_virtual_width.m_value,
                                                                                                cmd_line->m_virtual_height.m_value));
    }


  //clear color all white
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
  glViewport(0, 0, width, height);
}

void CounterExample::paint(void)
{
  WRATHTextDataStream stream;
  int32_t ms, total_ms;
  
  if(m_frame==0)
    {
      ms=total_ms=0;
      m_time.restart();
      m_total_time.restart();
    }
  else
    {
      ms=m_time.restart();
      total_ms=m_total_time.elapsed();
    }

  stream.stream() << WRATHText::set_pixel_size(m_virtual_height/2)
                  << WRATHText::set_color(0, 0, 0)
                  << WRATHText::set_font(WRATHFontDatabase::FontProperties()
                                         .family_name("DejaVuSans"),
                                         type_tag<WRATHTextureFontFreeType_Analytic>())
                  << m_frame
                  << "\n" << ms << " ms\n";

  m_text_widget->clear();
  m_text_widget->add_text(stream);

  for(unsigned int i=0, endi=m_rects.size(); i<endi ; ++i)
    {
      vec2 p;
      float r0, r1, theta, d;

      theta=(static_cast<float>(total_ms)/500.0f + static_cast<float>(i)/4.0f) * 2.0f * float(M_PI);
      d=static_cast<float>(i)/static_cast<float>(endi) * static_cast<float>(m_virtual_height);
      p.x()=d*cosf(theta);
      p.y()=d*sinf(theta);
      r0=0.0f;
      r1=m_virtual_height*2.0f;
      m_rects[i]->set_gradient(p, r0, p, r1);
    }

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
