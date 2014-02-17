/*! 
 * \file WRATHTextureFontFreeType.hpp
 * \brief file WRATHTextureFontFreeType.hpp
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




#ifndef WRATH_HEADER_TEXTURE_FONT_FreeType_HPP_
#define WRATH_HEADER_TEXTURE_FONT_FreeType_HPP_

#include "WRATHConfig.hpp"
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "WRATHFreeTypeSupport.hpp"
#include "c_array.hpp"
#include "WRATHTextureFont.hpp"
#include "WRATHNew.hpp"
#include "vectorGL.hpp"
#include "WRATHTextureFontUtil.hpp"
#include "WRATHFontDatabase.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\class WRATHTextureFontFreeType
  The class WRATHTextureFontFreeType is a conveniant
  base class for texture font classes which fetch
  their font data from an FT_Face. The class
  implements the virtual methods:
  - glyph_data(glyph_index_type) 
  - glyph_index(character_code_type) 
  - character_code(glyph_index_type)
  - number_glyphs(void) 
  - kerning_offset(glyph_index_type, glyph_index_type)
  - new_line_height(void)

  A derived class from WRATHTextureFontFreeType needs to implement
  the protected method:
  - generate_character() which creates the glyph_data_type object for a given glyph code

  In addition, a derived class needs to implement the following
  pure virtual methods from WRATHTextureFont:
  - texture_size(int)
  - texture_binder(int)
  - number_texture_pages(void)
  - fragment_source(void)
  
 */
class WRATHTextureFontFreeType:public WRATHTextureFont
{
public:
  /*!\fn WRATHTextureFontFreeType
    Ctor.
    \param pface handle to a WRATHFreeTypeSupport::LockableFace whose
                 underlying FT_Face is used to create the data 
                 of the WRATHTextureFontFreeType_Coverage.
                 Note! Each time a glyph is created the
                 data within the FT_Face will be modified.

    \param presource_name specifies the WRATHFontDatabase::Font source
                          and the pixel size, becomes the key for the font.

    \param pfetcher function pointer with which to construct
                    sibling fonts, i.e texture fonts of the exact 
                    same type sourced from different 
                    WRATHFontDatabase::Font::const_handle
                    objects
   */
  WRATHTextureFontFreeType(WRATHFreeTypeSupport::LockableFace::handle pface, 
                           const WRATHTextureFontKey &presource_name,
                           font_fetcher_t pfetcher):
    WRATHTextureFont(presource_name, pfetcher),
    m_glyph_data(this, pface)
  {
    m_new_line_height=m_glyph_data.new_line_height(pixel_size());
  }

  virtual
  ~WRATHTextureFontFreeType()
  {}
    
  virtual
  const glyph_data_type&
  glyph_data(glyph_index_type glyph)
  {
    glyph_data_type *ptr;

    ptr=m_glyph_data.data(glyph);
    return (ptr!=NULL)?
      *ptr:
      empty_glyph();
  }

  virtual
  glyph_index_type
  glyph_index(character_code_type C)
  {
    return m_glyph_data.glyph_index(C);
  }

  virtual
  int
  number_glyphs(void)
  {
    return m_glyph_data.number_glyphs();
  }

  virtual
  ivec2
  kerning_offset(glyph_index_type left_glyph,
                 glyph_index_type right_glyph)
  {
    return m_glyph_data.kerning_offset(pixel_size(), left_glyph, right_glyph);    
  }
  
  virtual
  float
  new_line_height(void)
  {
    return m_new_line_height;
  }

  /*!\fn WRATHFreeTypeSupport::LockableFace::handle ttf_face
    Returns the FT_Face that this 
    WRATHTextureFontFreeType is using.
   */
  WRATHFreeTypeSupport::LockableFace::handle
  ttf_face(void) 
  {
    return m_glyph_data.face();
  }

  /*!\fn generate_all_glyphs
    Generates the texture data for all glyphs
    stored in the FT_Face of the font.
    \param show_progress if true, print to std::cout
                         a progress bar.
   */
  int
  generate_all_glyphs(bool show_progress=true)
  {
    return m_glyph_data.generate_all_glyphs(show_progress);
  }

  virtual
  character_code_type
  character_code(glyph_index_type G)
  {
    return m_glyph_data.character_code(G);
  }
  
protected:
  
  /*!\fn glyph_data_type* generate_character
    To be implemented by a derived class to create
    a glyph_data_type for the named glyph_index_type
    value. The returned object will be deleted
    by the WRATHTextureFontFreeType deconstructor.
    \param G glyph index code of glyph to generate
   */
  virtual
  glyph_data_type*
  generate_character(glyph_index_type G)=0;

  /*!\fn WRATHFreeTypeSupport::CharacterMapSupport<glyph_data_type>::Stats glyph_data_stats
    Returns the stats of the underlying 
    glyph data collection used by the
    WRATHTextureFontFreeType object.
    to hold the glyph data.
   */
  WRATHFreeTypeSupport::CharacterMapSupport<glyph_data_type>::Stats
  glyph_data_stats(void)
  {
    return m_glyph_data.stats();
  }

private:

  class LocalCharacterMapSupport:
    public WRATHFreeTypeSupport::CharacterMapSupport<glyph_data_type>
  {
  public:
    explicit
    LocalCharacterMapSupport(WRATHTextureFontFreeType *p, 
                             WRATHFreeTypeSupport::LockableFace::handle pface):
      WRATHFreeTypeSupport::CharacterMapSupport<glyph_data_type>(pface),
      m_master(p)
    {}

    ~LocalCharacterMapSupport()
    {}

    virtual
    glyph_data_type*
    generate_data(glyph_index_type G)
    {
      return m_master->generate_character(G);
    }

  private:
    WRATHTextureFontFreeType *m_master;
  };

  
  float m_new_line_height;
  LocalCharacterMapSupport m_glyph_data;
};


/*!\class WRATHTextureFontFreeTypeT
  Provides a the static method
  \code
  WRATHTextureFont* fetch_font(int, const WRATHFontDatabase::Font::const_handle&)
  \endcode
  and provides the WRATHTextureFont::font_fetcher_t arugment
  for the ctor to WRATHTextureFontFreeType.
  The use pattern of the template class is:
  \code
  class FontClass:public WRATHTextureFontFreeTypeT<FontClass>
  {
    //methods, etc
  };
  \endcode
  
 */
template<typename F>
class WRATHTextureFontFreeTypeT:public WRATHTextureFontFreeType
{
public:
  /*!\fn WRATHTextureFontFreeTypeT
    Ctor. Echoes parameters to \ref WRATHTextureFontFreeType ctor
    \param pface passed to WRATHTextureFontFreeType ctor
    \param presource_name passed to WRATHTextureFontFreeType ctor
   */
  WRATHTextureFontFreeTypeT(WRATHFreeTypeSupport::LockableFace::handle pface, 
                            const WRATHTextureFontKey &presource_name):
    WRATHTextureFontFreeType(pface, presource_name, fetch_font)
  {}

  /*!\fn WRATHTextureFont* fetch_font(int, const WRATHFontDatabase::Font::const_handle&)
    Returns, and if necessary creates, a WRATHTextureFont
    of type F.
    \param psize pixel size of returned font
    \param fnt data from which to generate font glyphs
   */
  static
  WRATHTextureFont*
  fetch_font(int psize, const WRATHFontDatabase::Font::const_handle &fnt)
  {
    return WRATHFreeTypeSupport::fetch_font<F>(psize, fnt);
  }
  
  /*!\fn WRATHTextureFont* fetch_font(int, const std::string&, int)
    Returns, and if necessary creates, a WRATHTextureFont
    of type F.
    \param psize pixel size of returned font
    \param pfilename filename of font data
    \param pface_index index into file if multiple fonts are within the file
   */
  static
  WRATHTextureFont*
  fetch_font(int psize, const std::string &pfilename, int pface_index=0)
  {
    return WRATHFreeTypeSupport::fetch_font<F>(psize, pfilename, pface_index);
  }

};

/*! @} */

#endif
