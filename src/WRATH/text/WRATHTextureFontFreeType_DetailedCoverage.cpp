/*! 
 * \file WRATHTextureFontFreeType_DetailedCoverage.cpp
 * \brief file WRATHTextureFontFreeType_DetailedCoverage.cpp
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
#include <sys/time.h>
#include "WRATHTextureFontFreeType_DetailedCoverage.hpp"
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
  class common_data_type:boost::noncopyable
  {
  public:
    common_data_type(void);

    WRATHImage::TextureAllocatorHandle
    fetch_index_allocator(uint32_t sz) const;

    void
    add_pixel_size(int sz);

    void
    clear_pixel_size_choices(void);

    void
    get_pixel_sizes(std::vector<int> &out_sizes);

    void
    get_pixel_sizes(std::vector<int> &out_sizes, int max_size);

    
    WRATHImage::TextureAllocatorHandle m_coverage_allocator;

    WRATHImage::ImageFormatArray m_index_format;
    WRATHImage::ImageFormatArray m_coverage_format;
    WRATHTextureFont::GlyphGLSL m_glyph_glsl;

  private:
    WRATHMutex m_mutex;
    std::map<uint32_t, WRATHImage::TextureAllocatorHandle> m_index_allocators;
    std::set<int> m_pixel_sizes;
    
  };


  common_data_type&
  common_data(void)
  {
    WRATHStaticInit();
    static common_data_type R;
    return R;
  }


  struct size_choices_t
  {
    int m_end_size;
    int m_slot_advance;
  };

  

  class local_glyph_data:public WRATHTextureFont::glyph_data_type
  {
  public:
    local_glyph_data(const std::vector<WRATHImage*> &cvg,
                     WRATHImage *pI);

    ~local_glyph_data();

    std::vector<WRATHImage*> m_coverage_data;
    WRATHImage *m_index_data;
  };
}


#if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
  #define COVERAGE_FORMAT GL_LUMINANCE
#else
  #define COVERAGE_FORMAT GL_RED
#endif

///////////////////////////////
// common_data_type methods
common_data_type::
common_data_type(void):
  m_index_format(WRATHImage::ImageFormat()
                 .internal_format(GL_RGBA)
                 .pixel_data_format(GL_RGBA)
                 .pixel_type(GL_UNSIGNED_BYTE)
                 .magnification_filter(GL_NEAREST)
                 .minification_filter(GL_NEAREST)),

  m_coverage_format(WRATHImage::ImageFormat()
                    .internal_format(COVERAGE_FORMAT)
                    .pixel_data_format(COVERAGE_FORMAT)
                    .pixel_type(GL_UNSIGNED_BYTE)
                    .magnification_filter(GL_LINEAR)
                    .minification_filter(GL_LINEAR))
{
  for(uint32_t dim=1, i=0; i<=8; ++i, dim*=2)
    {
      WRATHImage::TextureAllocatorHandle R;

      R=WRATHImage::create_texture_allocator(true);
      R.texture_atlas_dimension(dim, 256);
      m_index_allocators[dim]=R;
    }
  m_coverage_allocator=WRATHImage::create_texture_allocator(true, 256);


  //initialize m_pixel_sizes as.. something reasonable:
  struct size_choices_t size_choices[]=
    {
      {24, 2}, //up to size 24, we advance pixel size by 2
      {32, 4}, //from size 24 to 32 be advance pixel size by 4
      //after size 32 we do not add anymore.
    };
  const int num_choices=sizeof(size_choices)/sizeof(size_choices_t);
  const int start_size=8, last_size=32;
  for(int i=0, current_sz=start_size; i<num_choices; 
      current_sz=size_choices[i].m_end_size, ++i)
    {
      for(;current_sz<=size_choices[i].m_end_size and current_sz<last_size;
          current_sz+=size_choices[i].m_slot_advance)
        {
          m_pixel_sizes.insert(current_sz);
        }
    }
  m_pixel_sizes.insert(last_size);

  m_glyph_glsl.m_texture_page_data_size=0;
  m_glyph_glsl.m_vertex_processor[WRATHTextureFont::GlyphGLSL::linear_glyph_position]
    .add_source("font_detailed_linear.vert.wrath-shader.glsl", WRATHGLShader::from_resource);

  m_glyph_glsl.m_fragment_processor[WRATHTextureFont::GlyphGLSL::linear_glyph_position]
    .add_source("font_detailed_base.frag.wrath-shader.glsl", WRATHGLShader::from_resource)
    .add_source("font_detailed_linear.frag.wrath-shader.glsl", WRATHGLShader::from_resource);

  m_glyph_glsl.m_vertex_processor[WRATHTextureFont::GlyphGLSL::nonlinear_glyph_position]
    .add_source("font_detailed_nonlinear.vert.wrath-shader.glsl", WRATHGLShader::from_resource);

  m_glyph_glsl.m_fragment_processor[WRATHTextureFont::GlyphGLSL::nonlinear_glyph_position]
    .add_source("font_detailed_base.frag.wrath-shader.glsl", WRATHGLShader::from_resource)
    .add_source("font_detailed_nonlinear.frag.wrath-shader.glsl", WRATHGLShader::from_resource);
  
  m_glyph_glsl.m_sampler_names.push_back("wrath_DetailedCoverageTexture");
  m_glyph_glsl.m_sampler_names.push_back("wrath_DetailedIndexTexture");

  m_glyph_glsl.m_global_names.push_back("wrath_detailed_wrath_glyph_compute_coverage");
  m_glyph_glsl.m_global_names.push_back("wrath_detailed_wrath_glyph_is_covered");
  m_glyph_glsl.m_global_names.push_back("wrath_DetailedNormalizedCoord_Position");
  m_glyph_glsl.m_global_names.push_back("wrath_DetailedGlyphIndex");
  m_glyph_glsl.m_global_names.push_back("wrath_DetailedGlyphRecipSize_GlyphIndex");

  m_glyph_glsl.m_custom_data_use.push_back(0);
}

WRATHImage::TextureAllocatorHandle
common_data_type::
fetch_index_allocator(uint32_t sz) const
{
  WRATHassert(sz>0 and sz<=256);
  WRATHassert(WRATHUtil::is_power_of_2(sz));

  std::map<uint32_t, WRATHImage::TextureAllocatorHandle>::const_iterator iter;
  iter=m_index_allocators.find(sz);
  WRATHassert(iter!=m_index_allocators.end());

  return iter->second;
}

void
common_data_type::
get_pixel_sizes(std::vector<int> &out_sizes)
{
  WRATHAutoLockMutex(m_mutex);
  out_sizes.resize(m_pixel_sizes.size());
  std::copy(m_pixel_sizes.begin(), m_pixel_sizes.end(), out_sizes.begin());
}

void
common_data_type::
get_pixel_sizes(std::vector<int> &out_sizes, int max_size)
{
  WRATHAutoLockMutex(m_mutex);
  for(std::set<int>::iterator iter=m_pixel_sizes.begin(),
        end=m_pixel_sizes.end(); iter!=end and *iter<max_size; ++iter)
    {
      out_sizes.push_back(*iter);
    }
}

void
common_data_type::
clear_pixel_size_choices(void)
{
  WRATHAutoLockMutex(m_mutex);
  m_pixel_sizes.clear();
}

void
common_data_type::
add_pixel_size(int sz)
{
  WRATHAutoLockMutex(m_mutex);
  m_pixel_sizes.insert(sz);
}


////////////////////////////////
// local_glyph_data methods
local_glyph_data::
local_glyph_data(const std::vector<WRATHImage*> &cvg,
                 WRATHImage *pI):
  m_coverage_data(cvg),
  m_index_data(pI)
{
}

local_glyph_data::
~local_glyph_data()
{
  if(m_index_data!=NULL)
    {
      WRATHDelete(m_index_data);
    }

  for(std::vector<WRATHImage*>::iterator iter=m_coverage_data.begin(),
        end=m_coverage_data.end(); iter!=end; ++iter)
    {
      WRATHDelete(*iter);
    }
}

                 
                 

////////////////////////////////////////////
// WRATHTextureFontFreeType_DetailedCoverage::per_pixel_size_coverage_data methods
void
WRATHTextureFontFreeType_DetailedCoverage::per_pixel_size_coverage_data::
take_bitmap_data(FT_Face fc)
{
  m_size=ivec2(fc->glyph->bitmap.width,
               fc->glyph->bitmap.rows);
  m_raw_pitch=fc->glyph->bitmap.pitch;

  m_raw_pixels_from_freetype.resize( std::abs(m_raw_pitch)*fc->glyph->bitmap.rows);
  std::copy(fc->glyph->bitmap.buffer, 
            fc->glyph->bitmap.buffer+m_raw_pixels_from_freetype.size(),
            m_raw_pixels_from_freetype.begin());
}

void
WRATHTextureFontFreeType_DetailedCoverage::per_pixel_size_coverage_data::
take_bitmap_data(FT_Face fc, 
                 int this_pixel_size,
                 int max_pixel_size,
                 ivec2 offset_at_max_size)
{
  take_bitmap_data(fc);

  /*
    now compute m_bitmap_offset, which is 
    offset_at_max_size - t*offset_at_this_size

    where t= max_pixel_size/this_pixel_size
   */
  float t;
  vec2 local_bitmap_offset;

  local_bitmap_offset=vec2(fc->glyph->bitmap_left,
                           fc->glyph->bitmap_top - fc->glyph->bitmap.rows);

  t=static_cast<float>(max_pixel_size)/static_cast<float>(this_pixel_size);
  m_bitmap_offset=vec2(offset_at_max_size.x(), offset_at_max_size.y())
    - t*local_bitmap_offset;
}

void
WRATHTextureFontFreeType_DetailedCoverage::per_pixel_size_coverage_data::
create_pixel_data(void)
{
  m_pixels.resize(m_size.x()*m_size.y(), 0);
  
  for(int yy=0; yy<m_size.y(); ++yy)
    {
      for(int xx=0; xx<m_size.x(); ++xx)
        {
          int location, loctionbitmap;
          uint8_t v;
    
          location= xx + yy*m_size.x();
          loctionbitmap=xx + 
            (m_size.y()-1-yy)*m_raw_pitch;
          
          WRATHassert(loctionbitmap<static_cast<int>(m_raw_pixels_from_freetype.size()));

          v=m_raw_pixels_from_freetype[loctionbitmap];
          m_pixels[location]=v;
        }
    }
}

////////////////////////////////////
// WRATHTextureFontFreeType_DetailedCoverage methods
WRATHTextureFontFreeType_DetailedCoverage::
WRATHTextureFontFreeType_DetailedCoverage(WRATHFreeTypeSupport::LockableFace::handle pface,  
                                  const WRATHTextureFontKey &presource_name):
  WRATHTextureFontFreeTypeT<WRATHTextureFontFreeType_DetailedCoverage>(pface, presource_name)
{
  ctor_init();
}

WRATHTextureFontFreeType_DetailedCoverage::
~WRATHTextureFontFreeType_DetailedCoverage()
{}

void
WRATHTextureFontFreeType_DetailedCoverage::
ctor_init(void)
{
  WRATHassert((ttf_face()->face()->face_flags&FT_FACE_FLAG_SCALABLE)!=0);

  common_data().get_pixel_sizes(m_pixel_sizes, pixel_size());
  m_pixel_sizes.push_back(pixel_size());
 
  /*
    figure out how wide the index texture
    needs to be in order to capture each
    of the sizes listed in m_pixel_sizes:
   */
  int min_delta(pixel_size());
  for(unsigned int i=1, endi=m_pixel_sizes.size(); i<endi; ++i)
    {
      int delta;

      delta=m_pixel_sizes[i] - m_pixel_sizes[i-1];
      min_delta=std::min(delta, min_delta);
    }
  WRATHassert(min_delta>0);

  int index_texture_width;
  index_texture_width=256;

  /*
    now fill m_look_up_sizes.
   */
  m_look_up_sizes.resize(index_texture_width);
  for(int i=0;i<index_texture_width;++i)
    {
      float min_pixel_size;
      std::vector<int>::iterator iter;
      int K, Kplus1, Kminus1;

      min_pixel_size=(pixel_size()*i)/(index_texture_width-1);
      iter=std::lower_bound(m_pixel_sizes.begin(), m_pixel_sizes.end(),
                            min_pixel_size);
      /*
        we want the location within m_pixel_sizes,
        not the value.
       */
      K=std::distance(m_pixel_sizes.begin(), iter);
      Kminus1=std::max(K-1, 0);
      Kplus1=std::min(K+1, static_cast<int>(m_pixel_sizes.size())-1);

      if(abs(min_pixel_size-m_pixel_sizes[K])>abs(min_pixel_size-m_pixel_sizes[Kminus1]))
        {
          K=Kminus1;
        }

      if(abs(min_pixel_size-m_pixel_sizes[K])>abs(min_pixel_size-m_pixel_sizes[Kplus1]))
        {
          K=Kplus1;
        }

      m_look_up_sizes[i]=K;
    }
  //make sure last is the highest pixel size:
  m_look_up_sizes[index_texture_width-1]=m_pixel_sizes.size() - 1;
  m_index_texture_allocator=common_data().fetch_index_allocator(m_look_up_sizes.size());

}




WRATHImage*
WRATHTextureFontFreeType_DetailedCoverage::
allocate_glyph_room(const std::vector<ivec2> &bitmap_sizes,
                    std::vector<WRATHImage*> &out_images)
{
  enum return_code R;
  WRATHImage *indexImage;
  WRATHImage::BoundarySize slack_adder;

  slack_adder.m_maxX=1;
  slack_adder.m_maxY=1;
  slack_adder.m_minX=1;
  slack_adder.m_minY=1;

  
  WRATHassert(m_pixel_sizes.size()==bitmap_sizes.size());

  R=common_data()
    .m_coverage_allocator.allocate_multiple_images_on_same_page(common_data().m_coverage_format,
                                                                bitmap_sizes, slack_adder, out_images);

  if(R==routine_fail)
    {
      /*
        Can't allocate!
      */
      return NULL;
    }
  WRATHassert(m_pixel_sizes.size()==out_images.size());
    
  std::vector<uint8_t> values(4*m_look_up_sizes.size());

  for(int i=0, endi=m_look_up_sizes.size(); i<endi; ++i)
    {
      int K;

      K=m_look_up_sizes[i];
      values[4*i + 0]=out_images[K]->minX_minY().x();
      values[4*i + 1]=out_images[K]->minX_minY().y();
      values[4*i + 2]=out_images[K]->size().x();
      values[4*i + 3]=out_images[K]->size().y();
    }
  
  indexImage=WRATHNew WRATHImage(ivec2(m_look_up_sizes.size(), 1),
                                 common_data().m_index_format,
                                 WRATHImage::BoundarySize(),
                                 m_index_texture_allocator);
  WRATHassert(indexImage!=NULL);
  WRATHassert(indexImage->minX_minY().x()==0);

  indexImage->respecify_sub_image(0, //layer
                                  0, //LOD
                                  common_data().m_index_format[0].m_pixel_format,
                                  values,
                                  ivec2(0,0),
                                  ivec2(m_look_up_sizes.size(), 1));

  
  return indexImage;
}

WRATHImage*
WRATHTextureFontFreeType_DetailedCoverage::
create_and_set_images(std::vector<WRATHImage*> &out_images,
                      std::vector<per_pixel_size_coverage_data> &pixel_data)
{
  if(pixel_data.empty())
    {
      return NULL;
    }

  std::vector<ivec2> bitmap_sizes(pixel_data.size());
  WRATHImage *indexImage;

  for(int i=0, endi=pixel_data.size(); i<endi; ++i)
    {
      pixel_data[i].create_pixel_data();

      bitmap_sizes[i]=pixel_data[i].size();
    }

  indexImage=allocate_glyph_room(bitmap_sizes, out_images);
  if(indexImage!=NULL)
    {
      WRATHassert(bitmap_sizes.size()==out_images.size());
      for(int i=0, endi=pixel_data.size(); i<endi; ++i)
        {
          out_images[i]->respecify_sub_image(0, //LOD
                                             common_data().m_coverage_format[0].m_pixel_format,
                                             pixel_data[i].pixels(),
                                             ivec2(0,0), 
                                             bitmap_sizes[i]);
        }
    }
  
  return indexImage;
}


WRATHTextureFont::glyph_data_type*
WRATHTextureFontFreeType_DetailedCoverage::
generate_character(glyph_index_type G)
{
  std::vector<per_pixel_size_coverage_data> pixel_data(m_pixel_sizes.size());
  character_code_type C;
  ivec2 iadvance;
  ivec2 bitmap_sz, bitmap_offset;
  
  WRATHassert(m_pixel_sizes.back()==pixel_size());

  WRATHLockMutex(ttf_face()->mutex());

  
  WRATHassert(G.valid());
  C=character_code(G);

  FT_Set_Pixel_Sizes(ttf_face()->face(), pixel_size(), pixel_size());
  FT_Set_Transform(ttf_face()->face(), NULL, NULL);
  FT_Load_Glyph(ttf_face()->face(), G.value(), FT_LOAD_DEFAULT);
  FT_Render_Glyph(ttf_face()->face()->glyph, FT_RENDER_MODE_NORMAL);

  pixel_data.back().take_bitmap_data(ttf_face()->face());
  bitmap_sz=ivec2(ttf_face()->face()->glyph->bitmap.width,
                  ttf_face()->face()->glyph->bitmap.rows);
      
  bitmap_offset=ivec2(ttf_face()->face()->glyph->bitmap_left,
                      ttf_face()->face()->glyph->bitmap_top - ttf_face()->face()->glyph->bitmap.rows);
  
  iadvance=ivec2(ttf_face()->face()->glyph->advance.x,
                 ttf_face()->face()->glyph->advance.y);

  for(unsigned int i=0, endi=pixel_data.size()-1; i<endi; ++i)
    {
      FT_Set_Pixel_Sizes(ttf_face()->face(), m_pixel_sizes[i], m_pixel_sizes[i]);
      FT_Load_Glyph(ttf_face()->face(), G.value(), FT_LOAD_DEFAULT);
      FT_Render_Glyph(ttf_face()->face()->glyph, FT_RENDER_MODE_NORMAL);
      pixel_data[i].take_bitmap_data(ttf_face()->face(),
                                     m_pixel_sizes[i],
                                     pixel_size(),
                                     bitmap_offset);
    }

  WRATHUnlockMutex(ttf_face()->mutex());

  /*
    having grabbed the coverage bitmaps, now we make the WRATHImage's
    to hold the index and coverage data.
   */
  std::vector<WRATHImage*> coverage_images;
  WRATHImage* index_image(NULL);
  local_glyph_data *return_value(NULL);
  vecN<WRATHTextureChoice::texture_base::handle, 2> hnds;

  if(bitmap_sz.x()>=0 and bitmap_sz.y()>=0)
    {
      index_image=create_and_set_images(coverage_images, pixel_data);
    }
  return_value=WRATHNew local_glyph_data(coverage_images, index_image);

  if(index_image!=NULL)
    {
      WRATHassert(index_image->texture_binders().size()==1);
      hnds[1]=index_image->texture_binder(0);

      WRATHassert(!coverage_images.empty());
      WRATHassert(WRATHImage::uses_same_atlases(coverage_images.begin(), coverage_images.end()));
      WRATHassert(coverage_images[0]->texture_binders().size()==1);
      hnds[0]=coverage_images[0]->texture_binder(0);

      WRATHassert(index_image->minX_minY().x()==0);
      WRATHassert(index_image->size().x()==index_image->atlas_size().x());
      WRATHassert(index_image->minX_minY().y()<256 and index_image->minX_minY().y()>=0);
      return_value->m_custom_float_data.push_back(static_cast<float>(index_image->minX_minY().y())/255.0f);
    }
                                         
  return_value->iadvance(iadvance)
    .font(this)
    .texture_page(m_page_tracker.get_page_number(ivec2(256, 256), hnds))
    .texel_values(ivec2(0,0), bitmap_sz)
    .origin(bitmap_offset)
    .bounding_box_size(bitmap_sz+ivec2(1,1))
    .character_code(C)
    .glyph_index(G);


  return return_value;
}













const_c_array<WRATHTextureChoice::texture_base::handle>
WRATHTextureFontFreeType_DetailedCoverage::
texture_binder(int pg)
{
  return m_page_tracker.texture_binder(pg);
}

int
WRATHTextureFontFreeType_DetailedCoverage::
texture_page_data_size(void) const
{
  return 0; 
}

float
WRATHTextureFontFreeType_DetailedCoverage::
texture_page_data(int texture_page, int idx) const
{
  WRATHunused(texture_page);
  WRATHunused(idx);
  return 0.0f;
}

int
WRATHTextureFontFreeType_DetailedCoverage::
number_texture_pages(void)
{
  return m_page_tracker.number_texture_pages();
}

const WRATHTextureFont::GlyphGLSL*
WRATHTextureFontFreeType_DetailedCoverage::
glyph_glsl(void)
{
  return &common_data().m_glyph_glsl;
}

float
WRATHTextureFontFreeType_DetailedCoverage::
normalized_glyph_code_value(const glyph_data_type &G)
{
  return G.fetch_custom_float(0);
}

WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
WRATHTextureFontFreeType_DetailedCoverage::
texture_consumption(void)
{
  return common_data().m_coverage_allocator.texture_consumption();
}

void
WRATHTextureFontFreeType_DetailedCoverage::
add_additional_pixel_size(int sz)
{
  common_data().add_pixel_size(sz);
}

void
WRATHTextureFontFreeType_DetailedCoverage::
clear_additional_pixel_sizes(void)
{
  common_data().clear_pixel_size_choices();
}

void
WRATHTextureFontFreeType_DetailedCoverage::
additional_pixel_sizes(std::vector<int> &out_sizes)
{
  common_data().get_pixel_sizes(out_sizes);
}

