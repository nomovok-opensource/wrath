/*! 
 * \file WRATHTextureFontFreeType_CurveAnalytic.hpp
 * \brief file WRATHTextureFontFreeType_CurveAnalytic.hpp
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




#ifndef WRATH_HEADER_TEXTURE_FONT_FreeType_CURVE_ANALYTIC_HPP_
#define WRATH_HEADER_TEXTURE_FONT_FreeType_CURVE_ANALYTIC_HPP_

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

/*!\class WRATHTextureFontFreeType_CurveAnalytic
  A WRATHTextureFontFreeType_CurveAnalytic stores 
  outline data as sections of control points 
  into it's textures. The fragment shader
  for a WRATHTextureFontFreeType_CurveAnalytic is
  much more expensive than for coverage or
  distance shaders (and for that matter
  also WRATHTextureFontFreeType_Analytic), but
  does not exhibit the artifacts of any of the 
  others. Additionally, the texture memory
  usage of a WRATHTextureFontFreeType_CurveAnalytic
  is width*height + M*NumberCurves bytes
  per glyph. M is:
  - 22 for non-separate curve storage with no scaling data present 
  - 26 for non-separate curve storage with scaling data present 
  - 15 for separate curve storage with no scaling data present 
  - 17 for separate curve storage with scaling data present 

  The needed resolution for
  WRATHTextureFontFreeType_CurveAnalytic is much
  lower than that of any of the others,
  typically less that half the resolution
  in each dimension as needed for \ref 
  WRATHTextureFontFreeType_Distance. A good
  value, even for asian glyphs is pixel size
  of 32.

  WRATHTextureFontFreeType_CurveAnalytic has some
  limitations aside from a more expensive fragment
  shader:
  - only support quadratic and linear Bezier curves.
    As such, if a cubic Bezier curve is encountered
    it is approximated by a quadrative Bezier curve.
    Cubic's appear in OTF fonts, but NOT in TTF fonts\n
  - Glyphs that have more than 254 curves are not supported,
    those glyphs are rendered as solid blocks.

  Class is thread safe, i.e. glyphs (of the 
  same font) can be generated on a seperate thread 
  than the rendering thread and multiple glyphs may
  also be generated at the same time from multiple threads.
 */
class WRATHTextureFontFreeType_CurveAnalytic:
  public WRATHTextureFontFreeTypeT<WRATHTextureFontFreeType_CurveAnalytic>
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
  
  virtual
  ~WRATHTextureFontFreeType_CurveAnalytic();

  /*!\fn WRATHTextureFontFreeType_CurveAnalytic
    Ctor for WRATHTextureFontFreeType_CurveAnalytic, it is HIGHLY advised
    to use fetch_font() to create/get fonts from files.
    The ctor is exposed publicly for situations where
    one is handed a FT_Face that from which one wishes 
    to create a WRATHTextureFontFreeType_CurveAnalytic.
    \param pface holder of the FT_Face, raw FT_Face data from FreeType
                 to create the data of the WRATHTextureFontFreeType_CurveAnalytic.
                 Note! Each time a glyph is created the
                 data within the FT_Face will be modified.
    \param presource_name specifies the WRATHFontDatabase::Font source
                          and the pixel size, becomes the key for the font.
   */
  WRATHTextureFontFreeType_CurveAnalytic(WRATHFreeTypeSupport::LockableFace::handle pface, 
                                         const WRATHTextureFontKey &presource_name);

  
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
    return 1;
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
  
  /*!\fn normalized_glyph_code_value
    glyph_data_type objects of a WRATHTextureFontFreeType_CurveAnalytic
    carry an additional custom attribute common to the
    entire glyph. This value is a texture coordinate to
    feed to texture 1 as the t(aka y) coordinate.
    Returns the value normalized as a float.
    \param G glyph_data_type object to query
   */
  static
  float
  normalized_glyph_code_value(const glyph_data_type &G);

  /*!\fn GLint texture_creation_size(void)
    Gets the maximum size of the textures
    used by WRATHTextureFontFreeType_CurveAnalytic.
    The default value is 1024. 

    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  GLint
  texture_creation_size(void);

  /*!\fn void texture_creation_size(GLint)
    Sets the maximum size of the textures
    used by WRATHTextureFontFreeType_CurveAnalytic.
    The default value is 1024.

    Method is thread safe and may be called from
    multiple threads safely.
    \param v value to use
   */
  static
  void
  texture_creation_size(GLint v);

  /*!\fn bool force_power2_texture(void)
    Returns if the next WRATHTextureFontFreeType_CurveAnalytic
    will or will not force the texture size to
    always be a power of 2.

    Method is thread safe and may be called from
    multiple threads safely.
   */
  static
  bool 
  force_power2_texture(void);

  /*!\fn void force_power2_texture(bool)
    Set if the next WRATHTextureFontFreeType_CurveAnalytic
    will or will not force the texture size to
    always be a power of 2, default is false.

    Method is thread safe and may be called from
    multiple threads safely.
    \param v value to use
   */
  static
  void 
  force_power2_texture(bool v);

  /*!\fn GLint effective_texture_creation_size
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
   
  /*!\fn bool store_separate_curves(void)
    Returns true if the next created 
    WRATHTextureFontFreeType_CurveAnalytic
    will store individiual curves instead
    of curve corner-pairs. The former
    uses less memory and fewer texture
    units but has slightly higher cost 
    fragment shader. Default value is false.
   */
  static
  bool
  store_separate_curves(void);

  /*!\fn void store_separate_curves(bool)
    Sets the bool value that if true
    has that the next created 
    WRATHTextureFontFreeType_CurveAnalytic
    will store individiual curves instead
    of curve corner-pairs. The former
    uses less memory and fewer texture
    units but has slightly higher cost 
    fragment shader. Default value is false.
    \param v value to use
   */
  static
  void
  store_separate_curves(bool v);

  /*!\fn bool use_highp(void)
    The curve data for a 
    WRATHTextureFontFreeType_CurveAnalytic
    and computations for rendering
    can be in 32-bit float (highp)
    or 16-bit float (mediump).
    Returns true if computation and
    rendering for the next created
    WRATHTextureFontFreeType_CurveAnalytic
    will be 32-bit (highp). Default
    value is false.
   */
  static
  bool
  use_highp(void);

  /*!\fn void use_highp(bool)
    The curve data for a 
    WRATHTextureFontFreeType_CurveAnalytic
    and computations for rendering
    can be in 32-bit float (highp)
    or 16-bit float (mediump).
    Default value is false.
    \param v value to use
   */
  static
  void
  use_highp(bool v);
  
  /*!\fn curvature_collapse(float v)
    Sets the value C for the subsequently
    created WRATHTextureFontFreeType_CurveAnalytic
    objects where C is used as follows. Those 
    quadratic curves of a glyph's outline whose 
    cumalative curvature is less than  C, are 
    replaced by a line segement with the same 
    end points. Setting the value as negative
    disables this test. Default value is 0.05f.
    \param v value to use
   */
  static
  void
  curvature_collapse(float v);

  /*!\fn float curvature_collapse(void)
    Returns the value C for subsequently
    created WRATHTextureFontFreeType_CurveAnalytic
    objexts where C is used as follows. Those 
    quadratic curves of a glyph's outline whose 
    cumalative curvature is less than  C, are 
    replaced by a line segement with the same 
    end points. Setting the value as negative
    disables this test. Default value is 0.05f.
   */
  static
  float
  curvature_collapse(void);

  /*!\fn disable_curvature_collapse(void)
    Disables replacing low-curvature quadratic 
    curves with line segments for subsequently
    created WRATHTextureFontFreeType_CurveAnalytic
    objects. Equivalent to
    \code
    curvature_collapse(-1.0f).
    \endcode
    See also curvature_collapse(void) and
    curvature_collapse(float).
   */
  static
  void
  disable_curvature_collapse(void)
  {
    curvature_collapse(-1.0f);
  }

  /*!\fn WRATHImage::TextureAllocatorHandle::texture_consumption_data_type texture_consumption_index(void)
    Returns the texture utilization by the index
    texture of all WRATHTextureFontFreeType_CurveAnalytic 
    objects.
   */
  static
  WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
  texture_consumption_index(void);

  /*!\fn WRATHImage::TextureAllocatorHandle::texture_consumption_data_type texture_consumption_curve
    Returns the texture utilization by the curve
    texture of all WRATHTextureFontFreeType_CurveAnalytic 
    objects.
   */
  static
  WRATHImage::TextureAllocatorHandle::texture_consumption_data_type
  texture_consumption_curve(void);


private:
  
  enum
    {
      number_textures_per_page=5
    };
  
  virtual
  glyph_data_type*
  generate_character(glyph_index_type G);

  void
  on_create_texture_page(ivec2 texture_size,
                         std::vector<float> &custom_data);


  uint32_t m_flags;
  float m_curvature_collapse;

  WRATHTextureFontUtil::TexturePageTracker m_page_tracker;

};
/*! @} */

#endif
