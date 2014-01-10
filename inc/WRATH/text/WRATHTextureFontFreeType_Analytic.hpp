/*! 
 * \file WRATHTextureFontFreeType_Analytic.hpp
 * \brief file WRATHTextureFontFreeType_Analytic.hpp
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




#ifndef __WRATH_TEXTURE_FONT_FreeType_ANALYTIC_HPP__
#define __WRATH_TEXTURE_FONT_FreeType_ANALYTIC_HPP__

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

/*! \addtogroup Text
 * @{
 */


/*!\class WRATHTextureFontFreeType_Analytic
  A WRATHTextureFontFreeType_Analytic stores outline 
  data as normal vectors and offsets into it's 
  textures, such drawing of fonts is more expensive 
  but has fewer artifacts under _extreme_ 
  magnification than coverage or distance textures
  exhibit. In particular, corners are always sharp.
  Additionally, the texture data is NOT filtered.

  There are three different modes for such
  fonts:
  - 2 RGBA8 textures. One texture is used to store
    line normals, the second holds offsets for
    the line and a "hack" to produce relative
    pixel coordinates.
  - 1 RGBA8 and 1 LA_16F (2 channel 16bit float)
    The RGBA8 texture stores the line normals,
    the second holds the offsets in glyph coordinates,
    eliminating a potentially numerically instable
    hack.
  - 1 RGBA8 and 1 LA_32F (2 channel 32bit float)
    The RGBA8 texture stores the line normals,
    the second holds the offsets in glyph coordinates,
    eliminating a potentially numerically instable
    hack.

  None of these methods use filtered texture lookups,
  thus the first two require 8 bytes to be looked up
  and the last requires 12 bytes to be looked up. 
  WRATHTextureFontFreeType_Analytic effectively tesselates
  all curves of each glyph into line segements at the
  resolution of the glyph, hence under high magnification
  or if the pixel height is small, rounded glyphs
  look less curvy.

  Class is thread safe, i.e. glyphs (of the 
  same font) can be generated on a seperate thread 
  than the rendering thread and multiple glyphs may
  also be generated at the same time from multiple threads.

  WRATHTextureFontFreeType_Analytic inherits from
  WRATHTextureFont, thus is a resource managed object, 
  i.e. that class has a WRATHResourceManager,
  see \ref WRATH_RESOURCE_MANAGER_DECLARE. Although 
  WRATHTextureFontFreeType_Analytic objects can be 
  modified and created from threads outside of the GL 
  context, they must only be deleted from within the 
  GL context. In particular the resource manager may 
  only be cleared from within the GL context.
 */
class WRATHTextureFontFreeType_Analytic:
  public WRATHTextureFontFreeTypeT<WRATHTextureFontFreeType_Analytic>
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

  /*!\enum texture_mode_type
  
    Enumeration to describe which texture
    mode is in use.
   */
  enum texture_mode_type
    {
      /*!
        Use pixel local coordinate "hack",
        i.e. 2 RGBA8 textures
       */
      local_pixel_coordinates=0,

      /*!
        Do not use the pixel local 
        coordinate "hack", instead use
        a 16-bit floating point value,
        thus one RGBA8 and one 2-channel 
        16bit floating point texture
       */
      global_pixel_coordinates_16bit=1,

      /*!
        Do not use the pixel local 
        coordinate "hack",  instead use
        a 32-bit floating point value,
        thus one RGBA8 and one 2-channel 
        32bit floating point
        texture
       */
      global_pixel_coordinates_32bit=2,
    };
  
  virtual
  ~WRATHTextureFontFreeType_Analytic();

  /*!\fn WRATHTextureFontFreeType_Analytic
    Ctor for WRATHTextureFontFreeType_Analytic, it is HIGHLY advised
    to use fetch_font() to create/get fonts from files.
    The ctor is exposed publically for situations where
    one is handed a FT_Face that from which one wishes 
    to create a WRATHTextureFontFreeType_Analytic.
    \param pface handle to a WRATHFreeTypeSupport::LockableFace whose
                 underlying FT_Face is used to create the data 
                 of the WRATHTextureFontFreeType_Analytic.
                 Note! Each time a glyph is created the
                 data within the FT_Face will be modified.

    \param presource_name specifies the WRATHFontDatabase::Font source
                          and the pixel size, becomes the key for the font.

   */
  WRATHTextureFontFreeType_Analytic(WRATHFreeTypeSupport::LockableFace::handle pface,
                                    const WRATHTextureFontKey &presource_name);

 
  virtual
  ivec2
  texture_size(int texture_page);

  
  virtual
  const_c_array<WRATHTextureChoice::texture_base::handle>
  texture_binder(int texture_page);

  virtual
  int
  number_texture_pages(void);

  virtual
  const GlyphGLSL*
  glyph_glsl(void);

  
  /*!\fn enum texture_mode_type texture_mode
    Returns the texture mode (see \ref texture_mode_type
    and creation_texture_mode()) used to create
    this WRATHTextureFontFreeType_Analytic.
   */
  enum texture_mode_type
  texture_mode(void)
  {
    return m_texture_mode;
  }  

  /*!\fn GLint texture_creation_size(void)
    Gets the maximum size  of the textures
    used by WRATHTextureFontFreeType_Analytic, this value
    is min'ed with GL_MAX_TEXTURE_SIZE,
    the default value is 1024. 
   */
  static
  GLint
  texture_creation_size(void);

  /*!\fn void texture_creation_size(GLint)
    Sets the maximum size of the textures
    used by WRATHTextureFontFreeType_Analytic, this value
    is min'ed with GL_MAX_TEXTURE_SIZE,
    the default value is 1024. 
    \param v size to which to set the texture size
             of the next created WRATHTextureFontFreeType_Analytic
             object.
   */
  static
  void
  texture_creation_size(GLint v);

  /*!\fn void creation_texture_mode(enum texture_mode_type)
    Sets the texture type (see \ref texture_mode_type)
    for the next WRATHTextureFontFreeType_Analytic
    created. The default value is local_pixel_coordinates.
    \param v value to use
   */
  static
  void
  creation_texture_mode(enum texture_mode_type v);

  /*!\fn enum texture_mode_type creation_texture_mode(void)
    Gets the texture type (see \ref texture_mode_type)
    for the next WRATHTextureFontFreeType_Analytic
    created. The default value is local_pixel_coordinates.
   */
  static
  enum texture_mode_type
  creation_texture_mode(void);

  /*!\fn void generate_sub_quads(bool)
    Sets if the next created WRATHTextureFontFreeType_Analytic
    will also generate sub-primitives for its glyph data,
    see \ref WRATHTextureFont::glyph_data_type::sub_primitive_attributes(void)
    and \ref WRATHTextureFont::glyph_data_type::sub_primitive_indices(void).
    Default value is false.
    \param v value to use
   */
  static
  void
  generate_sub_quads(bool v);

  /*!\fn generate_sub_quads(void)
    Gets if the next created WRATHTextureFontFreeType_Analytic
    will also generate sub-primitives for its glyph data,
    see \ref WRATHTextureFont::glyph_data_type::sub_primitive_attributes(void)
    and \ref WRATHTextureFont::glyph_data_type::sub_primitive_indices(void).
    Default value is false.
   */
  static
  bool
  generate_sub_quads(void);

  /*!\fn void mipmap_level(unsigned int)
    Textures of a WRATHTextureFontFreeType_Analytic can
    be mipmapped, to support font minification better.
    One caveat/issue with mipmapping is that for
    font creation mode \ref local_pixel_coordinates,
    each degree of mipmapping loses one bit on
    the accuracy of the offset values within the texel.
    The default value is 0.
    \param N value to which to support mipmaps
   */
  static
  void
  mipmap_level(unsigned int N);

  /*!\fn unsigned int mipmap_level(void)
    Textures of a WRATHTextureFontFreeType_Analytic can
    be mipmapped, to support font minification better.
    One caveat/issue with mipmapping is that for
    font creation mode \ref local_pixel_coordinates,
    each degree of mipmapping loses one bit on
    the accuracy of the offset values within the texel.
    The default value is 0.
   */
  static
  unsigned int
  mipmap_level(void);

  /*!\fn WRATHImage::TextureAllocatorHandle::texture_consumption_data_type texture_consumption
    Returns the texture utilization of all 
    WRATHTextureFontFreeType_Analytic objects.
   */
  static
  WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
  texture_consumption(void);

private:

  
  enum
    {
      number_textures_per_page=2
    };


  
  
  void
  ctor_init(void);
  
 
  void
  pack_lines(ivec2 pt, int L, 
             const std::vector<WRATHFreeTypeSupport::OutlineData::curve_segment> &curves,
             int curve_count, float far_away_offset,
             vecN<c_array<uint8_t>, number_textures_per_page> analytic_data,
             bool &);

  WRATHImage*
  allocate_glyph(std::vector< vecN<std::vector<uint8_t>, number_textures_per_page> > &analytic_pixel_data,
                 const ivec2 &sz);

  void
  generate_LOD_bitmap(const WRATHFreeTypeSupport::OutlineData &outline_data,
                      const ivec2 &glyph_size,
                      boost::multi_array<int, 2> &covered,
                      const boost::multi_array<WRATHFreeTypeSupport::analytic_return_type, 2> &analytic_data);

  glyph_data_type*
  generate_character(glyph_index_type G);

  float m_new_line_height;
  bool m_generate_sub_quads;
  unsigned int m_mipmap_level;
  bool m_is_ttf;
  float m_pow2_mipmap_level;

  enum texture_mode_type m_texture_mode;
  vecN<int, number_textures_per_page> m_bytes_per_pixel;
  WRATHImage::ImageFormatArray m_format;

  WRATHTextureFontUtil::TexturePageTracker m_page_tracker;
};

/*! @} */
#endif
