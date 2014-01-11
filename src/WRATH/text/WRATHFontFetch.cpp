/*! 
 * \file WRATHFontFetch.cpp
 * \brief file WRATHFontFetch.cpp
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
#include "WRATHFontFetch.hpp"
#include "WRATHTextureFontFreeType_Mix.hpp"


namespace
{
  class LocalDatum:boost::noncopyable
  {
  public:
    
    LocalDatum(void):
      m_fetcher(&WRATHMixFontTypes<WRATHTextureFontFreeType_Distance>::mix::fetch_font),
      m_pixel_size(64)
    {
      /*
        make sure that the WRATHFontDatabase is filled, do this by 
        using fetch_font_entry_naive().
       */
      m_default_font=WRATHFontFetch::fetch_font_entry_naive(WRATHFontFetch::FontProperties()
                                                            .family_name("DejaVuSans")) ;
    }

    WRATHFontFetch::font_fetcher_t 
    fetcher(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return m_fetcher;
    }

    void
    fetcher(WRATHFontFetch::font_fetcher_t v)
    {
      WRATHAutoLockMutex(m_mutex);
      if(v!=NULL)
        {
          m_fetcher=v;
        }
    }

    int
    pixel_size(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return m_pixel_size;
    }

    void
    pixel_size(int v)
    {
      WRATHAutoLockMutex(m_mutex);
      m_pixel_size=v;
    }

    WRATHFontFetch::font_handle
    default_font(void)
    {
      WRATHAutoLockMutex(m_mutex);
      return m_default_font;
    }

    void
    default_font(const WRATHFontFetch::font_handle &v)
    {
      WRATHAutoLockMutex(m_mutex);
      m_default_font=v;
    }

  private:      
    WRATHMutex m_mutex;
    WRATHFontFetch::font_fetcher_t m_fetcher;
    int m_pixel_size;
    WRATHFontFetch::font_handle m_default_font;
  };

  LocalDatum&
  datum(void) 
  {
    WRATHStaticInit();
    static LocalDatum R;
    return R;
  }

}

void
WRATHFontFetch::
font_fetcher(font_fetcher_t v)
{
  datum().fetcher(v);
}

WRATHFontFetch::font_fetcher_t
WRATHFontFetch::
font_fetcher(void)
{
  return datum().fetcher();
}

void
WRATHFontFetch::
default_font_pixel_size(int v)
{
  datum().pixel_size(v);
}

int
WRATHFontFetch::
default_font_pixel_size(void)
{
  return datum().pixel_size();
}
  
void
WRATHFontFetch::
default_font(const font_handle &v)
{
  datum().default_font(v);
}

WRATHFontFetch::font_handle
WRATHFontFetch::
default_font(void)
{
  return datum().default_font();
}

WRATHTextureFont*
WRATHFontFetch::
fetch_font(int psize, const font_handle &fnt, font_fetcher_t v)
{
  if(v==NULL)
    {
      v=font_fetcher();
    }
  WRATHassert(v!=NULL);
  return v(psize, fnt);
}
