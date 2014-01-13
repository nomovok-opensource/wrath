/*! 
 * \file WRATHTextureFontFreeType_Mix.cpp
 * \brief file WRATHTextureFontFreeType_Mix.cpp
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
#include "WRATHUtil.hpp"
#include "WRATHTextureFontFreeType_Mix.hpp"
#include "WRATHStaticInit.hpp"
#include <boost/shared_ptr.hpp>

namespace
{
  class PerMixClassHoard
  {
  public:

    PerMixClassHoard(void)
    {
    }

    ~PerMixClassHoard(void)
    {
      
      WRATHAutoLockMutex(m_mutex);
      for(map_type::iterator iter=m_map.begin(),
            end=m_map.end(); iter!=end; ++iter)
        {
          WRATHDelete(iter->second);
        }
    }

    WRATHTextureFontFreeType_TMixSupport::PerMixClass&
    datum(const std::type_info &tp)
    {      
      map_type::iterator iter;
      
      WRATHAutoLockMutex(m_mutex);
      
      iter=m_map.find(tp);
      if(iter==m_map.end())
        {
          std::pair<map_type::iterator, bool> R;
          value_type p;
          
          p=WRATHNew WRATHTextureFontFreeType_TMixSupport::PerMixClass();
          R=m_map.insert(map_type::value_type(tp, p));
          WRATHassert(R.second);
          
          iter=R.first;
        }
      return *iter->second;
    }


  private:
    
    typedef WRATHTextureFontFreeType_TMixSupport::PerMixClass *value_type;
    typedef WRATHUtil::TypeInfoSortable key_type;

    typedef std::map<key_type, value_type> map_type;
    map_type m_map;   
    WRATHMutex m_mutex;
  };




  
  class fragmet_src_key:public boost::tuple<const WRATHTextureFont::GlyphGLSL*,
                                            int,
                                            const WRATHTextureFont::GlyphGLSL*,
                                            int,
                                            WRATHTextureFontFreeType_TMixSupport::PerMixClass*>
  {
  public:
    fragmet_src_key(WRATHTextureFont *a, 
                    WRATHTextureFont *b,
                    WRATHTextureFontFreeType_TMixSupport::PerMixClass *q)
    {
      get<0>()=a->glyph_glsl();
      get<1>()=a->pixel_size();
      get<2>()=b->glyph_glsl();
      get<3>()=b->pixel_size();
      get<4>()=q;
    }

    WRATHTextureFontFreeType_TMixSupport::PerMixClass&
    datum(void)
    {
      return *get<4>();
    }

    const WRATHTextureFont::GlyphGLSL*
    native_glyph_glsl(void)
    {
      return get<0>();
    }

    const WRATHTextureFont::GlyphGLSL*
    minified_glyph_glsl(void)
    {
      return get<2>();
    }
  };

  class glyph_glsl_hoard
  {
  public:
    const WRATHTextureFont::GlyphGLSL*
    fetch(WRATHTextureFont *a, WRATHTextureFont *b,
          WRATHTextureFontFreeType_TMixSupport::PerMixClass*,
          int, int, int);

  private:
    void
    add_glsl_wrath_font_page_data_original_function(WRATHTextureFont::GlyphGLSL &R);

    void
    add_block(const std::string &prefix_name,
              const WRATHTextureFont::GlyphGLSL* src,
              WRATHTextureFont::GlyphGLSL &target,
              int font_page_data_offset);

    void
    add_aliases_to_block(const std::string &prefix_name,
                         const std::vector<std::string> &src,
                         std::vector<std::string> &out_list,
                         WRATHTextureFont::GlyphGLSL &dest);

    void
    remove_aliases(const std::vector<std::string> &src,
                   WRATHTextureFont::GlyphGLSL &dest);
                         

    WRATHMutex m_mutex;
    std::map<fragmet_src_key, WRATHTextureFont::GlyphGLSL> m_data;
  };

 

}

////////////////////////////////////////////
//glyph_glsl_hoard methods
void
glyph_glsl_hoard::
add_aliases_to_block(const std::string &prefix_name,
                     const std::vector<std::string> &src,
                     std::vector<std::string> &out_list,
                     WRATHTextureFont::GlyphGLSL &dest)
{
  for(int i=0, endi=src.size(); i<endi; ++i)
    {
      std::ostringstream ostr;
      
      ostr.str("");
      ostr << prefix_name << src[i];
      for(int t=0; t<WRATHTextureFont::GlyphGLSL::num_linearity_types; ++t)
        {
          dest.m_fragment_processor[t].add_macro(src[i], ostr.str());
          dest.m_vertex_processor[t].add_macro(src[i], ostr.str());
        }
      out_list.push_back(ostr.str());
    }
}

void
glyph_glsl_hoard::
remove_aliases(const std::vector<std::string> &src,
               WRATHTextureFont::GlyphGLSL &dest)
{
  for(int i=0, endi=src.size(); i<endi; ++i)
    {
      for(int t=0; t<WRATHTextureFont::GlyphGLSL::num_linearity_types; ++t)
        {
          dest.m_fragment_processor[t].remove_macro(src[i]);
          dest.m_vertex_processor[t].remove_macro(src[i]);
        }
    }
}

void
glyph_glsl_hoard::
add_glsl_wrath_font_page_data_original_function(WRATHTextureFont::GlyphGLSL &R)
{
  for(int i=0;i<WRATHTextureFont::GlyphGLSL::num_linearity_types;++i)
    {
      const char *alias=
        "\nfloat wrath_font_page_data_original_function(in int idx)"
        "{ return wrath_font_page_data(idx); }\n";

      R.m_fragment_processor[i].add_source(alias, WRATHGLShader::from_string);
      R.m_vertex_processor[i].add_source(alias, WRATHGLShader::from_string);
    }
}


void
glyph_glsl_hoard::
add_block(const std::string &prefix_name,
          const WRATHTextureFont::GlyphGLSL* src,
          WRATHTextureFont::GlyphGLSL &R,
          int font_page_data_offset)
{
  for(int i=0;i<WRATHTextureFont::GlyphGLSL::num_linearity_types;++i)
    {
      R.m_fragment_processor[i]
        .add_macro("compute_coverage", prefix_name + "compute_coverage")
        .add_macro("is_covered", prefix_name + "is_covered");
      
      R.m_vertex_processor[i]
        .add_macro("pre_compute_glyph", prefix_name + "pre_compute_glyph");
    }

   add_aliases_to_block(prefix_name, 
                        src->m_sampler_names,
                        R.m_sampler_names,
                        R);

   add_aliases_to_block(prefix_name, 
                        src->m_global_names,
                        R.m_global_names,
                        R);

   


  for(int i=0;i<WRATHTextureFont::GlyphGLSL::num_linearity_types;++i)
    {
      R.m_fragment_processor[i]
        .add_macro("wrath_font_page_data", prefix_name + "wrath_font_page_data")
        .add_macro("WRATH_MIX_FONT_PAGE_DATA_OFFSET", font_page_data_offset)
        .add_source("font_mix_page_data_func.wrath-shader.glsl", WRATHGLShader::from_resource)
        .remove_macro("WRATH_MIX_FONT_PAGE_DATA_OFFSET")
        .absorb(src->m_fragment_processor[i])
        .remove_macro("compute_coverage")
        .remove_macro("is_covered")
        .remove_macro("wrath_font_page_data");
      
      R.m_vertex_processor[i]
        .add_macro("wrath_font_page_data", prefix_name + "wrath_font_page_data")
        .add_macro("WRATH_MIX_FONT_PAGE_DATA_OFFSET", font_page_data_offset)
        .add_source("font_mix_page_data_func.wrath-shader.glsl", WRATHGLShader::from_resource)
        .remove_macro("WRATH_MIX_FONT_PAGE_DATA_OFFSET")
        .absorb(src->m_vertex_processor[i])
        .remove_macro("pre_compute_glyph")
        .remove_macro("wrath_font_page_data");
    }

   remove_aliases(R.m_global_names, R);
   remove_aliases(R.m_sampler_names, R);

   R.m_global_names.push_back(prefix_name + "wrath_font_page_data");
   
}

const WRATHTextureFont::GlyphGLSL*
glyph_glsl_hoard::
fetch(WRATHTextureFont *a, WRATHTextureFont *b,
      WRATHTextureFontFreeType_TMixSupport::PerMixClass *q,
      int glyph_custom_mix_data_size,
      int glyph_custom_native_start,
      int glyph_custom_minified_start)
{
  WRATHAutoLockMutex(m_mutex);

  fragmet_src_key K(a, b, q);
  std::map<fragmet_src_key, WRATHTextureFont::GlyphGLSL>::iterator iter;

  iter=m_data.find(K);
  if(iter!=m_data.end())
    {
      return &iter->second;
    }
  
  WRATHTextureFont::GlyphGLSL &R(m_data[K]);

  /*
    make the frament source, and texture values:

    
    const float wrath_mix_font_ratio pixel height of "a" divided by "b" as a float

    #define compute_coverage wrath_native_compute_coverage
    #define is_covered wrath_native_is_covered
    #define wrath_font_page_data wrath_native_wrath_font_page_data
     -- for each sampler and global symbol of "a" font, similar define statement
     -- insert custom wrath_native_wrath_font_page_data() function
     -- include "a" source code

    #undef compute_coverage
    #undef is_covered
    #define compute_coverage wrath_minified_compute_coverage
    #define is_covered wrath_minified_is_covered
    #define wrath_font_page_data minified_wrath_font_page_data
     -- for each sampler and global symbol of "b" font, similar define statement
     -- insert custom wrath_minified_wrath_font_page_data() function
     -- include "b" source code

    #undef compute_coverage
    #undef is_covered

    
    

   */

  std::ostringstream mix_font_shader_ratio;
  float rr;

  rr=static_cast<float>(a->pixel_size()) / static_cast<float>(b->pixel_size());
  rr/=K.datum().minified_font_inflate_factor();

  mix_font_shader_ratio << "\nconst float wrath_mix_font_ratio="
                        << std::showpoint << rr << ";"
                        << "\nconst float wrath_mix_font_ratio_square="
                        << std::showpoint << rr*rr << ";";

  R.m_global_names.push_back("wrath_mix_font_ratio");
  R.m_global_names.push_back("wrath_mix_font_ratio_square");

  add_glsl_wrath_font_page_data_original_function(R);
  add_block("wrath_native_", K.native_glyph_glsl(), R, glyph_custom_native_start);
  add_block("wrath_minified_", K.minified_glyph_glsl(), R, glyph_custom_minified_start);

  
  
  R.m_vertex_processor[WRATHTextureFont::GlyphGLSL::linear_glyph_position]
    .add_source(mix_font_shader_ratio.str(), WRATHGLShader::from_string)
    .add_source("font_mix_linear.vert.wrath-shader.glsl",
                WRATHGLShader::from_resource);
  
  R.m_fragment_processor[WRATHTextureFont::GlyphGLSL::linear_glyph_position]
    .add_source(mix_font_shader_ratio.str(), WRATHGLShader::from_string)
    .add_source("font_mix_linear.frag.wrath-shader.glsl",
                WRATHGLShader::from_resource);
  
  R.m_vertex_processor[WRATHTextureFont::GlyphGLSL::nonlinear_glyph_position]
    .add_source(mix_font_shader_ratio.str(), WRATHGLShader::from_string)
    .add_source("font_mix_nonlinear.vert.wrath-shader.glsl",
                WRATHGLShader::from_resource);
  
  R.m_fragment_processor[WRATHTextureFont::GlyphGLSL::nonlinear_glyph_position]
    .add_source(mix_font_shader_ratio.str(), WRATHGLShader::from_string)
    .add_source("font_mix_nonlinear.frag.wrath-shader.glsl",
                WRATHGLShader::from_resource);


  /*
    texture page size data is just the union
    of the sources.
   */
  R.m_texture_page_data_size= 
    K.native_glyph_glsl()->m_texture_page_data_size
    + K.minified_glyph_glsl()->m_texture_page_data_size;

  /*
    custom data of a glyph is:
     - custom data used by mix font for
       making minified font functions work (glyph_custom_mix_data_size)
     - native custom data
     - minified custom data

    m_custom_data_use is an array of indices,
    the data packed into the uniform array is
    just the values indicated by m_custom_data_use
   */
  R.m_custom_data_use.resize(glyph_custom_mix_data_size 
                             + K.native_glyph_glsl()->m_custom_data_use.size()
                             + K.minified_glyph_glsl()->m_custom_data_use.size());

  /*
    custom data used by mix font for
    making minified font functions work
   */
  for(int i=0; i<glyph_custom_mix_data_size; ++i)
    {
      R.m_custom_data_use[i]=i;
      R.m_custom_data_use[i]=i;
    }

  /*
    now the values wanted from a->m_custom_data_use(),
    note that we increase the index by 
    -glyph_custom_mix_data_size- because
    that is where the custom data it located.
   */
  for(int i=0, endi=K.native_glyph_glsl()->m_custom_data_use.size(); i<endi; ++i)
    {
      R.m_custom_data_use[i+glyph_custom_mix_data_size]=
        K.native_glyph_glsl()->m_custom_data_use[i] 
        + glyph_custom_mix_data_size;
    }

  /*
    now get the values from b->m_custom_data_use(),
    note that the index is increated by 4+a_glyph_custom_float_data_size.
   */
  int a_glyph_custom_float_data_size(a->glyph_custom_float_data_size());

  for(int i=glyph_custom_mix_data_size+K.native_glyph_glsl()->m_custom_data_use.size(), 
        j=0, endj=K.minified_glyph_glsl()->m_custom_data_use.size(); 
      j<endj; ++i, ++j)
    {
      R.m_custom_data_use[i]=K.minified_glyph_glsl()->m_custom_data_use[j] 
        + glyph_custom_mix_data_size + a_glyph_custom_float_data_size;
    }
  

  return &R;
}


////////////////////////////////////////////
//WRATHTextureFontFreeType_TMixSupport methods
WRATHTextureFontFreeType_TMixSupport::PerMixClass&
WRATHTextureFontFreeType_TMixSupport::
datum(const std::type_info &tp)
{
  WRATHStaticInit();
  static PerMixClassHoard v;
  return v.datum(tp);
}


const WRATHTextureFont::GlyphGLSL*
WRATHTextureFontFreeType_TMixSupport::
glyph_glsl(WRATHTextureFont *native_fnt,
           WRATHTextureFont *minified_fnt,
           WRATHTextureFontFreeType_TMixSupport::PerMixClass *q,
           int glyph_custom_mix_data_size,
           int glyph_custom_native_start,
           int glyph_custom_minified_start)
{
  WRATHStaticInit();
  static glyph_glsl_hoard R;
  return R.fetch(native_fnt, minified_fnt, q,
                 glyph_custom_mix_data_size, 
                 glyph_custom_native_start, 
                 glyph_custom_minified_start);
}
