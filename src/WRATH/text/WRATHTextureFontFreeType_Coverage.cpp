/*! 
 * \file WRATHTextureFontFreeType_Coverage.cpp
 * \brief file WRATHTextureFontFreeType_Coverage.cpp
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
#include "WRATHTextureFontFreeType_Coverage.hpp"
#include "WRATHUtil.hpp"
#include "WRATHglGet.hpp"
#include "c_array.hpp"
#include "ostream_utility.hpp"
#include "WRATHStaticInit.hpp"
#include "WRATHFreeTypeSupport.hpp"
#include "WRATHImage.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

using namespace WRATHFreeTypeSupport;
namespace
{
  class local_glyph_type:public WRATHTextureFont::glyph_data_type
  {
  public:
    explicit
    local_glyph_type(WRATHImage *pImage):
      m_image(pImage)
    {}

    ~local_glyph_type()
    {
      WRATHDelete(m_image);
    }

    WRATHImage *m_image;
  };

  class common_coverage_data_type:boost::noncopyable
  {
  public:
    common_coverage_data_type(void):
      m_mipmap_slacking_threshhold_level(1),
      m_texture_creation_size(1024),
      m_force_power2_texture(true),
      m_magnification_filter(GL_LINEAR),
      m_minification_filter(GL_LINEAR_MIPMAP_NEAREST)
    {
      m_allocator=WRATHImage::create_texture_allocator(true, m_texture_creation_size);

      m_glyph_glsl.m_texture_page_data_size=2;

      m_glyph_glsl.m_vertex_processor[WRATHTextureFont::GlyphGLSL::linear_glyph_position]
        .add_source("font_coverage_linear.vert.wrath-shader.glsl",
                    WRATHGLShader::from_resource);

      m_glyph_glsl.m_fragment_processor[WRATHTextureFont::GlyphGLSL::linear_glyph_position]
        .add_source("font_coverage_linear.frag.wrath-shader.glsl",
                    WRATHGLShader::from_resource);


      m_glyph_glsl.m_vertex_processor[WRATHTextureFont::GlyphGLSL::nonlinear_glyph_position]
        .add_source("font_coverage_nonlinear.vert.wrath-shader.glsl",
                    WRATHGLShader::from_resource);

      m_glyph_glsl.m_fragment_processor[WRATHTextureFont::GlyphGLSL::nonlinear_glyph_position]
        .add_source("font_coverage_nonlinear.frag.wrath-shader.glsl",
                    WRATHGLShader::from_resource);

      
      m_glyph_glsl.m_sampler_names.push_back("wrath_CoverageField");
      m_glyph_glsl.m_global_names.push_back("wrath_CoverageFieldTexCoord");
      m_glyph_glsl.m_global_names.push_back("wrath_CoverageFieldPosition");
      m_glyph_glsl.m_global_names.push_back("wrath_CoverageFieldBottomLeft");

    }

    WRATHMutex m_mutex;
    int m_mipmap_slacking_threshhold_level;
    int m_texture_creation_size;
    bool m_force_power2_texture;
    GLenum m_magnification_filter;
    GLenum m_minification_filter;
    WRATHImage::TextureAllocatorHandle m_allocator;
    WRATHTextureFont::GlyphGLSL m_glyph_glsl;
  };

  common_coverage_data_type&
  common_data(void)
  {
    WRATHStaticInit();
    static common_coverage_data_type R;
    return R;
  }
  
  int
  number_mipmaps(bool use_mips, ivec2 sz, int pix_sz)
  {
    if(!use_mips)
      {
        return 1;
      }

    int m;
    for(m=1; (sz.x()>1 or sz.y()>1) and pix_sz>0; ++m, sz.x()>>=1, sz.y()>>=1, pix_sz>>=1)
      {}

    return m;
  }

  bool
  minification_needs_slack(GLenum s)
  {
    return s==GL_LINEAR_MIPMAP_NEAREST
      or s==GL_LINEAR_MIPMAP_LINEAR;
  }

}

///////////////////////////////////
// WRATHTextureFontFreeType_Coverage::glyph_mipmap_level methods
void
WRATHTextureFontFreeType_Coverage::glyph_mipmap_level::
take_bitmap_data(FT_Face fc)
{
  m_raw_size=ivec2(fc->glyph->bitmap.width,
                   fc->glyph->bitmap.rows);
  m_raw_pitch=fc->glyph->bitmap.pitch;

  m_raw_pixels_from_freetype.resize( std::abs(m_raw_pitch)*fc->glyph->bitmap.rows);
  std::copy(fc->glyph->bitmap.buffer, 
            fc->glyph->bitmap.buffer+m_raw_pixels_from_freetype.size(),
            m_raw_pixels_from_freetype.begin());
}

void
WRATHTextureFontFreeType_Coverage::glyph_mipmap_level::
create_pixel_data(ivec2 sz)
{
  m_pixels.resize( sz.x()*sz.y(), 0);
  m_size=sz;
  
  for(int yy=0; yy<m_raw_size.y() and yy<sz.y(); ++yy)
    {
      for(int xx=0; xx<m_raw_size.x() and xx<sz.x(); ++xx)
        {
          int location, loctionbitmap;
          uint8_t v;
    
          location= xx + yy*sz.x();
          loctionbitmap=xx + 
            (m_raw_size.y()-1-yy)*m_raw_pitch;
          
          WRATHassert(loctionbitmap<static_cast<int>(m_raw_pixels_from_freetype.size()));

          v=m_raw_pixels_from_freetype[loctionbitmap];
          m_pixels[location]=v;
        }
    }
}





/////////////////////////////////////////
// WRATHTextureFontFreeType_Coverage methods
WRATHTextureFontFreeType_Coverage::
WRATHTextureFontFreeType_Coverage(WRATHFreeTypeSupport::LockableFace::handle pface,  
                                  const WRATHTextureFontKey &presource_name):
  WRATHTextureFontFreeTypeT<WRATHTextureFontFreeType_Coverage>(pface, presource_name),

  m_minification_filter(minification_filter()),
  m_magnification_filter(magnification_filter()),
  m_use_mipmaps(WRATHImage::ImageFormat::requires_mipmaps(m_minification_filter)),
  m_mipmap_deepness_concern(mipmap_slacking_threshhold_level()),

  m_total_pixel_waste(0),
  m_total_pixel_use(0)
{
  ctor_init();
  m_page_tracker.connect(boost::bind(&WRATHTextureFontFreeType_Coverage::on_create_texture_page, this,
                                     _2, _4));
}


void
WRATHTextureFontFreeType_Coverage::
ctor_init(void)
{
  WRATHassert((ttf_face()->face()->face_flags&FT_FACE_FLAG_SCALABLE)!=0);
}

WRATHTextureFontFreeType_Coverage::
~WRATHTextureFontFreeType_Coverage()
{

#if defined(WRATH_FONT_GENERATION_STATS)
  /*
    I want to know how long it took to 
    generate the glyphs on average
   */
  std::cout << "[Coverage]" << simple_name() << " "
            << glyph_data_stats()
            << " spread across " 
            << m_page_tracker.number_texture_pages()
            << " pages, total_pixel_used="
            << m_total_pixel_use
            << ", pixel_waste=" << m_total_pixel_waste;

  if(m_total_pixel_use>0)
    {
      std::cout  << " utilization= " 
                 << 1.0f - static_cast<float>(m_total_pixel_waste)/static_cast<float>(m_total_pixel_use);
    }
  std::cout << "\n";
#endif

}

const_c_array<WRATHTextureChoice::texture_base::handle>
WRATHTextureFontFreeType_Coverage::
texture_binder(int pg)
{
  return m_page_tracker.texture_binder(pg);
}

void
WRATHTextureFontFreeType_Coverage::
on_create_texture_page(ivec2 texture_size,
                       std::vector<float> &custom_data)
{
  custom_data.resize(2);
  custom_data[0]=1.0f/static_cast<float>(std::max(1, texture_size.x()) );
  custom_data[1]=1.0f/static_cast<float>(std::max(1, texture_size.y()) );
}

int
WRATHTextureFontFreeType_Coverage::
texture_page_data_size(void) const
{
  return 2; //reciprocal texture size
}

float
WRATHTextureFontFreeType_Coverage::
texture_page_data(int texture_page, int idx) const
{
  return (0<=idx and idx<2)?
    m_page_tracker.custom_data(texture_page)[idx]:
    0;
}

int
WRATHTextureFontFreeType_Coverage::
number_texture_pages(void)
{
  return m_page_tracker.number_texture_pages();
}


const WRATHTextureFont::GlyphGLSL*
WRATHTextureFontFreeType_Coverage::
glyph_glsl(void)
{
  return &common_data().m_glyph_glsl;
}


WRATHImage*
WRATHTextureFontFreeType_Coverage::
create_glyph(std::vector<glyph_mipmap_level> &pdata)
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

  pImage=WRATHNew WRATHImage(pdata[0].size(),
                             WRATHImage::ImageFormat()
                             .internal_format(format)
                             .pixel_data_format(format)
                             .pixel_type(GL_UNSIGNED_BYTE)
                             .magnification_filter(m_magnification_filter)
                             .minification_filter(m_minification_filter)
                             .automatic_mipmap_generation(false),
                             WRATHImage::BoundarySize(),
                             common_data().m_allocator);
      
  /*
    Now use the WRATHImage API to set the texture
    data
  */
  for(unsigned int i=0, endi=pdata.size(); 
      i<endi and pdata[i].size().x()>0 and pdata[i].size().y()>0; ++i)
    {
      pImage->respecify_sub_image(i, //lod
                                  WRATHImage::PixelImageFormat()
                                  .pixel_data_format(format)
                                  .pixel_type(GL_UNSIGNED_BYTE),
                                  pdata[i].pixels(),
                                  ivec2(0,0),
                                  pdata[i].size());
      
      
    }  
  return pImage;
}


WRATHTextureFontFreeType_Coverage::glyph_data_type*
WRATHTextureFontFreeType_Coverage::
generate_character(WRATHTextureFont::glyph_index_type G)
{
  ivec2 bitmap_sz, bitmap_offset, glyph_size(0,0);
  ivec2 iadvance;
  ivec2 slack_added(0,0);
  character_code_type C;
  int slack(0);
  std::vector<glyph_mipmap_level> mipmaps;

  WRATHLockMutex(ttf_face()->mutex());
  
  FT_Set_Pixel_Sizes(ttf_face()->face(), pixel_size(), pixel_size());
      
  
  WRATHassert(G.valid());
  C=character_code(G);

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
                      ttf_face()->face()->glyph->bitmap_top - ttf_face()->face()->glyph->bitmap.rows);


  
  iadvance=ivec2(ttf_face()->face()->glyph->advance.x,
                 ttf_face()->face()->glyph->advance.y);

  int mip_levels;

  mip_levels=number_mipmaps(m_use_mipmaps, bitmap_sz, pixel_size());
  mipmaps.resize(mip_levels);

  //is adding some padding needed?
  if(bitmap_sz.x()>0 and bitmap_sz.y()>0)
    {
      unsigned int deepness, max_deepness;
      int h;

      max_deepness=m_mipmap_deepness_concern+1;
      mipmaps[0].take_bitmap_data(ttf_face()->face());

      for(deepness=1, h=(pixel_size()>>1);
          (ttf_face()->face()->glyph->bitmap.width>4 
           or ttf_face()->face()->glyph->bitmap.rows>4)
            and deepness<max_deepness and deepness<mipmaps.size() and h>=8; ++deepness, h>>=1)
        {
          
          /*
            there are two different ways one can render the
            glyph at a lower resolution: by changing the
            pixel size or by setting the transform via
            FT_Set_Transform. We get a better render 
            results if we use FT_Set_Pixel_Sizes().
          */
          FT_Set_Pixel_Sizes(ttf_face()->face(), h, 0);
          FT_Load_Glyph(ttf_face()->face(), G.value(), FT_LOAD_DEFAULT);
          FT_Render_Glyph(ttf_face()->face()->glyph, FT_RENDER_MODE_NORMAL);
        
          /* copy raw bitmap data to mipmaps[deepness] */
          mipmaps[deepness].take_bitmap_data(ttf_face()->face());

        }

      int scale_factor;
      scale_factor=(1<<(deepness-1));

      if(minification_needs_slack(m_minification_filter))
        {
          slack=scale_factor;
        }
      else 
        {
          slack=1;
        }

      glyph_size=ivec2(slack,slack)
        + scale_factor*ivec2(ttf_face()->face()->glyph->bitmap.width,
                             ttf_face()->face()->glyph->bitmap.rows);
      
      
     
      for(int mm=deepness, end_mm=mipmaps.size(); mm<end_mm; ++mm)
        {
          
          FT_Set_Pixel_Sizes(ttf_face()->face(), pixel_size()>>mm, pixel_size()>>mm);
          FT_Load_Glyph(ttf_face()->face(), G.value(), FT_LOAD_DEFAULT);
          FT_Render_Glyph(ttf_face()->face()->glyph, FT_RENDER_MODE_NORMAL);

          /* copy raw bitmap data to mipmaps[deepness] */
          mipmaps[mm].take_bitmap_data(ttf_face()->face());    
          
        }

        
      slack_added=glyph_size-bitmap_sz;   
    }

  int area_used(glyph_size.x()*glyph_size.y());
  int area_needed(bitmap_sz.x()*bitmap_sz.y());

  m_total_pixel_use+=area_used;
  m_total_pixel_waste+=(area_used - area_needed); 

  WRATHUnlockMutex(ttf_face()->mutex());

  for(unsigned int m=0; m<mipmaps.size(); ++m)
    {
      ivec2 sz;

      if(m==0)
        {
          sz=glyph_size;
        }
      else
        {
          sz=mipmaps[m-1].size()/2;
        }
      mipmaps[m].create_pixel_data(sz);
    }


  ivec2 texture_size(bitmap_sz);
  WRATHImage *glyph_image;
  glyph_data_type *return_value;

  glyph_image=create_glyph(mipmaps);
  return_value=WRATHNew local_glyph_type(glyph_image);

  glyph_data_type &glyph(*return_value);


  glyph
    .iadvance(iadvance)
    .font(this)
    .texture_page(m_page_tracker.get_page_number(glyph_image))
    .texel_values(glyph_image->minX_minY(), texture_size)
    .origin(bitmap_offset)
    .bounding_box_size(bitmap_sz+ivec2(1,1))
    .character_code(C)
    .glyph_index(G);

   return return_value;
}


GLenum
WRATHTextureFontFreeType_Coverage:: 
minification_filter(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_minification_filter;
}

void
WRATHTextureFontFreeType_Coverage:: 
minification_filter(GLenum v)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_minification_filter=v;
}


GLenum
WRATHTextureFontFreeType_Coverage:: 
magnification_filter(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_magnification_filter;
}


void
WRATHTextureFontFreeType_Coverage:: 
magnification_filter(GLenum v)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_magnification_filter=v;
}

GLint
WRATHTextureFontFreeType_Coverage::
texture_creation_size(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_texture_creation_size;
}

void
WRATHTextureFontFreeType_Coverage::
texture_creation_size(GLint v)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_texture_creation_size=v;
  v=WRATHTextureFontUtil::effective_texture_creation_size(v, common_data().m_force_power2_texture);
  common_data().m_allocator.texture_atlas_dimension(v);
}

bool
WRATHTextureFontFreeType_Coverage:: 
force_power2_texture(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_force_power2_texture;
}

void 
WRATHTextureFontFreeType_Coverage:: 
force_power2_texture(bool v)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_force_power2_texture=v;

  int d;
  d=common_data().m_texture_creation_size;
  d=WRATHTextureFontUtil::effective_texture_creation_size(d, v);
  common_data().m_allocator.texture_atlas_dimension(d);
}

GLint
WRATHTextureFontFreeType_Coverage:: 
effective_texture_creation_size(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  bool flag(common_data().m_force_power2_texture);
  int v(common_data().m_texture_creation_size);
  return WRATHTextureFontUtil::effective_texture_creation_size(v, flag);
}

int
WRATHTextureFontFreeType_Coverage:: 
mipmap_slacking_threshhold_level(void)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  return common_data().m_mipmap_slacking_threshhold_level;
}

void
WRATHTextureFontFreeType_Coverage:: 
mipmap_slacking_threshhold_level(int v)
{
  WRATHAutoLockMutex(common_data().m_mutex);
  common_data().m_mipmap_slacking_threshhold_level=v;
}



WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
WRATHTextureFontFreeType_Coverage::
texture_consumption(void)
{
  return common_data().m_allocator.texture_consumption();
}
