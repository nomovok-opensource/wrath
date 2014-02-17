/*! 
 * \file WRATHScaleXYTranslate.hpp
 * \brief file WRATHScaleXYTranslate.hpp
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




#ifndef WRATH_HEADER_SCALE_XY_TRANSLATE_HPP_
#define WRATH_HEADER_SCALE_XY_TRANSLATE_HPP_

#include "WRATHConfig.hpp"
#include "WRATHScaleTranslate.hpp"
#include "type_tag.hpp"
#include "vectorGL.hpp"
#include "matrixGL.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class WRATHScaleXYTranslate
  A WRATHScaleXYTranslate represents the composition
  of a scalingin x and y and a translation, i.e.
  \f[ f(x,y) = (ax+A, by+B) \f]
  where \f$ a \f$=scale().x(), \f$ b \f$=scale().y() and \f$ (A,B) \f$ =translation().
 */
class WRATHScaleXYTranslate
{
public:
  /*!\fn WRATHScaleXYTranslate(const WRATHScaleTranslate &)
    Ctor. Initialize a WRATHScaleXYTranslate from a
    \ref WRATHScaleTranslate object
    \param obj \ref WRATHScaleTranslate from which to initialize
   */
  WRATHScaleXYTranslate(const WRATHScaleTranslate &obj):
    m_scale(obj.scale(), obj.scale()),
    m_translation(obj.translation())
  {}

  /*!\fn WRATHScaleXYTranslate(const vec2&, float)
    Ctor. Initialize a WRATHScaleXYTranslate from a
    scaling factor and translation
    \param tr translation to use
    \param s scaling factor to apply to both x-axis and y-axis
   */
  WRATHScaleXYTranslate(const vec2 &tr=vec2(0.0f, 0.0f),
                        float s=1.0f):
    m_scale(s, s),
    m_translation(tr)
  {}

  /*!\fn WRATHScaleXYTranslate(float)
    Ctor. Initialize a WRATHScaleXYTranslate from a
    scaling factor
    \param s scaling factor to apply to both x-axis and y-axis
   */
  WRATHScaleXYTranslate(float s):
    m_scale(s, s),
    m_translation(0.0f, 0.0f)
  {}

  /*!\fn WRATHScaleXYTranslate inverse(void) const
    Returns the inverse transformation to this.
   */
  WRATHScaleXYTranslate
  inverse(void) const
  {
    WRATHScaleXYTranslate r;
    r.m_scale=vec2(1.0f, 1.0f) / m_scale;
    r.m_translation= -r.m_scale*m_translation;    
    return r;
  }

  /*!\fn const vec2& translation(void) const 
    Returns the translation of this
    WRATHScaleXYTranslate.
   */
  const vec2&
  translation(void) const
  {
    return m_translation;
  }

  /*!\fn WRATHScaleXYTranslate& translation(const vec2&) 
    Sets the translation of this.
    \param tr value to set translation to.
   */
  WRATHScaleXYTranslate&
  translation(const vec2 &tr)
  {
    m_translation=tr;
    return *this;
  }

  /*!\fn WRATHScaleXYTranslate& translation_x(float) 
    Sets the x-coordinate of the translation of this.
    \param x value to set translation to.
   */
  WRATHScaleXYTranslate&
  translation_x(float x)
  {
    m_translation.x()=x;
    return *this;
  }

  /*!\fn WRATHScaleXYTranslate& translation_y(float)
    Sets the y-coordinate of the translation of this.
    \param y value to set translation to.
   */
  WRATHScaleXYTranslate&
  translation_y(float y)
  {
    m_translation.y()=y;
    return *this;
  }

  /*!\fn const vec2& scale(void) const
    Returns the scale of this.
   */
  const vec2&
  scale(void) const
  {
    return m_scale;
  }

  /*!\fn WRATHScaleXYTranslate& scale(const vec2 &)
    Sets the scale of this.
    \param s value to set scale to.
   */
  WRATHScaleXYTranslate&
  scale(const vec2 &s)
  {
    m_scale=s;
    return *this;
  }

  /*!\fn WRATHScaleXYTranslate& scale_x(float) 
    Sets the x-coordinate of the translation of this.
    \param x value to set scale to.
   */
  WRATHScaleXYTranslate&
  scale_x(float x)
  {
    m_scale.x()=x;
    return *this;
  }

  /*!\fn WRATHScaleXYTranslate& scale_y(float)
    Sets the x-coordinate of the translation of this.
    \param y value to set scale to.
   */
  WRATHScaleXYTranslate&
  scale_y(float y)
  {
    m_scale.y()=y;
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
    WRATHScaleXYTranslate as a 4x4 matrix
   */
  float4x4
  matrix4(void) const
  {
    float4x4 M;
    M(0,0)=scale().x();
    M(1,1)=scale().y();      
    M(0,3)=translation().x();
    M(1,3)=translation().y();
    return M;
  }

  /*!\fn float3x3 matrix3
    Returns the transformation of this
    WRATHScaleXYTranslate as a 3x3 matrix
   */
  float3x3
  matrix3(void) const
  {
    float3x3 M;
    M(0,0)=scale().x();
    M(1,1)=scale().y();    
    M(0,2)=translation().x();
    M(1,2)=translation().y();
    return M;
  }

  /*!\fn WRATHScaleXYTranslate interpolate 
    Computes the interpolation between
    two WRATHScaleXYTranslate objects.
    \param a0 begin value of interpolation
    \param a1 end value of interpolation
    \param t interpolate, t=0 returns a0, t=1 returns a1
   */
  static
  WRATHScaleXYTranslate
  interpolate(const WRATHScaleXYTranslate &a0,
              const WRATHScaleXYTranslate &a1,
              float t)
  {
    WRATHScaleXYTranslate R;
    R.translation( a0.translation() + t*(a1.translation()-a0.translation()) );
    R.scale( a0.scale() + t*(a1.scale()-a0.scale()) );
    return R;
  }

private:
  vec2 m_scale;
  vec2 m_translation;
};

/*!\fn WRATHScaleXYTranslate operator*(const WRATHScaleXYTranslate&, const WRATHScaleXYTranslate&)
  Compose two WRATHScaleXYTranslate so that:
  a*b.apply_to_point(p) "=" a.apply_to_point( b.apply_to_point(p)).
  \param a left hand side of composition
  \param b right hand side of composition
 */
inline
WRATHScaleXYTranslate
operator*(const WRATHScaleXYTranslate &a, const WRATHScaleXYTranslate &b)
{
  WRATHScaleXYTranslate c;

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
