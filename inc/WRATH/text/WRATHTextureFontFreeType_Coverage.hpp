/*! 
 * \file WRATHTextureFontFreeType_Coverage.hpp
 * \brief file WRATHTextureFontFreeType_Coverage.hpp
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




#ifndef WRATH_HEADER_TEXTURE_FONT_FreeType_COVERAGE_HPP_
#define WRATH_HEADER_TEXTURE_FONT_FreeType_COVERAGE_HPP_


#include "WRATHConfig.hpp"
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "WRATHFreeTypeSupport.hpp"
#include "c_array.hpp"
#include "WRATHTextureFontFreeType.hpp"
#include "WRATHTextureFontUtil.hpp"
#include "WRATHNew.hpp"
#include "vectorGL.hpp"
#include "WRATHImage.hpp"

/*! \addtogroup Text
 * @{ 
 */

/*!\class WRATHTextureFontFreeType_Coverage
  A WRATHTextureFontFreeType_Coverage uses Freetype2
  to create a mipmapped texture. Each mipmap
  holds coverage values computed by FreeType2.
  The textures of a WRATHTextureFontFreeType_Coverage
  are GL_LUMINANCE or GL_RED textures (GL_LUMINANCE is 
  not supported in GL core profiles, but either way 
  the .r component gives the value).

  Class is thread safe, i.e. glyphs (of the 
  same font) can be generated on a seperate thread 
  than the rendering thread and multiple glyphs may
  also be generated at the same time from multiple threads.

  WRATHTextureFontFreeType_Coverage inherits from
  WRATHTextureFont, thus is a resource managed object, 
  i.e. that class has a WRATHResourceManager,
  see \ref WRATH_RESOURCE_MANAGER_DECLARE. Although 
  WRATHTextureFontFreeType_Coverage objects can be 
  modified and created from threads outside of the GL 
  context, they must only be deleted from within the 
  GL context. In particular the resource manager may 
  only be cleared from within the GL context.
*/
class WRATHTextureFontFreeType_Coverage:
  public WRATHTextureFontFreeTypeT<WRATHTextureFontFreeType_Coverage>
{
public:
  using WRATHTextureFont::glyph_data;

  enum
    {
      /*!
        Enumeration to indicate that the font 
        is NOT scalable.
       */
      font_scalability_value=font_is_not_scalable
    };

  /*!\fn WRATHTextureFontFreeType_Coverage
    Ctor for WRATHTextureFontFreeType_Coverage, it is HIGHLY advised
    to use fetch_font() to create/get fonts from files.
    The ctor is exposed publically for situations where
    one is handed a FT_Face that from which one wishes 
    to create a WRATHTextureFontFreeType_Coverage.
    \param pface handle to a WRATHFreeTypeSupport::LockableFace whose
                 underlying FT_Face is used to create the data 
                 of the WRATHTextureFontFreeType_Coverage.
                 Note! Each time a glyph is created the
                 data within the FT_Face will be modified.

    \param presource_name specifies the WRATHFontDatabase::Font source
                          and the pixel size, becomes the key for the font.
   */
  WRATHTextureFontFreeType_Coverage(WRATHFreeTypeSupport::LockableFace::handle pface,  
                                    const WRATHTextureFontKey &presource_name);
  
  virtual
  ~WRATHTextureFontFreeType_Coverage();

  

 
  virtual
  const_c_array<WRATHTextureChoice::texture_base::handle>
  texture_binder(int texture_page);

  virtual
  int
  glyph_custom_float_data_size(void) const
  {
    return 0;
  }

  virtual
  int
  texture_page_data_size(void) const;

  virtual
  float
  texture_page_data(int texture_page, int idx) const;

  virtual
  int
  number_texture_pages(void);

  virtual
  const GlyphGLSL*
  glyph_glsl(void);

  /*!\fn int total_pixel_waste
    Returns the number of pixels on the texture
    of this WRATHTextureFontFreeType_Coverage that are "allocated"
    beyond what would be stored for the raw
    bitmap from FreeType.
   */
  int
  total_pixel_waste(void) const
  {
    return m_total_pixel_waste;
  }

  /*!\fn int total_pixel_use
    Returns the total number of pixels allocated
    on this WRATHTextureFontFreeType_Coverage.
   */
  int
  total_pixel_use(void) const
  {
    return m_total_pixel_use;
  }
  
  /*!\fn GLenum minification_filter(void)
    Returns the minification filter used
    for the next created WRATHTextureFontFreeType_Coverage
    font. Acceptable values are the values
    accepted by glTexParameter with GL_MIN_FILTER,
    i.e. one of GL_LINEAR, GL_NEAREST,
    GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_NEAREST,
    GL_LINEAR_MIPMAP_LINEAR or GL_LINEAR_MIPMAP_LINEAR.
    The default value is GL_LINEAR_MIPMAP_NEAREST.

    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  GLenum
  minification_filter(void);

  /*!\fn void minification_filter(GLenum)
    Sets the minification filter used
    for the next created WRATHTextureFontFreeType_Coverage
    font. Acceptable values are the values
    accepted by glTexParameter with GL_MIN_FILTER,
    i.e. one of GL_LINEAR, GL_NEAREST,
    GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_NEAREST,
    GL_LINEAR_MIPMAP_LINEAR or GL_LINEAR_MIPMAP_LINEAR.
    The default value is GL_LINEAR_MIPMAP_NEAREST.
    
    Method is thread safe and may be called from
    multiple threads safely.

    \param v value to use.
   */
  static
  void
  minification_filter(GLenum v);

  /*!\fn GLenum magnification_filter(void)
    Returns the magnification filter used
    for the next created WRATHTextureFontFreeType_Coverage
    font. Acceptable values are the values
    accepted by glTexParameter with GL_MAG_FILTER,
    i.e. one of GL_LINEAR or GL_NEAREST.
    The defualt value is GL_LINEAR.
    
    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  GLenum
  magnification_filter(void);

  /*!\fn void magnification_filter(GLenum)
    Returns the magnification filter used
    for the next created WRATHTextureFontFreeType_Coverage
    font. Acceptable values are the values
    accepted by glTexParameter with GL_MAG_FILTER,
    i.e. one of GL_LINEAR or GL_NEAREST.
    The defualt value is GL_LINEAR.
    
    Method is thread safe and may be called from
    multiple threads safely.

    \param v value to use.
   */
  static
  void
  magnification_filter(GLenum v);


  /*!\fn bool force_power2_texture(void)
    Returns if the next WRATHTextureFontFreeType_Coverage
    will or will not force the texture size to
    always be a power of 2.
    
    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  bool 
  force_power2_texture(void);

  /*!\fn void force_power2_texture(bool)
    Set if the next WRATHTextureFontFreeType_Coverage
    will or will not force the texture size to
    always be a power of 2, default is true.
    
    Method is thread safe and may be called from
    multiple threads safely.

    \param v value to use.
   */
  static
  void 
  force_power2_texture(bool v);
 
  /*!\fn GLint texture_creation_size(void)
    Gets the size of the textures
    used by WRATHTextureFontFreeType_Coverage. 
    The default value is 1024. 
    
    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  GLint
  texture_creation_size(void);

  /*!\fn void texture_creation_size(GLint)
    Sets the size of the textures
    used by WRATHTextureFontFreeType_Coverage. 
    A user must make sure that the passed value
    does not exceed GL_MAX_TEXTURE_SIZE.
    The default value is 1024.  
    
    Method is thread safe and may be called from
    multiple threads safely.

    \param v value to which to set texture_creation_size(void)
   */
  static
  void
  texture_creation_size(GLint v);

  /*!\fn GLint effective_texture_creation_size
    Returns the effecitve texture size for
    a font page taking into account
    force_power2_texture(). 
    
    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  GLint
  effective_texture_creation_size(void);
  
  /*!\fn int mipmap_slacking_threshhold_level(void)
    When the minification filter is 
    GL_LINEAR_MIPMAP_NEAREST or
    GL_LINEAR_MIPMAP_LINEAR, empty
    boundary texels are added to mipmaps
    up to the level indicated by
    mipmap_slacking_threshhold_level(void).
    Default value is 1, i.e. add slack
    as needed for mipmap level 1.
    force_power2_texture(). 
    
    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  int
  mipmap_slacking_threshhold_level(void);

  /*!\fn void mipmap_slacking_threshhold_level(int)
    When the minification filter is 
    GL_LINEAR_MIPMAP_NEAREST or
    GL_LINEAR_MIPMAP_LINEAR, empty
    boundary texels are added to mipmaps
    up to the level indicated by
    mipmap_slacking_threshhold_level(void).
    Default value is 1, i.e. add slack
    as needed for mipmap level 1.
    
    Method is thread safe and may be called from
    multiple threads safely.

    \param v value to which to set mipmap_slacking_threshhold_level(void).
   */
  static
  void
  mipmap_slacking_threshhold_level(int v);

  /*!\fn WRATHImage::TextureAllocatorHandle::texture_consumption_data_type texture_consumption
    Returns the texture utilization of all 
    WRATHTextureFontFreeType_Coverage objects.
   */
  static
  WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
  texture_consumption(void);

private:
  
  enum
    {
      number_textures_per_page=1
    };

  class glyph_mipmap_level
  {
  public:
    glyph_mipmap_level(void):
      m_size(0,0),
      m_raw_size(0,0),
      m_raw_pitch(0)
    {}
    
    void
    take_bitmap_data(FT_Face fc);

    void
    create_pixel_data(ivec2 sz);

    const ivec2&
    size(void)
    {
      return m_size;
    }

    const ivec2&
    raw_size(void)
    {
      return m_raw_size;
    }

    std::vector<uint8_t>&
    pixels(void)
    {
      return m_pixels;
    }

  private:
    ivec2 m_size;
    ivec2 m_raw_size;
    int m_raw_pitch;
    std::vector<uint8_t> m_raw_pixels_from_freetype;
    std::vector<uint8_t> m_pixels;
  };
   
  void
  ctor_init(void);
  
  void
  on_create_texture_page(ivec2 texture_size,
                         std::vector<float> &custom_data);

 
  glyph_data_type*
  generate_character(glyph_index_type G);

  WRATHImage*
  create_glyph(std::vector<glyph_mipmap_level> &pdata);
 
  GLenum m_minification_filter, m_magnification_filter;
  bool m_use_mipmaps;
  int m_mipmap_deepness_concern;

  WRATHTextureFontUtil::TexturePageTracker m_page_tracker;
 
  int m_total_pixel_waste, m_total_pixel_use;
  

};

/*! @} */

#endif
