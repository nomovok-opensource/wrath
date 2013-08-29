/*! 
 * \file WRATH2DRigidTransformation.hpp
 * \brief file WRATH2DRigidTransformation.hpp
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




#ifndef __WRATH_2D_RIGID_TRANSFORMATION_HPP__
#define __WRATH_2D_RIGID_TRANSFORMATION_HPP__

#include "WRATHConfig.hpp"
#include <math.h>
#include <complex>
#include "type_tag.hpp"
#include "vectorGL.hpp"
#include "matrixGL.hpp"
#include "WRATHScaleTranslate.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class WRATH2DRigidTransformation
  A WRATH2DRigidTransformation represents a 2D transformation
  that comes from the composition of a rotation, scaling
  and translation. Note that shearing and reflection
  are not supported by a WRATH2DRigidTransformation.
  A WRATH2DRigidTransformation represents the transformation:
  \f[ f(x,y)= R(sx, sy) + (A,B) \f]
  where \f$s\f$=scale(), \f$R\f$=rotation(), \f$(A,B)\f$=translation().
 */
class WRATH2DRigidTransformation
{
public:
  
  /*!\enum rotation_enum_t
    Enumeration to indicate a rotation by a multiple
    of 90 degrees
   */
  enum rotation_enum_t
    {
      /*!
        No rotation
       */
      no_rotation,

      /*!
        Rotate by 90 degrees counter-clockwise
       */
      rotate_90_degrees,

      /*!
        Rotate by 180 degrees counter-clockwise
       */
      rotate_180_degrees,

      /*!
        Rotate by 270 degrees counter-clockwise
       */
      rotate_270_degrees,
    };

  /*!\fn WRATH2DRigidTransformation(void)
    Initialize the WRATH2DRigidTransformation as
    the identity.
   */
  WRATH2DRigidTransformation(void):
    m_rotation(1.0f, 0.0f),
    m_translation(0.0f, 0.0f),
    m_scale(1.0f)
  {}

  /*!\fn WRATH2DRigidTransformation(float, const vec2&, float)
    Initialize the WRATH2DRigidTransformation.
    \param angle angle of rotation in radians
    \param tr translation of transformation
    \param sc scaling factor or transformation
  */
  explicit
  WRATH2DRigidTransformation(float angle, 
                             const vec2 &tr=vec2(0.0f, 0.0f),
                             float sc=1.0f):
    m_translation(tr),
    m_scale(sc)
  {
    rotation(angle);
  }

  /*!\fn WRATH2DRigidTransformation(enum rotation_enum_t, const vec2&, float)
    Initialize the WRATH2DRigidTransformation.
    \param angle angle of rotation as an enumeration
    \param tr translation of transformation
    \param sc scaling factor or transformation
  */
  explicit
  WRATH2DRigidTransformation(enum rotation_enum_t angle, 
                             const vec2 &tr=vec2(0.0f, 0.0f),
                             float sc=1.0f):
    m_translation(tr),
    m_scale(sc)
  {
    rotation(angle);
  }

  /*!\fn WRATH2DRigidTransformation(const WRATHScaleTranslate&)
    Initialize the WRATH2DRigidTransformation from a WRATHScaleTranslate
    \param sc_tr WRATHScaleTranslate from which to initialize
  */
  explicit
  WRATH2DRigidTransformation(const WRATHScaleTranslate &sc_tr):
    m_rotation(1.0f, 0.0f),
    m_translation(sc_tr.translation()),
    m_scale(sc_tr.scale())
  {}

  /*!\fn WRATH2DRigidTransformation(const std::complex<float>&, const vec2&, float)
    Initialize the WRATH2DRigidTransformation.
    \param rot rotation of the WRATH2DRigidTransformation as a vec2,
               if the magnitude is small, then the rotation
               will be initialized as no rotation.
    \param tr translation of transformation
    \param sc scaling factor or transformation
  */
  explicit
  WRATH2DRigidTransformation(const std::complex<float> &rot, 
                             const vec2 &tr=vec2(0.0f, 0.0f),
                             float sc=1.0f):
    m_rotation(rot),
    m_translation(tr),
    m_scale(sc)
  {
    rotation(rot);
  }

  /*!\fn WRATH2DRigidTransformation inverse(void) const
    Returns the inverse transformation to this.
   */
  WRATH2DRigidTransformation
  inverse(void) const
  {
    WRATH2DRigidTransformation r;

    r.m_rotation=std::conj(m_rotation);
    r.m_scale=1.0f/m_scale;

    r.m_translation.x() = -r.m_scale*( r.m_rotation.real()*m_translation.x() - r.m_rotation.imag()*m_translation.y() );
    r.m_translation.y() = -r.m_scale*( r.m_rotation.real()*m_translation.y() + r.m_rotation.imag()*m_translation.x() );

    return r;
  }              

  /*!\fn const std::complex<float>& rotation(void) const
    Returns the rotation of this WRATH2DRigidTransformation,
    the rotation is realized as the first column of
    the matrix produced by the rotation. Note that
    complex multiplication is the exact operation to
    compose rotations.
   */
  const std::complex<float>&
  rotation(void) const
  {
    return m_rotation;
  }

  /*!\fn enum return_code rotation(const std::complex<float>&)
    Set the rotation of this WRATH2DRigidTransformation,
    the value will be normalized before being
    taken into use. If the passed value has
    too small magnitude, the routine fails.
    \param r rotation realized as a complex number
   */
  enum return_code
  rotation(const std::complex<float> &r);

  /*!\fn void rotation(enum rotation_enum_t)
    Set the rotation of this WRATH2DRigidTransformation
    from a rotation enumeration.
    \param r rotation realized as an enumeration from \ref rotation_enum_t
   */
  void
  rotation(enum rotation_enum_t r);

  /*!\fn void rotation(float)
    Sets the rotation of this WRATH2DRigidTransformation
    by an angle.
    \param angle_in_radians angle of rotation in _radians_
   */
  void
  rotation(float angle_in_radians)
  {
    float i, r;
    sincosf(angle_in_radians, &i, &r);
    m_rotation=std::complex<float>(r, i);
    
  }             

  /*!\fn const vec2& translation(void) const
    Returns the translation of this
    WRATH2DRigidTransformation.
   */
  const vec2&
  translation(void) const
  {
    return m_translation;
  }

  /*!\fn void translation(const vec2&)
    Sets the translation of this
    WRATH2DRigidTransformation.
    \param tr value to set translation to.
   */
  void
  translation(const vec2 &tr)
  {
    m_translation=tr;
  }

  /*!\fn float scale(void) const
    Returns the scale of this
    WRATH2DRigidTransformation, note
    that if the scale is negative,
    then the transformation is equivalent
    to haivng the same scale factor 
    as positive and the rotation composed
    with a rotation by PI radians (180 degrees).
   */
  float
  scale(void) const
  {
    return m_scale;
  }

  /*!\fn void scale(float)
    Sets the scale of this
    WRATH2DRigidTransformation,note
    that if the scale is negative,
    then the transformation is equivalent
    to haivng the same scale factor 
    as positive and the rotation composed
    \param s value to set scale to.
   */
  void
  scale(float s)
  {
    m_scale=s;
  }

  /*!\fn vec2 apply_to_point(const vec2&) const
    Returns the value of applying the transformation to a point.
    \param pt point to apply the transformation to.
   */
  vec2
  apply_to_point(const vec2 &pt) const
  {
    vec2 r;
    r=vec2( pt.x()*m_rotation.real() - pt.y()*m_rotation.imag(),
            pt.y()*m_rotation.real() + pt.x()*m_rotation.imag());
    r*=m_scale;
    r+=m_translation;
    return r;
  }

  /*!\fn vec4 value_as_vec4
    Returns the transformation packed into a vec4 as follows:\n
      .x = rotation().real()*scale()\n
      .y = rotation().imag()*scale()\n
      .z = translation().x()\n
      .w = translation().y()\n
   */
  vec4
  value_as_vec4(void) const
  {
    return vec4(rotation().real()*scale(),
                rotation().imag()*scale(),
                translation().x(),
                translation().y());
                
  }

  /*!\fn WRATH2DRigidTransformation interpolate(const WRATH2DRigidTransformation&, const WRATH2DRigidTransformation&, float)
    Computes the interpolation between
    two WRATH2DRigidTransformation objects.
    \param a0 begin value of interpolation
    \param a1 end value of interpolation
    \param t interpolate, t=0 returns a0, t=1 returns a1
   */
  static
  WRATH2DRigidTransformation
  interpolate(const WRATH2DRigidTransformation &a0,
              const WRATH2DRigidTransformation &a1,
              float t)
  {
    WRATH2DRigidTransformation R;

    R.translation( a0.translation() + t*(a1.translation()-a0.translation()) );

    /*
      We will cheese ball here and rather than getting the angle
      of a0 and a1 and such, we will just nomralize linear
      interpolate.
     */
    R.scale( a0.scale() + t*(a1.scale()-a0.scale()) );
    R.rotation( a0.m_rotation + t*(a1.m_rotation-a0.m_rotation)); 
    return R;
  }

  /*!\fn float4x4 matrix4
    Returns the transformation of this
    WRATHScaleTranslate as a 4x4 matrix
   */
  float4x4
  matrix4(void) const
  {
    float4x4 M;

    M(0,0)=M(1,1)=m_rotation.real()*m_scale;

    M(1,0)=m_rotation.imag()*m_scale;
    M(0,1)= -M(1,0);
         
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

    M(0,0)=M(1,1)=m_rotation.real()*m_scale;

    M(1,0)=m_rotation.imag()*m_scale;
    M(0,1)= -M(1,0);

     
    M(0,2)=translation().x();
    M(1,2)=translation().y();

    return M;
  }


private:
  std::complex<float> m_rotation;
  vec2 m_translation;
  float m_scale;
};

/*!\fn WRATH2DRigidTransformation operator*(const WRATH2DRigidTransformation&, const WRATH2DRigidTransformation&)
  Compose two WRATH2DRigidTransformations so that:
  a*b.apply_to_point(p) "=" a.apply_to_point( b.apply_to_point(p)).
  \param a left hand side of composition
  \param b right hand side of composition
 */
inline
WRATH2DRigidTransformation
operator*(const WRATH2DRigidTransformation &a, const WRATH2DRigidTransformation &b)
{
  WRATH2DRigidTransformation c;
  std::complex<float> r;

  //r.x()=a.rotation().real()*b.rotation().real()
  //- a.rotation().imag()*b.rotation().imag();

  //r.y()=a.rotation().real()*b.rotation().imag()
  //+ a.rotation().imag()*b.rotation().real();

  r=a.rotation()*b.rotation();

  //
  // c(p)= a( b(p) )
  //     = a.translation + a.rotation( a.scale*( b.rotation(b.scale*p) + b.translation ) )
  //     = a.translation + a.rotation(  a.scale*b.translation ) 
  //         + a.rotation*b.rotation( a.scale*b.scale*p )
  //
  //thus:
  //
  // c.scale=a.scale*b.scale
  // c.translation= a.apply_to_point(b.translation)
  // c.rotation = (complex multiply) a.rotation*b.rotation

  c.scale( a.scale()*b.scale());
  c.rotation(r);
  c.translation( a.apply_to_point(b.translation()) );

  return c;
}


/*! @} */

#endif
