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



/*!\details
  In this example we will create a custom 
  vertex and fragment shader for presenting
  glyphs. The key class for the interface
  is \ref WRATHFontShaderSpecifier
*/

/*
  Choose how the font is realized,
 */
//typedef WRATHTextureFontFreeType_Distance FontType;
typedef WRATHTextureFontFreeType_CurveAnalytic FontType;
//typedef WRATHMixFontTypes<WRATHTextureFontFreeType_Analytic>::mix FontType;

/*
  Choose how to pack per-node values
 */
//typedef WRATHLayerItemWidgetSupport::DefaultNodePacker Packer;
typedef WRATHLayerNodeValuePackerUniformArrays Packer;
//typedef WRATHLayerNodeValuePackerTextureFP32 Packer;
//typedef WRATHLayerNodeValuePackerTextureFP16 Packer;

class cmd_line_type:public DemoKernelMaker
{
public:
  command_line_argument_value<std::string> m_text;
  command_line_argument_value<bool> m_text_from_file;
  command_line_argument_value<int> m_r, m_g, m_b, m_a;
  command_line_argument_value<bool> m_bold, m_italic, m_draw_outline;
  command_line_argument_value<std::string> m_family;  
  command_line_argument_value<int> m_pixel_size, m_wrath_font_size;
  command_line_argument_value<bool> m_show_font_file_name;
  command_line_argument_value<bool> m_animate;

  cmd_line_type(void):
    m_text("Hello Wavy World\n\tscroll by panning"
           "\n\tzoom by holding then panning",
           "text", "Text to use for demo", *this),
    m_text_from_file(false, "text_from_file", 
                     "If true, text command line paramater indicates text file to display", *this),
    m_r(255, "color_r", "Red component in range [0,255] of text color", *this),
    m_g(255, "color_g", "Green component in range [0,255] of text color", *this),
    m_b(255, "color_b", "Blue component in range [0,255] of text color", *this),
    m_a(255, "color_a", "Alpha component in range [0,255] of text color", *this),
    m_bold(false, "bold", "if true, use bold font", *this),
    m_italic(false, "italic", "uf true, use italic font", *this),
    m_draw_outline(false, "draw_outline", "If true, draw a colored outline pattern for text", *this),
    m_family("DejaVuSans", "family", "Family of font", *this),
    m_pixel_size(32, "pixel_size", "Pixel size at which to display the text", *this),
    m_wrath_font_size(48, "wrath_font_size", "Pixel size to realize the font at", *this),
    m_show_font_file_name(false, "show_font_file_name", 
                          "If true also display filename of font", *this),
    m_animate(true, "animate", "If true, make the waviness of the text animate", *this)
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
  typedef WRATHLayerItemNodeTranslate BaseNode;
  typedef WobblyNode<BaseNode> Node;
  typedef WRATHLayerItemWidget<Node, Packer>::FamilySet FamilySet; 
  typedef FamilySet::PlainFamily PlainFamily;
  typedef PlainFamily::NodeWidget NodeWidget;
  typedef PlainFamily::TextWidget TextWidget;

  
  void
  handle_touch_begin(vec2 pt);

  void
  handle_touch_begin(ivec2 pt)
  {
    handle_touch_begin(vec2(pt.x(), pt.y()));
  }

  void
  handle_touch_end(vec2 pt);

  void
  handle_touch_end(ivec2 pt)
  {
    handle_touch_end(vec2(pt.x(), pt.y()));
  }

  void
  handle_touch_move(vec2 pt, vec2 delta); 

  void
  handle_touch_move(ivec2 pt, ivec2 delta)
  {
    handle_touch_move(vec2(pt.x(), pt.y()),
                      vec2(delta.x(), delta.y()));
  }
  
  void
  move_node(Node *pnode, float delta_t);

  WRATHFontShaderSpecifier *m_present_text;
  TextWidget *m_text_widget;
  NodeWidget *m_node_widget;

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  WRATHTime m_time, m_total_time;
  bool m_first_frame;

  vec2 m_zoom_pivot;
  WRATHTime m_zoom_time;
  bool m_is_zooming, m_button_down;
  WRATHScaleTranslate m_zoom_start_transformation;

  int32_t m_zoom_gesture_begin_time;
  float m_zoom_dividier;

  bool m_animate;
};




WavyTextExample::
WavyTextExample(cmd_line_type *cmd_line):
  DemoKernel(cmd_line),
  m_first_frame(true),
  m_is_zooming(false),
  m_button_down(false),
  m_zoom_gesture_begin_time(500),
  m_zoom_dividier(40.0f),
  m_animate(cmd_line->m_animate.m_value)
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
  m_node_widget=WRATHNew NodeWidget(m_layer);

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
  m_present_text=WRATHNew WRATHFontShaderSpecifier("my custom font presenter");

  /*
    append vertex shader code
   */
  m_present_text->append_vertex_shader_source()
    .add_source("wobbly.vert.glsl", WRATHGLShader::from_resource);

  /*
    append fragment sahder code
  */
  if(cmd_line->m_draw_outline.m_value)
    {
      m_present_text->append_fragment_shader_source()
        .add_macro("DRAW_OUTLINE");
    }
  m_present_text->append_fragment_shader_source()
    .add_source("wobbly.frag.glsl", WRATHGLShader::from_resource);

  /*
    the presentation shader of wobbly.vert/frag.glsl
    uses non-linear for position of fragment within
    a glyph.
   */
  m_present_text->linear_glyph_position(false);


  m_text_widget=WRATHNew TextWidget(m_node_widget, WRATHTextItemTypes::text_opaque);

  WRATHFontFetch::default_font_pixel_size(cmd_line->m_wrath_font_size.m_value);

  WRATHTextDataStream stream;
  stream.stream() << WRATHText::set_pixel_size(cmd_line->m_pixel_size.m_value)
                  << WRATHText::set_font(WRATHFontDatabase::FontProperties()
                                         .bold(cmd_line->m_bold.m_value)
                                         .italic(cmd_line->m_italic.m_value)
                                         .family_name(cmd_line->m_family.m_value),
                                         type_tag<FontType>())
                  << WRATHText::set_font_shader(m_present_text);

  if(cmd_line->m_show_font_file_name.m_value)
    {
      WRATHTextureFont *fnt;
      stream.stream() << "\nFont File:\""
                      << WRATHText::set_color(255-cmd_line->m_r.m_value, 
                                              cmd_line->m_g.m_value, 
                                              255-cmd_line->m_b.m_value, 
                                              cmd_line->m_a.m_value)
                      << WRATHText::get_font(fnt);
  
      stream.stream() << fnt->source_font()->name()
                      << "\"\n";
    }

  stream.stream() << WRATHText::set_color(cmd_line->m_r.m_value, 
                                          cmd_line->m_g.m_value, 
                                          cmd_line->m_b.m_value, 
                                          cmd_line->m_a.m_value);

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
  m_text_widget->m_wobble_magnitude=0.1f;
  m_text_widget->m_wobble_phase=0.0f;
  m_text_widget->m_wobble_freq=2.0f;
   
                    
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

  if(m_animate)
    {
      uint32_t modulas, period_in_ms(3000);
      float cycle;
      
      modulas=m_total_time.elapsed()%period_in_ms;
      cycle=static_cast<float>(modulas)/static_cast<float>(period_in_ms);
      
      pnode->m_wobble_magnitude=0.1f;
      pnode->m_wobble_phase=cycle*M_PI*2.0f;  
      pnode->m_wobble_freq=2.0f;
    }
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
  switch(ev->type())
    {
    case FURYEvent::Resize:
      {
        FURYResizeEvent::handle rev(ev.static_cast_handle<FURYResizeEvent>());
        resize(rev->new_size().x(), rev->new_size().y());
        ev->accept();
      }
      break;

    case FURYEvent::TouchDown:
      {
        FURYTouchEvent::handle tev(ev.static_cast_handle<FURYTouchEvent>());

        handle_touch_begin(tev->position());
        tev->accept();          
      }
      break;

    case FURYEvent::TouchUp:
      {
        FURYTouchEvent::handle tev(ev.static_cast_handle<FURYTouchEvent>());
        handle_touch_end(tev->position());
      }
      break;

    case FURYEvent::TouchMotion:
      {
        FURYTouchEvent::handle tev(ev.static_cast_handle<FURYTouchEvent>());
        handle_touch_move(tev->position(), tev->delta()); 
        tev->accept();
      }
      break;
      
    case FURYEvent::MouseMotion:
      if(m_button_down)
        {
          FURYMouseMotionEvent::handle mev(ev.static_cast_handle<FURYMouseMotionEvent>());
          handle_touch_move(mev->pt(), mev->delta());
        }
      break;

    case FURYEvent::MouseButtonUp:
        {
          FURYMouseButtonEvent::handle me(ev.static_cast_handle<FURYMouseButtonEvent>());
          m_button_down=false;
          handle_touch_end(me->pt());
          ev->accept();
        }
      break;

    case FURYEvent::MouseButtonDown:
        {
          FURYMouseButtonEvent::handle me(ev.static_cast_handle<FURYMouseButtonEvent>());
          m_button_down=true;
          handle_touch_begin(me->pt());
          ev->accept();
        }
      break;
    

    default:
      break;
    }

}

void
WavyTextExample::
handle_touch_begin(vec2 pt)
{
  m_zoom_pivot=pt;                  
  m_zoom_start_transformation=m_node_widget->transformation();
  m_zoom_time.restart();
}

void
WavyTextExample::
handle_touch_end(vec2)
{
  m_is_zooming=false;
}


void
WavyTextExample::
handle_touch_move(vec2 pt, vec2 delta)
{
  if(m_zoom_time.elapsed()>m_zoom_gesture_begin_time)
    {
      m_is_zooming=true;
    }  
  
  if(!m_is_zooming)
    {
      float zdx(pt.x()-m_zoom_pivot.x());
      float zdy(pt.y()-m_zoom_pivot.y());
      
      m_node_widget->translation( m_node_widget->translation() + delta);
      
      //if zooming did not start yet and the touch event
      //is too far from the zoom pivot point, then zooming
      //is not going to happen, rather than have yet another flag,
      //we just restart the timer an dupdate the zoom pivot
      //position. This way, if a user does not release their
      //finger from the device but holds it steady, they can shift
      //into a zoom gesture.
      if(std::abs(zdx)>m_zoom_dividier or std::abs(zdy)>m_zoom_dividier)
        {
          m_zoom_time.restart();
          m_zoom_pivot=pt;
          m_zoom_start_transformation=m_node_widget->transformation();
        }
    }
  else
    {
      float zoom_factor(pt.y()-m_zoom_pivot.y());
      vec2 p0(m_zoom_pivot);
      WRATHScaleTranslate R, P(m_zoom_start_transformation);
      
      zoom_factor/=m_zoom_dividier;
      if(zoom_factor<0.0f)
        {
          zoom_factor=-1.0f/std::min(-1.0f, zoom_factor);
        }
      else
        {
          zoom_factor=std::max(1.0f, zoom_factor);
        }
      
      R.scale(zoom_factor);
      R.translation( (1.0f-zoom_factor)*p0);
      m_node_widget->transformation(R*P);
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
