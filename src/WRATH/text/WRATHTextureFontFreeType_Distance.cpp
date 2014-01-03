/*! 
 * \file WRATHTextureFontFreeType_Distance.cpp
 * \brief file WRATHTextureFontFreeType_Distance.cpp
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
#include <sstream>
#include <limits>
#include <math.h>
#include <fstream>
#include <iomanip>
#include <boost/multi_array.hpp>
#include "WRATHTextureFontFreeType_Distance.hpp"
#include "WRATHUtil.hpp"
#include "WRATHglGet.hpp"
#include "c_array.hpp"
#include "WRATHStaticInit.hpp"
#include "ostream_utility.hpp"
#include "WRATHFreeTypeSupport.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

using namespace WRATHFreeTypeSupport;



namespace
{
  class character:public WRATHTextureFont::glyph_data_type
  {
  public:
    character(WRATHImage *pImage):
      m_image(pImage)
    {}

    ~character()
    {
      WRATHDelete(m_image);
    }
    
    WRATHImage *m_image;
  };

  class common_distance_data_type:boost::noncopyable
  {
  public:
    common_distance_data_type(void):
      m_force_power2_texture(false),
      m_texture_creation_size(1024),
      m_max_L1_distance(96.0f),
      m_fill_rule(WRATHTextureFontFreeType_Distance::non_zero_winding_rule)
    {
      m_allocator=WRATHImage::create_texture_allocator(true, m_texture_creation_size,
                                                       GL_REPEAT,
                                                       GL_REPEAT);
      

      m_vertex_source[linear_glyph_position].m_fragment_processor
        .add_source("font_common_linear.vert.wrath-shader.glsl",
                    WRATHGLShader::from_resource);

      

      m_vertex_source[nonlinear_glyph_position].m_fragment_processor
        .add_source("font_common_nonlinear.vert.wrath-shader.glsl",
                    WRATHGLShader::from_resource);


      m_fragment_source[linear_glyph_position].m_fragment_processor
        .add_source("font_distance_linear.frag.wrath-shader.glsl",
                    WRATHGLShader::from_resource);

      

      m_fragment_source[nonlinear_glyph_position].m_fragment_processor
        .add_source("font_distance_nonlinear.frag.wrath-shader.glsl",
                    WRATHGLShader::from_resource);

      
      m_fragment_source.m_fragment_processor_sampler_names
        .push_back("DistanceField");
    }

    WRATHMutex m_mutex;
    bool m_force_power2_texture;
    int m_texture_creation_size;
    float m_max_L1_distance;
    enum WRATHTextureFontFreeType_Distance::fill_rule_type m_fill_rule;
    WRATHImage::TextureAllocatorHandle m_allocator;
    WRATHTextureFont::FragmentSource m_fragment_source;
  };

  common_distance_data_type&
  common_data(void)
  {
    WRATHStaticInit();
    static common_distance_data_type R;
    return R;
  }
  
  
  inline
  GLubyte
  pixel_value_from_distance(float dist, bool outside)
  {  
    GLubyte v;

    v=static_cast<GLubyte>(127.0f*dist);
    //
    //note that 127 is "-0" and 128 is "+0".
    return (outside)?127-v:128+v;

    /*
    dist=std::max(std::min(dist, 1.0f), 0.0f);

    if(outside)
      dist*=-1.0f;
    
    dist+=1.0f;
    dist/=2.0f;
    WRATHassert(dist>=0.0f);

    dist*=255.0f;
    int rv;

    rv=std::min(255, static_cast<int>(dist));
    rv=std::max(0, rv);
    return static_cast<GLubyte>(rv);
    */
  }

  
}

/////////////////////////////////////////
// WRATHTextureFontFreeType_Distance methods
WRATHTextureFontFreeType_Distance::
WRATHTextureFontFreeType_Distance(WRATHFreeTypeSupport::LockableFace::handle pface, 
                                  const WRATHTextureFontKey &presource_name):
  WRATHTextureFontFreeTypeT<WRATHTextureFontFreeType_Distance>(pface, presource_name),
  m_max_distance(max_L1_distance()), 
  m_fill_rule(fill_rule())
{
  ctor_init();
}

void
WRATHTextureFontFreeType_Distance::
ctor_init(void)
{
  WRATHassert((ttf_face()->face()->face_flags&FT_FACE_FLAG_SCALABLE)!=0);
}

WRATHTextureFontFreeType_Distance::
~WRATHTextureFontFreeType_Distance()
{

#if defined(WRATH_FONT_GENERATION_STATS)
  /*
    I want to know how long it took to 
    generate the glyphs on average
   */
  std::cout << "[Distance]" << simple_name() << " "
            << glyph_data_stats()
            << " spread across " 
            << m_page_tracker.number_texture_pages()
            << " pages\n";
#endif

}

const_c_array<WRATHTextureChoice::texture_base::handle>
WRATHTextureFontFreeType_Distance::
texture_binder(int pg)
{
  return m_page_tracker.texture_binder(pg);
}

ivec2
WRATHTextureFontFreeType_Distance::
texture_size(int pg, 
             enum texture_coordinate_size)
{  
  return m_page_tracker.main_texture_size(pg);
}

int
WRATHTextureFontFreeType_Distance::
number_texture_pages(void)
{
  return m_page_tracker.number_texture_pages();
}

const WRATHTextureFont::FragmentSource*
WRATHTextureFontFreeType_Distance::
fragment_source(void)
{
  return &common_data().m_fragment_source;
}
  
WRATHImage*
WRATHTextureFontFreeType_Distance::
create_glyph(std::vector<uint8_t> &pdata, const ivec2 &sz)
{
  WRATHImage *pImage;
  GLenum format;

  #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
  {
    format=GL_LUMINANCE;
  }
  #else
  {
    format=GL_RED;
  }
  #endif

  pImage=WRATHNew WRATHImage(sz,
                             WRATHImage::ImageFormat()
                             .internal_format(format)
                             .pixel_data_format(format)
                             .pixel_type(GL_UNSIGNED_BYTE)
                             .magnification_filter(GL_LINEAR)
                             .minification_filter(GL_LINEAR)
                             .automatic_mipmap_generation(false),
                             WRATHImage::BoundarySize(),
                             common_data().m_allocator);

  pImage->respecify_sub_image(0,
                              WRATHImage::PixelImageFormat()
                              .pixel_data_format(format)
                              .pixel_type(GL_UNSIGNED_BYTE),
                              pdata,
                              ivec2(0,0), sz);

  return pImage;
  
}


WRATHTextureFont::glyph_data_type*
WRATHTextureFontFreeType_Distance::
generate_character(WRATHTextureFont::glyph_index_type G)
{
  ivec2 bitmap_sz, bitmap_offset, glyph_size;
  ivec2 slack_added(0,0);
  std::vector<WRATHFreeTypeSupport::point_type> pts;
  std::ostream *stream_ptr(NULL);
  character_code_type C;
  ivec2 iadvance;


  geometry_data dbg(stream_ptr, pts);
  
  
  WRATHassert(G.valid());
  C=character_code(G);
  
  /* lock ttf_face() since we are referencing it via FT*/
  WRATHLockMutex(ttf_face()->mutex());

  FT_Set_Pixel_Sizes(ttf_face()->face(), pixel_size(), pixel_size());


  //Load the name glyph, this puts the glyph data
  //into ttf_face()->face()->glyph
  FT_Set_Transform(ttf_face()->face(), NULL, NULL);
  FT_Load_Glyph(ttf_face()->face(), G.value(), FT_LOAD_DEFAULT);

  //tell Freetype2 to render the glyph to a bitmap,
  //this bitmap is located at ttf_face()->face()->glyph->bitmap
  FT_Render_Glyph(ttf_face()->face()->glyph, FT_RENDER_MODE_NORMAL);

  
  bitmap_sz=ivec2(ttf_face()->face()->glyph->bitmap.width,
                  ttf_face()->face()->glyph->bitmap.rows);
      
  bitmap_offset=ivec2(ttf_face()->face()->glyph->bitmap_left,
                      ttf_face()->face()->glyph->bitmap_top 
                      - ttf_face()->face()->glyph->bitmap.rows);

  //generate the outline data of the glyph:
  OutlineData outline_data(ttf_face()->face()->glyph->outline, 
                           bitmap_sz, bitmap_offset, dbg);
  std::vector<uint8_t> coverage_values;
  int local_pitch(0), local_rows(0);

  if(bitmap_sz.x()>0 and bitmap_sz.y()>0)
    {
      glyph_size=bitmap_sz+ivec2(1,1);
      slack_added=glyph_size-bitmap_sz;   
    }
  else
    {
      glyph_size=ivec2(0,0);
    }

  iadvance=ivec2(ttf_face()->face()->glyph->advance.x,
                 ttf_face()->face()->glyph->advance.y);
    
  
  if(m_fill_rule==freetype_render)
    {
      int sz;

      local_pitch=std::abs(ttf_face()->face()->glyph->bitmap.pitch);
      local_rows=ttf_face()->face()->glyph->bitmap.rows;
      sz=local_rows*local_pitch;

      coverage_values.resize(sz);
     
      std::copy(ttf_face()->face()->glyph->bitmap.buffer,
                ttf_face()->face()->glyph->bitmap.buffer+sz,
                coverage_values.begin());
    }
  WRATHUnlockMutex(ttf_face()->mutex());
  /* unlocked ttf_face() safe as now we do not touch it*/

  /*
    Generate the distance values:
   */
  int num_bytes(glyph_size.x()*glyph_size.y());  
  std::vector<uint8_t> image_buffer(num_bytes, 0);
  boost::multi_array<distance_return_type, 2> 
    distance_values(boost::extents[bitmap_sz.x()][bitmap_sz.y()]);


  outline_data.compute_distance_values(distance_values, m_max_distance, 
                                       m_fill_rule==non_zero_winding_rule);

  for(int yy=0; yy<bitmap_sz.y(); ++yy)
    {
      for(int xx=0; xx<bitmap_sz.x(); ++xx)
        {
          int location;
          bool outside;
          float v0;
          
          location= xx + yy*glyph_size.x();
          WRATHassert(location>=0 and location<glyph_size.x()*glyph_size.y());

         

          switch(m_fill_rule)
            {
            default:
            case non_zero_winding_rule:
              outside=(distance_values[xx][yy].m_solution_count.winding_number()==0);
              break;

            case odd_even_rule:
              outside=distance_values[xx][yy].m_solution_count.outside();
              break;

            case freetype_render:
              {
                int vvvvv;
                vvvvv=xx + (local_rows - 1 - yy)*local_pitch;
                outside=coverage_values[vvvvv]<=127;
              }
              break;
            }
          
          v0=distance_values[xx][yy].m_distance.value();
          v0=std::min(v0/m_max_distance, 1.0f);
                 
          image_buffer[location]=pixel_value_from_distance(v0, outside);
            
        }
    }

  
  character *return_value;

  return_value=WRATHNew character(create_glyph(image_buffer, glyph_size));
  glyph_data_type &glyph(*return_value);

  glyph
    .iadvance(iadvance)
    .font(this)
    .texture_page(m_page_tracker.get_page_number(return_value->m_image))
    .texel_values(return_value->m_image->minX_minY(), bitmap_sz, native_value)
    .texel_values(return_value->m_image->minX_minY(), bitmap_sz, minified_value) 
    .origin(bitmap_offset, native_value)
    .origin(bitmap_offset, minified_value)   
    .bounding_box_size(bitmap_sz+ivec2(1,1))
    .character_code(C)
    .glyph_index(G);

  return return_value;

}



GLint
WRATHTextureFontFreeType_Distance::
texture_creation_size(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_texture_creation_size;
}

void
WRATHTextureFontFreeType_Distance::
texture_creation_size(GLint v)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_texture_creation_size=v;
  v=WRATHTextureFontUtil::effective_texture_creation_size(v, common_data().m_force_power2_texture);
  common_data().m_allocator.texture_atlas_dimension(v);
}


float
WRATHTextureFontFreeType_Distance::
max_L1_distance(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_max_L1_distance;
}

  
void
WRATHTextureFontFreeType_Distance::
max_L1_distance(float v)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_max_L1_distance=v;
}

bool
WRATHTextureFontFreeType_Distance:: 
force_power2_texture(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_force_power2_texture;
}

void 
WRATHTextureFontFreeType_Distance:: 
force_power2_texture(bool b)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  if(b!=common_data().m_force_power2_texture)
    {
      int v;
      v=WRATHTextureFontUtil::effective_texture_creation_size(common_data().m_texture_creation_size, b);

      common_data().m_force_power2_texture=b;
      common_data().m_allocator.texture_atlas_dimension(v);
    }
}

enum WRATHTextureFontFreeType_Distance::fill_rule_type
WRATHTextureFontFreeType_Distance:: 
fill_rule(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_fill_rule;
}


void
WRATHTextureFontFreeType_Distance:: 
fill_rule(enum WRATHTextureFontFreeType_Distance::fill_rule_type b)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_fill_rule=b;
}

GLint
WRATHTextureFontFreeType_Distance:: 
effective_texture_creation_size(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  bool flag(common_data().m_force_power2_texture);
  int v(common_data().m_texture_creation_size);
  return WRATHTextureFontUtil::effective_texture_creation_size(v, flag);
}



WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
WRATHTextureFontFreeType_Distance::
texture_consumption(void)
{
  return common_data().m_allocator.texture_consumption();
}


