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
#include <png.h>

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
#include "wrath_demo_image_support.hpp"

/*!
\details
This example demonstrates the basic usage of the \ref Text
Module API for the typical "hello world" example.
 */

namespace bmpWriter
{
  template<typename T>
  void
  sfwrite(const T &value, std::ofstream &file)
  {
    file.write(reinterpret_cast<const char*>(&value), sizeof(T));
  }

  class FileHeader
  {
  public:
    FileHeader(int w, int h):
      m_filetype(19778),
      m_filesize(54+4*w*h),
      m_reserved(0),
      m_offsetToData(54)
    {}

    uint16_t m_filetype; //value should be 19778
    uint32_t m_filesize;
    uint32_t m_reserved; //should be 0
    uint32_t m_offsetToData; //offset to image data!    

    void write(std::ofstream &f)
    {
      sfwrite(m_filetype, f);
      sfwrite(m_filesize, f);
      sfwrite(m_reserved, f);
      sfwrite(m_offsetToData, f);
    }
  };

  class InfoHeader
  {
  public:    
    InfoHeader(int w, int h):
      m_headerSize(40),
      m_width(w),
      m_height(h),
      m_numPlanes(1),
      m_bpp(32),
      m_compressionType(0),
      m_imageSize(0),
      m_XpixelsPerMeter(0),
      m_YpixelsPerMeter(0),
      m_numberOfColorsUsed(0),
      m_numberOfImportantColors(0)
    {}

    uint32_t m_headerSize; //should be 40
    uint32_t m_width;
    uint32_t m_height;
    uint16_t m_numPlanes; //should be 1
    uint16_t m_bpp; 
    uint32_t m_compressionType;
    uint32_t m_imageSize; //if image is not compressed make this 0
    uint32_t m_XpixelsPerMeter;
    uint32_t m_YpixelsPerMeter;
    uint32_t m_numberOfColorsUsed;
    uint32_t m_numberOfImportantColors; //0=All

    void write(std::ofstream &f)
    {
       sfwrite(m_headerSize, f);
       sfwrite(m_width, f);
       sfwrite(m_height, f);
       sfwrite(m_numPlanes, f);
       sfwrite(m_bpp, f);
       sfwrite(m_compressionType, f);
       sfwrite(m_imageSize, f);
       sfwrite(m_XpixelsPerMeter, f);
       sfwrite(m_YpixelsPerMeter, f);
       sfwrite(m_numberOfColorsUsed, f);
       sfwrite(m_numberOfImportantColors, f);
    }
  };
  
  void write_bmp(int w, int h, std::vector<vecN<uint8_t,4> > &pixels, std::ofstream &f)
  {
    FileHeader file_header(w,h);
    InfoHeader info_header(w,h);
    
    file_header.write(f);
    info_header.write(f);
    for(int i=0, endi=w*h; i<endi; ++i)
      {
        /*
          bmp format is bgra,
          pixel format is rgba
         */
        sfwrite(pixels[i][2],f);
        sfwrite(pixels[i][1],f);
        sfwrite(pixels[i][0],f);
        sfwrite(pixels[i][3],f);
      }

  }
}

static
void 
save_png(int w, int h, std::vector<vecN<uint8_t,4> > &pixels, FILE *file)
{
  png_structp png_ptr;
  png_infop info_ptr;
  std::vector<vecN<uint8_t,4> > framebuffer(w*h);
  if(file==NULL)
    {
      return;
    }

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); 
  if (png_ptr==NULL) 
    {
      return;
    }
  info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr==NULL)
    {
      return;
    }
  
  if (setjmp(png_jmpbuf(png_ptr)))
    {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return;
    }
  
  png_init_io(png_ptr, file);
  png_set_IHDR(png_ptr, info_ptr, w, h, 8, 
               PNG_COLOR_TYPE_RGBA,
               PNG_INTERLACE_NONE, 
               PNG_COMPRESSION_TYPE_DEFAULT, 
               PNG_FILTER_TYPE_DEFAULT);
  
  png_write_info(png_ptr, info_ptr);
  
  for(int y=0; y<h; ++y) 
    {     
      png_write_row(png_ptr, &pixels[(h-1-y)*w][0]);
    }
  png_write_end(png_ptr, NULL);
  
  png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
}

class cmd_line_type:public DemoKernelMaker
{
public:
  command_line_argument_value<int> m_virtual_height, m_virtual_width, m_layer_count;
  command_line_argument_value<bool> m_gradient, m_blend, m_disable_depth_test, m_show_ms;
  command_line_argument_value<std::string> m_image;
  command_line_argument_value<unsigned int> m_num_frames;
  command_line_argument_value<std::string> m_record_frame;
  command_line_argument_value<bool> m_save_png;

  cmd_line_type(void):
    m_virtual_height(128, "virtual_height", "Virtual height to which to scale display, negative values mean no scaling", *this),
    m_virtual_width(256, "virtual_width", "Virtual width to which to scale display", *this),
    m_layer_count(100, "layer_count", "# of full screen blends underneath text", *this),
    m_gradient(true, "gradient", "if true, layers are painted with a radial gradient", *this),
    m_blend(true, "blend", "if true, layers are blended", *this),
    m_disable_depth_test(false, "disable_depth_test", "if true layers are drawn with depth test and depth writes off", *this),
    m_show_ms(true, "show_ms", "if true show ms to display frame", *this),
    m_image("", "image", "if a valid image use image in addition to gradient", *this),
    m_num_frames(0, "num_frames", "if non-zero exit, after given number of frames", *this),
    m_record_frame("", "record_frame", "if non-empty record frames to files prefixed with value", *this),
    m_save_png(true, "save_png", "if true save frames as png, if false save as bmp", *this)
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

  void 
  save_framebuffer(const std::string &filename);

  void 
  save_framebuffer_png(const std::string &filename);

  void 
  save_framebuffer_bmp(const std::string &filename);

  typedef WRATHLayerTranslateFamilySet FamilySet;
  typedef FamilySet::PlainFamily::TextWidget TextWidget;
  typedef FamilySet::ColorRadialGradientSimpleXSimpleYImageFamily::RectWidget RectWidget;

  WRATHTripleBufferEnabler::handle m_tr;
  WRATHLayer *m_layer;
  WRATHLayer *m_child_layer;
  
  unsigned int m_frame;
  WRATHTime m_time, m_total_time;
  unsigned int m_virtual_height;

  WRATHGradient *m_gradient;
  WRATHImage *m_image;

  std::vector<RectWidget*> m_rects;
  TextWidget *m_text_widget;
  bool m_show_ms;
  unsigned int m_num_frames;
  std::string m_record_frame;
  bool m_save_png;
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
  m_text_widget(0),
  m_show_ms(cmd_line->m_show_ms.m_value),
  m_num_frames(cmd_line->m_num_frames.m_value),
  m_record_frame(cmd_line->m_record_frame.m_value),
  m_save_png(cmd_line->m_save_png.m_value)
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
  m_text_widget=WRATHNew TextWidget(m_layer, WRATHTextItemTypes::text_transparent);
  m_text_widget->z_order(-1);

  /*
    add the text to the text widget
   */
  m_text_widget->position(vec2(0.0f, 0.0f));
  
  WRATHDrawType draw_type;
  if(cmd_line->m_blend.m_value)
    {
      if(cmd_line->m_disable_depth_test.m_value)
        {
          draw_type=WRATHDrawType::overdraw_transparent_pass();
        }
      else
        {
          draw_type=WRATHDrawType::transparent_pass();
        }
    }
  else
    {
      if(cmd_line->m_disable_depth_test.m_value)
        {
          draw_type=WRATHDrawType::overdraw_opaque_pass();
        }
      else
        {
          draw_type=WRATHDrawType::opaque_pass();
        }
    }
  
  if(cmd_line->m_gradient.m_value)
    {
      m_gradient=WRATHNew WRATHGradient("my little gradient");
      m_gradient->set_color(0.00f, WRATHGradient::color(1.0f, 0.0f, 0.0f, 1.0f));
      m_gradient->set_color(0.25f, WRATHGradient::color(0.0f, 1.0f, 0.0f, 1.0f));
      m_gradient->set_color(0.50f, WRATHGradient::color(0.0f, 0.0f, 1.0f, 1.0f));
      m_gradient->set_color(0.75f, WRATHGradient::color(1.0f, 1.0f, 1.0f, 1.0f));
    }
  else
    {
      m_gradient=NULL;
    }

  m_image=WRATHDemo::fetch_image(cmd_line->m_image.m_value,
                                 WRATHImage::ImageFormat()
                                 .internal_format(GL_RGBA)
                                 .pixel_data_format(GL_RGBA)
                                 .pixel_type(GL_UNSIGNED_BYTE)
                                 .magnification_filter(GL_LINEAR)
                                 .minification_filter(GL_LINEAR),
                                 false,
                                 WRATHDemo::dont_flip_y);

  //create the brush, the node type specifies the shader
  WRATHBrush brush(type_tag<RectWidget::Node>(), m_gradient, m_image);

  //create the drawer from the brush
  RectWidget::Drawer drawer(brush, draw_type);

  if(cmd_line->m_blend.m_value)
    {
      //specify how blending is done on the rect items.
      drawer.m_draw_passes[0]
        .m_draw_state.add_gl_state_change(WRATHNew WRATHGLStateChange::blend_state(GL_SRC_ALPHA, GL_ONE));
    }

  m_rects.resize(std::max(0, cmd_line->m_layer_count.m_value));
  for(unsigned int i=0, endi=m_rects.size(); i<endi ; ++i)
    {
      float alpha;
      alpha = (endi>10) ? 0.2f : 1.0f/static_cast<float>(endi);

      m_rects[i]=WRATHNew RectWidget(m_child_layer, drawer);
      m_rects[i]->color(WRATHGradient::color(1.0f, 1.0f, 1.0f, alpha));
      m_rects[i]->z_order(i);
      m_rects[i]->properties()
        ->set_parameters(WRATHDefaultRectAttributePacker::rect_properties(cmd_line->m_virtual_width.m_value,
                                                                          cmd_line->m_virtual_height.m_value));
    }

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

  stream.stream() << WRATHText::set_pixel_size(64)
                  << WRATHText::set_color(255, 0, 0)
                  << WRATHText::set_font(WRATHFontDatabase::FontProperties()
                                         .family_name("DejaVuSans"),
                                         type_tag<WRATHTextureFontFreeType_Analytic>())
                  << m_frame << "\n";

  if(m_show_ms)
    {
      stream.stream() << ms << " ms\n";
    }

  m_text_widget->clear();
  m_text_widget->add_text(stream);

  for(unsigned int i=0, endi=m_rects.size(); i<endi ; ++i)
    {
      vec2 p;
      float r0, r1, theta, d;

      theta=(static_cast<float>(total_ms)/500.0f + static_cast<float>(i+1)/4.0f) * 2.0f * float(M_PI);
      d=static_cast<float>(i+1)/static_cast<float>(endi) * static_cast<float>(m_virtual_height) * 0.5f;
      p.x()=d*cosf(theta) + d;
      p.y()=d*sinf(theta) + d;
      r0=0.0f;
      r1=(sinf(0.1f*theta)+2.0f)*static_cast<float>(m_virtual_height) * 0.1f; 
      m_rects[i]->set_gradient(p, r0, p, r1);
    }

  m_tr->signal_complete_simulation_frame();
  m_tr->signal_begin_presentation_frame();
  m_layer->clear_and_draw();

  if(!m_record_frame.empty())
    {
      std::ostringstream filename;
    
      filename << m_record_frame << std::setfill('0') << std::setw(5) << m_frame;

      if(m_save_png)
        {
          filename << ".png";
          save_framebuffer_png(filename.str());
        }
      else
        {
          filename << ".bmp";
          save_framebuffer_bmp(filename.str());
        }
    }

  ++m_frame;
  update_widget();

  if(m_num_frames>0 && m_frame>=m_num_frames)
    {
      end_demo();
    }
}

void
CounterExample::
save_framebuffer_bmp(const std::string &filename)
{
  std::ofstream file(filename.c_str());
  std::vector<vecN<uint8_t,4> > framebuffer(width()*height());
  
  glReadPixels(0, 0, width(), height(), GL_RGBA, GL_UNSIGNED_BYTE, &framebuffer[0]);
  bmpWriter::write_bmp(width(), height(), framebuffer, file);    
}

void
CounterExample::
save_framebuffer_png(const std::string &filename)
{
  FILE *file;
  std::vector<vecN<uint8_t,4> > framebuffer(width()*height());

  file=fopen(filename.c_str(), "wb");
  if(file==NULL)
    {
      return;
    }

  glReadPixels(0, 0, width(), height(), GL_RGBA, GL_UNSIGNED_BYTE, &framebuffer[0]);
  save_png(width(), height(), framebuffer, file); 
  fclose(file);
}


void 
CounterExample::
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
