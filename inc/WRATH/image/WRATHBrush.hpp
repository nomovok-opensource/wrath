/*! 
 * \file WRATHBrush.hpp
 * \brief file WRATHBrush.hpp
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



/*! \addtogroup Imaging
 * @{
 */

#ifndef __WRATH_BRUSH_HPP__
#define __WRATH_BRUSH_HPP__

#include "WRATHConfig.hpp"
#include "WRATHUtil.hpp"
#include "WRATHGradient.hpp"
#include "WRATHImage.hpp"
#include "WRATHGradientSourceBase.hpp"
#include "WRATHTextureCoordinateSourceBase.hpp"
#include "WRATHColorValueSource.hpp"
#include "WRATHItemDrawState.hpp"

/*!\namespace WRATHBrushBits
  Enumeration to hold bit-flags for different
  configurations of a \ref WRATHShaderBrush.
 */
namespace WRATHBrushBits
{

  /*!\enum brush_bits
    Enumeration bit values indicating
    various shader options applied
    to a brush
   */
  enum brush_bits
    {
      /*!
	Bit up indicates to add anti-aliasing logic
	to shading. 
       */
      anti_alias_bit=1,

      /*!
	Bit up indicates to perform alpha test
	against image alpha
       */
      image_alpha_test_bit=2,

      /*!
	Bit up indicates to perform alpha test
	against gradient alpha
       */
      gradient_alpha_test_bit=4,

      /*!
	Bit up indicates to perform alpha
	test on const-color value
       */
      color_alpha_test_bit=8,

      /*!
	Bit up indicates to perform alpha
	test on final, cumalative, color value.
       */
      final_color_alpha_test_bit=16,

      /*!
	Bit up indicates to pre-multiply alpha
       */
      premultiply_alpha_bit=32,

      /*!
        Bit flag to indicate to discard
        a fragment if the gradient interpolate
        is less than 0.
       */
      gradient_interpolate_enforce_positive_bit=64,

      /*!
        Bit flag to indicate to discard
        a fragment if the gradient interpolate
        is greater than 1.0
       */
      gradient_interpolate_enforce_greater_than_one_bit=128,

      /*!
        Bit flag to indicate how to enforce the gradient
        interpolate requirements. 
        If the bit is up, then it is enforced by making the
        gradient color as (0,0,0,0), ideal for blending.
        If the bit is down, then is enforced by discard,
        required for opaque items.
       */
      gradient_interpolate_enforce_by_blend_bit=256,

      /*!
        If bit is up, indicates to flip the image in the
        y-coordinate
       */
      flip_image_y_bit=512,
    };

  /*!\class BrushBits
    Conveniance class to hold the enumerations
    of above and provide methods to set them.
    The usual use-pattern is:
    \code
    class Foo:public BrushBits<Foo>
    {
    //...
    };
    \endcode
    \tparam T type to return for setting.
   */
  template<typename T>
  class BrushBits
  {
  public:
    /*!\fn BrushBits
      Ctor. 
      \param bits value to which to initialize \var m_bits
     */
    explicit
    BrushBits(uint32_t bits=0):
      m_bits(bits),
      m_custom_bits(0)
    {}

    #define BIT_PROP(X)\
      T& X(bool b) { m_bits=WRATHUtil::apply_bit_flag(m_bits, b, X##_bit); return *static_cast<T*>(this); } \
      bool X(void) const { return m_bits & X##_bit; }
  
    BIT_PROP(anti_alias)
    BIT_PROP(image_alpha_test)
    BIT_PROP(gradient_alpha_test)
    BIT_PROP(color_alpha_test)
    BIT_PROP(final_color_alpha_test)
    BIT_PROP(premultiply_alpha)
    BIT_PROP(gradient_interpolate_enforce_positive)
    BIT_PROP(gradient_interpolate_enforce_greater_than_one)
    BIT_PROP(gradient_interpolate_enforce_by_blend)
    BIT_PROP(flip_image_y)
    
    #undef BIT_PROP

    /*!\var m_bits
      Bit field from the enumeration listed by 
      \ref brush_bits
     */
    uint32_t m_bits;

    /*!\var m_custom_bits
      Bit field affecting shader construction
      with bit meanings from \ref brush_bits
     */
    uint32_t m_custom_bits;
  };
};

/*!\class WRATHShaderBrush
  A WRATHShaderBrush represents how(and if) 
  to apply a gradient, texture and color, it is
  a container for essentialy GLSL shader code.
 */
class WRATHShaderBrush:public WRATHBrushBits::BrushBits<WRATHShaderBrush>
{
public:
  /*!\fn WRATHShaderBrush(const WRATHGradientSourceBase*,
		   const WRATHTextureCoordinateSourceBase*,
		   const WRATHColorValueSource*,
		   uint32_t)
    Ctor.
    \param grad value to which to initalize \ref m_gradient_source
    \param tex value to which to initalize \ref m_texture_coordinate_source
    \param color value to which to initalize \ref m_color_value_source
    \param pbits bitfield indicating shader bits, see ctor of
                 \ref WRATHBrushBits::BrushBits
   */
  WRATHShaderBrush(const WRATHGradientSourceBase *grad=NULL,
		   const WRATHTextureCoordinateSourceBase *tex=NULL,
		   const WRATHColorValueSource *color=NULL,
		   uint32_t pbits=0):
    WRATHBrushBits::BrushBits<WRATHShaderBrush>(pbits),
    m_gradient_source(grad),
    m_texture_coordinate_source(tex),
    m_color_value_source(color)
  {}

  /*!\fn WRATHShaderBrush(uint32_t)
    Ctor, \ref m_gradient_source, \ref m_texture_coordinate_source
    and \ref m_color_value_source initialized as NULL.
    \param pbits bitfield indicating shader bits, see ctor of
                 \ref WRATHBrushBits::BrushBits
   */
  explicit
  WRATHShaderBrush(uint32_t pbits):
    WRATHBrushBits::BrushBits<WRATHShaderBrush>(pbits),
    m_gradient_source(NULL),
    m_texture_coordinate_source(NULL),
    m_color_value_source(NULL)
  {}
  
  /*!\fn WRATHShaderBrush& gradient_source
    Set \ref m_gradient_source
    \param v value to which to set \ref m_gradient_source
   */
  WRATHShaderBrush&
  gradient_source(const WRATHGradientSourceBase* v)
  {
    m_gradient_source=v;
    return *this;
  }
  
  /*!\fn WRATHShaderBrush& texture_coordinate_source
    Set \ref m_texture_coordinate_source
    \param v value to which to set \ref m_texture_coordinate_source
   */
  WRATHShaderBrush&
  texture_coordinate_source(const WRATHTextureCoordinateSourceBase* v)
  {
    m_texture_coordinate_source=v;
    return *this;
  }
  
  /*!\fn WRATHShaderBrush& color_value_source
    Set \ref m_color_value_source
    \param v value to which to set \ref m_color_value_source
   */
  WRATHShaderBrush&
  color_value_source(const WRATHColorValueSource* v)
  {
    m_color_value_source=v;
    return *this;
  }

  /*!\fn bool operator<(const WRATHShaderBrush&) const
    Comparison operator
    \param rhs value to which to compare
   */
  bool
  operator<(const WRATHShaderBrush &rhs) const;
  
  /*!\var m_gradient_source
    Holds the GLSL code for computing the gradient interpolate 
   */
  const WRATHGradientSourceBase *m_gradient_source;

  /*!\var m_texture_coordinate_source
    Holds the GLSL code for computing the texture coordinate
   */
  const WRATHTextureCoordinateSourceBase *m_texture_coordinate_source;

  /*!\var m_color_value_source
    Holds the GLSL code for computing the color value
   */
  const WRATHColorValueSource *m_color_value_source;

  /*!\var m_draw_state
    Additional GL state needed/wanted by the shader code
    to work correctly.
   */
  WRATHSubItemDrawState m_draw_state; 
};

/*!\class WRATHBrush
  A WRATHBrush represents how/if to
  apply a texture, gradient and color
  and what texture, gradient and color
  to apply.
 */
class WRATHBrush:public WRATHShaderBrush
{
public:

  /*!\fn WRATHBrush(WRATHImage*, WRATHGradient*, uint32_t)
    Ctor.
    \param pImage value to which to initialize \ref m_image
    \param pGradient value to which to initialize \ref m_gradient
    \param pbits bitfield indicating shader bits, see ctor of
                 \ref WRATHBrushBits::BrushBits
   */
  WRATHBrush(WRATHImage *pImage=NULL,
             WRATHGradient *pGradient=NULL,
             uint32_t pbits=0):
    WRATHShaderBrush(pbits),
    m_image(pImage),
    m_gradient(pGradient)
  {}

  /*!\fn WRATHBrush(WRATHGradient*, WRATHImage*, uint32_t)
    Ctor.
    \param pGradient value to which to initialize \ref m_gradient
    \param pImage value to which to initialize \ref m_image
    \param pbits bitfield indicating shader bits, see ctor of
                 \ref WRATHBrushBits::BrushBits
   */
  WRATHBrush(WRATHGradient *pGradient,
             WRATHImage *pImage=NULL,
             uint32_t pbits=0):
    WRATHShaderBrush(pbits),
    m_image(pImage),
    m_gradient(pGradient)
  {}

  /*!\fn bool image_consistent
    Returns true if \ref m_image
    value is consistent with \ref 
    WRATHShaderBrush::m_texture_coordinate_source
    value, i.e. returns true if both are 
    NULL or both are non-NULL
   */
  bool
  image_consistent(void) const
  {
    return bool(m_image==NULL)==bool(m_texture_coordinate_source==NULL);
  }

  /*!\fn void make_image_consistent
    Makes \ref m_image and \ref m_texture_coordinate_source
    consitent by setting both to NULL if one is already NULL.
   */
  void
  make_image_consistent(void)
  {
    m_image=(m_texture_coordinate_source!=NULL)?
      m_image:
      NULL;

    m_texture_coordinate_source=(m_image!=NULL)?
      m_texture_coordinate_source:
      NULL;
  }

  /*!\fn bool gradient_consistent
    Returns true if \ref m_gradient
    value is consistent with \ref 
    WRATHShaderBrush::m_gradient_source
    value, i.e. returns true if both are 
    NULL or both are non-NULL.
   */
  bool
  gradient_consistent(void) const
  {
    return bool(m_gradient==NULL)==bool(m_gradient_source==NULL);
  }

   /*!\fn void make_gradient_consistent
    Makes \ref m_gradient and \ref m_gradient_source
    consitent by setting both to NULL if one is already NULL.
   */
  void
  make_gradient_consistent(void)
  {
    m_gradient=(m_gradient_source!=NULL)?
      m_gradient:
      NULL;

    m_gradient_source=(m_gradient!=NULL)?
      m_gradient_source:
      NULL;
  }

  /*!\fn bool consistent
    Provided as a conveniance, equivalent to
    \code
    image_consistent() and gradient_consistent()
    \endcode
   */
  bool
  consistent(void) const
  {
    return image_consistent() and gradient_consistent();
  }

  /*!\fn void make_consistent
    Provided as a conveniance, equivalent to
    \code
    make_image_consistent();
    make_gradient_consistent();
    \endcode
   */
  void
  make_consistent(void)
  {
    make_gradient_consistent();
    make_image_consistent();
  }

  /*!\var m_image
    Image to apply to the brush.
   */
  WRATHImage *m_image;

  /*!\var m_gradient
    Gradient color values to apply to the brush.
   */
  WRATHGradient *m_gradient;
};

/*! @} */

#endif
