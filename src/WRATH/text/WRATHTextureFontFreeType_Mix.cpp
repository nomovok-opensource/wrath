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




  
  class fragmet_src_key:public boost::tuple<const WRATHTextureFont::FragmentSource*,
                                            int,
                                            const WRATHTextureFont::FragmentSource*,
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

    const WRATHTextureFont::FragmentSource*
    native_fragment_src(void)
    {
      return get<0>();
    }

    const WRATHTextureFont::FragmentSource*
    minified_fragment_src(void)
    {
      return get<2>();
    }
  };

  class fragment_source_hoard
  {
  public:
    const WRATHTextureFont::FragmentSource*
    fetch(WRATHTextureFont *a, WRATHTextureFont *b,
          WRATHTextureFontFreeType_TMixSupport::PerMixClass*);

  private:
    void
    add_block(const std::string &prefix_name,
              const WRATHTextureFont::FragmentSource* src,
              WRATHTextureFont::FragmentSource &target);

    WRATHMutex m_mutex;
    std::map<fragmet_src_key, WRATHTextureFont::FragmentSource> m_data;
  };

 

}

////////////////////////////////////////////
//fragment_source_hoard methods
void
fragment_source_hoard::
add_block(const std::string &prefix_name,
          const WRATHTextureFont::FragmentSource* src,
          WRATHTextureFont::FragmentSource &R)
{
   R.m_fragment_processor
     .add_macro("compute_coverage", prefix_name+ "compute_coverage")
     .add_macro("is_covered", prefix_name + "is_covered");

   for(int i=0, endi=src->m_fragment_processor_sampler_names.size();
        i<endi; ++i)
    {
      std::ostringstream ostr;

      ostr.str("");
      ostr << prefix_name << src->m_fragment_processor_sampler_names[i];

      R.m_fragment_processor.add_macro(src->m_fragment_processor_sampler_names[i], ostr.str());
      R.m_fragment_processor_sampler_names.push_back(ostr.str());
    }

  R.m_fragment_processor
    .absorb(src->m_fragment_processor)
    .add_source("\n#undef compute_coverage\n", WRATHGLShader::from_string)
    .add_source("\n#undef is_covered\n", WRATHGLShader::from_string);

  for(int i=0, endi=src->m_fragment_processor_sampler_names.size();
        i<endi; ++i)
    {
      std::ostringstream ostr;

      ostr.str("");
      ostr << "\n#undef " << src->m_fragment_processor_sampler_names[i]
           << "\n";
      R.m_fragment_processor.add_source(ostr.str(), WRATHGLShader::from_string);
    }
}

const WRATHTextureFont::FragmentSource*
fragment_source_hoard::
fetch(WRATHTextureFont *a, WRATHTextureFont *b,
      WRATHTextureFontFreeType_TMixSupport::PerMixClass *q)
{
  WRATHAutoLockMutex(m_mutex);

  fragmet_src_key K(a, b, q);
  std::map<fragmet_src_key, WRATHTextureFont::FragmentSource>::iterator iter;

  iter=m_data.find(K);
  if(iter!=m_data.end())
    {
      return &iter->second;
    }
  
  WRATHTextureFont::FragmentSource &R(m_data[K]);

  /*
    make the frament source, and texture values:

    
    #define MIX_FONT_SHADER pixel height of "a" divided by "b" as a float

    #define compute_coverage native_compute_coverage
    #define is_covered native_is_covered
     -- for each sampler of "a" font similar define statement
     -- include "a" source code

    #undef compute_coverage
    #undef is_covered
    #define compute_coverage minified_compute_coverage
    #define is_covered minified_is_covered
     -- for each sampler of "b" font similar define statement
     -- include "b" source code

    #undef compute_coverage
    #undef is_covered

    
    -- create is_covered and compute_coverage as check scaling factor,
       and if smaller or larger then return from ready ones.

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

  mix_font_shader_define << "\n#ifndef MIX_FONT_SHADER\n"
                         << "\n#define MIX_FONT_SHADER "
                         << std::showpoint << rr
                         << "\n#endif\n";

  R.m_pre_vertex_processor
    .add_source(mix_font_shader_define.str(), WRATHGLShader::from_string);

  
  R.m_pre_fragment_processor
    .add_source(mix_font_shader_define.str(), WRATHGLShader::from_string);


  add_block("native_",
            K.native_fragment_src(),
            R);

  /*
    insert font_common_base to have it redefine
    various macros.
   */
  R.m_fragment_processor
    .add_source("font_common_base.frag.wrath-shader.glsl",
                WRATHGLShader::from_resource);

  add_block("minified_",
            K.minified_fragment_src(),
            R);

  /*
    insert font_mix_base to have it redefine
    various macros.
   */
  R.m_fragment_processor
    .add_source("font_mix_base.frag.wrath-shader.glsl",
                WRATHGLShader::from_resource);


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


const WRATHTextureFont::FragmentSource*
WRATHTextureFontFreeType_TMixSupport::
fragment_source(WRATHTextureFont *native_fnt,
                WRATHTextureFont *minified_fnt,
                WRATHTextureFontFreeType_TMixSupport::PerMixClass *q)
{
  WRATHStaticInit();
  static fragment_source_hoard R;
  return R.fetch(native_fnt, minified_fnt, q);
}
