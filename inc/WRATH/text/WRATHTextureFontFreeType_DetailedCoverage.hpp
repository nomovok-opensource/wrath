/*! 
 * \file WRATHTextureFontFreeType_DetailedCoverage.hpp
 * \brief file WRATHTextureFontFreeType_DetailedCoverage.hpp
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




#ifndef __WRATH_TEXTURE_FONT_FreeType_DETAILED_COVERAGE_HPP__
#define __WRATH_TEXTURE_FONT_FreeType_DETAILED_COVERAGE_HPP__


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
#include "WRATHAtlas.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\class WRATHTextureFontFreeType_DetailedCoverage
  A WRATHTextureFontFreeType_DetailedCoverage 
  uses Freetype2 to generate coverage fonts
  of glyphs at a user-defined set of resolutions.
  In contrast to WRATHTextureFontFreeType_Coverage,
  these sizes are not power of 2 divides
  of the highest resolution. This class aims
  to solve the issue of providing reasonably
  fast glyph rendering of small glyphs.

  A page of WRATHTextureFontFreeType_DetailedCoverage glyphs
  has two textures:
  - texture 0: L texture holding the coverage values, not-mipmapped. 
  - texture 1: index texture RGBA, texture coordinates are 
              essentially (displayed_glyph_size/max_glyph_size, normalized_localized_glyph_code),
              .rg gives location within texture 0 of glyph and .ba gives
              the size of the glyph within texture 0. Here max_glyph_size is
              the maximum pixel size of the glyph stored, which is given by 
              WRATHTextureFont::resource_name().second. Note that displayed_glyph_size/max_glyph_size
              is just the zoom factor applied to displaying the glyph clamped to [0,1]

  The glyph data of a WRATHTextureFontFreeType_DetailedCoverage
  contains a custom integer value: 
  - the normalized_localized_glyph_code to feed as the .y texture coordinate to feed to texture 0,
    stored as an unsigned byte (thus 0-->0.0f, 255->1.0f, etc). Ideally this should be a dedicated
    attribute to a vertex shader. The value normalized can be fetched via 
    \ref normalized_glyph_code_value().

  glyph_data_type 's of WRATHTextureFontFreeType_DetailedCoverage have that
  - glyph_data_type::texel_lower_left() always returns (0,0).
  - glyph_data_type::texel_lower_right() == glyph_data_type::texel_size() always

  Class is thread safe, i.e. glyphs (of the 
  same font) can be generated on a seperate thread 
  than the rendering thread and multiple glyphs may
  also be generated at the same time from multiple threads.

  WRATHTextureFontFreeType_DetailedCoverage inherits from
  WRATHTextureFont, thus is a resource managed object, 
  i.e. that class has a WRATHResourceManager,
  see \ref WRATH_RESOURCE_MANAGER_DECLARE. Although 
  WRATHTextureFontFreeType_DetailedCoverage objects can be 
  modified and created from threads outside of the GL 
  context, they must only be deleted from within the 
  GL context. In particular the resource manager may 
  only be cleared from within the GL context.
*/
class WRATHTextureFontFreeType_DetailedCoverage:
  public WRATHTextureFontFreeTypeT<WRATHTextureFontFreeType_DetailedCoverage>
{
public:
  using WRATHTextureFont::glyph_data;


  enum
    {
      /*!
        Enumeration to indicate that the font 
        is scalable.
       */
      font_scalability_value=font_is_scalable
    };

  /*!\fn WRATHTextureFontFreeType_DetailedCoverage
    Ctor for WRATHTextureFontFreeType_DetailedCoverage, it is HIGHLY advised
    to use fetch_font() to create/get fonts from files.
    The ctor is exposed publically for situations where
    one is handed a FT_Face that from which one wishes 
    to create a WRATHTextureFontFreeType_DetailedCoverage.

    \param pface handle to a WRATHFreeTypeSupport::LockableFace whose
                 underlying FT_Face is used to create the data 
                 of the WRATHTextureFontFreeType_DetailedCoverage.
                 Note! Each time a glyph is created the
                 data within the FT_Face will be modified.

    \param presource_name specifies the WRATHFontDatabase::Font source
                          and the pixel size, becomes the key for the font.
   */
  WRATHTextureFontFreeType_DetailedCoverage(WRATHFreeTypeSupport::LockableFace::handle pface,  
                                            const WRATHTextureFontKey &presource_name);

  virtual
  ~WRATHTextureFontFreeType_DetailedCoverage();

  
  virtual
  const_c_array<WRATHTextureChoice::texture_base::handle>
  texture_binder(int texture_page);

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

  /*!\fn float normalized_glyph_code_value
    glyph_data_type objects of a WRATHTextureFontFreeType_DetailedCoverage
    carry an additional custom attribute common to the
    entire glyph. This value is a texture coordinate to
    feed to texture 1 as the t(aka y) coordinate.
    Returns the value normalized as a float.
   */
  static
  float
  normalized_glyph_code_value(const glyph_data_type &G);

  /*!\fn const_c_array<int> pixel_sizes
    Returns the pixel sizes (included resource_name().second)
    that this WRATHTextureFontFreeType_DetailedCoverage
    object uses. Sizes are ordered in increasing order.
   */
  const_c_array<int>
  pixel_sizes(void) const
  {
    return m_pixel_sizes;
  }
  
  /*!\fn void additional_pixel_sizes(std::vector<int> &)
    Returns the "additional" pixel sizes, in increasing order,
    active for creating WRATHTextureFontFreeType_DetailedCoverage
    objects. The next object created will all those sizes
    listed in this function that are strictly smaller than
    it's pixel size (i.e. \ref resource_name().second)
    together with it's pixel size.

    Method is thread safe and may be called from
    multiple threads safely.

    \param out_sizes std::vector to which to write the sizes
   */
  static
  void
  additional_pixel_sizes(std::vector<int> &out_sizes);

  /*!\fn void add_additional_pixel_size(int)
    Adds a pixel size to the set of "additional"
    pixels sizes, see \ref additional_pixel_sizes().

    Method is thread safe and may be called from
    multiple threads safely.

    \param sz pixel size to which to add
   */
  static
  void
  add_additional_pixel_size(int sz);

  /*!\fn void clear_additional_pixel_sizes
    Clears the set of "additional" pixel
    sizes, see \ref additional_pixel_sizes().
   */
  static
  void
  clear_additional_pixel_sizes(void);

  /*!\fn void add_additional_pixel_sizes(iterator, iterator)
    Adds an STL range of sizes to the
    set of "additional" pixel sizes,
    see \ref additional_pixel_sizes() 
    and \ref add_additional_pixel_size().
    \tparam iterator iterator type to int
    \param begin iterator to 1st size to add
    \param end iterator one past the last size to add
   */
  template<typename iterator>
  static
  void
  add_additional_pixel_sizes(iterator begin, iterator end)
  {
    for(;begin!=end; ++begin)
      {
        add_additional_pixel_size(*begin);
      }
  }

  /*!\fn void set_additional_pixel_sizes(iterator, iterator)
    Sets the "additional" pixels sizes,
    equivalent to
    \code
    clear_pixel_size_choices();
    add_additional_pixel_sizes<iterator>(begin, end);
    \endcode
    \tparam iterator iterator type to int
    \param begin iterator to 1st size to use
    \param end iterator one past the last size to use
   */
  template<typename iterator>
  static
  void
  set_additional_pixel_sizes(iterator begin, iterator end)
  {
    clear_additional_pixel_sizes();
    add_additional_pixel_sizes<iterator>(begin, end);
  }

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

  
  class per_pixel_size_coverage_data
  {
  public:
    per_pixel_size_coverage_data(void):
      m_size(0,0),
      m_raw_pitch(0),
      m_bitmap_offset(0.0f, 0.0f)
    {}

    void
    take_bitmap_data(FT_Face fc);
    
    void
    take_bitmap_data(FT_Face fc, 
                     int this_pixel_size,
                     int max_pixel_size,
                     ivec2 offset_at_max_size);

    void
    create_pixel_data(void);

    const vec2&
    bitmap_offset(void)
    {
      return m_bitmap_offset;
    }

    const ivec2&
    size(void)
    {
      return m_size;
    }

    std::vector<uint8_t>&
    pixels(void)
    {
      return m_pixels;
    }

  private:
    ivec2 m_size;
    int m_raw_pitch;
    vec2 m_bitmap_offset;
    int m_this_pixel_size, m_max_pixel_size;
    std::vector<uint8_t> m_raw_pixels_from_freetype;
    std::vector<uint8_t> m_pixels;

  };

  /*
    implementation details:
    texture 0 is ALWAYS 256x256
    texture 1's y-dimension (height) is always 256,
    the width (recall the x-coordinate is the normalized
    size) is set as a heuristic dependend on the pixel
    size (given by resource_name().second) and
    pixel_sizes()
   */  
  glyph_data_type*
  generate_character(glyph_index_type G);

  void
  ctor_init(void);

  /*
    Returns the WRATHImage holding the index data,
    the return value's bottom_left().y() gives the
    y-coordinate within the index texture (duh!)
  */
  WRATHImage*
  allocate_glyph_room(const std::vector<ivec2> &bitmap_sizes,
                      std::vector<WRATHImage*> &out_rects);

  
  WRATHImage*
  create_and_set_images(std::vector<WRATHImage*> &out_rects,
                        std::vector<per_pixel_size_coverage_data> &pixel_data);
 
  /*
    m_pixel_sizes is the list of pixel sizes that this
    font creates for each glyph, size is in
    _increasing_ order.
   */
  std::vector<int> m_pixel_sizes;

  /*
    m_look_up_sizes[i] gives
    an index into m_pixel_sizes
    for the pixel size to use
    as follows. Let G by a glpyh

    Let n=normalized size, 
    
    Let I=index texture, then
    I(n,G)=image location data for drawing
           glpyh G at size S=n*pixel_size() 
    
    m_lookup_sizes[n*width(I)] gives
    an index J where m_pixel_sizes[J] is the
    actual pixel size to use.

    Also, m_lookup_sizes.size() is
    a power of 2.
   */
  std::vector<int> m_look_up_sizes;
  
  /*
    The allocator reffered to by m_index_texture_allocator 
    has that it's atlas width is _exactly_ m_lookup_sizes.size().
   */
  WRATHImage::TextureAllocatorHandle m_index_texture_allocator;

  /*
    We will have that each glyph will have a
    set of pointers to WRATHImage's giving
    the region of each glyph size for the
    glyph, in addition each glyph will also
    have a WRATHImage for it's region defining
    it's index texture. The significant gotcha's:

    (1) for a fixed glyph G, each of the WRATHImage's 
        holding the glpyh coverage values must be
        on the exact same texture.

    (2) the width of the texture on which the
        index image lies must be the width of
        the index texture...
   */
  WRATHTextureFontUtil::TexturePageTracker m_page_tracker;

  

};

/*! @} */

#endif
