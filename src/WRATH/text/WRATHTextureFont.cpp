/*! 
 * \file WRATHTextureFont.cpp
 * \brief file WRATHTextureFont.cpp
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
#include "WRATHTextureFont.hpp"
#include "WRATHFontDatabase.hpp"
#include "WRATHatomic.hpp"

namespace
{

  class MetaTextureFont:boost::noncopyable
  {
  public:
    typedef WRATHTextureFont::glyph_index_type glyph_index_type;
    typedef WRATHTextureFont::character_code_type character_code_type;
    typedef WRATHTextureFont::font_glyph_index font_glyph_index;
    

    explicit
    MetaTextureFont(const WRATHFontDatabase::MetaFont &psrc,
                    WRATHTextureFont::font_fetcher_t pfetcher,
                    int ppixel_size);

    ~MetaTextureFont();

    font_glyph_index
    fetch(character_code_type ch);

  private:
    typedef std::map<character_code_type, font_glyph_index> map_type;

    void //called when mutex is ALREADY locked
    on_font_add(WRATHFontDatabase::Font::const_handle);

    void
    flush_dirty_list(void);

    const WRATHFontDatabase::MetaFont &m_meta_font;
    WRATHTextureFont::font_fetcher_t m_fetcher;
    int m_pixel_size;

    WRATHMutex m_mutex;
    WRATHFontDatabase::MetaFont::connect_t m_connection;
    map_type m_map;
    std::list<WRATHFontDatabase::Font::const_handle> m_fonts_to_register;
  };

  class MetaTextureFontCollection:boost::noncopyable
  {
  public:
    typedef boost::tuple<int, 
                         WRATHTextureFont::font_fetcher_t,
                         const WRATHFontDatabase::MetaFont*> key;

    ~MetaTextureFontCollection();

    MetaTextureFont*
    fetch(const key &K);

  private:
    typedef std::map<key, MetaTextureFont*> map_type;
    
    WRATHMutex m_mutex;
    map_type m_map;
  };

  MetaTextureFontCollection&
  meta_texture_font_collection(void)
  {
    WRATHStaticInit();
    static MetaTextureFontCollection R;
    return R;
  }

  MetaTextureFont*
  fetch_meta_texture_font(int pixel_size,
                          WRATHTextureFont::font_fetcher_t fetcher,
                          const WRATHFontDatabase::MetaFont *pfont)
  {
    MetaTextureFontCollection::key K(pixel_size, fetcher, pfont);
    return meta_texture_font_collection().fetch(K);
  }

  MetaTextureFont*
  fetch_meta_texture_font(int pixel_size,
                          WRATHTextureFont::font_fetcher_t fetcher,
                          WRATHFontDatabase::Font::const_handle fnt,
                          int IDX)
  {
    enum WRATHFontDatabase::meta_font_matching v;

    v=static_cast<enum WRATHFontDatabase::meta_font_matching>(IDX);
    return fetch_meta_texture_font(pixel_size, fetcher,
                                   fnt->meta_font(v));
  }
}


/////////////////////////////////////////////
// MetaTextureFontCollection methods
MetaTextureFontCollection::
~MetaTextureFontCollection(void)
{
  WRATHAutoLockMutex(m_mutex);
  for(map_type::iterator iter=m_map.begin(),
        end=m_map.end(); iter!=end; ++iter)
    {
      WRATHDelete(iter->second);
    }
}

MetaTextureFont*
MetaTextureFontCollection::
fetch(const key &K)
{
  if(K.get<2>()==NULL)
    {
      return NULL;
    }

  WRATHAutoLockMutex(m_mutex);

  map_type::iterator iter;
  MetaTextureFont *ptr;

  iter=m_map.find(K);
  if(iter!=m_map.end())
    {
      return iter->second;
    }

  ptr=WRATHNew MetaTextureFont(*K.get<2>(), K.get<1>(), K.get<0>());
  m_map[K]=ptr;

  return ptr;
  
}


//////////////////////////////////////////////
// MetaTextureFont methods
MetaTextureFont::
MetaTextureFont(const WRATHFontDatabase::MetaFont &psrc,
                WRATHTextureFont::font_fetcher_t pfetcher,
                int ppixel_size):
  m_meta_font(psrc),
  m_fetcher(pfetcher),
  m_pixel_size(ppixel_size)
{
  WRATHAutoLockMutex(m_mutex);
  m_connection=
    m_meta_font.connect_and_append(boost::bind(&MetaTextureFont::on_font_add,
                                               this, _1),
                                   m_fonts_to_register);
}

MetaTextureFont::
~MetaTextureFont()
{
  m_connection.disconnect();
}

void
MetaTextureFont::
on_font_add(WRATHFontDatabase::Font::const_handle fnt)
{
  WRATHAutoLockMutex(m_mutex);
  m_fonts_to_register.push_back(fnt);
}

void
MetaTextureFont::
flush_dirty_list(void)
{
  for(std::list<WRATHFontDatabase::Font::const_handle>::iterator
        iter=m_fonts_to_register.begin(), end=m_fonts_to_register.end();
      iter!=end; ++iter)
    {
      WRATHTextureFont *fnt;

      fnt=m_fetcher(m_pixel_size, *iter);
      for(int G=0, endG=fnt->number_glyphs(); G<endG; ++G)
        {
          glyph_index_type gl(G);
          character_code_type ch;

          ch=fnt->character_code(gl);
          if(ch.m_value!=0)
            {
              m_map[ch]=font_glyph_index(fnt, gl);
            }
        }
    }

  m_fonts_to_register.clear();
}

MetaTextureFont::font_glyph_index
MetaTextureFont::
fetch(character_code_type ch)
{
  WRATHAutoLockMutex(m_mutex);
  flush_dirty_list();
  
  map_type::iterator iter;
  iter=m_map.find(ch);
  if(iter!=m_map.end())
    {
      return iter->second;
    }

  return font_glyph_index(NULL, glyph_index_type());
}


////////////////////////////////////
//WRATHTextureFont::sub_primitive_attribute methods
void
WRATHTextureFont::sub_primitive_attribute::
set(const WRATHTextureFont::glyph_data_type &in_glyph,
    int relative_native_texel_coordinate_x,
    int relative_native_texel_coordinate_y)
{
  vec2 as_float(static_cast<float>(relative_native_texel_coordinate_x),
                static_cast<float>(relative_native_texel_coordinate_y));

  ivec2 sz(in_glyph.texel_size());

  vec2 sz_as_float(std::max(1, sz.x()), 
                   std::max(1, sz.y()) );

  as_float.x()/=sz_as_float.x();
  as_float.y()/=sz_as_float.y();

  m_position_within_glyph_coordinate=as_float;
  
  m_texel_coordinates=in_glyph.texel_lower_left()
    + ivec2(relative_native_texel_coordinate_x, 
            relative_native_texel_coordinate_y);
}

//////////////////////////////////
// WRATHTextureFont methods
WRATH_RESOURCE_MANAGER_IMPLEMENT(WRATHTextureFont, WRATHTextureFontKey);

WRATHTextureFont::
WRATHTextureFont(const WRATHTextureFontKey &pname,
                 WRATHTextureFont::font_fetcher_t pfetcher):
  m_name(pname),
  m_fetcher(pfetcher),
  m_use_count(0),
  m_source_font_deleted(0)
{
  WRATHassert(m_name.get<0>().valid());

  resource_manager().add_resource(m_name, this);
  m_empty_glyph.font(this);

  for(unsigned int i=0; i<m_meta_texture_font.size(); ++i)
    {
      m_meta_texture_font[i]=fetch_meta_texture_font(pixel_size(),
                                                     m_fetcher,
                                                     source_font(), i);
    }

  if(!source_font()->is_registered_font())
    {
      m_connect=source_font()->connect_unregistered_delete(boost::bind(&WRATHTextureFont::on_font_delete, this));
    }

#ifdef WRATHDEBUG
  m_self=this;
#endif
}

WRATHTextureFont::
~WRATHTextureFont()
{
  m_connect.disconnect();
  m_dtor_signal();
  resource_manager().remove_resource(this);

  /*
    TODO: what a mess. If the glyphs of this WRATHTextureFont
    are in used by a MetaTextureFont, we need to remove
    using of this WRATHTextureFont's glyphs from the 
    MetaTextureFont. On the otherhand, fonts generally do
    not get deleted...
   */

#ifdef WRATHDEBUG
  m_self=NULL;
#endif
}

void
WRATHTextureFont::
on_font_delete(void)
{
  WRATHassert(!source_font()->is_registered_font());

  m_connect.disconnect(); //make sure on_font_delete() is called only once.
  /*
    m_source_font_deleted is initialized as 0,
    adding one makes it true.
   */
  WRATHAtomicAddAndFetch(&m_source_font_deleted, 1);
  if(WRATHAtomicAddAndFetch(&m_use_count, 0)==0)
    {
      WRATHDelete(this);
    }
}

WRATHTextureFont::font_glyph_index
WRATHTextureFont::
glyph_index_meta(WRATHTextureFont::character_code_type ch)
{
  font_glyph_index return_value(this, glyph_index(ch));
   
  for(unsigned int i=0; !return_value.second.valid() and i<m_meta_texture_font.size(); ++i)
    {
      MetaTextureFont *fnt;
      fnt=static_cast<MetaTextureFont*>(m_meta_texture_font[i]);

      if(fnt!=NULL)
        {
          return_value=fnt->fetch(ch);
        }
    }

  if(!return_value.second.valid())
    {
      return_value=font_glyph_index(this, glyph_index_type(0));
    }
  return return_value;
}

ivec2
WRATHTextureFont::
kerning_offset(std::pair<WRATHTextureFont*, glyph_index_type> left_glyph,
               std::pair<WRATHTextureFont*, glyph_index_type> right_glyph)
{
  if(left_glyph.first==right_glyph.first and left_glyph.first!=NULL)
    {
      return left_glyph.first->kerning_offset(left_glyph.second, right_glyph.second);
    }
  return ivec2(0,0);
}


void
WRATHTextureFont::
increment_use_count(void)
{
  WRATHAtomicAddAndFetch(&m_use_count, 1);
  on_increment_use_count();
}

void
WRATHTextureFont::
decrement_use_count(void)
{
  int c;

  on_decrement_use_count();

  c=WRATHAtomicSubtractAndFetch(&m_use_count, 1);
  if(c==0 and WRATHAtomicAddAndFetch(&m_source_font_deleted, 0)>0)
    {
      WRATHDelete(this);
    }
}
