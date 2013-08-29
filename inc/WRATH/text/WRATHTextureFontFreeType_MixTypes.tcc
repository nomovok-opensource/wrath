/*  -*- C++ -*- */

/*! 
 * \file WRATHTextureFontFreeType_MixTypes.tcc
 * \brief file WRATHTextureFontFreeType_MixTypes.tcc
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



#if !defined(__WRATH_TEXTURE_FONT_FreeType_MIX_HPP__)  || defined(__WRATH_TEXTURE_FONT_FreeType_MIX_TYPES_TCC__)
#error "Direction inclusion of private header file WRATHTextureFontFreeType_MixType.tcc"
#endif


#define __WRATH_TEXTURE_FONT_FreeType_MIX_TYPES_TCC__
#include "WRATHConfig.hpp"

#include <typeinfo>
#include "WRATHTextureFont.hpp"

namespace WRATHTextureFontFreeType_TMixSupport
{
  class PerMixClass:boost::noncopyable
  {
  public:
    PerMixClass(void):
      m_minified_font_inflate_factor(1.0),
      m_default_size_divider(4.0)
    {}

    float
    minified_font_inflate_factor(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return m_minified_font_inflate_factor;
    }

    void
    minified_font_inflate_factor(float v)
    {
      WRATHAutoLockMutex(m_mutex);
      m_minified_font_inflate_factor=v;
    }

    float
    default_size_divider(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return m_default_size_divider;
    }

    void
    default_size_divider(float v)
    {
      WRATHAutoLockMutex(m_mutex);
      m_default_size_divider=v;
    }

  private:
    WRATHMutex m_mutex;
    float m_minified_font_inflate_factor;
    float m_default_size_divider;
  };


  PerMixClass&
  datum(const std::type_info &tp);

  const WRATHTextureFont::FragmentSource*
  fragment_source(WRATHTextureFont *native_fnt,
                  WRATHTextureFont *minified_fnt,
                  PerMixClass*);
                  
}