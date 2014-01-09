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
           WRATHTextureFontFreeType_TMixSupport::PerMixClass *q)
{
  return native_fnt->glyph_glsl();
}
