/*! 
 * \file WRATH2DTransformation.hpp
 * \brief file WRATH2DTransformation.hpp
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




#ifndef WRATH_HEADER_2D_TRANSFORMATION_HPP_
#define WRATH_HEADER_2D_TRANSFORMATION_HPP_

#include "WRATHConfig.hpp"
#include <math.h>
#include <complex>
#include "type_tag.hpp"
#include "vectorGL.hpp"
#include "matrixGL.hpp"
#include "WRATHScaleTranslate.hpp"
#include "WRATH2DRigidTransformation.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class WRATH2DTransformation
  A WRATH2DTransformation represents a 2D transformation
  that comes from the composition of a linear mapping
  with a translation. Note that shearing and reflection
  are supported by a WRATH2DTransformation.
  A WRATH2DTransformation represents the transformation:
  \f[ f(x,y) = L(x,y) + (A,B) \f]
  where \f$ L \f$ = linear_transformation() and \f$(A,B)\f$ = translation().
 */
class WRATH2DTransformation
{
public:
  /*!\fn WRATH2DTransformation(void)
    Initialize the WRATH2DTransformation as
    the identity.
   */
  WRATH2DTransformation(void):
    m_rotation(1.0f, 0.0f),
    m_linear_transformation()
  {}

  /*!\fn WRATH2DTransformation(const float2x2&, const vec2&)
    Initialize the WRATH2DTransformation.
    \param lin linear transformation as a 2x2 matrix
    \param tr translation of transformation
  */
  explicit
  WRATH2DTransformation(const float2x2 &lin, 
                        const vec2 &tr=vec2(0.0f, 0.0f)):
    m_translation(tr),
    m_linear_transformation(lin)
  {
  }

  /*!\fn WRATH2DTransformation(const WRATHScaleTranslate&)
    Initialize the WRATHTransformation from a WRATHScaleTranslate
    \param sc_tr WRATHScaleTranslate from which to initialize
  */
  explicit
  WRATH2DTransformation(const WRATHScaleTranslate &sc_tr):
    m_translation(sc_tr.translation())
  {
    m_linear_transformation(0,0)=sc_tr.scale();
    m_linear_transformation(1,1)=sc_tr.scale();
  }

  /*!\fn WRATH2DTransformation(const WRATH2DRigidTransformation&)
    Initialize the WRATH2DTransformation from a WRATH2DRigidTransformation
    \param sc_tr WRATH2DRigidTransformation from which to initialize
  */
  explicit
  WRATH2DTransformation(const WRATH2DRigidTransformation &sc_tr):
    m_translation(sc_tr.translation())
  {
    m_linear_transformation(0,0)=sc_tr.scale()*sc_tr.rotation().real();
    m_linear_transformation(1,1)=m_linear_transformation(0,0);
    m_linear_transformation(1,0)=sc_tr.scale()*sc_tr.rotation().image();
    m_linear_transformation(0,1)=-m_linear_transformation(1,0);
  }

  /*!\fn WRATH2DTransformation inverse(void) const
    Returns the inverse transformation to this.
   */
  WRATH2DTransformation
  inverse(void) const
  {
    WRATH2DTransformation r;
    float det, det_recip;
    
    det=m_linear_transformation(0,0)*m_linear_transformation(1,1)
      - m_linear_transformation(0,1)*m_linear_transformation(1,0);

    det_recip=(det!=0.0f)?
      1.0f/det:0.0f;

    /*
      inverse( | a b | ) = |  d  -b | * 1/det
               | c d |     | -c   a |
     */
    
    r.m_linear_transformation(0,0)=m_linear_transformation(1,1)*det_recip;
    r.m_linear_transformation(1,0)= - m_linear_transformation(1,0)*det_recip;
    r.m_linear_transformation(0,1)= - m_linear_transformation(0,1)*det_recip;
    r.m_linear_transformation(1,1)=m_linear_transformation(0,0)*det_recip;


    /*
      solve for p:

      q= R(p) + T

      p= inverse(R)( q - T)
      = inverse(R)(q) - inverse(R)(T)
     */
    r.m_translation= - r.m_linear_transformation * m_translation;

    return r;
  }
                      

  /*!\fn const float2x2& linear_transformation(void) const
    Returns the linear transformation of this 
    WRATH2DTransformation
   */
  const float2x2&
  linear_transformation(void) const
  {
    return m_linear_transformation;
  }

  /*!\fn WRATH2DTransformation& linear_transformation(const float2x2 &r)
    Set the linear transformation of this WRATH2DTransformation
    \param r value to which to set the linear transformation
   */
  WRATH2DTransformation&
  linear_transformation(const float2x2 &r)
  {
    m_linear_transformation=r;
    return *this;
  }               

  /*!\fn const vec2& translation(void) const
    Returns the translation of this
    WRATH2DTransformation.
   */
  const vec2&
  translation(void) const
  {
    return m_translation;
  }

  /*!\fn WRATH2DTransformation& translation(const vec2&)
    Sets the translation of this WRATH2DTransformation.
    \param tr value to set translation to.
   */
  WRATH2DTransformation&
  translation(const vec2 &tr)
  {
    m_translation=tr;
    return *this;
  }

  /*!\fn vec2 apply_to_point(const vec2&) const
    Returns the value of applying the transformation to a point.
    \param pt point to apply the transformation to.
   */
  vec2
  apply_to_point(const vec2 &pt) const
  {
    return m_translation + m_linear_transformation*pt;
  }

  /*!\fn float4x4 matrix4
    Returns the transformation of this
    WRATHScaleTranslate as a 4x4 matrix
   */
  float4x4
  matrix4(void) const
  {
    float4x4 M;

    M(0,0)=m_linear_transformation(0,0);
    M(0,1)=m_linear_transformation(0,1);
    M(1,0)=m_linear_transformation(1,0);
    M(1,1)=m_linear_transformation(1,1);
         
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

    M(0,0)=m_linear_transformation(0,0);
    M(0,1)=m_linear_transformation(0,1);
    M(1,0)=m_linear_transformation(1,0);
    M(1,1)=m_linear_transformation(1,1);
     
    M(0,2)=translation().x();
    M(1,2)=translation().y();

    return M;
  }


private:
  float2x2 m_linear_transformation;
  vec2 m_translation;
};

/*!\fn WRATH2DTransformation operator*(const WRATH2DTransformation&, const WRATH2DTransformation&)
  Compose two WRATH2DTransformations so that:
  a*b.apply_to_point(p) "=" a.apply_to_point( b.apply_to_point(p)).
  \param a left hand side of composition
  \param b right hand side of composition
 */
inline
WRATH2DTransformation
operator*(const WRATH2DTransformation &a, const WRATH2DTransformation &b)
{
  WRATH2DTransformation c;

  c.linear_transformation(a.linear_transformation()*b.linear_transformation());
  c.translation( a.apply_to_point(b.translation()) );

  return c;
}


/*! @} */

#endif
