/*! 
 * \file WRATHFontDatabase.cpp
 * \brief file WRATHFontDatabase.cpp
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
#include "WRATHStaticInit.hpp"
#include "WRATHFontDatabase.hpp"
#include "WRATHFreeTypeSupport.hpp"

/*********************************
WRATHFontDatabase requires that some other file somewhere implements:
 
  Description: 
    Perform -some kind of- font matching so that given
    a font description, return a handle to a Font.
  Font::const_handle
  fetch_font_entry(const FontProperties &properties)
 
  Description: 
     Called only once for lifetime of program,
     its purpose it so populate the WRATHFontDatabase by calling
     fetch_font_entry(const std::string &pfilename, int pface_index)
     to add fonts to the database
  void
  populate_database(void);

  

**********************************/


/*
  We've made the friend magic so that the class
  WRATHFontDatabaseImplement is a friend of all
  those defined classes WRATHFontDatabase so that
  it can create the objects..
 */
namespace WRATHFontDatabase
{
  typedef std::pair<std::string, int>  font_key;

  void
  populate_database(void);

  class FontDatabaseImplement:boost::noncopyable
  {
  public:
    FontDatabaseImplement(void);
    ~FontDatabaseImplement();

    /*
      creates the font if necessary and returns it.
     */
    Font::const_handle
    fetch_font(const std::string &pfilename, int pindex,
               const FontMemorySource::const_handle &h,
               bool register_font);

    std::vector<Font::const_handle>
    fetch_fonts(const std::string &pfilename, 
                const FontMemorySource::const_handle &h,
                bool register_font);

    /*
      attempts a match...
     */
    Font::const_handle
    fetch_font(FontProperties P);

    enum return_code
    release_unregistered_font(const Font::const_handle &hnd);

    MetaFont*
    master_meta_font(void)
    {
      return m_all_fonts;
    }

  private:

    typedef std::map<FontProperties, MetaFont*> map_type;

    void
    place_font_entry_into_meta_fonts(const Font::handle &h);

    MetaFont*
    fetch_meta_font(const FontProperties &prop, int idx);

    void
    absorb_into_meta_font(const FontProperties &prop, int idx, 
                          const Font::handle &h);
    void
    absorb_into_meta_font(MetaFont *meta, const Font::handle &h);

    Font::const_handle
    first_font_of_meta_font(const FontProperties &prop, int idx);

    /*
      Key entries in 
         exact_match have all fields
         family_style_bold_italic_match have that m_foundry_name is empty string
         style_bold_italic_match has m_foundry_name and m_family_name as empty string
         bold_italic_match has m_foundry_name, m_family_name and m_style_name as empty string
     */

    vecN<map_type, 1+last_resort> m_meta_fonts;
    MetaFont *m_all_fonts;

    WRATHMutex m_fonts_mutex;
    std::map<font_key, Font::handle> m_fonts; 
  };

  class NaiveFontFetcher:boost::noncopyable
  {
  public:
    NaiveFontFetcher(void)
    {
      populate_database();
    }

    Font::const_handle
    fetch_font(FontProperties P);
  };
}

namespace
{
  

  WRATHFontDatabase::FontDatabaseImplement&
  font_database(void)
  {
    WRATHStaticInit();
    static WRATHFontDatabase::FontDatabaseImplement R;
    return R;
  }

  WRATHFontDatabase::NaiveFontFetcher&
  naive_font_fetcher(void)
  {
    WRATHStaticInit();
    font_database();
    static WRATHFontDatabase::NaiveFontFetcher R;
    return R;
  }
}

////////////////////////////////////////
// WRATHFontDatabase::NaiveFontFetcher methods
WRATHFontDatabase::Font::const_handle
WRATHFontDatabase::NaiveFontFetcher::
fetch_font(FontProperties P)
{
  return font_database().fetch_font(P);
}


////////////////////////////////////////
// WRATHFontDatabase::FontDatabaseImplement methodsx
WRATHFontDatabase::FontDatabaseImplement::
FontDatabaseImplement()
{
  m_all_fonts=WRATHNew MetaFont();
}

WRATHFontDatabase::FontDatabaseImplement::
~FontDatabaseImplement()
{
  for(unsigned int i=0; i<m_meta_fonts.size(); ++i)
    {
      for(map_type::iterator iter=m_meta_fonts[i].begin(), 
            end=m_meta_fonts[i].end(); iter!=end; ++iter)
        {
          WRATHDelete(iter->second);
        }
    }
  WRATHDelete(m_all_fonts);
}

enum return_code
WRATHFontDatabase::FontDatabaseImplement::
release_unregistered_font(const Font::const_handle &hnd)
{
  if(hnd.valid() and !hnd->is_registered_font())
    {
      hnd->m_signal();
      return routine_success;
    }
  return routine_fail;
}


WRATHFontDatabase::MetaFont*
WRATHFontDatabase::FontDatabaseImplement::
fetch_meta_font(const FontProperties &prop, int idx)
{
  map_type::iterator iter;

  iter=m_meta_fonts[idx].find(prop);
  if(iter!=m_meta_fonts[idx].end())
    {
      return iter->second;
    }

  MetaFont *p;

  p=WRATHNew MetaFont();
  m_meta_fonts[idx][prop]=p;

  return p;
}

std::vector<WRATHFontDatabase::Font::const_handle>
WRATHFontDatabase::FontDatabaseImplement::
fetch_fonts(const std::string &pfilename, 
            const FontMemorySource::const_handle &h,
            bool register_font)
{
  FT_Face face(NULL);
  FT_Library lib(NULL);
  int lib_error, face_error;
  std::vector<WRATHFontDatabase::Font::const_handle> H;

  lib_error=FT_Init_FreeType(&lib);
  if(lib_error)
    {
      return H;
    }

  if(!h.valid())
    {
      face_error=FT_New_Face(lib,
                             pfilename.c_str(),
                             0, &face);
    }
  else
    {
      /*
        read from memory
       */
      face_error=FT_New_Memory_Face(lib,
                                    h->data().c_ptr(),
                                    h->data().size(),
                                    0, &face);
    }

  if(face_error!=0 or face==NULL)
    {
      if(face!=NULL)
        {
          FT_Done_Face(face);
        }
      FT_Done_FreeType(lib);
      return H;
    }

  H.reserve(face->num_faces);
  for(int i=0; i<face->num_faces; ++i)
    {
      Font::const_handle R;
      R=fetch_font(pfilename, i, h, register_font);

      if(R.valid())
        {
          H.push_back(R);
        }
    }
  return H;
}

WRATHFontDatabase::Font::const_handle
WRATHFontDatabase::FontDatabaseImplement::
fetch_font(const std::string &pfilename, int pindex,
           const FontMemorySource::const_handle &h,
           bool register_font)
{
  std::map<font_key, Font::handle>::iterator iter;

  WRATHAutoLockMutex(m_fonts_mutex);

  if(register_font)
    {
      iter=m_fonts.find( font_key(pfilename, pindex));
      if(iter!=m_fonts.end())
        {
          return iter->second;
        }
    }

  Font::handle H;
  
  /*
    first attempt to get the FT_Face from freetype
   */
  FT_Face face(NULL);
  FT_Library lib(NULL);
  int lib_error, face_error;

  lib_error=FT_Init_FreeType(&lib);
  if(lib_error)
    {
      return H;
    }

  if(!h.valid())
    {
      face_error=FT_New_Face(lib,
                             pfilename.c_str(),
                             pindex, &face);
    }
  else
    {
      /*
        read from memory
       */
      face_error=FT_New_Memory_Face(lib,
                                    h->data().c_ptr(),
                                    h->data().size(),
                                    pindex, &face);
    }

  if(face_error!=0 or face==NULL or (face->face_flags&FT_FACE_FLAG_SCALABLE)==0) 
    {
      if(face!=NULL)
          {
            FT_Done_Face(face);
          }
      FT_Done_FreeType(lib);
      return H;
    }


  H=WRATHNew Font(h, pfilename, pindex, face); 
  H->m_is_registered_font=register_font;

  if(register_font)
    {
      m_fonts[font_key(pfilename, pindex)]=H;
    }
  place_font_entry_into_meta_fonts(H);

  
  FT_Done_Face(face);
  FT_Done_FreeType(lib);

  return H;
}

void
WRATHFontDatabase::FontDatabaseImplement::
place_font_entry_into_meta_fonts(const Font::handle &h)
{
  /*
    we need to place the font the font...
   */
  FontProperties temp_props(h->properties());

  /*
    first the exact match
   */
  absorb_into_meta_font(temp_props, exact_match, h);

  temp_props.m_foundry_name="";
  absorb_into_meta_font(temp_props, family_style_bold_italic_match, h);

  temp_props.m_style_name="";
  absorb_into_meta_font(temp_props, family_bold_italic_match, h);

  temp_props.m_family_name="";
  absorb_into_meta_font(temp_props, bold_italic_match, h);
  
  h->m_meta_font[last_resort]=m_all_fonts;
  absorb_into_meta_font(m_all_fonts, h);
}

void
WRATHFontDatabase::FontDatabaseImplement::
absorb_into_meta_font(const FontProperties &props, int idx, const Font::handle &h)
{
  MetaFont *meta;

  meta=fetch_meta_font(props, idx);
  h->m_meta_font[idx]=meta;

  if(h->is_registered_font())
    {
      absorb_into_meta_font(meta, h);
    }
}

void
WRATHFontDatabase::FontDatabaseImplement::
absorb_into_meta_font(MetaFont *meta, const Font::handle &h)
{
  WRATHassert(meta!=NULL);
  WRATHassert(h.valid());

  meta->add_font(h);
}

WRATHFontDatabase::Font::const_handle
WRATHFontDatabase::FontDatabaseImplement::
first_font_of_meta_font(const FontProperties &prop, int idx)
{
  map_type::iterator iter;

  iter=m_meta_fonts[idx].find(prop);
  if(iter!=m_meta_fonts[idx].end())
    {
      return iter->second->first_font();
    }

  return Font::const_handle();
}

WRATHFontDatabase::Font::const_handle
WRATHFontDatabase::FontDatabaseImplement::
fetch_font(FontProperties prop)
{
  Font::const_handle return_value;
  
  WRATHAutoLockMutex(m_fonts_mutex);
  
  return_value=first_font_of_meta_font(prop, exact_match);
  if(return_value.valid())
    {
      return return_value;
    }

  prop.m_foundry_name="";
  return_value=first_font_of_meta_font(prop, family_style_bold_italic_match);
  if(return_value.valid())
    {
      return return_value;
    }

  prop.m_style_name="";
  return_value=first_font_of_meta_font(prop, family_bold_italic_match);
  if(return_value.valid())
    {
      return return_value;
    }

  prop.m_family_name="";
  return_value=first_font_of_meta_font(prop, bold_italic_match);
  if(return_value.valid())
    {
      return return_value;
    }

  return m_all_fonts->first_font();
}

/////////////////////////////////////////////
// WRATHFontDatabase::FontProperties methods
bool
WRATHFontDatabase::FontProperties::
operator<(const FontProperties &rhs) const
{
  if(m_bold!=rhs.m_bold)
    {
      return m_bold < rhs.m_bold;
    }

  if(m_italic!=rhs.m_italic)
    {
      return m_italic < rhs.m_italic;
    }

  if(m_style_name!=rhs.m_style_name)
    {
      return m_style_name < rhs.m_style_name;
    }

  if(m_family_name!=rhs.m_family_name)
    {
      return m_family_name < rhs.m_family_name;
    }

  if(m_foundry_name!=rhs.m_foundry_name)
    {
      return m_foundry_name < rhs.m_foundry_name;
    }

  return false;
}

////////////////////////////////////////////
// WRATHFontDatabase::MetaFont methods
WRATHFontDatabase::MetaFont::
MetaFont(void)
{}

void
WRATHFontDatabase::MetaFont::
add_font(const Font::const_handle &hnd)
{
  WRATHAutoLockMutex(m_mutex);
  if(m_font_set.insert(hnd).second)
    {
      //only append to the list
      //and fire the signal if the
      //font was not already present.
      m_font_list.push_back(hnd);
      m_signal(hnd);
    }
}



WRATHFontDatabase::MetaFont::connect_t
WRATHFontDatabase::MetaFont::
connect_and_append(const signal_t::slot_type &subscriber, int gp_order, 
                   std::list<Font::const_handle> &out_list) const
{
  WRATHAutoLockMutex(m_mutex);
  std::copy(m_font_list.begin(), m_font_list.end(),
            std::inserter(out_list, out_list.end()) );
  return m_signal.connect(gp_order, subscriber);
}

////////////////////////////////////////////
// WRATHFontDatabase::Font methods
WRATHFontDatabase::Font::
Font(const FontMemorySource::const_handle &h,
     const std::string &pfilename, int pindex, FT_Face pface):
  m_memory_source(h),
  m_filename(pfilename), 
  m_face_index(pindex)
{
  std::ostringstream ostr;

  ostr << m_filename << ":" << m_face_index;
  m_label=ostr.str();

  /*
    get the values for m_properties from pface.
   */
  if(pface->family_name!=NULL)
    {
      m_properties.m_family_name=pface->family_name;
    }

  if(pface->style_name!=NULL)
    {
      m_properties.m_style_name=pface->style_name;
    }

  m_properties.m_bold= (pface->style_flags & FT_STYLE_FLAG_BOLD);
  m_properties.m_italic= (pface->style_flags & FT_STYLE_FLAG_ITALIC);

  /*
    the foundry name is a touch hosed from Freetype,
    so we don't get it and the foundry name is empty.
    sighs.
   */

}


////////////////////////////////////////////
// WRATHFontDatabase methods
namespace WRATHFontDatabase
{
  
  Font::const_handle
  fetch_font_entry(const std::string &pfilename, int pface_index,
                   const FontMemorySource::const_handle &h)
  {
    return font_database().fetch_font(pfilename, pface_index, h, true);
  }

  std::vector<Font::const_handle>
  fetch_font_entries(const std::string &pfilename,
                     const FontMemorySource::const_handle &h)
  {
    return font_database().fetch_fonts(pfilename, h, true);
  }

  Font::const_handle
  create_unregistered_font(const std::string &pfilename, int pface_index,
                           const FontMemorySource::const_handle &h)
  {
    return font_database().fetch_font(pfilename, pface_index, h, false);
  }

  
  std::vector<Font::const_handle>
  create_unregistered_fonts(const std::string &pname, 
                            const FontMemorySource::const_handle &h)
  {
    return font_database().fetch_fonts(pname, h, false);
  }

    
  Font::const_handle
  fetch_font_entry_naive(const FontProperties &properties)
  {
    return naive_font_fetcher().fetch_font(properties);
  }

  const MetaFont*
  master_meta_font(void)
  {
    return font_database().master_meta_font();
  }

  enum return_code
  release_unregistered_font(const Font::const_handle &hnd)
  {
    return font_database().release_unregistered_font(hnd);
  }

}
