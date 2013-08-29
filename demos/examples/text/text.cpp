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
#include "WRATHNew.hpp"
#include <fstream>
#include <iomanip>
#include <limits>
#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "WRATHmalloc.hpp"
#include "WRATHUtil.hpp"
#include <stdlib.h>
#include "WRATHShapeItem.hpp"
#include "WRATHShapeSimpleTessellator.hpp"
#include "WRATHDefaultFillShapeAttributePacker.hpp"
#include "WRATHDefaultStrokeAttributePacker.hpp"
#include "WRATHLayerItemWidgetsTranslate.hpp"
#include "WRATHTextureFontFreeType_Mix.hpp"
#include "WRATHFontFetch.hpp"
#include <dirent.h>
#include "ngl_backend.hpp"
#include <stdio.h>
#include "wrath_demo.hpp"

/*!
\details
This example demonstrates the basic usage of the \ref Text
Module API for the typical "hello world" example.
 */

class silly_type
{
public:
  silly_type(void)
  {}

  friend
  std::ostream&
  operator<<(std::ostream &ostr, silly_type)
  {
    ostr << "silly_type";
    return ostr;
  }

};

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

class TextExample:public DemoKernel
{
public:
  TextExample(cmd_line_type *cmd_line);
  ~TextExample();
  
  void resize(int width, int height);
  virtual void handle_event(FURYEvent::handle ev);
  virtual void paint(void);

private:

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;

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
  return WRATHNew TextExample(this);
}
  

TextExample::
TextExample(cmd_line_type *cmd_line):
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

  /*
    create the text widget
   */
  m_text_widget=WRATHNew TextWidget(m_layer, WRATHTextItemTypes::text_opaque);

  WRATHTextDataStream stream;
  vec4 color(1.0f, 0.55f, 0.33f, 1.0f);
  int some_integer(12);

  stream.stream() << WRATHText::set_pixel_size(24)
                  << WRATHText::set_color(0, 0, 0)
                  << "Hello in pixel size 24\n"
                  << "can print other objects: " << silly_type()
                  << "\ninfact, just overload"
                  << "std::ostream &operator<<(std::osream&, const ObjectType&) "
                  << "\nand it will work, like integers" << some_integer
                  << "\nIO manipulators work too: " 
                  << "\nInt:" << std::setw(5) << 123 << "i"
                  << "\nInt:" << std::setw(5) << 12 << "i"
                  << "\nInt:" << std::setw(5) << 1 << "i"
                  << "\nInt:" << std::setw(5) << 1234 << "i"
                  << "\njust remember that fonts are not usually fixed width"
                  << WRATHText::set_color(222, 0, 200)
                  << "\nChanged the color to (222, 0, 200)"
                  << WRATHText::set_font(WRATHFontDatabase::FontProperties()
                                         .bold(true)
                                         .style_name("DejaVuSans"))
                  << "\nChanged the font to DejaVu Sans bold font\n"
                  << "\nC" << WRATHText::set_color(10, 155, 255)
                  << "h" << WRATHText::set_color(155, 0, 255)
                  << "a" << WRATHText::set_color(155, 255, 0)
                  << "n" << WRATHText::set_color(0, 255, 0)
                  << "g" << WRATHText::set_color(255, 255, 0)
                  << "e" << WRATHText::set_color(155, 255, 155)
                  << " the color at any time without affecting formatting"
                  << WRATHText::set_font(WRATHFontDatabase::FontProperties()
                                         .bold(true)
                                         .style_name("DejaVuSans"),
                                         type_tag<WRATHTextureFontFreeType_Analytic>())
                  << "\nSame font but a different realization";

  stream.wstream() << L"\nAlso Wide streams are supported, since this "
                   << L"was streamed after the stream() above it comes after AND "
                   << L"the format, color, font, shader, etc are also applied too ";
    
  stream.stream() << "\nThis text will appear AFTER the wide character text above"; 

  /*
    add the text to the text widget
   */
  m_text_widget->add_text(stream);
  m_text_widget->position(vec2(0.0f, 0.0f));
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

TextExample::~TextExample()
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

void TextExample::resize(int width, int height)
{
  float_orthogonal_projection_params proj_params(0, width, height, 0);
  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));
  glViewport(0, 0, width, height);
}

void TextExample::paint(void)
{
  m_tr->signal_complete_simulation_frame();
  m_tr->signal_begin_presentation_frame();
  m_layer->clear_and_draw();
}

void TextExample::handle_event(FURYEvent::handle ev)
{
  if(ev->type()==FURYEvent::Resize)
    {
      FURYResizeEvent::handle rev(ev.static_cast_handle<FURYResizeEvent>());
      resize(rev->new_size().x(), rev->new_size().y());
    }
}

int main(int argc, char **argv)
{
    cmd_line_type cmd_line;
    return cmd_line.main(argc, argv);
}
