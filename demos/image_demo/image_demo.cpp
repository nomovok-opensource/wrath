/*! 
 * \file image_demo.cpp
 * \brief file image_demo.cpp
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
#include <dirent.h>
#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "WRATHmalloc.hpp"
#include "WRATHRectItem.hpp"
#include "WRATHTextureFontFreeType_Mix.hpp"
#include "WRATHTextItem.hpp"
#include "WRATHgluniform.hpp"
#include "WRATHUtil.hpp"
#include "WRATHImage.hpp"
#include "WRATHFontFetch.hpp"
#include "WRATHDefaultTextAttributePacker.hpp"
#include "WRATHDefaultRectAttributePacker.hpp"
#include "WRATHTime.hpp"
#include "WRATHLayerItemNodeRotateTranslate.hpp"
#include "WRATHLayerItemWidgetsRotateTranslate.hpp"
#include "NodePacker.hpp"
#include "ngl_backend.hpp"
#include "wrath_demo.hpp"
#include "wrath_demo_image_support.hpp"

//use node type without additions for this demo
typedef NodePacker::FamilySet::PlainFamily PlainFamily;
typedef NodePacker::FamilySet::SimpleXSimpleYImageFamily ImageFamily;
class draw_order:public WRATHDrawOrder
{
public:
  explicit
  draw_order(float pz):
    m_z(pz)
  {}

  void
  z(float pz)
  {
    m_z=pz;
    note_change();
  };

  float
  z(void) const
  {
    return m_z;
  }

  void
  print_stats(std::ostream& ostr) const
  {
    ostr << m_z;
  }

private:
  float m_z;
};

class draw_order_comparer:public WRATHDrawOrderComparer
{
public:
  enum draw_sort_order_type
  compare_objects(WRATHDrawOrder::const_handle lhs, 
                  WRATHDrawOrder::const_handle rhs) const
  {
    if(lhs==rhs)
      {
        return equal_draw_sort_order;
      }

    if(!lhs.valid() and rhs.valid())
      {
        return less_draw_sort_order;
      }

    if(lhs.valid() and !rhs.valid())
      {
        return greater_draw_sort_order;
      }

    float z1, z2;
    z1=lhs.static_cast_handle<draw_order>()->z();
    z2=rhs.static_cast_handle<draw_order>()->z();

    if(z1<z2)
      {
        return less_draw_sort_order;
      }
    else if(z1>z2)
      {
        return greater_draw_sort_order;
      }
    else
      {
        return equal_draw_sort_order;
      }
  }
};

class item_type
{
public:
  PlainFamily::TextWidget *m_text;
  ImageFamily::RectWidget *m_image;
  PlainFamily::NodeWidget *m_rotation;
  PlainFamily::NodeWidget *m_translate;
  WRATHImage *m_img_src;

  std::string m_label;
  int m_ID;
  float m_im_z;
  ivec3 m_text_color;
  float m_text_scale;
  WRATHDrawOrder::handle m_force_draw_order_text, m_force_draw_order_image;

  void
  rebuild_text_item(WRATHTextureFont *pfont, 
                    enum WRATHTextItemTypes::text_opacity_t tp,
                    WRATHFontShaderSpecifier *spec)
  {
    if(m_text!=NULL)
      {
        WRATHPhasedDelete(m_text);
      }

    m_text=WRATHNew PlainFamily::TextWidget(m_rotation, tp, spec,
                                                        m_force_draw_order_text);


    WRATHTextDataStream visible_text;
    visible_text.stream() 
      << WRATHText::set_z_position(m_im_z)
      << WRATHText::set_scale(m_text_scale)
      << WRATHText::set_pixel_size(pfont->pixel_size())
      << WRATHText::set_color(m_text_color.x(), m_text_color.y(), m_text_color.z())
      << WRATHText::set_font(pfont)
      << m_label
      << m_ID;
    
    m_text->properties()->add_text(visible_text);
  }
                    

  item_type(void):
    m_text(NULL),
    m_image(NULL),
    m_rotation(NULL),
    m_translate(NULL),
    m_img_src(NULL)
  {}
};

class cmd_line_type:public DemoKernelMaker
{
public:

  command_line_argument_value<int> m_max_transformations;
  command_line_argument_value<bool> m_vs_force_highp, m_fs_force_highp;

  command_line_argument_value<std::string> m_log_GL;
  command_line_argument_value<std::string> m_log_alloc;

  command_line_argument_value<std::string> m_tex_attr_prec, m_tex_varying_vs_prec;
  command_line_argument_value<std::string> m_tex_varying_fs_prec, m_tex_recip_prec; 
  command_line_argument_value<int> m_text_renderer;
  command_line_argument_value<int> m_text_renderer_line_analytic_format;
  command_line_argument_value<bool> m_text_renderer_curve_analytic_format;
  command_line_argument_value<int> m_text_renderer_sub_choice;
  command_line_argument_value<int> m_text_renderer_coverage_min_filter;
  command_line_argument_value<int> m_text_renderer_converage_mag_filter;
  command_line_argument_value<int> m_text_renderer_converage_deepness_slack; 
  command_line_argument_value<int> m_text_renderer_analytic_mipmap_level;
  command_line_argument_value<float> m_mix_font_div_ratio;
  command_line_argument_value<float> m_font_discard_thresh;
  command_line_argument_value<float> m_max_distance_font_generation;
  command_line_argument_value<GLint> m_font_texture_size;
  command_line_argument_value<bool> m_font_texture_force_power2; 

  command_line_argument_value<bool> m_font_lazy_z;


  command_line_argument_value<int> m_atlas_size;
  command_line_argument_value<bool> m_image_use_mipmaps; 
  command_line_argument_value<bool> m_manual_mipmap_generation; 
  command_line_argument_value<std::string> m_image_filename;
  command_line_argument_value<std::string> m_image_filename2;
  command_line_argument_value<std::string> m_font_filename;
  command_line_argument_value<bool> m_use_config_font;
  command_line_argument_value<int> m_font_face_index;
  command_line_argument_value<int> m_font_size;
  command_line_argument_value<uint32_t> m_time_ms;
  command_line_argument_value<bool> m_fast_quit;
  command_line_argument_value<int> m_count;
  command_line_argument_value<bool> m_draw_text, m_draw_images;
  command_line_argument_value<int> m_toggle_visibility;
  command_line_argument_value<float> m_scale_text;
  command_line_argument_value<float> m_item_font_scale_factor;
  command_line_argument_value<bool> m_show_fps_on_items;
  command_line_argument_value<float> m_item_size_x, m_item_size_y;
  command_line_argument_value<int> m_number_per_row;
  command_line_argument_value<float> m_velocity_x, m_velocity_y, m_velocity_rotation;
  command_line_argument_value<std::string> m_text;
  command_line_argument_value<bool> m_rotate;
  command_line_argument_value<int> m_text_red, m_text_blue, m_text_green;
  command_line_argument_value<int> m_bg_red, m_bg_blue, m_bg_green, m_bg_alpha;

  command_line_argument_value<bool> m_time_limit_off;
  command_line_argument_value<float> m_max_zoom_factor;
  command_line_argument_value<float> m_min_zoom_factor;
  command_line_argument_value<bool> m_print_info;

  command_line_argument_value<std::string> m_image_dir;
  command_line_argument_value<bool> m_show_atlases;
  command_line_argument_value<bool> m_use_atlases;

  command_line_argument_value<float> m_z_translate_pre_rotate, m_z_translate_post_rotate;
  command_line_argument_value<bool> m_perspective_on;
  command_line_argument_value<int> m_number_z_perspective_layers;
  command_line_argument_value<float> m_z_perspective_layer_dist;


  command_line_argument_value<bool> m_issue_gl_finish, m_emulate_touch_event;
  command_line_argument_value<bool> m_stress_test_deletion_creation;

  command_line_argument_value<bool> m_force_draw_order;

  cmd_line_type(void):
    DemoKernelMaker(),    
    
    m_max_transformations(100, "max_tr", 
                          "Maximum number of transformation nodes per draw call", 
                          *this),

    m_vs_force_highp(false, "vs_force_highp", 
                     "if true, all variables in vertex shader are highp", *this),
    m_fs_force_highp(false, "fs_force_highp", 
                     "if true, all variables in fragment shader are highp", *this),
    

    m_log_GL("", "log_gl", "If non empty, logs GL commands to the named file", *this),
    m_log_alloc("", "log_alloc", 
                "If non empty, logs allocs and deallocs to the named file", *this),


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


    m_atlas_size(2048, "atlas_size", "Size of texture atlas allowd", *this),
    m_image_use_mipmaps(true, "image_use_mipmaps",
                        "If true, use mipmap filtering for images", *this),
    m_manual_mipmap_generation(false, "manual_mipmaps",
                               "If true and if using mipmaps, will generate "
                               "mipmaps via QImage calls rather than using GL's "
                               "glGenerateMipmap", *this),
    m_image_filename("images/image.png", "image", "Image filename to use for texture", *this),
    m_image_filename2("images/image.png", "image2", "Image filename to use for texture2", *this),
    m_font_filename("ttf/FreeSerif.ttf", "font", "True Type Font to use", *this),
    m_use_config_font(false, "use_config_font", "if true, use Fontconfig to fetch the font", *this),
    m_font_face_index(0, "font_face", "Font Face index", *this),
    m_font_size(64, "font_size", "True Type Font Size", *this),
    m_time_ms(500, "time", "time in ms to run test", *this),
    m_fast_quit(false, "quit", "quit after one draw loop", *this),
    m_count(10, "count", "Number of elements to draw", *this),
    m_draw_text(true, "draw_text", "Draw text on each item", *this),
    m_draw_images(true, "draw_image", "Draw images on each item", *this),
    m_toggle_visibility(0, "vis_toggle_time", 
                        "Visibility toggle time in ms, non-positive indicates no toggle", 
                        *this),
    m_scale_text(1.0f, "scale_text", "Scale font factor for FPS display", *this),
    m_item_font_scale_factor(1.0f, "item_font_scale_factor", 
                             "Font size multipier to use for each item", *this),
    m_show_fps_on_items(true, "extra_text_fps", "Show FPS on extra text items", *this),
    m_item_size_x(100, "item_size_x", "horizontal size in pixels of each item", *this),
    m_item_size_y(100, "item_size_y", "vertical size in pixels of each item", *this),
    m_number_per_row(20, "per_row_count", "Number of items per row", *this),
    m_velocity_x(1.0f, "v_x", "velocity in x direction measured in pixels per second", *this),
    m_velocity_y(1.0f, "v_y", "velocity in y direction measured in pixels per second", *this),
    m_velocity_rotation(10*M_PI, "omega", "angular velocity of items measured in radians/second", *this),
    m_text("WRATH/WRATH Demo", "text", 
           "specify text test string", *this),
    m_rotate(false, "rotatexy", "exchange x with y coordinate", *this),

    m_text_red(0x00, "text_red", "red value for text, [0-255]", *this), 
    m_text_blue(0xFF, "text_blue", "blue value for text, [0-255]", *this), 
    m_text_green(0xFF, "text_green", "green value for text, [0-255]", *this),
    m_bg_red(0xF0, "bg_red", "red value for background, [0-255]", *this), 
    m_bg_blue(0xF0, "bg_blue", "blue value for background, [0-255]", *this), 
    m_bg_green(0xF0, "bg_green", "green value for background, [0-255]", *this),
    m_bg_alpha(0x00, "bg_alpha", "alpha value for background, [0-255]", *this),

    m_time_limit_off(false, "ignore_time", 
                     "If true, then applicaion does not automatically quit after time ms", *this),
    m_max_zoom_factor(4.0f, "max_dyn_zoom", "Maximum zoom factor for dynamic scaled text", *this),
    m_min_zoom_factor(1.0f, "min_dyn_zoom", "Minimum zoom factor for dynamic scaled text", *this),
    m_print_info(false, "print_info", "Print GL/EGL information", *this),

    m_image_dir("", "image_dir", 
                "If non-empty string, use all images from the specified image directory", *this),
    m_show_atlases(false, "show_atlas", 
                   "If true the first N images are entire atlases, where N=#atlases", *this),
    m_use_atlases(true, "use_atlas", "Use texture atlas", *this),
    m_z_translate_pre_rotate(1.0f, "pre_z", "Translation in z before rotation", *this),
    m_z_translate_post_rotate(-1.5f, "post_z", "Translation in z after rotation", *this),
    m_perspective_on(false, "perspective", "demo perspective Transformations", *this),
    m_number_z_perspective_layers(1, "number_z_perspective_layers", 
                                  "number of distinct layers in z for perspective transformations", 
                                  *this),
    m_z_perspective_layer_dist(17.0f, "z_perspective_layer_dist",
                               "distance between distinct z layers", *this),

    m_issue_gl_finish(false, "gl_finish", 
                      "If true calls glFinish at the end of paint()", *this),
    m_emulate_touch_event(false, "emulate_touch",
                          "if true, touch events are emulated by mouse events",
                          *this),
    m_stress_test_deletion_creation(false, "stress_deletion_creation",
                                    "if true, each frame will create and delete a text item",
                                    *this),

    m_force_draw_order(false, "force_draw_order", 
                       "if true, elements are forced to be drawn back to front",
                       *this)
  {}

  virtual
  DemoKernel*
  make_demo(void);

  virtual
  void
  delete_demo(DemoKernel*);
};


class DemoImage:public DemoKernel
{
public:
  DemoImage(cmd_line_type &cmd_line);

  ~DemoImage();

  virtual
  void
  paint(void);

  virtual
  void
  handle_event(FURYEvent::handle);

private:
  const cmd_line_type &cmd_line;
  WRATHTime m_time;

  void
  clean_up(void);

  void
  handle_touch_end(int x, int y);

  void
  update_z_s(void);

  int32_t
  get_time(void)
  {
    return m_time.elapsed();
  }

  WRATHImage*
  add_image(std::map<std::string, WRATHImage*> &R,
            const std::string &pname);

  bool
  recursrive_load_images(const std::string &full_path);

  void
  set_perspetive_matrix(void);

  void
  stress_ui_clip_container_creation_deletion(void);

  WRATHTripleBufferEnabler::handle m_tr;
  enum WRATHTextItemTypes::text_opacity_t m_text_opacity;

  std::vector<WRATHImage*> atlas_list;
  std::set<WRATHTextureChoice::texture_base::handle> atlas_set;
  std::map<std::string, WRATHImage*> all_images;
  std::vector<WRATHImage*> ims;
  WRATHImage *im1, *im2;
  std::string image_dir;
  WRATHTextureFont *pfont;
  WRATHFontShaderSpecifier *text_shader_specifier;
  WRATHLayer *root;
  float4x4 pers_mat;
  std::vector<item_type> items;
  PlainFamily::TextWidget *scaling_text, *fps_text;
  PlainFamily::NodeWidget *draw_at_bottom;
  int frame_count;
  uint32_t start_record_time, running_time, end_record_time;
  uint32_t last_running_time,  simulation_time, delta_time, last_swap_time;
  bool paused;
  WRATHLayer::draw_information stats;
  std::vector<vec3> velocities;
  float fps;
  ivec2 window_size;  
  std::ofstream *gl_log_stream;
  float m_text_ratio;
  bool vis_flag;

  bool use_atlas;

  WRATHLayer *m_ultimate_stresser;
  item_type m_ultimate_stresser_item;

  float m_z_translate_pre_rotate, m_z_translate_post_rotate;
  bool m_perspective_on;
  int m_number_z_perspective_layers;
  float m_z_perspective_layer_dist;
};



int
main(int argc, char **argv)
{
  cmd_line_type cmd_line;
  return cmd_line.main(argc, argv);
}


DemoKernel*
cmd_line_type::
make_demo(void)
{
  return WRATHNew DemoImage(*this);
}

void
cmd_line_type::
delete_demo(DemoKernel *k)
{
  if(k!=NULL)
    {
      WRATHDelete(k);
    }
}


WRATHImage*
DemoImage::
add_image(std::map<std::string, WRATHImage*> &R,
          const std::string &pname)
{

  if(R.find(pname)==R.end())
    {
      WRATHImage *im;
      GLenum min_filter;
      
      if(cmd_line.m_image_use_mipmaps.m_value)
        {
          min_filter=GL_LINEAR_MIPMAP_NEAREST;
        }
      else
        {
          min_filter=GL_LINEAR;
        }

      WRATHImage::ImageFormat fmt;

      fmt
        .internal_format(GL_RGBA)
        .pixel_data_format(GL_RGBA)
        .pixel_type(GL_UNSIGNED_BYTE)
        .magnification_filter(GL_LINEAR)
        .minification_filter(min_filter)
        .automatic_mipmap_generation(!cmd_line.m_manual_mipmap_generation.m_value);

      im=WRATHDemo::fetch_image(pname, fmt, !use_atlas);

      if(im!=NULL)
        {
          WRATHTextureChoice::texture_base::handle atlas;

          R[pname]=im;
          atlas=im->texture_binder();
          if(atlas_set.find(atlas)==atlas_set.end())
            {
              std::ostringstream im_name;
              WRATHImage *atlas_image;

              //HACK: Bind texture to make the texture live
              atlas->bind_texture(GL_TEXTURE0);

              im_name << "Atlas#" << atlas_set.size();
              atlas_image=WRATHNew WRATHImage(im_name.str(),
                                              im->image_format(0),
                                              im->texture_atlas_glname(),
                                              ivec2(0,0),
                                              im->atlas_size());
                                              

              atlas_list.push_back(atlas_image);
              atlas_set.insert(atlas);
            }
        }
      return im;
    }
  return NULL;
}

bool
DemoImage::
recursrive_load_images(const std::string &full_path)
{
  DIR *ptr;
      
  ptr=::opendir(full_path.c_str());
  if(ptr==NULL)
    {
      return false;
    }
  else
    {
      for(struct dirent *currentEntry=::readdir(ptr);
          currentEntry!=NULL;currentEntry=::readdir(ptr))
        {
          if(!std::strcmp(currentEntry->d_name,".") or !std::strcmp(currentEntry->d_name,".."))
             continue;

          if(!recursrive_load_images(full_path+currentEntry->d_name+"/"))
            {

              WRATHImage *return_image;
              
             
              
              return_image=add_image(all_images, 
                                     full_path+std::string(currentEntry->d_name));
              
              if(return_image!=NULL)
                {
                  ims.push_back(return_image);
                }
            }
        }
      ::closedir(ptr);
      return true;
    }
}


DemoImage::
DemoImage(cmd_line_type &pcmd_line):
  DemoKernel(&pcmd_line),
  cmd_line(pcmd_line),
  im1(NULL), im2(NULL),
  image_dir(cmd_line.m_image_dir.m_value),
  pfont(NULL), 
  text_shader_specifier(NULL),
  root(NULL),
  items(std::max(0, cmd_line.m_count.m_value)),
  scaling_text(NULL), fps_text(NULL),
  draw_at_bottom(NULL),
  frame_count(0),
  start_record_time(0), running_time(0), end_record_time(0),
  last_running_time(0),  simulation_time(0), delta_time(0),
  last_swap_time(0),
  paused(false),
  velocities(items.size()),
  gl_log_stream(NULL),
  m_text_ratio(1.0f),
  vis_flag(true),

  m_ultimate_stresser(NULL),

  m_z_translate_pre_rotate(cmd_line.m_z_translate_pre_rotate.m_value), 
  m_z_translate_post_rotate(cmd_line.m_z_translate_post_rotate.m_value),
  m_perspective_on(cmd_line.m_perspective_on.m_value),
  m_number_z_perspective_layers(cmd_line.m_number_z_perspective_layers.m_value),
  m_z_perspective_layer_dist(cmd_line.m_z_perspective_layer_dist.m_value*0.5f)
{
  use_atlas=cmd_line.m_use_atlases.m_value;

  m_text_opacity=(cmd_line.m_font_lazy_z.m_value)?
    WRATHTextItemTypes::text_transparent:
    WRATHTextItemTypes::text_opaque;

  WRATHTextureFont* (*fetcher)(int psize, 
                               const std::string &pfilename, 
                               int face_index);

  if(!cmd_line.m_log_GL.m_value.empty())
    {
      gl_log_stream=WRATHNew std::ofstream(cmd_line.m_log_GL.m_value.c_str());
      ngl_LogStream(gl_log_stream);
      ngl_log_gl_commands(true);
    }


  m_tr=WRATHNew WRATHTripleBufferEnabler();

  window_size=ivec2(width(), height());

  if(cmd_line.m_print_info.m_value)
    {
      std::cout << "GL extensions:\n" 
                << glGetString(GL_EXTENSIONS)
                << "\n";
      //boilerplate::print_gl_information(std::cout);
      //boilerplate::print_egl_information(std::cout);
    }
  WRATHImage::texture_atlas_dimension(cmd_line.m_atlas_size.m_value);

  im1=add_image(all_images, 
                cmd_line.m_image_filename.m_value);
  im2=add_image(all_images,
                cmd_line.m_image_filename2.m_value);

  if(im1!=NULL)
    {
      ims.push_back(im1);
    }

  if(im2!=NULL)
    {
      ims.push_back(im2);
    }

  if(!image_dir.empty())
    {
      if(*image_dir.rbegin()!='/')
        {
          image_dir.push_back('/');
        }
      recursrive_load_images(image_dir);
    }

  std::cout << "\n" << ims.size() 
            << " images created " << atlas_set.size() << " atlases\n";

  /*
    command to specify maximum number of nodes per draw call...
   */
  NodePacker::max_node_count()=cmd_line.m_max_transformations.m_value;
  
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
  
  /*
    set fetcher:
   */ 
#define SET_LOAD_FONT_MIX(P, G) \
   do {\
     fetcher=WRATHMixFontTypes<P>::G::fetch_font;\
     WRATHMixFontTypes<P>::G::default_size_divider(default_mix_size_divider);\
} while(0)

#define SET_LOAD_FONT(P) \
   do {\
     fetcher=P::fetch_font;\
} while(0)
  
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

  text_shader_specifier=WRATHNew WRATHFontShaderSpecifier("text drawer",
                                                          WRATHFontShaderSpecifier::default_vertex_shader(),
                                                          WRATHFontShaderSpecifier::default_aa_fragment_shader());
  text_shader_specifier->font_discard_thresh(cmd_line.m_font_discard_thresh.m_value);

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

  

  WRATHTextureFontFreeType_Distance::texture_creation_size(cmd_line.m_font_texture_size.m_value);
  WRATHTextureFontFreeType_Distance::max_L1_distance(cmd_line.m_max_distance_font_generation.m_value);
  WRATHTextureFontFreeType_Distance::force_power2_texture(cmd_line.m_font_texture_force_power2.m_value);
  WRATHTextureFontFreeType_Distance::fill_rule(WRATHTextureFontFreeType_Distance::non_zero_winding_rule);
  
  WRATHTextureFontFreeType_Analytic::texture_creation_size(cmd_line.m_font_texture_size.m_value);

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
  if(cmd_line.m_use_config_font.m_value)
    {
      spec=WRATHFontFetch::font_handle(WRATHFontFetch::FontProperties()
				       .family_name(cmd_line.m_font_filename.m_value));
    }
  else
    {
      spec=WRATHFontFetch::font_handle(cmd_line.m_font_filename.m_value,
				       cmd_line.m_font_face_index.m_value);
    }

  if(!spec.valid())
    {
      spec=WRATHFontFetch::default_font();
    }
  pfont=fetcher(cmd_line.m_font_size.m_value, 
		spec->name(), spec->face_index());
    

 
  

  root=WRATHNew WRATHLayer(m_tr,
                           WRATHLayerClipDrawer::handle(),
                           WRATHNew draw_order_comparer());

  //set perspective matrix of root:
  if(cmd_line.m_rotate.m_value)
    {
      float4x4 Rxy(vec3(0,0,0), vec3(0,-1,0), vec3(1,0,0), vec3(0,0,1));
      float_orthogonal_projection_params proj_params(0,window_size.y(),
                                                     window_size.x(), 0);


      pers_mat.orthogonal_projection_matrix(proj_params);

      //now we want to rotate x,y normalized device co-ordinates.
      pers_mat=Rxy*pers_mat;
      std::swap(window_size.x(), window_size.y());
    }
  else
    {
      float_orthogonal_projection_params proj_params(0, window_size.x(),
                                                     window_size.y(), 0);


      pers_mat.orthogonal_projection_matrix(proj_params);
      
    }
  root->simulation_matrix(WRATHLayer::modelview_matrix, float4x4());
  root->simulation_composition_mode(WRATHLayer::modelview_matrix, WRATHLayer::use_this_matrix);
  root->simulation_matrix(WRATHLayer::projection_matrix, pers_mat);
  root->simulation_composition_mode(WRATHLayer::projection_matrix, WRATHLayer::use_this_matrix);

  
  float delta_x, delta_y;
  int row_count;

  row_count=std::max( size_t(1), items.size()/cmd_line.m_number_per_row.m_value);
  delta_x=static_cast<float>(window_size.x())/static_cast<float>(cmd_line.m_number_per_row.m_value);
  delta_y=static_cast<float>(window_size.y())/static_cast<float>(row_count);

  WRATHDefaultRectAttributePacker::Rect::handle rect;
  vec2 target_size(cmd_line.m_item_size_x.m_value,
                   cmd_line.m_item_size_y.m_value);
  rect=WRATHNew WRATHDefaultRectAttributePacker::Rect();


  for(int i=0, row=1, col=0; i<cmd_line.m_count.m_value; ++i, ++col)
    {
      int image_index;
      float im_z(-1.0f);

      if(m_perspective_on and m_number_z_perspective_layers>0)
        {
          int v(i%m_number_z_perspective_layers);

          if(i&1)
            {
              v-=2*m_number_z_perspective_layers;
            }
          im_z=m_z_perspective_layer_dist*static_cast<float>(v);
        }
      
      if(cmd_line.m_force_draw_order.m_value)
        {
          items[i].m_force_draw_order_image=WRATHNew draw_order(-2*i);
          items[i].m_force_draw_order_text=WRATHNew draw_order(-2*i-1);
        }
      
      
      image_index=i%(ims.size());
      
      items[i].m_ID=i;
      items[i].m_im_z=im_z;
      items[i].m_text_color=ivec3(cmd_line.m_text_red.m_value,
                                  cmd_line.m_text_green.m_value,
                                  cmd_line.m_text_blue.m_value);
      items[i].m_text_scale=m_text_ratio*cmd_line.m_item_font_scale_factor.m_value;

      items[i].m_translate=WRATHNew PlainFamily::NodeWidget(root);
      items[i].m_translate->position(vec2( static_cast<float>(col)*delta_x + delta_x/2.0, 
                                           static_cast<float>(row)*delta_y - delta_y/2.0));
      
      if(col>=cmd_line.m_number_per_row.m_value)
        {
          col=-1;
          ++row;
        }

      items[i].m_rotation=WRATHNew PlainFamily::NodeWidget(items[i].m_translate);

      if(cmd_line.m_draw_images.m_value)
        {
          WRATHBrush brush;
          if(i<(int)atlas_list.size() and cmd_line.m_show_atlases.m_value)
            {
              brush.m_image=atlas_list[i];
            }
          else
            {
              brush.m_image=ims[image_index];
            }
          items[i].m_img_src=brush.m_image;

          WRATH2DRigidTransformation sf, tr;
          vec2 scale_factor, image_size;
          image_size=vec2(brush.m_image->size());
          scale_factor=target_size/vec2(image_size);
          sf.scale(std::max(scale_factor.x(), scale_factor.y()));
          tr.translation(-0.5f*image_size);

          PlainFamily::NodeWidget *offset_node;
          offset_node=WRATHNew PlainFamily::NodeWidget(items[i].m_rotation);
          offset_node->transformation(sf*tr);

          ImageFamily::RectWidget::Node::set_shader_brush(brush);
          WRATHRectItemTypes::Drawer image_drawer(brush);
          image_drawer.m_draw_passes[0].m_force_draw_order=items[i].m_force_draw_order_image;
          items[i].m_image=WRATHNew ImageFamily::RectWidget(offset_node, image_drawer);
          items[i].m_image->set_from_brush(brush);
          items[i].m_image->z_order(-2*i);
          rect->m_width_height=image_size;
          items[i].m_image->properties()->set_parameters(rect);
        }

      if(pfont!=NULL and cmd_line.m_draw_text.m_value)
        {
          if(i<(int)atlas_list.size() and cmd_line.m_show_atlases.m_value)
            {
              std::ostringstream str;
              str << "Atlas#" << i;
              items[i].m_label=str.str();
            }
          else
            {
              const std::string &raw(ims[image_index]->resource_name());
              std::string::size_type iter;

              iter=raw.find_last_of('/');
              if(iter!=std::string::npos)
                {
                  items[i].m_label=raw.substr(iter+1);
                }
              else
                {
                  items[i].m_label=raw;
                }
            }

          items[i].rebuild_text_item(pfont, m_text_opacity, text_shader_specifier);
          items[i].m_text->z_order(-2*i-1);
        }
    }

  draw_at_bottom=WRATHNew PlainFamily::NodeWidget(root);
  draw_at_bottom->position( vec2(0.0f, window_size.y()) );  

  if(pfont!=NULL)
    {   
      
      scaling_text=
        WRATHNew PlainFamily::TextWidget(draw_at_bottom, m_text_opacity, text_shader_specifier);
      scaling_text->z_order(std::numeric_limits<int16_t>::min()+1); 
      

      WRATHTextDataStream visible_text;
      visible_text.stream()
        << WRATHText::set_scale(m_text_ratio)
        << WRATHText::set_font(pfont)
        << WRATHText::set_color(cmd_line.m_text_red.m_value,
                              cmd_line.m_text_green.m_value,
                              cmd_line.m_text_blue.m_value)
        << cmd_line.m_text.m_value;
      
      visible_text.format(WRATHColumnFormatter::LayoutSpecification().add_leading_eol(false) );

      scaling_text->properties()->clear();
      scaling_text->properties()->add_text(visible_text);

      
      fps_text
        =WRATHNew PlainFamily::TextWidget(root, m_text_opacity, text_shader_specifier);

      WRATHTextDataStream fps_message;
      fps_message.stream() 
        << WRATHText::set_scale(m_text_ratio*cmd_line.m_scale_text.m_value)
        << WRATHText::set_font(pfont) 
        << "\n" << cmd_line.m_text.m_value;

      
      fps_message.format(WRATHColumnFormatter::LayoutSpecification().add_leading_eol(false) );

      fps_text->properties()->clear();
      fps_text->properties()->add_text(fps_message);
    }

  for(unsigned int i=0; i<velocities.size(); ++i)
    {
      float mul[2]={-1.0f, 1.0f};

      velocities[i].x()=cmd_line.m_velocity_x.m_value*(1.0f+0.5f*cosf(static_cast<float>(i)))*0.0001;
      velocities[i].y()=cmd_line.m_velocity_y.m_value*(1.0f+0.5f*sinf(static_cast<float>(i+1)))*0.0001;
      velocities[i].z()=cmd_line.m_velocity_rotation.m_value*(1.0f+0.5f*sinf(static_cast<float>(i+1)))*0.0001;

      velocities[i]*=mul[i&1];
    }
  glClearColor( static_cast<float>(cmd_line.m_bg_red.m_value)/255.0f,
                static_cast<float>(cmd_line.m_bg_green.m_value)/255.0f,
                static_cast<float>(cmd_line.m_bg_blue.m_value)/255.0f,
                static_cast<float>(cmd_line.m_bg_alpha.m_value)/255.0f);
}

void
DemoImage::
paint(void)
{
  if(window_size.x()!=width() 
     or window_size.y()!=height())
    {
      window_size=ivec2(width(), height());
      glViewport(0, 0, window_size.x(), window_size.y());
      set_perspetive_matrix();      
    }
  
  if(!cmd_line.m_fast_quit.m_value and 
     (cmd_line.m_time_limit_off.m_value
      or running_time-start_record_time<cmd_line.m_time_ms.m_value))
    {
      WRATHLayer::draw_information localstats;

      last_running_time=running_time;
      running_time=get_time();
      delta_time=running_time-last_running_time;

      if(!paused)
        {
          simulation_time+=delta_time;
        }

      if(frame_count<=5)
        {
          start_record_time=running_time;
        }

      if(delta_time!=0)
        {
          fps=static_cast<int>(1000.0f/static_cast<float>(delta_time));
        }
      else
        {
          fps=1000;
        }

      m_tr->signal_complete_simulation_frame();
      m_tr->signal_begin_presentation_frame();

      
      //drawing it!
      root->clear_and_draw(&localstats);

      bool change_visibility;
      if(last_swap_time + delta_time > (uint32_t)cmd_line.m_toggle_visibility.m_value
         and cmd_line.m_toggle_visibility.m_value>0)
        {
          change_visibility=true;
          vis_flag=!vis_flag;
          last_swap_time=0;
        }
      else
        {
          change_visibility=false;
          last_swap_time+=delta_time;
        }

      //update transformations for animation:
      for(int i=0; !paused and i<cmd_line.m_count.m_value; ++i)
        {
          vec2 tr;
          vec2 delta_tr(velocities[i].x(), velocities[i].y());
          std::complex<float> new_rot;

          tr=items[i].m_translate->global_values().m_transformation.translation();
          delta_tr*=static_cast<float>( std::max(1u,delta_time) );

          sincosf(velocities[i].z()*static_cast<float>(std::max(1u,delta_time)), 
                  &new_rot.imag(), &new_rot.real());

          if(tr.x()+delta_tr.x()>window_size.x() and velocities[i].x()>0.0f)
            {
              velocities[i].x()*=-1.0f;
            }
          else if(tr.x()+delta_tr.x()<0.0f and velocities[i].x()<0.0f)
            {
              velocities[i].x()*=-1.0f;
            }

          if(tr.y()+delta_tr.y()>window_size.y() and velocities[i].y()>0.0f)
            {
              velocities[i].y()*=-1.0f;
            }
          else if(tr.y()+delta_tr.y()<0.0f and velocities[i].y()<0.0f)
            {
              velocities[i].y()*=-1.0f;
            }

          if(change_visibility)
            {
              if(items[i].m_text!=NULL)
                {
                  items[i].m_text->visible((i%2==0) xor (vis_flag));
                }

              if(items[i].m_image!=NULL)
                {
                  items[i].m_image->visible((i%2==1) xor (!vis_flag));
                }
            }
          

          items[i].m_translate->position(tr+delta_tr);
          items[i].m_rotation->rotation( items[i].m_rotation->rotation()*new_rot);
        }

      float sc;
      float cc;
      float mid=(cmd_line.m_max_zoom_factor.m_value+cmd_line.m_min_zoom_factor.m_value)*0.5f;
      float amp=(cmd_line.m_max_zoom_factor.m_value-cmd_line.m_min_zoom_factor.m_value)*0.5f;

      cc=cosf( static_cast<float>(simulation_time)/1000.0f);
      sc=mid + amp*cc;  

      if(fps_text!=NULL)
        {
          WRATHTextDataStream fps_message;      
          fps_message.stream() 
            << WRATHText::set_scale(m_text_ratio*cmd_line.m_scale_text.m_value)
            << WRATHText::set_font(pfont)
            << WRATHText::set_color(cmd_line.m_text_red.m_value,
                                  cmd_line.m_text_green.m_value,
                                  cmd_line.m_text_blue.m_value)
            << "\nFPS:" << std::setw(4) << fps
            << " (" << std::setw(3) << delta_time << " ms)"
            << "\nZ=" << std::setw(6) << sc;

          fps_message.format(WRATHColumnFormatter::LayoutSpecification().add_leading_eol(false) );

          fps_text->properties()->clear();
          fps_text->properties()->add_text(fps_message);
        }
      
      if(m_perspective_on)
        {
          float4x4 modelview;
          float r, rsin, rcos;
          float xc, yc;
          
          r=static_cast<float>(simulation_time)/1000.0f;
          sincosf(r, &rsin, &rcos);

          xc=width()/2.0f;
          yc=height()/2.0f;

          modelview=
            float4x4( vec3(xc, yc, m_z_translate_post_rotate),
                      vec3(1, 0, 0), 
                      vec3(0, 1, 0),
                      vec3(0, 0, 1) )
            *
            float4x4( vec3(0,0,0),
                      vec3(1,0,0), 
                      vec3(0, rcos, rsin),
                      vec3(0,-rsin, rcos) )
            * 
            float4x4( vec3(-xc, -yc, m_z_translate_pre_rotate),
                      vec3(1, 0, 0), 
                      vec3(0, 1, 0),
                      vec3(0, 0, 1) );

          root->simulation_matrix(WRATHLayer::modelview_matrix, modelview);
        }
      
      scaling_text->scaling_factor(sc);
      ++frame_count;
      stats=localstats;


      /*
        Test excessive creation and destruction of items:
       */
      int rebuild_index;
      if(pfont!=NULL and 
         !items.empty() and 
         cmd_line.m_draw_text.m_value and
         cmd_line.m_stress_test_deletion_creation.m_value)
        {
          rebuild_index=frame_count%items.size();
          if(items[rebuild_index].m_text!=NULL)
            {
              items[rebuild_index].rebuild_text_item(pfont, 
                                                     m_text_opacity,
                                                     text_shader_specifier);
            }
          stress_ui_clip_container_creation_deletion();
        }

      update_widget();
    }
  else if(frame_count>1)
    {
      end_demo();
    }

  if(cmd_line.m_issue_gl_finish.m_value)
    {
      glFinish();
    }
    
}


DemoImage::
~DemoImage()
{
  clean_up();
}

void
DemoImage::
clean_up(void)
{
  if(root==NULL)
    {
      return;
    }

  if(gl_log_stream!=NULL)
    {
      WRATHDelete(gl_log_stream);
      ngl_LogStream(&std::cerr);
      ngl_log_gl_commands(false);
    }

  end_record_time=get_time();
  std::cout << "\n" 
            << frame_count << " frames in "
            << (end_record_time-start_record_time)
            << " ms\n"
            << "Res=" << window_size << "\nN=" 
            << cmd_line.m_count.m_value << " ["
            << static_cast<float>(end_record_time-start_record_time)/static_cast<float>(frame_count)
            << " ms per frame, "
            << 1000.0f*static_cast<float>(frame_count)/static_cast<float>(end_record_time-start_record_time)
            << " FPS]\n"
            << "stats per frame:"
            << "\n\t m_draw_count=" << stats.m_draw_count
            << "\n\t m_program_count=" << stats.m_program_count
            << "\n\t m_texture_choice_count=" << stats.m_texture_choice_count
            << "\n\t m_gl_state_change_count=" << stats.m_gl_state_change_count
            << "\n\t m_attribute_change_count=" << stats.m_attribute_change_count
            << "\n\t m_buffer_object_bind_count=" << stats.m_buffer_object_bind_count
            << "\n\t m_clip_container_count=" << stats.m_layer_count
            << "\n\t atlas size=" << WRATHImage::texture_atlas_dimension()
            << "\n\n";
  WRATHPhasedDelete(root);
  root=NULL;
  

  atlas_list.clear();
  atlas_set.clear();

  WRATHResourceManagerBase::clear_all_resource_managers();

  m_tr->purge_cleanup();
  m_tr=NULL;
}


void
DemoImage::
update_z_s(void)
{
  for(int i=0, endi=items.size(); i<endi; ++i)
    {
      float im_z(-1.0f);

      if(m_perspective_on and m_number_z_perspective_layers>0)
        {
          int v(i%m_number_z_perspective_layers);

          if(i&1)
            {
              v-=2*m_number_z_perspective_layers;
            }
          im_z=m_z_perspective_layer_dist*static_cast<float>(v);
        }

      if(items[i].m_image!=NULL)
        {
          WRATHDefaultRectAttributePacker::Rect::handle rect;
          rect=WRATHNew WRATHDefaultRectAttributePacker::Rect(items[i].m_img_src->size().x(),
                                                              items[i].m_img_src->size().y(),
                                                              im_z);
          items[i].m_image->properties()->set_parameters(rect);
        }

      
    }

  if(!m_perspective_on)
    {
      root->simulation_matrix(WRATHLayer::modelview_matrix, float4x4());
    }
}

void
DemoImage::
set_perspetive_matrix(void)
{
 
  //set perspective matrix of root:
  if(cmd_line.m_rotate.m_value)
    {
      float4x4 Rxy(vec3(0,0,0), vec3(0,-1,0), vec3(1,0,0), vec3(0,0,1));
      float_orthogonal_projection_params proj_params(0,window_size.y(),
                                                     window_size.x(), 0);


      pers_mat.orthogonal_projection_matrix(proj_params);
      
      //now we want to rotate x,y normalized device co-ordinates.
      pers_mat=Rxy*pers_mat;
      std::swap(window_size.x(), window_size.y());
    }
  else
    {
      float_orthogonal_projection_params proj_params(0, window_size.x(),
                                                     window_size.y(), 0);
      
      
      pers_mat.orthogonal_projection_matrix(proj_params);
      
    }
  root->simulation_matrix(WRATHLayer::projection_matrix, pers_mat);
}

void
DemoImage::
handle_touch_end(int x, int y)
{
  /*
    Bottom right quits
    top right toggles perspective
    all others pause
   */
  int xthresh0(width()/3), xthresh1(2*xthresh0);
  int ythresh0(height()/3), ythresh1(2*ythresh0);

  if(x>xthresh1 and y>ythresh1)
    {
      end_demo();
    }
  else if(x>xthresh1 and y<ythresh0)
    {
      m_perspective_on=!m_perspective_on;
      update_z_s();
    }
  else
    {
      paused=!paused;
    }
}
 
void
DemoImage::
handle_event(FURYEvent::handle ev)
{
  switch(ev->type())
    {
    case FURYEvent::TouchUp:
      {
        FURYTouchEvent::handle tev(ev.static_cast_handle<FURYTouchEvent>());
        
        handle_touch_end(tev->position().x(), tev->position().y());
        tev->accept();
      }
      break;

    case FURYEvent::MouseButtonUp:
      if(cmd_line.m_emulate_touch_event.m_value)
        {
          FURYMouseButtonEvent::handle me(ev.static_cast_handle<FURYMouseButtonEvent>());
          handle_touch_end(me->pt().x(), me->pt().y());
          ev->accept();        
        }
      break;

    case FURYEvent::KeyUp:
      {
        FURYKeyEvent::handle qe(ev.static_cast_handle<FURYKeyEvent>());
        switch(qe->key().m_value)
          {
          case FURYKey_Space:
            paused=!paused;
            break;
          case FURYKey_P:
            m_perspective_on=!m_perspective_on;
            update_z_s();
            break;

          default:
            end_demo();
          }
      }
      break;

    default:
      break;
    }
  ev->accept();


 
}


void
DemoImage::
stress_ui_clip_container_creation_deletion(void)
{
  if(m_ultimate_stresser==NULL)
    {
      m_ultimate_stresser_item.m_ID=-1;
      m_ultimate_stresser_item.m_im_z=-1.0f;
      m_ultimate_stresser_item.m_label="Stresser Item";
      m_ultimate_stresser_item.m_text_color=ivec3(cmd_line.m_text_red.m_value,
                                                  cmd_line.m_text_green.m_value,
                                                  cmd_line.m_text_blue.m_value);
      m_ultimate_stresser_item.m_text_scale=m_text_ratio*cmd_line.m_item_font_scale_factor.m_value;
    }
  else
    {
      //WRATHPhasedDelete(m_ultimate_stresser_item.m_text);
      m_ultimate_stresser_item.m_text=NULL;
      WRATHPhasedDelete(m_ultimate_stresser);
    }

  m_ultimate_stresser=WRATHNew WRATHLayer(root);

  /**/
  WRATHLayer *q;
  PlainFamily::TextWidget *p;
  q=WRATHNew WRATHLayer(m_ultimate_stresser);
  
  /**/
  p=WRATHNew PlainFamily::TextWidget(q, m_text_opacity);
  WRATHTextDataStream ostr;

  ostr.stream() << WRATHText::set_font(pfont)
                << "Bonus WHITE Deletion@" << p->properties();
  p->properties()->add_text(ostr);
  /**/

  m_ultimate_stresser_item.m_translate=
    WRATHNew PlainFamily::NodeWidget(m_ultimate_stresser);
  m_ultimate_stresser_item.m_rotation=
    WRATHNew PlainFamily::NodeWidget(m_ultimate_stresser_item.m_translate);
  
  m_ultimate_stresser_item.rebuild_text_item(pfont, m_text_opacity, text_shader_specifier);
  m_ultimate_stresser_item.m_translate->position( vec2(window_size.x()/4, window_size.y()/2) );
}
