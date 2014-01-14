/*! 
 * \file wavy-text.cpp
 * \brief file wavy-text.cpp
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
#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "WRATHmalloc.hpp"
#include "WRATHUtil.hpp"
#include "WRATHWidget.hpp"
#include "WRATHTime.hpp"
#include "WRATHFontFetch.hpp"
#include "WRATHLayerItemNodeTranslate.hpp"
#include "WRATHLayerItemWidgets.hpp"

#include "WRATHTextureFontFreeType_Coverage.hpp"
#include "WRATHTextureFontFreeType_DetailedCoverage.hpp"
#include "WRATHTextureFontFreeType_Distance.hpp"
#include "WRATHTextureFontFreeType_Analytic.hpp"
#include "WRATHTextureFontFreeType_CurveAnalytic.hpp"
#include "WRATHTextureFontFreeType_Mix.hpp"

#include "wrath_demo.hpp"
#include "wrath_demo_image_support.hpp"

#include "wobbly_node.hpp"

typedef WRATHTextureFontFreeType_CurveAnalytic FontType;

/*!\details
  In this example we will create a custom 
  vertex and fragment shader for presenting
  glyphs. The key class for the interface
  is \ref WRATHFontShaderSpecifier
*/

class cmd_line_type:public DemoKernelMaker
{
public:
  command_line_argument_value<std::string> m_text;
  command_line_argument_value<bool> m_text_from_file;
  command_line_argument_value<int> m_r, m_g, m_b, m_a;
  command_line_argument_value<bool> m_bold, m_italic;
  command_line_argument_value<std::string> m_style;  
  command_line_argument_value<int> m_pixel_size;

  cmd_line_type(void):
    m_text("Hello Wavy World", "text", "Text to use for demo", *this),
    m_text_from_file(false, "text_from_file", 
                     "If true, text command line paramater indicates text file to display", *this),
    m_r(255, "color_r", "Red component in range [0,255] of text color", *this),
    m_g(255, "color_g", "Green component in range [0,255] of text color", *this),
    m_b(255, "color_b", "Blue component in range [0,255] of text color", *this),
    m_a(255, "color_a", "Alpha component in range [0,255] of text color", *this),
    m_bold(false, "bold", "Bold text", *this),
    m_italic(false, "italic", "Italic text", *this),
    m_style("DejaVuSans", "style", "Style of font", *this),
    m_pixel_size(32, "pixel_size", "Pixel size at which to display the text", *this)
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

class WavyTextExample:public DemoKernel
{
public:
  WavyTextExample(cmd_line_type *cmd_line);
  ~WavyTextExample();
  
  void resize(int width, int height);
  virtual void handle_event(FURYEvent::handle ev);
  virtual void paint(void);

private:
  typedef WobblyNode<WRATHLayerItemNodeTranslate> Node;
  typedef WRATHLayerItemWidget<Node>::FamilySet FamilySet; 
  typedef FamilySet::PlainFamily PlainFamily;
  typedef PlainFamily::TextWidget TextWidget;
  
  void
  move_node(Node *pnode, float delta_t);

  WRATHFontShaderSpecifier *m_present_text;
  TextWidget *m_text_widget;

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  WRATHTime m_time, m_total_time;
  bool m_first_frame;
};




WavyTextExample::
WavyTextExample(cmd_line_type *cmd_line):
  DemoKernel(cmd_line),
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

  /*
    create our WRATHFontShaderSpecifier
   */
  m_present_text=WRATHNew WRATHFontShaderSpecifier("my custom font presenter",
                                                   WRATHGLShader::shader_source()
                                                   .add_source("wobbly.vert.glsl", WRATHGLShader::from_resource),
                                                   WRATHGLShader::shader_source()
                                                   .add_source("wobbly.frag.glsl", WRATHGLShader::from_resource));
  /*
    the presentation shader of wobbly.vert/frag.glsl
    uses non-linear for position of fragment within
    a glyph.
   */
  m_present_text->linear_glyph_position(false);


  m_text_widget=WRATHNew TextWidget(m_layer, WRATHTextItemTypes::text_opaque);

  WRATHTextDataStream stream;
  stream.stream() << WRATHText::set_pixel_size(cmd_line->m_pixel_size.m_value)
                  << WRATHText::set_color(cmd_line->m_r.m_value, 
                                          cmd_line->m_g.m_value, 
                                          cmd_line->m_b.m_value, 
                                          cmd_line->m_a.m_value)
                  << WRATHText::set_font(WRATHFontDatabase::FontProperties()
                                         .bold(cmd_line->m_bold.m_value)
                                         .italic(cmd_line->m_italic.m_value)
                                         .style_name(cmd_line->m_style.m_value),
                                         type_tag<FontType>())
                  << WRATHText::set_font_shader(m_present_text);

  if(!cmd_line->m_text_from_file.m_value)
    {
      stream.stream() << "\n" << cmd_line->m_text.m_value << "\n";
    }
  else
    {
      std::ifstream file(cmd_line->m_text.m_value.c_str());
      if(!file)
        {
          stream.stream() << "\nUnable to open file \""
                          << cmd_line->m_text.m_value
                          << "\" for reading";
        }
      else
        {
          stream.stream() << file.rdbuf();
        }
    }

  m_text_widget->add_text(stream);
  m_text_widget->position(vec2(0.0f, 0.0f));

   
                    
  glClearColor(0.0, 0.0, 0.0, 0.0);
}

WavyTextExample::
~WavyTextExample()
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
WavyTextExample::
resize(int width, int height)
{
  float_orthogonal_projection_params proj_params(0, width, height, 0);
  m_layer->simulation_matrix(WRATHLayer::projection_matrix, float4x4(proj_params));
  glViewport(0, 0, width, height);
}

void
WavyTextExample::
move_node(Node *pnode, float delta_t)
{
  vec2 oldp, newp;

  oldp=pnode->position();
  
  newp=oldp + delta_t*pnode->m_velocity;
  pnode->position(newp);

  //make the center within the screen.
  newp+=pnode->m_size*0.5f;

  if(newp.x()<0.0f or newp.x()>static_cast<float>(width()) )
    {
      pnode->m_velocity.x()=-pnode->m_velocity.x();
    }

  if(newp.y()<0.0f or newp.y()>static_cast<float>(height()) )
    {
      pnode->m_velocity.y()=-pnode->m_velocity.y();
    }

  uint32_t modulas, period_in_ms(3000);
  float cycle;

  modulas=m_total_time.elapsed()%period_in_ms;
  cycle=static_cast<float>(modulas)/static_cast<float>(period_in_ms);

  pnode->m_wobble_magnitude=0.1f;
  pnode->m_wobble_phase=cycle*M_PI*2.0f;  
  pnode->m_wobble_freq=2.0;
}

void 
WavyTextExample::
paint(void)
{
  /*
    on the first frame, we make delta_t
    but we also need to restart m_time
    because it started at its construction.
   */
  float secs;
  secs=static_cast<float>(m_time.restart())/1000.0f;
  secs=m_first_frame?
    0.0f:secs;

  
  move_node(m_text_widget, secs);

  m_tr->signal_complete_simulation_frame();
  m_tr->signal_begin_presentation_frame();
  m_layer->clear_and_draw();
  m_first_frame=false;

  update_widget();
}

void 
WavyTextExample::
handle_event(FURYEvent::handle ev)
{
  if(ev->type()==FURYEvent::Resize)
    {
      FURYResizeEvent::handle rev(ev.static_cast_handle<FURYResizeEvent>());
      resize(rev->new_size().x(), rev->new_size().y());
    }
}



DemoKernel* 
cmd_line_type::
make_demo(void)
{
  return WRATHNew WavyTextExample(this);
}
  

int 
main(int argc, char **argv)
{
    cmd_line_type cmd_line;
    return cmd_line.main(argc, argv);
}
