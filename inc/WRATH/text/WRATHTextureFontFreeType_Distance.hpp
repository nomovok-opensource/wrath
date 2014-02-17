/*! 
 * \file WRATHTextureFontFreeType_Distance.hpp
 * \brief file WRATHTextureFontFreeType_Distance.hpp
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




#ifndef WRATH_HEADER_TEXTURE_FONT_FreeType_DISTANCE_HPP_
#define WRATH_HEADER_TEXTURE_FONT_FreeType_DISTANCE_HPP_




/*! \addtogroup Text
 * @{
 */

#include "WRATHConfig.hpp"
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "WRATHFreeTypeSupport.hpp"
#include "c_array.hpp"
#include "WRATHTextureFontFreeType.hpp"
#include "WRATHNew.hpp"
#include "vectorGL.hpp"
#include "WRATHTextureFontUtil.hpp"

/*!\class WRATHTextureFontFreeType_Distance

  A WRATHTextureFontFreeType_Distance holds a distance
  texture texture for font rendering. The textures
  holding the distance values are in GL_LUMINANCE
  or GL_RED format (GL_LUMINANCE is not supported
  in GL core profiles, but either way the .r component
  gives the value).

  Class is thread safe, i.e. glyphs (of the 
  same font) can be generated on a seperate thread 
  than the rendering thread and multiple glyphs may
  also be generated at the same time from multiple threads.

  WRATHTextureFontFreeType_Distance inherits from
  WRATHTextureFont, thus is a resource managed object, 
  i.e. that class has a WRATHResourceManager,
  see \ref WRATH_RESOURCE_MANAGER_DECLARE. Although 
  WRATHTextureFontFreeType_Distance objects can be 
  modified and created from threads outside of the GL 
  context, they must only be deleted from within the 
  GL context. In particular the resource manager may 
  only be cleared from within the GL context.
 */
class WRATHTextureFontFreeType_Distance:
  public WRATHTextureFontFreeTypeT<WRATHTextureFontFreeType_Distance>
{
public:

  enum
    {
      /*!
        Enumeration to indicate that the font 
        is scalable.
       */
      font_scalability_value=font_is_scalable
    };

  /*!\enum fill_rule_type
  
    Enumeration type to govern how to compute
    if a texel is inside out outside the glyph.
   */
  enum fill_rule_type
    {
      /*!
        Compute the windining number
        of the texel center, if the
        winding number is non-zero then
        the texel is considered inside.
        The computation time for the
        non-zero winding rule is only
        slightly higher than odd-even
        fill rule.
       */
      non_zero_winding_rule,

      /*!
        Count the number of intersections
        from a ray starting at the texel
        center. If the count is odd, then
        the texel center is considered inside
        the glyph. For glyphs whose contours
        intersect, this rule can give incorrect 
        results.
       */
      odd_even_rule,

      /*!
        Use FreeType to render the glyph to
        a bitmap, if the coverage of the texel
        is atleast 50%, then the texel is 
        considered to be inside the glyph.
       */
      freetype_render
    };
  
  /*!\fn WRATHTextureFontFreeType_Distance
  
    Ctor for WRATHTextureFontFreeType_Distance, it is HIGHLY advised
    to use fetch_font() to create/get fonts from files.
    The ctor is exposed publically for situations where
    one is handed a FT_Face that from which one wishes 
    to create a WRATHTextureFontFreeType_Distance.
    \param pface handle to a WRATHFreeTypeSupport::LockableFace whose
                 underlying FT_Face is used to create the data 
                 of the WRATHTextureFontFreeType_Distance.
                 Note! Each time a glyph is created the
                 data within the FT_Face will be modified.

    \param presource_name specifies the WRATHFontDatabase::Font source
                          and the pixel size, becomes the key for the font.
   */
  WRATHTextureFontFreeType_Distance(WRATHFreeTypeSupport::LockableFace::handle pface,
                                    const WRATHTextureFontKey &presource_name);


  
  
  virtual
  ~WRATHTextureFontFreeType_Distance();

  virtual
  const_c_array<WRATHTextureChoice::texture_base::handle>
  texture_binder(int texture_page);

  virtual
  int
  texture_page_data_size(void) const;

  virtual
  int
  glyph_custom_float_data_size(void) const
  {
    return 0;
  }

  virtual
  float
  texture_page_data(int texture_page, int idx) const;

  virtual
  int
  number_texture_pages(void);

  virtual
  const GlyphGLSL*
  glyph_glsl(void);
      
  /*!\fn GLint texture_creation_size(void)
    Gets the maximum size of the textures
    used by WRATHTextureFontFreeType_Distance.
    The default value is 1024. 

    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  GLint
  texture_creation_size(void);

  /*!\fn void texture_creation_size(GLint)
    Sets the maximum size of the textures
    used by WRATHTextureFontFreeType_Distance.
    The default value is 1024.

    Method is thread safe and may be called from
    multiple threads safely.
    \param v new value to use
   */
  static
  void
  texture_creation_size(GLint v);

  /*!\fn float max_L1_distance(void)
    Returns the maximum distance value used 
    by the next WRATHTextureFontFreeType_Distance to be 
    created, this value gives the maximum
    unnormalized distance that is allowed,
    the default value is 96.0f. 
    Note that each pixel is 64.0 units wide,
    thus 96.0f corresponds to 1 and a half
    pixels away.

    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  float
  max_L1_distance(void);

  /*!\fn void max_L1_distance(float)
    Sets the maximum distance value used 
    by the next WRATHTextureFontFreeType_Distance to be 
    created, this value gives the maximum
    unnormalized distance that is allowed,
    the default value is 96.0f. 
    Note that each pixel is 64.0 units wide,
    thus 96.0f corresponds to 1 and a half
    pixels away.

    Method is thread safe and may be called from
    multiple threads safely.

    \param v new value to use
   */
  static
  void
  max_L1_distance(float v);

  /*!\fn bool force_power2_texture(void)
    Returns if the next WRATHTextureFontFreeType_Distance
    will or will not force the texture size to
    always be a power of 2.

    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  bool 
  force_power2_texture(void);

  /*!\fn void force_power2_texture(bool)
    Set if the next WRATHTextureFontFreeType_Distace
    will or will not force the texture size to
    always be a power of 2, default is false.

    Method is thread safe and may be called from
    multiple threads safely.

    \param v new value to use
   */
  static
  void 
  force_power2_texture(bool v);

  /*!\fn effective_texture_creation_size
  
    Returns the effecitve texture size for
    a font page taking into account
    both GL_MAX_TEXTURE_SIZE and 
    force_power2_texture().

    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  GLint
  effective_texture_creation_size(void);

  /*!\fn void fill_rule(enum fill_rule_type)
    Sets the fill rule to use for the next
    created WRATHTextureFontFreeType_Distance
    will use to compute if a texel is inside 
    or outside the glyph, default value is 
    non_zero_winding_rule, see also \ref 
    fill_rule_type. 

    Method is thread safe and may be called from
    multiple threads safely.

    \param v fill rule to which to use for the
             next constructed WRATHTextureFontFreeType_Distance 
   */
  static
  void
  fill_rule(enum fill_rule_type v);

  /*!\fn enum fill_rule_type fill_rule(void)
    Returns the value last set in 
    \ref fill_rule(enum fill_rule_type).
    
    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  enum fill_rule_type
  fill_rule(void);

  /*!\fn WRATHImage::TextureAllocatorHandle::texture_consumption_data_type texture_consumption
    Returns the texture utilization of all 
    WRATHTextureFontFreeType_Distance objects.
   */
  static
  WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
  texture_consumption(void);

private:
  
  enum
    {
      number_textures_per_page=1
    };

  void
  ctor_init(void);

  void
  on_create_texture_page(ivec2 texture_size,
                         std::vector<float> &custom_data);
  
  /*
    Note: performs an std::swap with
    pdata on success, i.e. no data
    copy.
    */
  WRATHImage*
  create_glyph(std::vector<uint8_t> &pdata, const ivec2 &sz);

  glyph_data_type*
  generate_character(glyph_index_type G);

  float m_max_distance;

  enum fill_rule_type m_fill_rule;

  WRATHTextureFontUtil::TexturePageTracker m_page_tracker;
};
/*! @} */


#endif
