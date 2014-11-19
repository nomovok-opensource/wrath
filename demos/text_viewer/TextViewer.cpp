/*! 
 * \file TextViewer.cpp
 * \brief file TextViewer.cpp
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
#include <typeinfo>
#include <dirent.h>
#include "vecN.hpp"
#include "ngl_backend.hpp"
#include "WRATHNew.hpp"
#include "WRATHmalloc.hpp"
#include "WRATHLayer.hpp"
#include "WRATHLayerItemNodeRotateTranslate.hpp"
#include "WRATHDefaultTextAttributePacker.hpp"
#include "WRATHTextureFontFreeType_Mix.hpp"
#include "WRATHTextureFontFreeType_DetailedCoverage.hpp"
#include "WRATHTextureFontFreeType_CurveAnalytic.hpp"
#include "WRATHFontFetch.hpp"
#include "WRATHDefaultStrokeAttributePacker.hpp"
#include "WRATHDefaultFillShapeAttributePacker.hpp"
#include "WRATHDefaultRectAttributePacker.hpp"
#include "WRATHTime.hpp"
#include "WRATHLayerItemDrawerFactory.hpp"
#include "WRATHGLExtensionList.hpp"


#include "NodePacker.hpp"
#include "FilePacket.hpp"
#include "FileData.hpp"

#include "wrath_demo.hpp"
#include "wrath_demo_image_support.hpp"

#ifdef __WIN32
  #define DIRECTORY_SLASH '\\'
  #define DIRECTORY_SLASH_STR "\\"
#else
  #define DIRECTORY_SLASH '/'
  #define DIRECTORY_SLASH_STR "/"
#endif

#ifdef WRATH_QT
#include "WRATHQTFontSupport.hpp"
#define SET_LOAD_FONT_VIA_QT(F, P, G) do { F.m_font_via_qt=WRATHQT::fetch_font<CustomShaderFont<P>::G>; } while(0)
#else
#define SET_LOAD_FONT_VIA_QT(F, P, G) do { } while(0)
#endif

#define SET_LOAD_FONT_VIA_FILE(F, P, G) do { F.m_font_via_resource=CustomShaderFont<P>::G::fetch_font; } while(0)
#define SET_LOAD_FONT(F, P, G) do { SET_LOAD_FONT_VIA_QT(F,P,G); SET_LOAD_FONT_VIA_FILE(F,P,G); } while(0)





class fragment_sources:boost::noncopyable
{
public:
  fragment_sources(void):
    m_use_custom(false)
  {}

  template<typename F>
  const WRATHTextureFont::GlyphGLSL*
  fetch_source(const WRATHTextureFont::GlyphGLSL *src)
  {
    key K(typeid(F));

    map::iterator iter;

    iter=m_map.find(K);
    if(iter!=m_map.end())
      {
        return &iter->second;
      }

    std::pair<map::iterator, bool> v;

    v=m_map.insert( map::value_type(K, *src));
    WRATHassert(v.second);

    if(m_use_custom)
      {
        v.first->second.m_fragment_processor[WRATHTextureFont::GlyphGLSL::linear_glyph_position]=m_font_fragment_processor;
      }

    return &v.first->second;
  }

  static
  fragment_sources&
  obj(void)
  {
    static fragment_sources R;
    return R;
  }

  WRATHGLShader::shader_source m_font_fragment_processor;
  bool m_use_custom;

private:

  class key
  {
  public:
    key(const std::type_info &v):
      m_info(v)
    {}

    bool
    operator<(key rhs) const { return m_info.before(rhs.m_info); }

  private:

    const std::type_info &m_info;
  };

  typedef std::map<key, WRATHTextureFont::GlyphGLSL> map;
  map m_map;
};




template<typename F>
class CustomShaderFont:public F
{
public:
  CustomShaderFont(WRATHFreeTypeSupport::LockableFace::handle pface,
                   const WRATHTextureFontKey &presource_name):
    F(pface, presource_name)
  {}

  virtual
  const WRATHTextureFont::GlyphGLSL*
  glyph_glsl(void)
  {
    return fragment_sources::obj().fetch_source<F>(F::glyph_glsl());
  }

  static
  WRATHTextureFont*
  fetch_font(int psize, const std::string &pfilename, int face_index)
  {
    return WRATHFreeTypeSupport::fetch_font<CustomShaderFont>(psize, pfilename, face_index);
  }

  static
  WRATHTextureFont*
  fetch_font(int psize, const WRATHFontDatabase::Font::const_handle &fnt)
  {
    return WRATHFreeTypeSupport::fetch_font<CustomShaderFont>(psize, fnt);
  }

  static
  void
  default_size_divider(float f)
  {
    mix::default_size_divider(f);
    hq_mix::default_size_divider(f);
    self_mix::default_size_divider(f);
  }

  static
  void
  minified_font_inflate_factor(float f)
  {
    mix::minified_font_inflate_factor(f);
    hq_mix::minified_font_inflate_factor(f);
    self_mix::minified_font_inflate_factor(f);
  }

  typedef CustomShaderFont base;
  typedef WRATHTextureFontFreeType_TMix<base> mix;
  typedef WRATHTextureFontFreeType_TMix<base, WRATHTextureFontFreeType_DetailedCoverage> hq_mix;
  typedef WRATHTextureFontFreeType_TMix<base, base> self_mix;
};




class cmd_line_type:public DemoKernelMaker
{
public:
  command_line_argument_value<int> m_max_transformations;
  command_line_argument_value<bool> m_vs_force_highp, m_fs_force_highp;
   
  command_line_argument_value<std::string> m_tex_attr_prec, m_tex_varying_vs_prec;
  command_line_argument_value<std::string> m_tex_varying_fs_prec, m_tex_recip_prec; 
  command_line_argument_value<int> m_text_renderer;
  command_line_argument_value<bool> m_text_renderer_curve_analytic_separate_curve_storage;
  command_line_argument_value<bool> m_text_renderer_curve_analytic_highp;
  command_line_argument_value<int> m_text_renderer_sub_choice;
  command_line_argument_value<int> m_text_renderer_coverage_min_filter;
  command_line_argument_value<int> m_text_renderer_converage_mag_filter;
  command_line_argument_value<int> m_text_renderer_converage_deepness_slack; 
  command_line_argument_value<int> m_text_renderer_analytic_mipmap_level;
  command_line_argument_value<float> m_mix_font_div_ratio;
  command_line_argument_value<float> m_mix_font_minified_inflate_factor;
  command_line_argument_value<float> m_font_discard_thresh;
  command_line_argument_value<float> m_max_distance_font_generation;
  command_line_argument_value<GLint> m_font_texture_size;
  command_line_argument_value<bool> m_font_texture_force_power2; 
  command_line_argument_value<std::string> m_custom_font_shader;
  command_line_argument_value<std::string> m_font_present_shader;

  command_line_argument_value<bool> m_font_lazy_z;

  command_line_argument_value<int> m_atlas_size;
  command_line_argument_value<bool> m_image_use_mipmaps;
  command_line_argument_value<bool> m_manual_mipmap_generation;

  command_line_argument_value<int> m_display_font_size;
  command_line_argument_value<int> m_font_size;
  command_line_argument_value<std::string> m_font_name;
  command_line_argument_value<int> m_font_face_index;
  command_line_argument_value<bool> m_use_font_config;

  command_line_argument_value<int> m_text_red, m_text_blue, m_text_green;
  command_line_argument_value<int> m_bg_red, m_bg_blue, m_bg_green, m_bg_alpha;
  
  command_line_argument_value<bool> m_show_perf_stats, m_smart_update;

  command_line_argument_value<int> m_up_key, m_down_key;
  command_line_argument_value<int> m_left_key, m_right_key;
  command_line_argument_value<int> m_zoom_in_key, m_zoom_out_key;
  command_line_argument_value<int> m_quit_key;
  command_line_argument_value<int> m_reload_key;
  command_line_argument_value<int> m_back_key;
  command_line_argument_value<int> m_print_texture_consumption;
  command_line_argument_value<bool> m_print_events;

  command_line_argument_value<int> m_text_chunk_size;
  command_line_argument_value<std::string> m_file_to_view;

  command_line_argument_value<bool> m_use_vbo;

  command_line_argument_value<bool> m_disable_culling;
  command_line_argument_value<bool> m_rotate;
  command_line_argument_value<std::string> m_titlebar;


  command_line_argument_value<bool> m_issue_gl_finish;
  command_line_argument_value<bool> m_grab_keyboard, m_grab_mouse;

  command_line_argument_value<bool> m_animate_with_rotation;
  command_line_argument_value<int> m_animation_time_ms;
  command_line_argument_value<bool> m_transition_on_jump;
  command_line_argument_value<float> m_automatic_scroll_speed;
  command_line_argument_value<int> m_max_time_for_automatic_scroll;
  command_line_argument_value<bool> m_auto_scroll;

  command_line_argument_value<int> m_zoom_gesture_begin_time;
  command_line_argument_value<float> m_zoom_dividier;

  command_line_argument_value<bool> m_touch_emulate;
  command_line_argument_value<float> m_flick_deacceleration;
  command_line_argument_value<float> m_touch_speed_multiplier;
  command_line_argument_value<float> m_max_flick_speed;
  command_line_argument_value<bool> m_enable_flick;

  command_line_argument_value<bool> m_load_font_in_thread;
  command_line_argument_value<bool> m_font_render_use_sub_quads;

  command_line_argument_value<bool> m_enable_fill_aa;
  command_line_argument_value<bool> m_enable_stroke_aa;

  cmd_line_type(void):    
    DemoKernelMaker(),    

    m_max_transformations(100, "max_tr", 
                          "Maximum number of transformation nodes per draw call", 
                          *this),
    m_vs_force_highp(false, "vs_force_highp", 
                     "if true, all variables in vertex shader are highp", *this),
    m_fs_force_highp(false, "fs_force_highp", 
                     "if true, all variables in fragment shader are highp", *this),

    m_tex_attr_prec("highp", "font_tex_attr", 
                    "Precision qualifier for font texture coordiante attribute", *this), 
    m_tex_varying_vs_prec("highp", "font_tex_vary_vs", 
                          "Precision qualifier for font texture coordiante varying in vertex shader", 
                          *this), 
    m_tex_varying_fs_prec("mediump", "font_tex_vary_fs", 
                          "Precision qualifier for font texture coordiante varying in fragment shader", 
                          *this),
    m_tex_recip_prec("mediump", "font_tex_unif", 
                     "Precision qualifier for font texture coordiante reciprocal uniform", 
                     *this),

    m_text_renderer(3, "text_renderer", 
                    "Specify text renderer, 0=FreeType alpha, "
                    "1=multi-res coverage, 2=distance, 3=analytic, 4=curve_analytic",
                    *this),

                                     
    m_text_renderer_curve_analytic_separate_curve_storage(false, "curve_analytic_separate",
                                                          "Only has affect if text_renderer is 4 "
                                                          "if on, curve analytic stores seperate curves "
                                                          "instead of curve corner pairs, thus using fewer "
                                                          "textures and less texture memory but at cost "
                                                          "of more expensive fragment shader",
                                                          *this),
    m_text_renderer_curve_analytic_highp(true, "curve_analytic_highp",
                                         "Only has affect if text_renderer is 4 "
                                         "if on, then floating point texture storage "
                                         "and rendering are done in 32-bit float, "
                                         "when off, done in 16-bit float",
                                         *this),

    m_text_renderer_sub_choice(1, "text_renderer_sub_choice",
                               "0=no AA, 1=AA, 2=mix with coverage "
                               "3=mix with multi-res coverage, "
                               "4=mix with same shader type",
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

    m_mix_font_div_ratio(4.0f, "min_font_div",
                         "When rendering text with 2 seperate font objects, "
                         "determines the ratio of the native pixel size font "
                         "to the minified pixel size font. Parameter "
                         "only has effect if m_text_renderer_sub_choice is 2, 3, 4 or 5",
                         *this),

    m_mix_font_minified_inflate_factor(1.0f, "mix_font_inflate_factor",
                                       "When rendering text with 2 seperate font objects, "
                                       "specifies a multiplier for the threshhold to *USE* "
                                       "the minified font, a value of C indicates to use "
                                       "the minified font if a glyph's display size is less "
                                       "than C*M where M is the size of the minified font",
                                       *this),

   
    m_font_discard_thresh(0.9f, "discard_thresh", "Font blending threshold", *this),
    
    m_max_distance_font_generation(96.0f, "font_max_dist", 
                                   "Max distance value used in generating font distance values", 
                                   *this),
    m_font_texture_size(1024, "font_texture_size", 
                        "Max size of each dimention texture of font glyph cache", *this),
    m_font_texture_force_power2(true, "font_pow2", 
                                "If true, font texture size is always a power of 2", *this),

    m_custom_font_shader("", "custom_font_shader",
                         "If set use a custom font shader named by the file", *this),
    m_font_present_shader("font_animated.frag.glsl", "font_present_shader",
                          "Shader to use to _present the font, i.e. dictates color, etc", *this),
    

    m_font_lazy_z(true, "font_lazy_z", 
                  "if true, overlapping text not necessarily drawn in correct order", *this),  

    m_atlas_size(2048, "atlas_size", "Size of texture atlas(es)", *this),

    m_image_use_mipmaps(true, "image_use_mipmaps",
                        "If true, use mipmap filtering for images", *this),
    m_manual_mipmap_generation(false, "manual_mipmaps",
                               "If true and if using mipmaps, will generate "
                               "mipmaps in CPU calls rather than using GL's "
                               "glGenerateMipmap", *this),

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

    m_text_red(0x00, "text_red", "red value for text, [0-255]", *this), 
    m_text_blue(0x00, "text_blue", "blue value for text, [0-255]", *this), 
    m_text_green(0x00, "text_green", "green value for text, [0-255]", *this),
    m_bg_red(0xff, "bg_red", "red value for background, [0-255]", *this), 
    m_bg_blue(0xff, "bg_blue", "blue value for background, [0-255]", *this), 
    m_bg_green(0xff, "bg_green", "green value for background, [0-255]", *this),
    m_bg_alpha(0xff, "bg_alpha", "alpha value for background, [0-255]", *this),

    m_show_perf_stats(false, "show_perf", "Show performance/debug stats", *this),
    m_smart_update(true, "smart_update", "Only repaint when necessary", *this),

    m_up_key(FURYKey_Up,"up_key", "FURY-Key code for scroll up", *this),
    m_down_key(FURYKey_Down,"down_key", "FURY-Key code for scroll down", *this),
    m_left_key(FURYKey_Left,"left_key", "FURY-Key code for scroll left", *this), 
    m_right_key(FURYKey_Right,"right_key", "FURY-Key code for scroll right", *this),
    m_zoom_in_key(FURYKey_A,"zoom_in_key", "FURY-Key code for zoom in", *this), 
    m_zoom_out_key(FURYKey_S,"zoom_out_key", "FURY-Key code for zoom out", *this),
    m_quit_key(FURYKey_Escape,"quit_key", "FURY-Key code to quit", *this),
    m_reload_key(FURYKey_R, "reload_key", "FURY-Key code to reload page", *this),
    m_back_key(FURYKey_B, "back_key", "FURY-Key code to go back one page", *this),
    m_print_texture_consumption(FURYKey_P, "print_font texture consumption",
                                "FURY-Key code to print font texture consumption data", *this),

    m_print_events(false, "print_events", "Print events as they come", *this),

    m_text_chunk_size(1000, "text_chunk_size", 
                      "Number of characters per text chunk", *this),
    m_file_to_view("text_viewer_data/tutorial.txt", "filename", 
                   "UTF8 encoded file to view", *this),

    m_use_vbo(true, "use_vbo", "Use Vertex buffer objects for vertex data", *this),
    m_disable_culling(false, "disable_culling", "Disable viewport culling", *this),
    m_rotate(false, "rotate", "rotate display 90 degree", *this),
    m_titlebar("qt_text_viewer", "titlebar", "Titlebar label", *this),


    m_issue_gl_finish(false, "gl_finish", 
                      "If true calls glFinish at the end of paint()", *this),
    m_grab_keyboard(false, "grab_keyboard", 
                    "If true grabs the keyboard", *this), 
    m_grab_mouse(false, "grab_mouse", 
                 "If true grabs the mouse", *this),


    m_animate_with_rotation(false, "transition_rotate", 
                            "If true, display rotates in animation transition", *this),
    m_animation_time_ms(1000, "transition_time",
                        "Time in ms of transition animation", *this),
    m_transition_on_jump(false, "transition_on_jump",
                         "If true, even links that are within the same file "
                         "trigger animation",
                         *this),

    m_automatic_scroll_speed(1500.0f, "automatic_scroll_speed",
                             "Speed in pixels/second of scrolling when automatically scrolling",
                             *this),
    m_max_time_for_automatic_scroll(500, "max_auto_scroll_time",
                                    "Maximum time allowed for autoscrolling", *this),
    m_auto_scroll(true, "autoscroll", "Allow auto scroll to keep document in view", *this),

    m_zoom_gesture_begin_time(500, "zoom_time", "Time in ms to trigger zoom gesture", *this),
    m_zoom_dividier(40.0f, "zoom_div", "Zoom divider", *this),

    m_touch_emulate(false, "emulate_touch", 
                    "If true, mouse events are used to "
                    "emulate touch events", *this),

    m_flick_deacceleration(0.0025f, "flick_deaccel", "Flick deacceration in pixels/ms^2", *this),
    m_touch_speed_multiplier(1.0f, "touch_speed_multiplier", 
                             "Touch speed multiplier for flick", *this),
    m_max_flick_speed(2.0f, "max_flick_speed", "Maximum flick speed in pixels/ms", *this),
    m_enable_flick(true, "enable_flick", "If false, flicking is disabled", *this),

    m_load_font_in_thread(false, "background_font_load",
                          "If true all glyphs of fonts are loaded in a background thread",
                          *this),

    m_font_render_use_sub_quads(false, "font_use_subquads",
                                "If true will use sub-quads for drawing fonts, "
                                "decreases pixel coverage at cost of increasing primitive count",
                                *this),

    m_enable_fill_aa(true, "enable_fill_aa", "if true enable anti-aliasing on filling shapes", *this),
    m_enable_stroke_aa(true, "enable_stroke_aa", "if true enable anti-aliasing on stroking shapes", *this)
  {}

  virtual
  DemoKernel*
  make_demo(void);

  virtual
  void
  delete_demo(DemoKernel*);
};

class TextViewer:public DemoKernel
{
public:
  TextViewer(cmd_line_type &cmd_line);

  ~TextViewer();

  virtual
  void
  paint(void);

  virtual
  void
  handle_event(FURYEvent::handle);

private:

  enum page_animation_type
    {
      page_no_animation=0,
      page_disappearing,
      page_appearing
    };

  class stack_entry
  {
  public:
    stack_entry(FileData *ptr):
      m_file(ptr),
      m_transformation(ptr->transformation_node().values().m_transformation)
    {}

    FileData *m_file;
    WRATH2DRigidTransformation m_transformation;
  };

  typedef void (TextViewer::*on_key_function)(bool);
  typedef std::pair<on_key_function, uint32_t> on_key_command;

  static
  vec2
  compute_translation(const vec2 &in_mouse_pt,
                      WRATHLayerItemNodeRotateTranslate &node,
                      const vec2 &dest_pt);

  bool
  check_for_links(int x, int y);

  void
  handle_touch_begin(int x, int y);

  void
  handle_touch_end(int x, int y);

  void
  handle_touch_move(const vec2 &pos, const vec2 &delta);

  void
  on_page_back(bool);

  void
  on_quit(bool);

  void
  on_reload(bool);

  bool
  common_on_transformation_key_begin(bool);

  void
  common_on_transformation_key_end(bool);

  void
  on_left(bool);

  void
  on_right(bool);

  void
  on_up(bool);

  void
  on_down(bool);

  void
  on_zoom_in(bool);

  void
  on_zoom_out(bool);

  void
  on_print_font_texture_consumption(bool);

  void
  handle_jump(void);

  void
  scroll_animate(void);

  void
  flick_scroll_animate(void);

  void
  trigger_scroll_animate(void);

  bool
  fit_translation(float scale, vec2 &in_out_pt);

  int
  compute_scroll_time(const vec2 &start, const vec2 &end);

  void
  update_transformation(void);

  void
  reset_transformation_time(void);

  void
  update_page_animation(void);

  bool
  transformation_changing(void);

  void
  clean_up(void);

  static
  GLubyte
  bound_to_byte(int c)
  {
    return std::max(0, std::min(c,255));
  } 

  static
  float
  as_float(GLubyte v)
  {
    return static_cast<float>(v)/255.0f;
  }

  static
  vec4
  as_float(vecN<GLubyte,4> v)
  {
    return vec4(as_float(v[0]),
                as_float(v[1]),
                as_float(v[2]),
                as_float(v[3]));
                
  }

    
  
  template<typename T>
  WRATHUniformData::uniform_setter_base::handle
  create_animation_fx_uniform(const std::string &pname, T *ref)
  {
    
    return WRATHNew typename WRATHUniformData::uniform_by_name_ref<T>(pname, 
                                                                      ref);
  }

  template<typename T>
  WRATHUniformData::uniform_setter_base::handle
  create_non_fx_uniform(const std::string &pname, T v)
  {
    return WRATHNew typename WRATHUniformData::uniform_by_name<T>(pname, 
                                                                  v);
    
  }

  std::vector<on_key_command> m_key_commands;

  
  vec4 m_bg_color;

  WRATHTextureFont *m_font;
  vecN<WRATHUniformData::uniform_setter_base::handle, 8> m_page_animation_iterpol;
  float m_page_animation_iterpol_value;
  vecN<WRATHUniformData::uniform_setter_base::handle, 8> m_animation_matrix;
  float2x2 m_animation_matrix_value;
  bool m_animate_with_rotation;
  int m_animation_time_ms;
  bool m_transition_on_jump;

  FilePacket::misc_drawers_type m_more_drawers;

  WRATHTextItem *m_fps_text;
  WRATHLayerItemNodeRotateTranslate *m_fps_text_vis;

  vecN<WRATHUniformData::uniform_setter_base::handle, 3> m_fps_uniforms;
  vecN<WRATHUniformData::uniform_setter_base::handle, 3> m_fps_uniforms_mats;
  
  WRATHTime m_fps_time;
  WRATHLayer::draw_information m_stats;

  bool m_smart_update, m_disable_culling;
  bool m_scroll_left, m_scroll_right;
  bool m_scroll_up, m_scroll_down;
  bool m_zoom_in, m_zoom_out;
  WRATHTime m_animation_time;

  int m_up_key, m_down_key;
  int m_left_key, m_right_key;
  int m_zoom_in_key, m_zoom_out_key;
  int m_reload_key, m_quit_key;
  int m_print_texture_consumption;
  int m_back_key;
  bool m_print_events;

  typedef WRATHImage::TextureAllocatorHandle TextureAllocatorHandle;
  typedef TextureAllocatorHandle::texture_consumption_data_type texture_consumption_data_type;

  typedef texture_consumption_data_type (*consumption_query)(void);

  consumption_query m_print_consumption;
  consumption_query m_print_consumption_extra;
  consumption_query m_print_consumption_mix;
  
  int m_consumption_bpp, m_consumption_extra_bpp;


  ivec2 m_viewport_sz, m_culling_window;
  bool m_rotate;

  WRATHTime m_page_animation_time;
  enum page_animation_type m_page_animation_stage;

  WRATHTime m_scroll_animation_time;
  vec2 m_scroll_animation_start, m_scroll_animation_end;
  int m_scroll_animation_period;
  bool m_scroll_animating;
  float m_automatic_scroll_speed;
  bool m_auto_scroll;
  int m_max_time_for_automatic_scroll;

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_root;
  FilePacket *m_all_contents;
  FileData *m_current_display_contents;
  FileData *m_next_display_contents;
  std::pair<bool, WRATH2DRigidTransformation> m_next_display_transformation;
  std::pair<bool, std::string> m_next_display_jump_to;
  vec2 m_ms_pt;

  std::vector<stack_entry> m_link_stack;

  std::string m_titlebar;
  bool m_titlebar_fixed;
  bool m_issue_gl_finish;

  //touch state jazz
  bool m_is_zooming;
  vec2 m_zoom_pivot;
  WRATHTime m_zoom_time;
  int m_zoom_gesture_begin_time;
  float m_zoom_dividier;
  WRATH2DRigidTransformation m_zoom_start_transformation;
  bool m_touch_emulate;

  vec2 m_flick_speed_magnitudes, m_flick_speed_signs;
  vec2 m_flick_begin_point, m_flick_end_times;
  float m_flick_deacceleration, m_touch_speed_multiplier;
  float m_max_flick_speed;
  WRATHTime m_flick_time;
  vecN<bool,2> m_flick_scrolling;
  vec2 m_last_touch_speed;
  bool m_enable_flick;

  //touch emulation
  bool m_button_down;
  vec2 m_last_ms_position;

  //whatevers
  bool m_show_stats, m_load_font_in_thread;
  bool m_need_to_update_culling;
};


int
main(int argc, char **argv)
{
  cmd_line_type cmd_line;

  return cmd_line.main(argc, argv);
}


///////////////////////////////////
// DemoKernel methods
DemoKernel*
cmd_line_type::
make_demo(void)
{
  return WRATHNew TextViewer(*this);
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


///////////////////////////////
// TextViewer methods
TextViewer::
TextViewer(cmd_line_type &cmd_line):
  DemoKernel(&cmd_line),

  
  m_bg_color(cmd_line.m_bg_red.m_value,
             cmd_line.m_bg_green.m_value,
             cmd_line.m_bg_blue.m_value,
             cmd_line.m_bg_alpha.m_value),

  m_font(NULL), 
  m_page_animation_iterpol_value(0.0f),
  m_animate_with_rotation(cmd_line.m_animate_with_rotation.m_value),
  m_animation_time_ms(cmd_line.m_animation_time_ms.m_value),
  m_transition_on_jump(cmd_line.m_transition_on_jump.m_value),

  m_fps_text(NULL),
  m_fps_text_vis(NULL),

  m_smart_update(cmd_line.m_smart_update.m_value),
  m_disable_culling(cmd_line.m_disable_culling.m_value),
  m_scroll_left(false),
  m_scroll_right(false),
  m_scroll_up(false),
  m_scroll_down(false),
  m_zoom_in(false),
  m_zoom_out(false),

  m_up_key(cmd_line.m_up_key.m_value),
  m_down_key(cmd_line.m_down_key.m_value),
  m_left_key(cmd_line.m_left_key.m_value),
  m_right_key(cmd_line.m_right_key.m_value),
  m_zoom_in_key(cmd_line.m_zoom_in_key.m_value),
  m_zoom_out_key(cmd_line.m_zoom_out_key.m_value),
  m_reload_key(cmd_line.m_reload_key.m_value),
  m_quit_key(cmd_line.m_quit_key.m_value),
  m_print_texture_consumption(cmd_line.m_print_texture_consumption.m_value),
  m_back_key(cmd_line.m_back_key.m_value),
  m_print_events(cmd_line.m_print_events.m_value),

  m_print_consumption(NULL),
  m_print_consumption_extra(NULL),
  m_print_consumption_mix(NULL),

  m_viewport_sz(-1, -1), 
  m_rotate(cmd_line.m_rotate.m_value),

  m_page_animation_stage(page_no_animation),
  m_scroll_animation_period(0),
  m_scroll_animating(false),
  m_automatic_scroll_speed(cmd_line.m_automatic_scroll_speed.m_value),
  m_auto_scroll(cmd_line.m_auto_scroll.m_value),
  m_max_time_for_automatic_scroll(cmd_line.m_max_time_for_automatic_scroll.m_value),

  m_root(NULL),
  m_all_contents(NULL),
  m_current_display_contents(NULL),
  
  m_next_display_transformation(false, WRATH2DRigidTransformation()),
  m_next_display_jump_to(false, ""),

  m_titlebar_fixed(cmd_line.m_titlebar.set_by_command_line()),
  m_issue_gl_finish(cmd_line.m_issue_gl_finish.m_value),

  m_is_zooming(false),
  m_zoom_gesture_begin_time(cmd_line.m_zoom_gesture_begin_time.m_value),
  m_zoom_dividier(cmd_line.m_zoom_dividier.m_value),
  m_touch_emulate(cmd_line.m_touch_emulate.m_value),

  m_flick_speed_magnitudes(-1.0f, -1.0f),
  m_flick_speed_signs(0.0f, 0.0f),
  m_flick_deacceleration(cmd_line.m_flick_deacceleration.m_value),
  m_touch_speed_multiplier(cmd_line.m_touch_speed_multiplier.m_value),
  m_max_flick_speed(cmd_line.m_max_flick_speed.m_value),
  m_flick_scrolling(false, false),
  m_enable_flick(cmd_line.m_enable_flick.m_value),

  m_button_down(false),

  m_show_stats(cmd_line.m_show_perf_stats.m_value),
  m_load_font_in_thread(cmd_line.m_load_font_in_thread.m_value),
  m_need_to_update_culling(true)
{

  WRATHImage::texture_atlas_dimension(cmd_line.m_atlas_size.m_value);
  FilePacket::Loader fetcher;


  if(cmd_line.m_custom_font_shader.set_by_command_line())
    {
      fragment_sources::obj().m_use_custom=true;

      fragment_sources::obj().m_font_fragment_processor
        .add_source(cmd_line.m_custom_font_shader.m_value);
    }

  
  if(cmd_line.m_grab_keyboard.m_value)
    {
      grab_keyboard(true);
    }

  if(cmd_line.m_grab_mouse.m_value)
    {
      grab_mouse(true);
    }

  //The brains behond the loading:
  enum WRATHDefaultTextAttributePacker::PackerType packer_type;
  WRATHTextAttributePacker *text_packer;

  packer_type=(cmd_line.m_font_render_use_sub_quads.m_value)?
    WRATHDefaultTextAttributePacker::SubPrimitivePacker:
    WRATHDefaultTextAttributePacker::SingleQuadPacker;
  text_packer=WRATHDefaultTextAttributePacker::fetch(packer_type);

  
  /*
    command to specify maximum number of nodes per draw call...
   */
  NodePacker::max_node_count()=cmd_line.m_max_transformations.m_value;


  int analytic_mip_value(std::max(1,cmd_line.m_text_renderer_analytic_mipmap_level.m_value));
  float mix_size_divider(cmd_line.m_mix_font_div_ratio.m_value);
  float mix_inflate(cmd_line.m_mix_font_minified_inflate_factor.m_value);
  WRATHFontShaderSpecifier *text_shader_specifier;

  WRATHTextureFontFreeType_Analytic::mipmap_level(analytic_mip_value);
  WRATHTextureFontFreeType_CurveAnalytic::store_separate_curves(cmd_line.m_text_renderer_curve_analytic_separate_curve_storage.m_value);
  WRATHTextureFontFreeType_CurveAnalytic::use_highp(cmd_line.m_text_renderer_curve_analytic_highp.m_value);
  
  std::string font_fragment_shader;
  enum WRATHGLShader::shader_source_type fragment_shader_type;
  
  if(cmd_line.m_font_present_shader.set_by_command_line())
    {
      font_fragment_shader=cmd_line.m_font_present_shader.m_value;
      fragment_shader_type=WRATHGLShader::from_file;
    }
  else
    {
      font_fragment_shader="font_animated.frag.glsl";
      fragment_shader_type=WRATHGLShader::from_resource;
    }

  switch(cmd_line.m_text_renderer.m_value)
    {
    case 0: //alpha
      SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_Coverage, base);
      m_print_consumption=WRATHTextureFontFreeType_Coverage::texture_consumption;
      m_consumption_bpp=1;
      break;

    case 1: //multi-res alpha
      SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_DetailedCoverage, base);
      m_print_consumption=WRATHTextureFontFreeType_DetailedCoverage::texture_consumption;
      m_consumption_bpp=1;
      break;

    default:
    case 2: //distance
      m_print_consumption=WRATHTextureFontFreeType_Distance::texture_consumption;
      m_consumption_bpp=1;
      CustomShaderFont<WRATHTextureFontFreeType_Distance>::default_size_divider(mix_size_divider);
      CustomShaderFont<WRATHTextureFontFreeType_Distance>::minified_font_inflate_factor(mix_inflate);

      switch(cmd_line.m_text_renderer_sub_choice.m_value)
        {
        case 0: //no AA
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_Distance, base);
          break;
          
        default: 
        case 1: //AA
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_Distance, base);
          break;
          
        case 2: //mix with converage
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_Distance, mix);
          m_print_consumption_mix=WRATHTextureFontFreeType_Coverage::texture_consumption;
          break;

        case 3: //mix with multi-res coverage
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_Distance, hq_mix);
          m_print_consumption=WRATHTextureFontFreeType_Distance::texture_consumption;
          m_print_consumption_mix=WRATHTextureFontFreeType_DetailedCoverage::texture_consumption;
          m_consumption_bpp=1;
          break;
          
        case 4: //mix with same shader type
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_Distance, self_mix);
          m_print_consumption=WRATHTextureFontFreeType_Distance::texture_consumption;
          m_print_consumption_mix=WRATHTextureFontFreeType_Distance::texture_consumption;
          m_consumption_bpp=1;
          break;
        }
      break;
      
    case 3: //analytic
      m_print_consumption=WRATHTextureFontFreeType_Analytic::texture_consumption;
      m_consumption_bpp=8;
      CustomShaderFont<WRATHTextureFontFreeType_Analytic>::default_size_divider(mix_size_divider);
      CustomShaderFont<WRATHTextureFontFreeType_Analytic>::minified_font_inflate_factor(mix_inflate);

      switch(cmd_line.m_text_renderer_sub_choice.m_value)
        {
        case 0: //no AA
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_Analytic, base);
          break;

        default: 
        case 1: //AA
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_Analytic, base);
          break;

        case 2: //mix with converage
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_Analytic, mix);
          m_print_consumption_mix=WRATHTextureFontFreeType_Coverage::texture_consumption;
          break;

        case 3: //mix with multi-res coverage
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_Analytic, hq_mix);
          m_print_consumption=WRATHTextureFontFreeType_Analytic::texture_consumption;
          m_print_consumption_mix=WRATHTextureFontFreeType_DetailedCoverage::texture_consumption;
          m_consumption_bpp=8;
          break;

        case 4: //mix with same shader type
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_Analytic, self_mix);
          m_print_consumption=WRATHTextureFontFreeType_Analytic::texture_consumption;
          m_print_consumption_mix=WRATHTextureFontFreeType_Analytic::texture_consumption;
          m_consumption_bpp=8;
          break;
        }
      break;

    case 4: //curve analytic
      SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_CurveAnalytic, base);
      CustomShaderFont<WRATHTextureFontFreeType_CurveAnalytic>::default_size_divider(mix_size_divider);
      CustomShaderFont<WRATHTextureFontFreeType_CurveAnalytic>::minified_font_inflate_factor(mix_inflate);
     
      m_print_consumption=WRATHTextureFontFreeType_CurveAnalytic::texture_consumption_index;
      m_print_consumption_extra=WRATHTextureFontFreeType_CurveAnalytic::texture_consumption_curve;
      m_consumption_bpp=1;
      m_consumption_extra_bpp=3*sizeof(vecN<uint16_t,4>) + sizeof(vecN<uint16_t,2>) + 2;
      m_consumption_extra_bpp+=sizeof(vecN<uint16_t,2>);
        

      switch(cmd_line.m_text_renderer_sub_choice.m_value)
        {
        case 0: //no AA
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_CurveAnalytic, base);
          break;

        default: 
        case 1: //AA
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_CurveAnalytic, base);
          break;

        case 2: //mix with coverage
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_CurveAnalytic, mix);
          m_print_consumption_mix=WRATHTextureFontFreeType_Coverage::texture_consumption;
          break;
          
        case 3: //mix with multi-res coverage
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_CurveAnalytic, hq_mix);
          m_print_consumption_mix=WRATHTextureFontFreeType_DetailedCoverage::texture_consumption;
          break;

        case 4: //mix with same shader type
          SET_LOAD_FONT(fetcher, WRATHTextureFontFreeType_CurveAnalytic, self_mix);
          m_print_consumption_mix=m_print_consumption;
          break;
        }


      break;
    }

  text_shader_specifier
    =WRATHNew WRATHFontShaderSpecifier("animated",
                                       WRATHGLShader::shader_source()
                                       .add_source("simple_ui_font.vert.glsl", WRATHGLShader::from_resource));

  if(cmd_line.m_text_renderer_sub_choice.m_value==0)
    {
      text_shader_specifier->append_fragment_shader_source()
        .add_macro("NO_AA");
    }

  text_shader_specifier->append_fragment_shader_source()
    .add_source(font_fragment_shader, fragment_shader_type);
                                       
  text_shader_specifier->append_pre_vertex_shader_source()
    .force_highp(cmd_line.m_vs_force_highp.m_value)
    .add_macro("TEX_ATTRIBUTE_TYPE", cmd_line.m_tex_attr_prec.m_value)
    .add_macro("TEX_VARYING_TYPE", cmd_line.m_tex_varying_vs_prec.m_value)
    .add_macro("TEX_RECIP_TYPE", cmd_line.m_tex_recip_prec.m_value);
  

  text_shader_specifier->append_pre_fragment_shader_source()
    .force_highp(cmd_line.m_fs_force_highp.m_value)
    .add_macro("TEX_ATTRIBUTE_TYPE", cmd_line.m_tex_attr_prec.m_value)
    .add_macro("TEX_VARYING_TYPE", cmd_line.m_tex_varying_fs_prec.m_value)
    .add_macro("TEX_RECIP_TYPE", cmd_line.m_tex_recip_prec.m_value);

     
  text_shader_specifier->font_discard_thresh(cmd_line.m_font_discard_thresh.m_value);

  
                                      
  for(int i=0;i<3;++i)
    {
      m_page_animation_iterpol[i]
        =create_animation_fx_uniform<float>("animation_fx_interpol",
                                            &m_page_animation_iterpol_value);
      m_animation_matrix[i]
        =create_animation_fx_uniform<float2x2>("animation_matrix",
                                               &m_animation_matrix_value);

      m_fps_uniforms[i]
        =create_non_fx_uniform<float>("animation_fx_interpol", 
                                      0.0f);
      m_fps_uniforms_mats[i]
        =create_non_fx_uniform<float2x2>("animation_matrix", 
                                         float2x2());
    }

  

  WRATHTextureFontFreeType_Distance::texture_creation_size(cmd_line.m_font_texture_size.m_value);
  WRATHTextureFontFreeType_Distance::max_L1_distance(cmd_line.m_max_distance_font_generation.m_value);
  WRATHTextureFontFreeType_Distance::force_power2_texture(cmd_line.m_font_texture_force_power2.m_value);
  WRATHTextureFontFreeType_Distance::fill_rule(WRATHTextureFontFreeType_Distance::non_zero_winding_rule);

  
 
  WRATHTextureFontFreeType_Analytic::texture_creation_size(cmd_line.m_font_texture_size.m_value);
  WRATHTextureFontFreeType_Analytic::generate_sub_quads(cmd_line.m_font_render_use_sub_quads.m_value);

  WRATHTextureFontFreeType_Coverage::texture_creation_size(cmd_line.m_font_texture_size.m_value);
  WRATHTextureFontFreeType_Coverage::force_power2_texture(cmd_line.m_font_texture_force_power2.m_value);

  int mag_filter, min_filter;
  vecN<GLenum, 6> filter_tags(GL_NEAREST,
                              GL_LINEAR,
                              GL_NEAREST_MIPMAP_NEAREST,
                              GL_LINEAR_MIPMAP_NEAREST,
                              GL_NEAREST_MIPMAP_LINEAR,
                              GL_LINEAR_MIPMAP_LINEAR);

  min_filter=std::min(5, std::max(0, cmd_line.m_text_renderer_coverage_min_filter.m_value));
  mag_filter=std::min(1, std::max(0, cmd_line.m_text_renderer_converage_mag_filter.m_value));
  WRATHTextureFontFreeType_Coverage::minification_filter(filter_tags[min_filter]);
  WRATHTextureFontFreeType_Coverage::magnification_filter(filter_tags[mag_filter]);
  WRATHTextureFontFreeType_Coverage::mipmap_slacking_threshhold_level(cmd_line.m_text_renderer_converage_deepness_slack.m_value);

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

  m_font=fetcher.m_font_via_resource(cmd_line.m_font_size.m_value,
                                     spec->name(), 
                                     spec->face_index());

  if(m_font==NULL)
    {
      //load a fail safe value:
      m_font=fetcher.m_font_via_resource(cmd_line.m_font_size.m_value,
                                         WRATHFontFetch::default_font()->name(), 
                                         WRATHFontFetch::default_font()->face_index());
    }

  //m_root:
  m_tr=WRATHNew WRATHTripleBufferEnabler();
  m_root=WRATHNew WRATHLayer(m_tr);
  m_root->simulation_matrix(WRATHLayer::modelview_matrix, float4x4());
  m_root->simulation_matrix(WRATHLayer::projection_matrix, float4x4());
  m_root->simulation_composition_mode(WRATHLayer::modelview_matrix, WRATHLayer::use_this_matrix);
  m_root->simulation_composition_mode(WRATHLayer::projection_matrix, WRATHLayer::use_this_matrix);

  

  m_more_drawers.m_image_drawer
    =WRATHNew WRATHShaderSpecifier("animated image drawer",
                                   WRATHGLShader::shader_source()
                                   .add_source("simple_ui.vert.glsl", WRATHGLShader::from_resource),
                                   WRATHGLShader::shader_source()
                                   .add_source("simple.frag.glsl", WRATHGLShader::from_resource),
                                   WRATHShaderSpecifier::Initializer()
                                   .add_sampler_initializer("utex", 0));
  
   

  m_more_drawers.m_distance_field_drawer
    =WRATHNew WRATHShaderSpecifier("animated distance field drawer",
                                   WRATHGLShader::shader_source()
                                   .add_source("simple_ui.vert.glsl", WRATHGLShader::from_resource),
                                   WRATHGLShader::shader_source()
                                   .add_source("distance_image.frag.glsl", WRATHGLShader::from_resource),
                                   WRATHShaderSpecifier::Initializer()
                                   .add_sampler_initializer("utex", 0));
  std::string aa_fill("AA_HINT"), aa_stroke("AA_HINT");
  if(!cmd_line.m_enable_fill_aa.m_value)
    {
      aa_fill="NO_AA_HINT";
    }
  if(!cmd_line.m_enable_stroke_aa.m_value)
    {
      aa_stroke="NO_AA_HINT";
    }

  m_more_drawers.m_filled_shape_drawer
    =WRATHNew WRATHShaderSpecifier("filled shape drawer",
                                   WRATHGLShader::shader_source()
                                   .add_macro(aa_fill)
                                   .add_source("simple_ui_shape.vert.glsl", WRATHGLShader::from_resource),
                                   WRATHGLShader::shader_source()
                                   .add_macro(aa_fill)
                                   .add_source("simple_ui_shape.frag.glsl", WRATHGLShader::from_resource));

  m_more_drawers.m_stroked_shape_drawer=
    WRATHNew WRATHShaderSpecifier("stroked shape drawer",
                                  WRATHGLShader::shader_source()
                                  .add_macro(aa_stroke)
                                  .add_source("simple_ui_shape.vert.glsl", WRATHGLShader::from_resource),
                                  WRATHGLShader::shader_source()
                                  .add_macro(aa_stroke)
                                  .add_source("simple_ui_shape.frag.glsl", WRATHGLShader::from_resource));
  
    
  
  m_more_drawers.m_line_drawer=
    WRATHNew WRATHShaderSpecifier("line drawer",
                                  WRATHGLShader::shader_source()
                                   .add_source("simple_ui_line.vert.glsl", WRATHGLShader::from_resource),
                                   WRATHGLShader::shader_source()
                                   .add_source("simple_const_color.frag.glsl", WRATHGLShader::from_resource));


  
 


  m_page_animation_iterpol[3]
    =create_animation_fx_uniform<float>("animation_fx_interpol",
                                        &m_page_animation_iterpol_value);

  m_animation_matrix[3]
    =create_animation_fx_uniform<float2x2>("animation_matrix",
                                           &m_animation_matrix_value);
  
  m_page_animation_iterpol[4]
    =create_animation_fx_uniform<float>("animation_fx_interpol",
                                        &m_page_animation_iterpol_value);

  m_animation_matrix[4]
    =create_animation_fx_uniform<float2x2>("animation_matrix",
                                           &m_animation_matrix_value);

  m_page_animation_iterpol[5]
    =create_animation_fx_uniform<float>("animation_fx_interpol",
                                        &m_page_animation_iterpol_value);

  m_animation_matrix[5]
    =create_animation_fx_uniform<float2x2>("animation_matrix",
                                           &m_animation_matrix_value);

  m_page_animation_iterpol[6]
    =create_animation_fx_uniform<float>("animation_fx_interpol",
                                        &m_page_animation_iterpol_value);

  m_animation_matrix[6]
    =create_animation_fx_uniform<float2x2>("animation_matrix",
                                           &m_animation_matrix_value);

  m_page_animation_iterpol[7]
    =create_animation_fx_uniform<float>("animation_fx_interpol",
                                        &m_page_animation_iterpol_value);

  m_animation_matrix[7]
    =create_animation_fx_uniform<float2x2>("animation_matrix",
                                           &m_animation_matrix_value);

  

  FilePacket::ExtraDrawState ex;
  WRATHTextItem::ExtraDrawState fps_ex;
  WRATHGLStateChange::blend_state::handle blender;
  
  blender=WRATHNew WRATHGLStateChange::blend_state(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  ex.m_line_extra_state
    .add_uniform(m_page_animation_iterpol[3])
    .add_uniform(m_animation_matrix[3])
    .add_gl_state_change(blender);

  ex.m_image_extra_state
    .add_uniform(m_page_animation_iterpol[4])
    .add_uniform(m_animation_matrix[4])
    .add_gl_state_change(blender);

  ex.m_stroked_shape_extra_state
    .add_uniform(m_page_animation_iterpol[5])
    .add_uniform(m_animation_matrix[5])
    .add_gl_state_change(blender);
  
  ex.m_distance_field_extra_state
    .add_uniform(m_page_animation_iterpol[6])
    .add_uniform(m_animation_matrix[6])
    .add_gl_state_change(blender);
  
  ex.m_filled_shape_extra_state
    .add_uniform(m_page_animation_iterpol[7])
    .add_uniform(m_animation_matrix[7])
    .add_gl_state_change(blender);


  ex.m_text_extra_state.opaque_pass_state()
    .add_uniform(m_animation_matrix[0])
    .add_uniform(m_page_animation_iterpol[0]);

  fps_ex.opaque_pass_state()
    .add_uniform(m_fps_uniforms_mats[0])
    .add_uniform(m_fps_uniforms[0]);

  if(!cmd_line.m_font_lazy_z.m_value)
    {
      ex.m_text_extra_state.translucent_pass_state()
        .add_uniform(m_animation_matrix[1])
        .add_uniform(m_page_animation_iterpol[1]);

      fps_ex.translucent_pass_state()
        .add_uniform(m_fps_uniforms_mats[1])
        .add_uniform(m_fps_uniforms[1]);
    }
  else
    {
      ex.m_text_extra_state.translucent_pass_state()
        .add_uniform(m_animation_matrix[2])
        .add_uniform(m_page_animation_iterpol[2]);

      fps_ex.translucent_pass_state()
        .add_uniform(m_fps_uniforms_mats[2])
        .add_uniform(m_fps_uniforms[2]);
    }

  if(!cmd_line.m_use_vbo.m_value)
    {
      ex.m_line_extra_state.m_buffer_object_hint=GL_INVALID_ENUM;
      ex.m_image_extra_state.m_buffer_object_hint=GL_INVALID_ENUM;
      fps_ex.m_common_pass_state.m_buffer_object_hint=GL_INVALID_ENUM;
      fps_ex.opaque_pass_state().m_buffer_object_hint=GL_INVALID_ENUM;
      fps_ex.translucent_pass_state().m_buffer_object_hint=GL_INVALID_ENUM;
      ex.m_text_extra_state.m_common_pass_state.m_buffer_object_hint=GL_INVALID_ENUM;
      ex.m_text_extra_state.opaque_pass_state().m_buffer_object_hint=GL_INVALID_ENUM;
      ex.m_text_extra_state.translucent_pass_state().m_buffer_object_hint=GL_INVALID_ENUM;
    }

  m_bg_color/=255.0f;

 
  m_all_contents=
    WRATHNew FilePacket(m_root,
                        WRATHTextItem::Drawer(text_shader_specifier, text_packer),
                        m_more_drawers,
                        cmd_line.m_display_font_size.m_value,
                        m_font, 
                        vecN<GLubyte,4>(bound_to_byte(cmd_line.m_text_red.m_value),
                                        bound_to_byte(cmd_line.m_text_blue.m_value),
                                        bound_to_byte(cmd_line.m_text_green.m_value),
                                        255),
                        m_bg_color,
                        cmd_line.m_text_chunk_size.m_value,
                        cmd_line.m_font_lazy_z.m_value, 
                        ex, fetcher,
                        m_load_font_in_thread,
                        cmd_line.m_manual_mipmap_generation.m_value);

  if(!cmd_line.m_image_use_mipmaps.m_value)
    {
      m_all_contents->m_minification_image_filter=GL_LINEAR;
    }

  //load the file by asking to fetch it:
  /*
    check if it's a directory.
   */
  ::DIR *ptr(NULL);
  enum FileType::file_fetch_type load_type(FileType::load_interpreted);
  std::string filename(WRATHUtil::filename_fullpath(cmd_line.m_file_to_view.m_value));

  ptr=::opendir(filename.c_str());
  if(ptr!=NULL)
    {
      load_type=FileType::load_directory;
      ::closedir(ptr);
      if(filename.empty() or *filename.rbegin()!=DIRECTORY_SLASH)
        {
          filename.push_back(DIRECTORY_SLASH);
        }
    }
  m_current_display_contents=
    m_all_contents->fetch_file(filename, load_type);
  m_current_display_contents->container().visible(true);


  //only for debug:
  if(m_show_stats or m_load_font_in_thread)
    {
      
      m_fps_text_vis=
        WRATHNew WRATHLayerItemNodeRotateTranslate(m_root->triple_buffer_enabler());
      m_fps_text_vis->z_order(std::numeric_limits<int16_t>::min()+1); 

      m_fps_text=
        WRATHNew WRATHTextItem(NodePacker::Factory(), 0,
                               m_root,   
                               WRATHLayer::SubKey(m_fps_text_vis),
                               WRATHTextItemTypes::text_transparent,
                               WRATHTextItem::Drawer(),
                               WRATHTextItem::draw_order(),
                               fps_ex);
      //m_fps_text->draw_order_hint(m_fps_text_vis->node_depth_value());
    }

  if(m_titlebar_fixed)
    {
      m_titlebar=cmd_line.m_titlebar.m_value;
    }
  else
    {
      m_titlebar=filename;
    }
  titlebar(m_titlebar);

  m_key_commands.push_back(on_key_command(&TextViewer::on_page_back, m_back_key));
  m_key_commands.push_back(on_key_command(&TextViewer::on_quit, m_quit_key));
  m_key_commands.push_back(on_key_command(&TextViewer::on_reload, m_reload_key));

  m_key_commands.push_back(on_key_command(&TextViewer::on_left, m_left_key));
  m_key_commands.push_back(on_key_command(&TextViewer::on_right, m_right_key));
  m_key_commands.push_back(on_key_command(&TextViewer::on_up, m_up_key));
  m_key_commands.push_back(on_key_command(&TextViewer::on_down, m_down_key));
  m_key_commands.push_back(on_key_command(&TextViewer::on_zoom_in, m_zoom_in_key));
  m_key_commands.push_back(on_key_command(&TextViewer::on_zoom_out, m_zoom_out_key));

  m_key_commands.push_back(on_key_command(&TextViewer::on_print_font_texture_consumption,
                                          m_print_texture_consumption));

  glClearColor(m_bg_color[0], m_bg_color[1], m_bg_color[2], m_bg_color[3]);
  update_transformation();
}

TextViewer::
~TextViewer()
{
  clean_up();
}

void
TextViewer::
on_quit(bool)
{
  m_smart_update=true;
  end_demo();
}

void
TextViewer::
on_reload(bool v)
{
  if(v and m_current_display_contents!=NULL)
    {
      m_current_display_contents->reload_file();
      update_widget();
    }
}

void
TextViewer::
on_page_back(bool v)
{
  if(v and !m_link_stack.empty())
    {
      m_next_display_transformation.first=true;
      m_next_display_transformation.second=m_link_stack.back().m_transformation;
      
      m_next_display_contents=m_link_stack.back().m_file;
      m_link_stack.pop_back(); 
      
      m_page_animation_stage=page_disappearing;
      m_page_animation_time.restart();
      update_widget();
    }
}


void
TextViewer::
clean_up(void)
{
  if(m_root==NULL)
    {
      return;
    }

  if(m_fps_text!=NULL)
    {
      WRATHPhasedDelete(m_fps_text);
      WRATHPhasedDelete(m_fps_text_vis);
    }

  WRATHPhasedDelete(m_all_contents);
  WRATHPhasedDelete(m_root);
  m_root=NULL;

  WRATHResourceManagerBase::clear_all_resource_managers();

  int cycle_count;

  cycle_count=m_tr->purge_cleanup();
  m_tr=NULL;

  std::cout << "\nTook " << cycle_count 
            << " cycles to terminate\n";


}

void
TextViewer::
update_page_animation(void)
{
  int elapsed_time;

  elapsed_time=m_page_animation_time.elapsed();

  if(elapsed_time>m_animation_time_ms)
    {
      if(m_page_animation_stage==page_disappearing)
        {
          m_current_display_contents->container().visible(false);

          m_current_display_contents=m_next_display_contents;
          if(m_next_display_transformation.first)
            {
              m_current_display_contents
                ->transformation_node()
                .transformation(m_next_display_transformation.second);
            }

          m_zoom_start_transformation=m_current_display_contents->
             transformation_node().values().m_transformation;

          m_current_display_contents->container().visible(true);
          m_need_to_update_culling=true;
          m_page_animation_time.restart();       
          m_page_animation_stage=page_appearing;         

          if(m_next_display_jump_to.first)
            {
              handle_jump();
              m_next_display_jump_to.first=false;
            }


          if(!m_titlebar_fixed)
            {
              m_titlebar=m_current_display_contents->filename();
              titlebar(m_titlebar);
            }
        }
      else
        {
          WRATHassert(m_page_animation_stage==page_appearing);
          m_page_animation_stage=page_no_animation;
          m_bg_color=m_current_display_contents->background_color();
          m_animation_matrix_value=float2x2();
          m_page_animation_iterpol_value=0.0f;
        }
    }
  else
    {
      float tau(elapsed_time), bg_tau;
      const float v_range_ready(1.0f), v_range_not_ready(0.7f);
      const float v_number_rotations(0.5f);
      float v_cos, v_sin;

      tau/=static_cast<float>(m_animation_time_ms);
      bg_tau=0.5f*tau;
      
      if(m_page_animation_stage==page_appearing)
        {
          tau=1.0-tau;
          bg_tau+=0.5f;
        }

      if(m_animate_with_rotation)
        {
          sincosf(tau*2.0f*M_PI*v_number_rotations, &v_sin, &v_cos);
          m_animation_matrix_value(0,0)=v_cos;
          m_animation_matrix_value(1,0)=-v_sin;
          m_animation_matrix_value(0,1)=v_sin;
          m_animation_matrix_value(1,1)=v_cos;
        }

      

      m_bg_color=(1.0f-bg_tau)*m_current_display_contents->background_color()
        + (bg_tau)*m_next_display_contents->background_color();

      if(m_page_animation_stage==page_disappearing and
         !m_next_display_contents->file_loaded())
        {
          m_page_animation_iterpol_value=v_range_not_ready*tau;
        }
      else
        {
          m_page_animation_iterpol_value=v_range_ready*tau;
        }
    }
  update_widget();

}
  
     
void
TextViewer::
paint(void)
{

  if(m_viewport_sz.x()!=width() or m_viewport_sz.y()!=height())
    {
      glViewport(0, 0, width(), height());

      m_viewport_sz.x()=width();
      m_viewport_sz.y()=height();
      m_culling_window=m_viewport_sz;

      if(m_rotate)
        {
          std::swap(m_culling_window.x(),
                    m_culling_window.y());
        }

      //update perspective matrix:
      float_orthogonal_projection_params proj_params(0, m_culling_window.x(),
                                                     m_culling_window.y(), 0);
      float4x4 pers_mat;
      pers_mat.orthogonal_projection_matrix(proj_params);

      if(m_rotate)
        {
          float4x4 Rxy(vec3(0,0,0), vec3(0,1,0), vec3(-1,0,0), vec3(0,0,1));
          pers_mat=Rxy*pers_mat;
        }

      m_root->simulation_matrix(WRATHLayer::projection_matrix, pers_mat);
      m_need_to_update_culling=true;
    }
  
  if(transformation_changing())
    {
      m_scroll_animating=false;
      m_flick_scrolling.x()=false;
      m_flick_scrolling.y()=false;
      update_transformation();
    }
  else if(m_flick_scrolling.x() or m_flick_scrolling.y())
    {
      m_scroll_animating=false;
      flick_scroll_animate();
    }
  else if(m_scroll_animating)
    {
      scroll_animate();
    }

  

  if(m_fps_text!=NULL)
    {
      WRATHTextDataStream ostr;
      vec4 colorf;
          
      colorf=vec4(1.f, 1.f, 1.f, 1.f) - m_current_display_contents->background_color();
      colorf.w()=0.5f;

      m_fps_text->clear();

      ostr.stream()
        << WRATHText::set_font(m_font)
        << WRATHText::set_pixel_size(25)
        << WRATHText::set_color(colorf)
        << "\n\n";
      
      
      if(m_load_font_in_thread)
        {
          if(m_all_contents->update_threaded_font_load_progress(ostr))
            {
              update_widget();
            }
        }

      if(m_show_stats and
         (!m_smart_update or transformation_changing() 
          or m_scroll_animating or m_page_animation_stage!=page_no_animation
          or m_flick_scrolling.x() or m_flick_scrolling.y()))
        {
          float delta_time=m_fps_time.restart();

          ostr.stream()
            << "FPS: " << static_cast<int>(1000.0f/std::max(1.0f, delta_time))

            << "\nzoom=" << m_current_display_contents->transformation_node().scaling_factor()
            << ", tr=" << m_current_display_contents->transformation_node().translation()

            << "\n#chars=" << m_current_display_contents->number_chars() 
            << "\n#streams=" << m_current_display_contents->number_streams()
            << "\n#pages=" << m_current_display_contents->number_chunks()
            << "\nstats per frame:"
            << "\n\t m_draw_count=" << m_stats.m_draw_count
            << "\n\t m_program_count=" << m_stats.m_program_count
            << "\n\t m_texture_choice_count=" << m_stats.m_texture_choice_count
            << "\n\t m_gl_state_change_count=" << m_stats.m_gl_state_change_count
            << "\n\t m_attribute_change_count=" << m_stats.m_attribute_change_count
            << "\n\t m_buffer_object_bind_count=" << m_stats.m_buffer_object_bind_count
            << "\n\t m_layer_count=" << m_stats.m_layer_count;
        }
    
      if(m_load_font_in_thread or m_show_stats)
        {
          m_fps_text->add_text(ostr);
        }
    }

  

  if(m_page_animation_stage!=page_no_animation)
    {
      update_page_animation();
    }
  else
    {
      m_bg_color=m_current_display_contents->background_color();
      m_page_animation_iterpol_value=0.0f;
    }

 

  m_tr->signal_complete_simulation_frame();

  if(m_need_to_update_culling)
    {
      /*
        We put the update culling AFTER signaling
        completing a simulation frame because
        the culling code in FileData.cpp needs
        the transformation from the screen to
        the node (FileData::transformation_node()).
        That value is updated when the signal
        complete simulation frame is fired.

        TODO: change from using a function
        update_culling to creating the TextChunk
        objects' WRATHLayer object with a
        WRATHLayerClipDrawer which does not
        draw but does the computation with
        m_culling_window to skip it's WRATHLayer
       */
      m_need_to_update_culling=false;
      m_current_display_contents->update_culling(m_culling_window, m_disable_culling);
    }
  m_tr->signal_begin_presentation_frame();


  glDepthMask(GL_TRUE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glClearColor(m_bg_color[0], m_bg_color[1], m_bg_color[2], m_bg_color[3]);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
  

  
  
  m_stats=WRATHLayer::draw_information();
  m_root->clear_and_draw(&m_stats);

  
  if(transformation_changing() or !m_smart_update)
    {
      update_widget();
    }

  if(m_issue_gl_finish)
    {
      glFinish();
    }
}


vec2
TextViewer::
compute_translation(const vec2 &in_mouse_pt,
                    WRATHLayerItemNodeRotateTranslate &node,
                    const vec2 &p)
{
  vec2 return_value, V, W;

  node.walk_hierarchy_if_necessary();
  W=node.global_values().m_transformation.apply_to_point(p);
  V=in_mouse_pt - W;
  return node.translation()+V;
}

int
TextViewer::
compute_scroll_time(const vec2 &start, const vec2 &end)
{
  float dist, time_seconds;

  dist=(start-end).L1norm();
  time_seconds=dist/m_automatic_scroll_speed;

  return 
    std::min(m_max_time_for_automatic_scroll,
             static_cast<int>(1000.0f*time_seconds));
}

void
TextViewer::
scroll_animate(void)
{
  int elapsed;

  elapsed=m_scroll_animation_time.elapsed();
  if(elapsed>=m_scroll_animation_period)
    {
      m_scroll_animating=false;
      m_current_display_contents->transformation_node().translation(m_scroll_animation_end);
      m_need_to_update_culling=true;
      update_widget();
    }
  else
    {
      float tau;
      vec2 tr;

      tau=static_cast<float>(elapsed)/static_cast<float>(m_scroll_animation_period);
      tr=m_scroll_animation_start + tau*(m_scroll_animation_end-m_scroll_animation_start);
      m_current_display_contents->transformation_node().translation(tr);
      m_need_to_update_culling=true;
      update_widget();
      
    }
}


bool
TextViewer::
fit_translation(float scale, vec2 &in_out_pt)
{
  const WRATHTextAttributePacker::BBox &bb(m_current_display_contents->bbox());
  bool return_value(false);

  if(!bb.empty())
    {
      vec2 max_bounds(m_culling_window.x() - scale*bb.max_corner().x(),
                      m_culling_window.y() - scale*bb.max_corner().y());

      if(in_out_pt.x()>0.0f)
        {
          return_value=true;
          in_out_pt.x()=0.0f;
        }
      else if(in_out_pt.x()<max_bounds.x() and max_bounds.x()<0.0f)
        {
          in_out_pt.x()=std::min(0.0f, max_bounds.x());
          return_value=true;
        }
      else if(max_bounds.x()>=0.0f and in_out_pt.x()<0.0f)
        {
          in_out_pt.x()=0.0f;
          return_value=true;          
        }

      
      if(in_out_pt.y()>0.0f)
        {
          return_value=true;
          in_out_pt.y()=0.0f;
        }
      else if(in_out_pt.y()<max_bounds.y() and max_bounds.y()<0.0f)
        {
          in_out_pt.y()=std::min(0.0f, max_bounds.y());
          return_value=true;
        }
      else if(max_bounds.y()>=0.0f and in_out_pt.y()<0.0f)
        {
          in_out_pt.y()=0.0f;
          return_value=true;          
        }
    }
  return return_value;
}

void
TextViewer::
handle_jump(void)
{
  
  std::pair<bool, vec2> R;    
  R=m_current_display_contents->jump_tag(m_next_display_jump_to.second);

  if(R.first)
    {
      vec2 tr; 
      
      tr=compute_translation(m_ms_pt,
                             m_current_display_contents->transformation_node(),
                             R.second);
                             
      tr.x()=std::min(0.0f, tr.x());
      m_current_display_contents->transformation_node().translation(tr);
      m_need_to_update_culling=true;

      update_widget();
    }
}


void
TextViewer::
handle_touch_begin(int ix, int iy)
{  
  int x(ix), y(iy);

  if(m_rotate)
    {

      x=height()-iy;
      y=ix;
    }

  m_flick_scrolling.x()=false;      
  m_flick_scrolling.y()=false;
  m_zoom_pivot=vec2(x, y);                    
  m_zoom_start_transformation=m_current_display_contents->
    transformation_node().transformation();
  m_zoom_time.restart();
  m_is_zooming=false;
}

bool
TextViewer::
check_for_links(int x, int y)
{  
  const FileData::LinkAtResult *link;
  bool push_stack(true);
  FileData *ptr(NULL);
 
  link=m_current_display_contents->link_at(x, y);
  if(link!=NULL)
    {
      if(link->m_is_quit_link)
        {
          end_demo();
          return false;
        }

      m_next_display_transformation.first=false;
      ptr=link->m_link_file;
      if(ptr==NULL and !m_link_stack.empty())
        {
          ptr=m_link_stack.back().m_file;
          m_next_display_transformation.first=m_transition_on_jump;
          m_next_display_transformation.second=m_link_stack.back().m_transformation;
          m_link_stack.pop_back();
          push_stack=false;
        }
    }
  
  if(ptr!=NULL)
    {
      if(push_stack)
        {
          m_link_stack.push_back(m_current_display_contents);
        }
      
      m_next_display_jump_to=link->m_jump_tag;
      m_ms_pt=vec2(0, 0);
      
      if(ptr!=m_current_display_contents or m_transition_on_jump)
        {
          m_next_display_contents=ptr;
          m_page_animation_stage=page_disappearing;
          m_page_animation_time.restart();       
        }
      else if(m_next_display_jump_to.first)
        {
          std::pair<bool, vec2> R;    
          R=ptr->jump_tag(m_next_display_jump_to.second);
          if(R.first)
            {
              m_scroll_animation_start=ptr->transformation_node().translation();
              m_scroll_animation_end=compute_translation(m_ms_pt,
                                                         ptr->transformation_node(),
                                                         R.second); 
              fit_translation(ptr->transformation_node().scaling_factor(),
                              m_scroll_animation_end);
              m_scroll_animation_period=compute_scroll_time(m_scroll_animation_start,
                                                            m_scroll_animation_end);
              
              m_scroll_animating=true;
              m_scroll_animation_time.restart();
            }
          
          m_next_display_jump_to.first=false;
        }
    }
  update_widget();
  return ptr!=NULL;
}

void
TextViewer::
handle_touch_end(int ix, int iy)
{
  int x(ix), y(iy);

  if(m_rotate)
    {
      x=height()-iy;
      y=ix;

    }

  /*
    NOTE: we check for m_is_zooming first, this way
    ending a zoom gesture on top of a link does not
    trigger hittin the link.
   */
  if(!m_is_zooming and !check_for_links(x,y) and m_enable_flick)
    {
      m_flick_begin_point=m_current_display_contents->transformation_node().translation();
      for(int i=0;i<2;++i)
        {
          if(m_last_touch_speed[i]>0.0f)
            {
              m_flick_speed_magnitudes[i]=m_last_touch_speed[i];
              m_flick_speed_signs[i]=1.0f;
            }
          else
            {
              m_flick_speed_magnitudes[i]=-m_last_touch_speed[i];
              m_flick_speed_signs[i]=-1.0f;
            }
        }

      m_flick_speed_magnitudes*=m_touch_speed_multiplier;
      m_flick_speed_magnitudes.x()=std::min(m_flick_speed_magnitudes.x(), m_max_flick_speed);
      m_flick_speed_magnitudes.y()=std::min(m_flick_speed_magnitudes.y(), m_max_flick_speed);

      m_flick_end_times=m_flick_speed_magnitudes/m_flick_deacceleration;

      m_flick_scrolling.x()=true;      
      m_flick_scrolling.y()=true;
      m_is_zooming=false;  
      m_flick_time.restart();
      update_widget();
    }
  else 
    {
      trigger_scroll_animate();
    }

}

void
TextViewer::
trigger_scroll_animate(void)
{
  vec2 ps(m_current_display_contents->transformation_node().translation());
  float scale(m_current_display_contents->transformation_node().scaling_factor());
  if(fit_translation(scale, ps) and m_auto_scroll)
    {
      m_scroll_animation_time.restart();
      m_scroll_animation_start=m_current_display_contents->transformation_node().translation();
      m_scroll_animation_end=ps;
      m_scroll_animation_period=compute_scroll_time(m_scroll_animation_start,
                                                    m_scroll_animation_end);
      m_scroll_animating=true;
      update_widget();
    }
} 

void
TextViewer::
flick_scroll_animate(void)
{
  float flick_time;
  vec2 pt, time_sep, sp(0.0f, 0.0f);

  flick_time=static_cast<float>(m_flick_time.elapsed());

  time_sep.x()=std::min(flick_time, m_flick_end_times.x());
  time_sep.y()=std::min(flick_time, m_flick_end_times.y());


  pt=m_current_display_contents->transformation_node().translation();

  /*
    Incrementally updating the speed and position
    does not work very well when the framerate is too
    low, as such we do it analytically:
  */
  pt.x()=m_flick_begin_point.x() 
    + time_sep.x()*m_flick_speed_magnitudes.x()*m_flick_speed_signs.x()
    - 0.5f*m_flick_deacceleration*m_flick_speed_signs.x()*time_sep.x()*time_sep.x();

  pt.y()=m_flick_begin_point.y() 
    + time_sep.y()*m_flick_speed_magnitudes.y()*m_flick_speed_signs.y() 
    - 0.5f*m_flick_deacceleration*m_flick_speed_signs.y()*time_sep.y()*time_sep.y();
    

  m_current_display_contents->transformation_node().translation(pt);
  m_need_to_update_culling=true;

  m_flick_scrolling.x()=m_flick_scrolling.x() and (m_flick_end_times.x()>flick_time);
  m_flick_scrolling.y()=m_flick_scrolling.y() and (m_flick_end_times.y()>flick_time);

  if(!m_flick_scrolling.x() and !m_flick_scrolling.y())
    {
      trigger_scroll_animate();
    } 
  
  update_widget();
}


void
TextViewer::
handle_touch_move(const vec2 &inpos, const vec2 &indelta)
{
  vec2 pos(inpos), delta(indelta);

  if(m_rotate)
    {
      pos.x()=height()-inpos.y();
      pos.y()=inpos.x();

      delta.x()=-indelta.y();
      delta.y()=indelta.x();
    }

  if(m_zoom_time.elapsed()>m_zoom_gesture_begin_time)
    {
      m_is_zooming=true;
    }  

  
  m_last_touch_speed=delta/static_cast<float>(std::max(1, m_zoom_time.elapsed()));
  
  if(!m_is_zooming)
    {
      vec2 ps;            
      float zdx(pos.x()-m_zoom_pivot.x());
      float zdy(pos.y()-m_zoom_pivot.y());
      
      ps=
        m_current_display_contents->transformation_node().translation() 
        + delta;
      
      m_current_display_contents->transformation_node().translation(ps);    
      
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
          m_zoom_pivot=pos;
          m_zoom_start_transformation=m_current_display_contents->
            transformation_node().transformation();
        }
    }
  else
    { 
      float zoom_factor(pos.y()-m_zoom_pivot.y());
      WRATH2DRigidTransformation R, P(m_zoom_start_transformation);
      vec2 p0(m_zoom_pivot);
      
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
      
      m_current_display_contents
        ->transformation_node().transformation(R*P);
    }
  m_scroll_animating=false;
  m_need_to_update_culling=true;   
  update_widget();
          
}



bool
TextViewer::
common_on_transformation_key_begin(bool v)
{
  if(v)
    {
      reset_transformation_time();
    }
  
  return transformation_changing();
}

void
TextViewer::
common_on_transformation_key_end(bool was_moving)
{
  if(was_moving and !transformation_changing())
    {
      trigger_scroll_animate();
    }
}


void
TextViewer::
on_left(bool v)
{
  bool was_moving;
  was_moving=common_on_transformation_key_begin(v);
  m_scroll_left=v;
  common_on_transformation_key_end(was_moving);
}


void
TextViewer::
on_right(bool v)
{
  bool was_moving;
  was_moving=common_on_transformation_key_begin(v);
  m_scroll_right=v;
  common_on_transformation_key_end(was_moving);
}


void
TextViewer::
on_up(bool v)
{
  bool was_moving;
  was_moving=common_on_transformation_key_begin(v);
  m_scroll_up=v;
  common_on_transformation_key_end(was_moving);
}


void
TextViewer::
on_down(bool v)
{
  bool was_moving;
  was_moving=common_on_transformation_key_begin(v);
  m_scroll_down=v;
  common_on_transformation_key_end(was_moving);
}


void
TextViewer::
on_zoom_in(bool v)
{
  bool was_moving;
  was_moving=common_on_transformation_key_begin(v);
  m_zoom_in=v;
  common_on_transformation_key_end(was_moving);
}


void
TextViewer::
on_zoom_out(bool v)
{
  bool was_moving;
  was_moving=common_on_transformation_key_begin(v);
  m_zoom_out=v;
  common_on_transformation_key_end(was_moving);
}

void
TextViewer::
on_print_font_texture_consumption(bool v)
{
  if(v and m_print_consumption!=NULL)
    {
      WRATHImage::TextureAllocatorHandle::texture_consumption_data_type v0, v1;


      v0=m_print_consumption();

      std::cout << "\n\nMain Font:\n\tnumber_texels in main font textures=" 
                << v0.m_number_texels << "(bytes=" 
                << v0.m_number_texels*m_consumption_bpp
                << ")\n\tnumber texels used="
                << v0.m_number_texels_used<< "(bytes=" 
                << v0.m_number_texels_used*m_consumption_bpp
                << ")\n\tutilization="
                << v0.utilization();
        
      if(m_print_consumption_extra!=NULL)
        {
          v1=m_print_consumption_extra();
          std::cout << "\nAdditional data:\n\tnumber_texels=" 
                    << v1.m_number_texels << "(bytes=" 
                    << v1.m_number_texels*m_consumption_extra_bpp
                    << ")\n\tnumber texels used="
                    << v1.m_number_texels_used << "(bytes=" 
                    << v1.m_number_texels_used*m_consumption_extra_bpp
                    << ")\n\tutilization="
                    << v1.utilization();
        }

      if(m_print_consumption_mix!=NULL and m_print_consumption_mix!=m_print_consumption)
        {
          v1=m_print_consumption_mix();
          std::cout << "\nCoverage Font:\n\tnumber_texels in coverage font textures=" 
                    << v1.m_number_texels
                    << "\n\tnumber texels used in coverage font="
                    << v1.m_number_texels_used
                    << "\n\tutilization="
                    << v1.utilization();
          int u, t;
          float f;

          u=v0.m_number_texels_used + v1.m_number_texels_used;
          t=std::max(1, v0.m_number_texels + v1.m_number_texels);
          f=static_cast<float>(u)/static_cast<float>(t);

          std::cout << "\nOverall utilization=" << f;
        }
    }
}


void
TextViewer::
handle_event(FURYEvent::handle ev)
{
  if(m_print_events)
    {
      std::cout << "\n";
      ev->log_event(std::cout);
    }
  
  
  switch(ev->type())
    {   
    case FURYEvent::TouchDown:
      {
        FURYTouchEvent::handle tev(ev.static_cast_handle<FURYTouchEvent>());

        handle_touch_begin(tev->position().x(), tev->position().y());
        tev->accept();
          
      }
      break;

    case FURYEvent::TouchUp:
      {
        FURYTouchEvent::handle tev(ev.static_cast_handle<FURYTouchEvent>());
        
        handle_touch_end(tev->position().x(), tev->position().y());
        tev->accept();
      }
      break;

    case FURYEvent::TouchMotion:
      {
        FURYTouchEvent::handle tev(ev.static_cast_handle<FURYTouchEvent>());
        handle_touch_move(tev->position(), tev->delta()); 
        tev->accept();
      }
      break;

    case FURYEvent::KeyDown:
    case FURYEvent::KeyUp:
      {
        FURYKeyEvent::handle qe(ev.static_cast_handle<FURYKeyEvent>());
        bool is_key_press(qe->type()==FURYEvent::KeyDown);
            
        for(std::vector<on_key_command>::iterator
              iter=m_key_commands.begin(), end=m_key_commands.end();
            iter!=end; ++iter)
          {
            if(iter->second==qe->key().m_value)
              {
                on_key_function fptr(iter->first);
                (this->*fptr)(is_key_press);
                break;
              }
          }
        update_widget();
        qe->accept();            
      }
      break;

    case FURYEvent::MouseButtonDown:
      if(m_touch_emulate)
        {
          FURYMouseButtonEvent::handle me(ev.static_cast_handle<FURYMouseButtonEvent>());

          m_last_ms_position=vec2(me->pt().x(), me->pt().y());
          m_button_down=true;

          handle_touch_begin(me->pt().x(), me->pt().y());
          ev->accept();
        }
      break;

    case FURYEvent::MouseButtonUp:
      if(m_touch_emulate)
        {
          FURYMouseButtonEvent::handle me(ev.static_cast_handle<FURYMouseButtonEvent>());
          m_button_down=false;
          handle_touch_end(me->pt().x(), me->pt().y());
          ev->accept();
        }
      break;

    case FURYEvent::MouseMotion:
      if(m_touch_emulate and m_button_down)
        {
          FURYMouseMotionEvent::handle me(ev.static_cast_handle<FURYMouseMotionEvent>());

          vec2 pos(me->pt().x(), me->pt().y());
          vec2 delta(pos-m_last_ms_position);
          
          m_last_ms_position=pos;
          handle_touch_move(pos, delta);
          ev->accept();
        }
      break;

    default:
      ev->accept();
    }
}

void
TextViewer::
reset_transformation_time(void)
{
  m_animation_time.restart();
}

bool
TextViewer::
transformation_changing(void)
{
  return m_scroll_left
    or m_scroll_right
    or m_scroll_up
    or m_scroll_down
    or m_zoom_in
    or m_zoom_out;
}

void
TextViewer::
update_transformation(void)
{
  float elapsed_time;
  vec2 pixels_per_ms(0.3f, 0.3f);
  vec2 pixels_to_advance;
  vec2 delta_value(0.0f, 0.0f);

  elapsed_time=static_cast<float>(m_animation_time.restart());
  pixels_to_advance=elapsed_time*pixels_per_ms;

  if(m_scroll_left)
    {
      delta_value.x()-=pixels_to_advance.x();
    }

  if(m_scroll_right)
    {
      delta_value.x()+=pixels_to_advance.x();
    }

  if(m_scroll_down)
    {
      delta_value.y()+=pixels_to_advance.y();
    }

  if(m_scroll_up)
    {
      delta_value.y()-=pixels_to_advance.y();
    }

  vec2 ps;
  ps=
    m_current_display_contents->transformation_node().translation() 
    - delta_value;

  m_current_display_contents->transformation_node()
    .translation(ps);

  if(m_zoom_in xor m_zoom_out)
    {
      float zoom_factor;
      WRATH2DRigidTransformation R, P;
      vec2 p0;

      zoom_factor=powf(1.001, elapsed_time);
      if(m_zoom_out)
        {
          zoom_factor=1.0f/zoom_factor;
        }

      // R(p) = zoom_factor*(p-p0) + p0
      //      = zoom_factor*p + (1-zoom_factor)*p0
      // where p0 is the zoom point.
      p0=vec2( width()/2, height()/2);
      R.scale(zoom_factor);
      R.translation( (1.0f-zoom_factor)*p0);
      
      P=m_current_display_contents->
        transformation_node().transformation();

      m_current_display_contents
        ->transformation_node().transformation(R*P);
    }
  m_need_to_update_culling=true;
}

