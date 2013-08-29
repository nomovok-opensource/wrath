/*! 
 * \file WRATHScaleTranslate.hpp
 * \brief file WRATHScaleTranslate.hpp
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




#ifndef __WRATH_SCALE_TRANSLATE_HPP__
#define __WRATH_SCALE_TRANSLATE_HPP__

#include "WRATHConfig.hpp"
#include "type_tag.hpp"
#include "vectorGL.hpp"
#include "matrixGL.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class WRATHScaleTranslate
  A WRATHScaleTranslate represents the composition
  of a scaling and a translation, i.e.
  \f[ f(x,y) = (sx,sy) + (A,B) \f]
  where \f$ s \f$ =scale() and \f$ (A,B) \f$ =translation().
 */
class WRATHScaleTranslate
{
public:
  /*!\fn WRATHScaleTranslate(const vec2&, float)
    Ctor. Initialize a WRATHScaleTranslate from a
    scaling factor and translation
    \param tr translation to use
    \param s scaling factor to apply to both x-axis and y-axis,
             _absolute_ value is stored, i.e. the sign of s
             is ignored
   */
  WRATHScaleTranslate(const vec2 &tr=vec2(0.0f, 0.0f),
                      float s=1.0f):
    m_scale(std::abs(s)),
    m_translation(tr)
  {}

  /*!\fn WRATHScaleTranslate(float)
    Ctor. Initialize a WRATHScaleTranslate from a
    scaling factor
    \param s scaling factor to apply to both x-axis and y-axis,
             _absolute_ value is stored, i.e. the sign of s
             is ignored
   */
  WRATHScaleTranslate(float s):
    m_scale(std::abs(s)),
    m_translation(0.0f, 0.0f)
  {}

  /*!\fn WRATHScaleTranslate inverse(void) const
    Returns the inverse transformation to this.
   */
  WRATHScaleTranslate
  inverse(void) const
  {
    WRATHScaleTranslate r;

    r.m_scale=1.0f/m_scale;
    r.m_translation= -r.m_scale*m_translation;
    
    return r;
  }

  /*!\fn const vec2& translation(void) const
    Returns the translation of this
    WRATHScaleTranslate.
   */
  const vec2&
  translation(void) const
  {
    return m_translation;
  }

  /*!\fn WRATHScaleTranslate& translation(const vec2&)
    Sets the translation of this.
    \param tr value to set translation to.
   */
  WRATHScaleTranslate&
  translation(const vec2 &tr)
  {
    m_translation=tr;
    return *this;
  }

  /*!\fn WRATHScaleTranslate& translation_x(float)
    Sets the x-coordinate of the translation of this.
    \param x value to set translation to.
   */
  WRATHScaleTranslate&
  translation_x(float x)
  {
    m_translation.x()=x;
    return *this;
  }

  /*!\fn WRATHScaleTranslate& translation_y(float)
    Sets the y-coordinate of the translation of this.
    \param y value to set translation to.
   */
  WRATHScaleTranslate&
  translation_y(float y)
  {
    m_translation.y()=y;
    return *this;
  }

  /*!\fn float scale(void) const
    Returns the scale of this.
    Scaling factor is _NEVER_
    negative.
   */
  float
  scale(void) const
  {
    return m_scale;
  }

  /*!\fn WRATHScaleTranslate& scale(float)
    Sets the scale of this.
    If a negative value is passed,
    it's absolute value is used.
    \param s value to set scale to.
   */
  WRATHScaleTranslate&
  scale(float s)
  {
    m_scale=std::abs(s);
    return *this;
  }

  /*!\fn vec2 apply_to_point
    Returns the value of applying the transformation to a point.
    \param pt point to apply the transformation to.
   */
  vec2
  apply_to_point(const vec2 &pt) const
  {
    return scale()*pt + translation();
  }

  /*!\fn float4x4 matrix4
    Returns the transformation of this
    WRATHScaleTranslate as a 4x4 matrix
   */
  float4x4
  matrix4(void) const
  {
    float4x4 M;

    M(0,0)=M(1,1)=scale();      
    M(0,3)=translation().x();
    M(1,3)=translation().y();

    return M;
  }

  /*!\fn float3x3 matrix3
    Returns the transformation of this
    WRATHScaleTranslate as a 3x3 matrix
   */
  float3x3
  matrix3(void) const
  {
    float3x3 M;

    M(0,0)=M(1,1)=scale();      
    M(0,2)=translation().x();
    M(1,2)=translation().y();

    return M;
  }

  /*!\fn WRATHScaleTranslate interpolate
    Computes the interpolation between
    two WRATHScaleTranslate objects.
    \param a0 begin value of interpolation
    \param a1 end value of interpolation
    \param t interpolate, t=0 returns a0, t=1 returns a1
   */
  static
  WRATHScaleTranslate
  interpolate(const WRATHScaleTranslate &a0,
              const WRATHScaleTranslate &a1,
              float t)
  {
    WRATHScaleTranslate R;

    R.translation( a0.translation() + t*(a1.translation()-a0.translation()) );
    R.scale( a0.scale() + t*(a1.scale()-a0.scale()) );
    return R;
  }

private:
  float m_scale;
  vec2 m_translation;
};

/*!\fn operator*(const WRATHScaleTranslate&, const WRATHScaleTranslate&)
  Compose two WRATHScaleTranslate so that:
  a*b.apply_to_point(p) "=" a.apply_to_point( b.apply_to_point(p)).
  \param a left hand side of composition
  \param b right hand side of composition
 */
inline
WRATHScaleTranslate
operator*(const WRATHScaleTranslate &a, const WRATHScaleTranslate &b)
{
  WRATHScaleTranslate c;

  //
  // c(p)= a( b(p) )
  //     = a.translation + a.scale*( b.scale*p + b.translation ) 
  //     = (a.translate + a.scale*b.translation) + (a.scale*b.scale)*p
  //
  //thus:
  //
  // c.scale=a.scale*b.scale
  // c.translation= a.apply_to_point(b.translation)
  c.scale( a.scale()*b.scale());
  c.translation( a.apply_to_point(b.translation()) );

  return c;
}

/*! @} */

#endif
