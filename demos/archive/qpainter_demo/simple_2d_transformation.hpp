/*! 
 * \file simple_2d_transformation.hpp
 * \brief file simple_2d_transformation.hpp
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


#ifndef SIMPLE2D_TRANSFORMATION
#define SIMPLE2D_TRANSFORMATION

#include <QPointF>
#include <complex>
#include <QTransform>

class Simple2DTransform
{
public:

  Simple2DTransform(void):
    m_translation(0,0),
    m_rotation(1,0),
    m_scale(1)
  {}

  const std::complex<float>&
  rotation(void) const
  {
    return m_rotation;
  }

  void
  rotation(const std::complex<float> &r);

  void
  rotation(float angle)
  {
    sincosf(angle, &m_rotation.imag(), &m_rotation.real());
  }

  void
  rotateby(float angle)
  {
    std::complex<float> R;

    sincosf(angle, &R.imag(), &R.real());
    m_rotation=m_rotation*R;
  }

  const std::complex<float>&
  translation(void) const
  {
    return m_translation;
  }

  void
  translation(const std::complex<float>&tr)
  {
    m_translation=tr;
  }

  void
  translation(float x, float y)
  {
    m_translation=std::complex<float>(x,y);
  }
  
  float
  scale(void) const
  {
    return m_scale;
  }

  void
  scale(float v)
  {
    m_scale=v;
  }

  std::complex<float>
  apply_to_point(const std::complex<float> &pt) const
  {
    std::complex<float> r;

    r= m_scale*(m_rotation*pt) + m_translation;
    return r;
  }

  QTransform
  transformation(void) const;

private:
  std::complex<float> m_translation;
  std::complex<float> m_rotation;
  float m_scale;
};

inline
Simple2DTransform
operator*(const Simple2DTransform &a, const Simple2DTransform &b)
{
  Simple2DTransform c;
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


#endif
