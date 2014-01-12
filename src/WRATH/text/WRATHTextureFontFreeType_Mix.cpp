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
      get<0>()=a->fragment_source();
      get<1>()=a->pixel_size();
      get<2>()=b->fragment_source();
      get<3>()=b->pixel_size();
      get<4>()=q;
    }

    WRATHTextureFontFreeType_TMixSupport::PerMixClass&
    datum(void)
    {
      return *get<4>();
    }

    const WRATHTextureFont::GlyphGLSL*
    native_fragment_src(void)
    {
      return get<0>();
    }

    const WRATHTextureFont::GlyphGLSL*
    minified_fragment_src(void)
    {
      return get<2>();
    }
  };

  class fragment_source_hoard
  {
  public:
    const WRATHTextureFont::GlyphGLSL*
    fetch(WRATHTextureFont *a, WRATHTextureFont *b,
          WRATHTextureFontFreeType_TMixSupport::PerMixClass*);

  private:
    void
    glsl_wrath_font_page_data_original_function(WRATHTextureFont::GlyphGLSL &R);

    void
    add_block(const std::string &prefix_name,
              const WRATHTextureFont::GlyphGLSL* src,
              WRATHTextureFont::GlyphGLSL &target);

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
//fragment_source_hoard methods
void
fragment_source_hoard::
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
fragment_source_hoard::
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
fragment_source_hoard::
glsl_wrath_font_page_data_original_function(WRATHTextureFont::GlyphGLSL &R)
{
  for(int i=0;i<WRATHTextureFont::GlyphGLSL::num_linearity_types;++i)
    {
      const char *alias=
        "\nfloat wrath_font_page_data_original_function(in int idx)"
        "{ return wrath_font_page_data(idx); }\n";

      R.m_fragment_processor[i].add_source(alias, WRATHGLShader::from_string);
      R.m_fragment_processor[i].add_source(alias, WRATHGLShader::from_string);
    }
}


void
fragment_source_hoard::
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
      
      R.m_vertex_process[i]
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
        .add_macro("wrath_font_page_data", prefix + "wrath_font_page_data")
        .add_macro("WRATH_MIX_FONT_PAGE_DATA_OFFSET", font_page_data_offset)
        .add_source("font_mix_page_data_func.wrath-shader.glsl", WRATHGLShader::from_resource)
        .remove_macro("WRATH_MIX_FONT_PAGE_DATA_OFFSET")
        .absorb(src->m_fragment_processor)
        .remove_macro("compute_coverage")
        .remove_macro("is_covered")
        .remove_macro("wrath_font_page_data");
      
      R.m_vertex_processor[i]
        .add_macro("wrath_font_page_data", prefix + "wrath_font_page_data")
        .add_macro("WRATH_MIX_FONT_PAGE_DATA_OFFSET", font_page_data_offset)
        .add_source("font_mix_page_data_func.wrath-shader.glsl", WRATHGLShader::from_resource)
        .remove_macro("WRATH_MIX_FONT_PAGE_DATA_OFFSET")
        .absorb(src->m_fragment_processor)
        .remove_macro("pre_compute_glyph")
        .remove_macro("wrath_font_page_data");
    }

   remove_aliases(R.m_global_names, R);
   remove_aliases(R.m_sampler_names, R);
   
}

const WRATHTextureFont::GlyphGLSL*
fragment_source_hoard::
fetch(WRATHTextureFont *a, WRATHTextureFont *b,
      WRATHTextureFontFreeType_TMixSupport::PerMixClass *q)
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

    
    #define MIX_FONT_SHADER pixel height of "a" divided by "b" as a float

    #define compute_coverage wrath_native_compute_coverage
    #define is_covered wrath_native_is_covered
    #define wrath_font_page_data wrath_native_wrath_font_page_data
     -- for each sampler and global symbol of "a" font similar define statement
     -- insert custom wrath_native_wrath_font_page_data() function
     -- include "a" source code

    #undef compute_coverage
    #undef is_covered
    #define compute_coverage wrath_minified_compute_coverage
    #define is_covered wrath_minified_is_covered
    #define wrath_font_page_data minified_wrath_font_page_data
     -- for each sampler and global symbol of "b" font similar define statement
     -- insert custom wrath_minified_wrath_font_page_data() function
     -- include "b" source code

    #undef compute_coverage
    #undef is_covered

    
    

   */

  /*
    make inclusion of MIX_FONT_SHADER conditional
    so that if a custom shader wishes to use a different
    value than the ratio of the sizes it can.....
   */
  std::ostringstream mix_font_shader_define;
  float rr;

  rr=static_cast<float>(a->pixel_size()) / static_cast<float>(b->pixel_size());
  rr/=K.datum().minified_font_inflate_factor();

  mix_font_shader_define << std::showpoint << rr;

  glsl_wrath_font_page_data_original_function(R);
  add_block("wrath_native_", K.native_fragment_src(), R);
  add_block("wrath_minified_", K.minified_fragment_src(), R);

  /*
    insert font_mix_base to have it implement the necessary
    functions.

    Danger: if there is a mix of a mix, then MIX_FONT_SHADER
    will already be defined, that is.. bad.
   */
  R.m_fragment_processor
    .add_macro("WRATH_MIX_FONT_SHADER", mix_font_shader_define.str())
    .add_source("font_mix_base.frag.wrath-shader.glsl",
                WRATHGLShader::from_resource)
    .remove_macro("WRATH_MIX_FONT_SHADER");


  /*
    texture page size data is just the union
    of the sources.
   */
  R.m_texture_page_data_size= a->m_texture_page_data_size
    + b->m_texture_page_data_size;

  /*
    custom data of a glyph is:
     - bottom left (2 floats)
     - glyph size (2 floats)
     - native custom data
     - minified custom data

    m_custom_data_use is an array of indices,
    the data packed into the uniform array is
    just the values indicated by m_custom_data_use
   */
  R.m_custom_data_use.resize(4 + K.native_fragment_src()->m_custom_data_use
                             + K.minified_fragment_src()->m_custom_data_use);

  /*
    bottom left and glyph size
   */
  R[0]=0;
  R[1]=1;
  R[2]=2;
  R[3]=3;

  /*
    now the values wanted from a->m_custom_data_use(),
    note that we increase the index by -4- because
    that is where the custom data it located.
   */
  for(int i=0, endi=K.native_fragment_src()->m_custom_data_use.size(); i<endi; ++i)
    {
      R[i+4]= K.native_fragment_src()->m_custom_data_use[i] + 4;
    }

  /*
    now get the values from b->m_custom_data_use(),
    note that the index is increated by 4+a_glyph_custom_float_data_size.
   */
  int a_glyph_custom_float_data_size(a->glyph_custom_float_data_size());

  for(int i=4+K.native_fragment_src()->m_custom_data_use.size(), j=0;
      endj=K.minified_fragment_src()->m_custom_data_use.size(); j<endj; ++i, ++j)
    {
      R[i]=K.minified_fragment_src()->m_custom_data_use[j] 
        + 4 + a_glyph_custom_float_data_size;
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
fragment_source(WRATHTextureFont *native_fnt,
                WRATHTextureFont *minified_fnt,
                WRATHTextureFontFreeType_TMixSupport::PerMixClass *q)
{
  WRATHStaticInit();
  static fragment_source_hoard R;
  return R.fetch(native_fnt, minified_fnt, q);
}
