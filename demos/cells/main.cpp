/*! 
 * \file main.cpp
 * \brief file main.cpp
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
#include "WRATHgl.hpp"
#include <dirent.h>
#include "WRATHFontFetch.hpp"
#include "WRATHTextureFontFreeType_Mix.hpp"
#include "WRATHDynamicStrokeAttributePacker.hpp"
#include "WRATHDefaultRectAttributePacker.hpp"
#include "WRATHDefaultTextAttributePacker.hpp"
#include "WRATHTime.hpp"
#include "NodePacker.hpp"

#include "Cell.hpp"
#include "Table.hpp"

#include "wrath_demo.hpp"
#include "wrath_demo_image_support.hpp"
#include "ngl_backend.hpp"

#ifdef WRATH_QT 
#define DefaultThickenKey FURYKey_VolumeDown 
#define DefaultThinnenKey FURYKey_VolumeUp
#else
#define DefaultThickenKey FURYKey_Q
#define DefaultThinnenKey FURYKey_W
#endif

class cmd_line_type:public DemoKernelMaker
{
public:

  //generic option for all jazz
  command_line_argument_value<int> m_max_transformations;

  //logging options
  command_line_argument_value<std::string> m_log_GL, m_log_alloc;    
  command_line_argument_value<bool> m_print_events;

  //text renderer options
  command_line_argument_value<int> m_text_renderer;
  command_line_argument_value<int> m_text_renderer_line_analytic_format;
  command_line_argument_value<bool> m_text_renderer_curve_analytic_format;
  command_line_argument_value<int> m_text_renderer_sub_choice;
  command_line_argument_value<int> m_text_renderer_coverage_min_filter;
  command_line_argument_value<int> m_text_renderer_converage_mag_filter;
  command_line_argument_value<int> m_text_renderer_converage_deepness_slack; 
  command_line_argument_value<int> m_text_renderer_analytic_mipmap_level;
  command_line_argument_value<bool> m_text_renderer_emulate_max_tex_level;
  command_line_argument_value<float> m_mix_font_div_ratio;
  command_line_argument_value<float> m_font_discard_thresh;
  command_line_argument_value<float> m_max_distance_font_generation;
  command_line_argument_value<GLint> m_font_texture_size;
  command_line_argument_value<bool> m_font_texture_force_power2; 
  command_line_argument_value<bool> m_font_lazy_z;

  //font loading parameters
  command_line_argument_value<float> m_display_font_size;
  command_line_argument_value<int> m_font_size;
  command_line_argument_value<std::string> m_font_name;
  command_line_argument_value<int> m_font_face_index;
  command_line_argument_value<bool> m_use_font_config;


  //image options
  command_line_argument_value<int> m_atlas_size;
  command_line_argument_value<bool> m_image_use_mipmaps;
  command_line_argument_value<bool> m_manual_mipmap_generation;

  //touch/screen options
  command_line_argument_value<bool> m_rotate;
  command_line_argument_value<bool> m_grab_keyboard, m_grab_mouse;  
  command_line_argument_value<bool> m_touch_emulate;
  command_line_argument_value<int> m_double_click_time;
  command_line_argument_value<int> m_zoom_gesture_begin_time;
  command_line_argument_value<float> m_zoom_dividier;
  
  //button options
  command_line_argument_value<uint32_t> m_thicken_key;
  command_line_argument_value<uint32_t> m_thinnen_key;
  command_line_argument_value<float> m_thicken_thinnen_rate;


  //what to display
  command_line_argument_value<int> m_cell_count_x;
  command_line_argument_value<int> m_cell_count_y;
  command_line_argument_value<float> m_table_size_x;
  command_line_argument_value<float> m_table_size_y;
  command_line_argument_value<std::string> m_image_dir;

  cmd_line_type(void):
    DemoKernelMaker(),   
    m_max_transformations(100, "max_tr", 
                          "Maximum number of transformation nodes per draw call", 
                          *this),
    m_log_GL("", "log_gl", "If non empty, logs GL commands to the named file", *this),
    m_log_alloc("", "log_alloc", 
                "If non empty, logs allocs and deallocs to the named file", *this),
    m_print_events(false, "print_events", "If true, print events to console", *this),


    //text options...
    m_text_renderer(3, "text_renderer", 
                    "Specify text renderer, 0=FreeType alpha, "
                    "1=multi-res coverage, 2=distance, 3=analytic, 4=curve_analytic",
                    *this),

    m_text_renderer_line_analytic_format(1, "line_analytic_format",
                                         "Only has affect if text_renderer is 3 "
                                         "Select texture format for analytic "
                                         "text renderer(only has affect for values "
                                         "0=use (GRBA8, RGBA8), "
                                         "1=use (RGBA8, LA_16F), "
                                         "2=use (RGBA8, LA_32F), ", 
                                         *this),

    m_text_renderer_curve_analytic_format(true, "curve_analytic_include_scale_data",
                                          "Only has affect if text_renderer is 4 "
                                          "if on curve analytic texture includes scaling data",
                                          *this),
                                     
    m_text_renderer_sub_choice(1, "text_renderer_sub_choice",
                               "0=no AA, 1=AA, 2=mix with coverage "
                               "3=mix with multi-res coverage, "
                               "4=mix with multi-res adaptive quad coverage "
                               "5=mix with same shader type",
                               *this),

    m_text_renderer_coverage_min_filter(3, "text_coverage_min", 
                                        "minification filter for coverage glyph texture: "
                                        "0=GL_NEAREST, 1=GL_LINEAR, "
                                        "2=GL_NEAREST_MIPMAP_NEAREST, "
                                        "3=GL_LINEAR_MIPMAP_NEAREST, "
                                        "4=GL_NEAREST_MIPMAP_LINEAR, "
                                        "5=GL_LINEAR_MIPMAP_LINEAR",
                                        *this),

    m_text_renderer_converage_mag_filter(1, "text_coverage_mag",
                                         "magnfication filter for glyph texture: "
                                         "0=GL_NEAREST, 1=GL_LINEAR", *this),

    m_text_renderer_converage_deepness_slack(3, "text_coverage_mip_deepness_slack",
                                             "When genering coverage fonts, if using "
                                             "mipmaps, determines the mipmap level used "
                                             "to which to add slack ",
                                             *this),

    m_text_renderer_analytic_mipmap_level(1, "analytic_mipmap_level",
                                          "Number of mipmap levels for an Analytic font "
                                          "to use, 0 indicates to NOT using mipmapping "
                                          "for analytic fonts, only affects if text_renderer is 3",
                                          *this),

    m_text_renderer_emulate_max_tex_level(false, "emulate_max_tex_level",
                                          "If true emulates the behavior of GL_TEXTURE_MAX_LEVEL "
                                          "within the shader, only has affect for analytic font "
                                          "rendering, i.e. text_renderer is 3",
                                          *this), 
    m_mix_font_div_ratio(4.0f, "min_font_div",
                         "When rendering text with 2 seperate font objects, "
                         "determines the ratio of the native pixel size font "
                         "to the minified pixel size font. Parameter "
                         "only has effect if m_text_renderer_sub_choice is 2, 3, 4 or 5",
                         *this),

   
    m_font_discard_thresh(0.9f, "discard_thresh", "Font blending threshold", *this),
    
    m_max_distance_font_generation(96.0f, "font_max_dist", 
                                   "Max distance value used in generating font distance values", 
                                   *this),
    m_font_texture_size(1024, "font_texture_size", 
                        "Max size of each dimention texture of font glyph cache", *this),
    m_font_texture_force_power2(true, "font_pow2", 
                                "If true, font texture size is always a power of 2", *this),

    m_font_lazy_z(true, "font_lazy_z", 
                  "if true, overlapping text not necessarily drawn in correct order", *this), 

    //font loading options    
    m_display_font_size(24, "display_font_size", 
                        "default pixel font size", *this),
    m_font_size(64, "wrath_font_size", 
               "PixelSize of underyling WRATHTextureFonts", *this),
    m_font_name("DejaVuSans", "font_name", 
                   "default font, if use_font_config is true, gives "
                   "the named passed to font config, if use_font_config is false, "
                   "then gives a filename from which to load the font",*this),
    m_font_face_index(0, "font_face", 
                     "face index of default font from "
                     "file named by font_name, "
                     "only used if use_qt_font is false", *this),
    m_use_font_config(true, "use_font_config", 
                      "if true, default font fetched is fetched via FontConfig", *this),


    //image options
    m_atlas_size(2048, "atlas_size", "Size of texture atlas(es)", *this),
    m_image_use_mipmaps(true, "image_use_mipmaps",
                        "If true, use mipmap filtering for images", *this),
    m_manual_mipmap_generation(false, "manual_mipmaps",
                               "If true and if using mipmaps, will generate "
                               "mipmaps in CPU calls rather than using GL's "
                               "glGenerateMipmap", *this),

    //touch/screen options
    m_rotate(false, "rotate", "rotate display 90 degree", *this),
    m_grab_keyboard(false, "grab_keyboard", 
                    "If true grabs the keyboard", *this), 
    m_grab_mouse(false, "grab_mouse", 
                 "If true grabs the mouse", *this),
    m_touch_emulate(false, "emulate_touch", 
                    "If true, mouse events are used to "
                    "emulate touch events", *this),
    m_double_click_time(200, "double_click_max_delay",
                        "Max time between mouse button clicks to consider as double click",
                        *this),
    m_zoom_gesture_begin_time(500, "zoom_time", "Time in ms to trigger zoom gesture", *this),
    m_zoom_dividier(40.0f, "zoom_div", "Zoom divider", *this),
    m_thicken_key(DefaultThickenKey, "thicken_keycode", "Key to press to thicken lines", *this),
    m_thinnen_key(DefaultThinnenKey, "thinnen_keycode", "Key to press to thinnen lines", *this),
    m_thicken_thinnen_rate(10.0f, "ticken_rate", "Thicken/Thinnen rate in pixels/sec", *this),
    

    //what to display
    m_cell_count_x(20, "cell_count_x", "Cell count x", *this),
    m_cell_count_y(10, "cell_count_y", "Cell count y", *this),
    m_table_size_x(4000, "table_size_x", "Table size in pixels x", *this),
    m_table_size_y(2000, "table_size_y", "Table size in pixels y", *this),
    m_image_dir("", "image_dir", 
                "If non-empty string, use all images from the specified image directory", 
                *this)
  {}

  virtual
  DemoKernel*
  make_demo(void);

  virtual
  void
  delete_demo(DemoKernel*);
};

class TableView:public DemoKernel
{
public:
  TableView(cmd_line_type &cmd_line);

  ~TableView();

  virtual
  void
  paint(void);

  virtual
  void
  handle_event(FURYEvent::handle);

private:

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
  generate_font(const cmd_line_type &cmd_line);

  void
  generate_table(const cmd_line_type &cmd_line);

  void
  clean_up(void);

  void
  animate_cells(float);

  void
  key_event(uint32_t k, bool p);

  bool
  create_image_pool(std::vector<WRATHImage*> &pool,
                    const std::string &path);

 
  WRATHTripleBufferEnabler::handle m_tr;
  Table *m_table;
  
  WRATHTextureFont *m_font;

  bool m_resized, m_button_down;
  WRATHTime m_time_since_button_down;
  Cell *m_selected_cell;
  WRATHScaleTranslate m_zoom_start_transformation;
  bool m_is_zooming;
  WRATHTime m_zoom_time;
  vec2 m_zoom_pivot;
  bool m_thicken_down, m_thinnen_down;
  WRATHTime m_paint_time, m_total_time;
  int m_number_frames;

  std::ostream *m_log_alloc_stream;
  std::ostream *m_gl_log_stream;


  bool m_touch_emulate;
  int32_t m_double_click_time;
  int32_t m_zoom_gesture_begin_time;
  float m_zoom_dividier;
  bool m_print_events;
  uint32_t m_thicken_key, m_thinnen_key;
  float m_thicken_thinnen_rate;

  WRATHLayer::draw_information m_stats;
};


////////////////////////////////
// cmd_line_type methods
DemoKernel*
cmd_line_type::
make_demo(void)
{
  return WRATHNew TableView(*this);
}

void
cmd_line_type::
delete_demo(DemoKernel *k)
{
  if(k!=NULL)
    {
      WRATHPhasedDelete(k);
    }
}


////////////////////////////////////
// TableView methods
TableView::
TableView(cmd_line_type &cmd_line):
  DemoKernel(&cmd_line),
  m_table(NULL),
  m_font(NULL),
  m_resized(true),
  m_button_down(false),
  m_selected_cell(NULL),
  m_is_zooming(false),
  m_thicken_down(false),
  m_thinnen_down(false),
  m_number_frames(0),
  m_log_alloc_stream(NULL),
  m_gl_log_stream(NULL),
  m_touch_emulate(cmd_line.m_touch_emulate.m_value),
  m_double_click_time(cmd_line.m_double_click_time.m_value),
  m_zoom_gesture_begin_time(cmd_line.m_zoom_gesture_begin_time.m_value),
  m_zoom_dividier(cmd_line.m_zoom_dividier.m_value),
  m_print_events(cmd_line.m_print_events.m_value),
  m_thicken_key(cmd_line.m_thicken_key.m_value), 
  m_thinnen_key(cmd_line.m_thinnen_key.m_value),
  m_thicken_thinnen_rate(cmd_line.m_thicken_thinnen_rate.m_value/1000.0f)
{
  m_tr=WRATHNew WRATHTripleBufferEnabler();

  if(cmd_line.m_grab_keyboard.m_value)
    {
      grab_keyboard(true);
    }

  if(cmd_line.m_grab_mouse.m_value)
    {
      grab_mouse(true);
    }

  enable_key_repeat(false);

  if(!cmd_line.m_log_GL.m_value.empty())
    {
      m_gl_log_stream=WRATHNew std::ofstream(cmd_line.m_log_GL.m_value.c_str());
      ngl_LogStream(m_gl_log_stream);
      ngl_log_gl_commands(true);
    }

  if(!cmd_line.m_log_alloc.m_value.empty())
    {
      m_log_alloc_stream=WRATHNew std::ofstream(cmd_line.m_log_alloc.m_value.c_str());
      WRATHMemory::set_new_log(m_log_alloc_stream);
    }

  /*
    command to specify maximum number of nodes per draw call...
   */
  NodePacker::max_node_count()=cmd_line.m_max_transformations.m_value;

  generate_font(cmd_line);
  generate_table(cmd_line);
}

TableView::
~TableView()
{
  clean_up();
}

void
TableView::
clean_up(void)
{
  if(m_gl_log_stream!=NULL)
    {
      ngl_LogStream(&std::cerr);
      ngl_log_gl_commands(false);
      WRATHPhasedDelete(m_gl_log_stream);
    }

  if(m_log_alloc_stream!=NULL)
    {
      WRATHMemory::set_new_log(NULL);
      WRATHPhasedDelete(m_log_alloc_stream);
    }

  float t( std::max(1,m_total_time.elapsed()));
  int nn(std::max(1,m_number_frames));

  std::cout << "\n"
            << m_number_frames
            << " frames in " 
            << t << " ms, fps="
            << static_cast<float>(1000*m_number_frames)/t
            << ", [" << t/static_cast<float>(nn)
            << " ms/frame ]\nStats:"
            << "\n\tDraw calls/frame=" << m_stats.m_draw_count/nn
            << "\n\tGLSL program changes=" << m_stats.m_program_count/nn
            << "\n\tTexture changes=" << m_stats.m_texture_choice_count/nn
            << "\n\tBufferBindings=" << m_stats.m_buffer_object_bind_count/nn
            << "\n\tVertexAttributeChanges=" << m_stats.m_attribute_change_count/nn
            << "\n" << std::flush;

  WRATHPhasedDelete(m_table);
  m_table=NULL;
  WRATHResourceManagerBase::clear_all_resource_managers();
  m_tr->purge_cleanup();
  m_tr=NULL;
}

void
TableView::
paint(void)
{
  if(m_table!=NULL)
    {
      float ticks(0.0f);

      if(m_resized)
        {
          glViewport(0.0f, 0.0f, width(), height());
          float_orthogonal_projection_params proj_params(0, width(),
                                                         height(), 0);
          m_table->layer().simulation_matrix(WRATHLayer::projection_matrix,
                                             float4x4(proj_params));
          m_resized=false;

          if(m_selected_cell!=NULL)
            {
              m_selected_cell->on_window_resize(width(), height());
            }
        }

      
      ticks=m_paint_time.restart();
      animate_cells(ticks);
      if(m_thicken_down)
        {
          m_table->stroke_width_internal_lines()+=ticks*m_thicken_thinnen_rate;
          m_table->stroke_width_external_lines()+=ticks*m_thicken_thinnen_rate;
        }
      else if(m_thinnen_down)
        {
          float v, d;

          d=ticks*m_thicken_thinnen_rate;
          v=std::max(m_table->stroke_width_internal_lines(),
                     m_table->stroke_width_external_lines());
          
          if(v>d)
            {
              m_table->stroke_width_internal_lines()-=d;
              m_table->stroke_width_external_lines()-=d;
            }
          else
            {
              m_table->stroke_width_internal_lines()-=v;
              m_table->stroke_width_external_lines()-=v;
            }
          
        }


      glClearColor(1.0f, 0.0f, 1.0f, 1.0f);

      m_tr->signal_complete_simulation_frame();
      m_tr->signal_begin_presentation_frame();


      

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      m_table->layer().clear_and_draw(&m_stats);

      update_widget();
    }

  if(m_number_frames==0)
    {
      m_total_time.restart();
    }

  ++m_number_frames;
}

void
TableView::
key_event(uint32_t k, bool p)
{
  if(k==m_thicken_key)
    {
      m_thicken_down=p;
    }
  else if(k==m_thinnen_key)
    {
      m_thinnen_down=p;
    }
}


void
TableView::
handle_event(FURYEvent::handle ev)
{
  if(m_print_events)
    {
      ev->log_event(std::cout);
      std::cout << "\n";
    }

  switch(ev->type())
    {
    case FURYEvent::Resize:
      {
        m_resized=true;
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
      if(m_touch_emulate and m_button_down)
        {
          FURYMouseMotionEvent::handle mev(ev.static_cast_handle<FURYMouseMotionEvent>());
          handle_touch_move(mev->pt(), mev->delta());
        }
      break;

    case FURYEvent::MouseButtonUp:
      if(m_touch_emulate)
        {
          FURYMouseButtonEvent::handle me(ev.static_cast_handle<FURYMouseButtonEvent>());
          m_button_down=false;
          handle_touch_end(me->pt());
          ev->accept();
        }
      break;

    case FURYEvent::MouseButtonDown:
      if(m_touch_emulate)
        {
          FURYMouseButtonEvent::handle me(ev.static_cast_handle<FURYMouseButtonEvent>());
          m_button_down=true;
          handle_touch_begin(me->pt());
          ev->accept();
        }
      break;

    case FURYEvent::KeyUp:
    case FURYEvent::KeyDown:
      {
        FURYKeyEvent::handle ke(ev.static_cast_handle<FURYKeyEvent>());
        key_event(ke->key().m_value, ke->pressed());
        ev->accept();
      }
      break;

    default:
      break;
    }

  update_widget();
}

void
TableView::
handle_touch_begin(vec2 pt)
{
  if(m_time_since_button_down.elapsed()<m_double_click_time)
    {
      /*
        convert pt to coordinate of m_table->root_node():
       */
      vec2 fpt;
      Cell *cell;

      fpt=m_table->root_node().transformation().inverse().apply_to_point(pt);
      cell=m_table->cell_at(fpt);

      
      if(m_selected_cell!=NULL)
        {
          vec4 cg;

          cg=m_selected_cell->background_color();
          cg.x()=1.0-cg.x();
          cg.y()=1.0-cg.y();
          cg.z()=1.0-cg.z();

          m_selected_cell->background_color(cg);
          m_selected_cell->pop_down();
          m_selected_cell=NULL;
        }
      else if(cell!=NULL)
        {
          vec4 cg;

          cg=cell->background_color();
          cg.x()=1.0-cg.x();
          cg.y()=1.0-cg.y();
          cg.z()=1.0-cg.z();

          cell->background_color(cg);
          m_selected_cell=cell;
          m_selected_cell->pop_up(width(), height());
        }
    }
  else
    {
      m_zoom_pivot=pt;                  
      m_zoom_start_transformation=m_table->root_node().transformation();
      m_zoom_time.restart();
    }

  m_time_since_button_down.restart();
}

void
TableView::
handle_touch_end(vec2)
{
  m_is_zooming=false;
}

void
TableView::
handle_touch_move(vec2 pt, vec2 delta)
{
  if(m_table!=NULL)
    {
      if(m_zoom_time.elapsed()>m_zoom_gesture_begin_time)
        {
          m_is_zooming=true;
        }  

      if(!m_is_zooming)
        {
          float zdx(pt.x()-m_zoom_pivot.x());
          float zdy(pt.y()-m_zoom_pivot.y());

          m_table->root_node().translation( m_table->root_node().translation() + delta);
          
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
              m_zoom_start_transformation=m_table->root_node().transformation();
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
          m_table->root_node().transformation(R*P);
        }
    }
}



void
TableView::
generate_font(const cmd_line_type &cmd_line)
{
  WRATHTextureFont* (*fetcher)(int psize, 
                               const std::string &pfilename, 
                               int face_index);

#define SET_LOAD_FONT_MIX(P, G) \
   do {\
     fetcher=WRATHMixFontTypes<P>::G::fetch_font;\
     WRATHMixFontTypes<P>::G::default_size_divider(default_mix_size_divider);\
} while(0)

#define SET_LOAD_FONT(P) \
   do {\
     fetcher=P::fetch_font;\
} while(0)


  //////////////////////////////////////
  //select font class + set font class parametes
  int analytic_mip_value(std::max(1,cmd_line.m_text_renderer_analytic_mipmap_level.m_value));
  float default_mix_size_divider(cmd_line.m_mix_font_div_ratio.m_value);
  

  WRATHTextureFontFreeType_Analytic::mipmap_level(analytic_mip_value);
  
  if(cmd_line.m_text_renderer_curve_analytic_format.m_value)
    {
      WRATHTextureFontFreeType_CurveAnalytic::include_scaling_data(true);
    }
  else 
    {
      WRATHTextureFontFreeType_CurveAnalytic::include_scaling_data(false);
    }

  switch(cmd_line.m_text_renderer_line_analytic_format.m_value)
    {
    default:
    case 0:
      {        
        WRATHTextureFontFreeType_Analytic::creation_texture_mode(WRATHTextureFontFreeType_Analytic::local_pixel_coordinates);
      }
      break;

    case 1:
      WRATHTextureFontFreeType_Analytic::creation_texture_mode(WRATHTextureFontFreeType_Analytic::global_pixel_coordinates_16bit);
      break;

    case 2: 
        WRATHTextureFontFreeType_Analytic::creation_texture_mode(WRATHTextureFontFreeType_Analytic::global_pixel_coordinates_32bit);
      break;
    }
  
  
  
  switch(cmd_line.m_text_renderer.m_value)
    {
    case 0: //alpha
      SET_LOAD_FONT(WRATHTextureFontFreeType_Coverage);
      break;

    case 1: //multi-res alpha
      SET_LOAD_FONT(WRATHTextureFontFreeType_DetailedCoverage);
      break;

    default:
    case 2: //distance
      switch(cmd_line.m_text_renderer_sub_choice.m_value)
        {
        case 0: //no AA
          SET_LOAD_FONT(WRATHTextureFontFreeType_Distance);
          break;
          
        default: 
        case 1: //AA
          SET_LOAD_FONT(WRATHTextureFontFreeType_Distance);
          break;
          
        case 2: //mix with converage
          SET_LOAD_FONT_MIX(WRATHTextureFontFreeType_Distance, mix);
          break;
          
        case 4: //mix with multi-res adaptive quad coverage
        case 3: //mix with multi-res coverage
          SET_LOAD_FONT_MIX(WRATHTextureFontFreeType_Distance, hq_mix);        
          break;
          
        case 5: //mix with same shader type
          SET_LOAD_FONT_MIX(WRATHTextureFontFreeType_Distance, self_mix);
          break;
        }
      break;
      
    case 3: //analytic
      switch(cmd_line.m_text_renderer_sub_choice.m_value)
        {
        case 0: //no AA
          SET_LOAD_FONT(WRATHTextureFontFreeType_Analytic);
          break;

        default: 
        case 1: //AA
          SET_LOAD_FONT(WRATHTextureFontFreeType_Analytic);
          break;

        case 2: //mix with converage
          SET_LOAD_FONT_MIX(WRATHTextureFontFreeType_Analytic, mix);
          break;
          
       
        case 3: //mix with multi-res coverage
          SET_LOAD_FONT_MIX(WRATHTextureFontFreeType_Analytic, hq_mix);
          break;

        case 5: //mix with same shader type
          SET_LOAD_FONT_MIX(WRATHTextureFontFreeType_Analytic, self_mix);
          break;
        }
      break;

    case 4: //curve analytic
      SET_LOAD_FONT(WRATHTextureFontFreeType_CurveAnalytic);
      switch(cmd_line.m_text_renderer_sub_choice.m_value)
        {
        case 0: //no AA
          SET_LOAD_FONT(WRATHTextureFontFreeType_CurveAnalytic);
          break;

        default: 
        case 1: //AA
          SET_LOAD_FONT(WRATHTextureFontFreeType_CurveAnalytic);
          break;

        case 2: //mix with coverage
          SET_LOAD_FONT_MIX(WRATHTextureFontFreeType_CurveAnalytic, mix);
          
          break;

        case 3: //mix with multi-res coverage
          SET_LOAD_FONT_MIX(WRATHTextureFontFreeType_CurveAnalytic, hq_mix);
          break;

        case 5: //mix with same shader type
          SET_LOAD_FONT_MIX(WRATHTextureFontFreeType_CurveAnalytic, self_mix);
          break;
        }
      break;
    }
  
 

  //////////////////////////////////////
  // load font:
  WRATHFontFetch::font_handle spec;
  if(cmd_line.m_use_font_config.m_value)
    {
      spec=WRATHFontFetch::font_handle(WRATHFontFetch::FontProperties()
				       .family_name(cmd_line.m_font_name.m_value));
    }
  else
    {
      spec=WRATHFontFetch::font_handle(cmd_line.m_font_name.m_value,
				       cmd_line.m_font_face_index.m_value);
    }

  if(!spec.valid())
    {
      spec=WRATHFontFetch::default_font();
    }
  m_font=fetcher(cmd_line.m_font_size.m_value, 
		 spec->name(), spec->face_index());
}

void
TableView::
generate_table(const cmd_line_type &cmd_line)
{
  Table::Drawer drawers;
  Table::ExtraDrawState extra_draw_state;
  
  
  drawers.m_stroked_shape_drawer=
    WRATHNew WRATHShaderSpecifier("stroked drawer",
                                  WRATHGLShader::shader_source()
                                  .add_macro("AA_HINT")
                                  .add_source("simple_ui_shape_translate_layer.vert.glsl", WRATHGLShader::from_resource),
                                  WRATHGLShader::shader_source()
                                  .specify_extension("GL_OES_standard_derivatives",
                                                     WRATHGLShader::enable_extension)
                                  .add_macro("AA_HINT")
                                  .add_source("simple_ui_shape.frag.glsl", WRATHGLShader::from_resource),
                                  WRATHShaderSpecifier::Initializer()
                                  .add<float>("animation_fx_interpol", 0.0f)
                                  .add<float2x2>("animation_matrix", float2x2())  );

  WRATHGLStateChange::blend_state::handle h;

  h=WRATHNew WRATHGLStateChange::blend_state(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  extra_draw_state
    .m_stroked_shape_extra_state.add_gl_state_change(h);


  m_table=WRATHNew Table(m_tr,
                         vec2(cmd_line.m_table_size_x.m_value, 
                              cmd_line.m_table_size_y.m_value),
                         drawers,
                         extra_draw_state,
                         ivec2(cmd_line.m_cell_count_x.m_value, 
                               cmd_line.m_cell_count_y.m_value));

  /*
    create pool of images..
   */
  std::vector<WRATHImage*> ims;
  std::string path(cmd_line.m_image_dir.m_value);

  if(!path.empty())
    {
      if(*path.rbegin()!='/')
        {
          path.push_back('/');
        }
      create_image_pool(ims, path);
    }

  /*
    init cells...
   */
  for(int x=0, endx=m_table->cell_count().x(), c=0; x<endx; ++x)
    {
      for(int y=0, endy=m_table->cell_count().y(); y<endy; ++y, ++c)
        {
          WRATHTextDataStream text;

          WRATHassert(m_font!=NULL);

          text.stream() << WRATHText::set_font(m_font)
                        << WRATHText::set_pixel_size(cmd_line.m_display_font_size.m_value)
                        << "\nCell " << ivec2(x,y)
                        << "\nSome text unique"
                        << "\nAnd some more text"
                        << "\nAnd some more";

          if(!ims.empty())
            {
              WRATHImage *im;
              

              im=ims[c%ims.size()];

              const std::string &raw(im->resource_name());
              std::string label( raw.substr(raw.find_last_of('/')) );


              text.stream() << "\nImage=" << label
                            << "\nsize=" << im->size();
              m_table->named_cell(x,y)->set_image(im);
            }
          m_table->named_cell(x,y)->set_text(text);

          if((x+y)&1)
            {
              m_table->named_cell(x,y)->background_color(vec4(1.0f, 0.0f, 0.0f, 1.0f));
            }
          else
            {
              m_table->named_cell(x,y)->background_color(vec4(0.0f, 0.0f, 1.0f, 1.0f));
            }

        }
    }
}


bool
TableView::
create_image_pool(std::vector<WRATHImage*> &ims,
                  const std::string &full_path)
{
  DIR *ptr;
      
  ptr=::opendir(full_path.c_str());
  if(ptr==NULL)
    {
      return false;
    }
  else
    {
      for(struct dirent *current=::readdir(ptr);
          current!=NULL; current=::readdir(ptr))
        {
          if(!std::strcmp(current->d_name,".") or !std::strcmp(current->d_name,".."))
             continue;

          if(!create_image_pool(ims, full_path+current->d_name+"/"))
            {
              WRATHImage *pimage;
              std::string filename(full_path+std::string(current->d_name));

              pimage=WRATHDemo::fetch_image(filename,
                                            WRATHImage::ImageFormat()
                                            .internal_format(GL_RGBA)
                                            .pixel_data_format(GL_RGBA)
                                            .pixel_type(GL_UNSIGNED_BYTE)
                                            .magnification_filter(GL_LINEAR)
                                            .minification_filter(GL_LINEAR_MIPMAP_NEAREST)
                                            .automatic_mipmap_generation(true));
              
              if(pimage!=NULL)
                {
                  ims.push_back(pimage);
                }
            }
        }
      ::closedir(ptr);
      return true;
    }

}


void
TableView::
animate_cells(float ticks)
{
  for(int x=0, endx=m_table->cell_count().x(); x<endx; ++x)
    {
      for(int y=0, endy=m_table->cell_count().y(); y<endy; ++y)
        {
          m_table->named_cell(x,y)->animate(ticks);
        }
    }
}


int
main(int argc, char **argv)
{
  cmd_line_type cmd_line;

  return cmd_line.main(argc, argv);
}
