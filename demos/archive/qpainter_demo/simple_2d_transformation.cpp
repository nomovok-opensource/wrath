/*! 
 * \file simple_2d_transformation.cpp
 * \brief file simple_2d_transformation.cpp
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


#include "simple_2d_transformation.hpp"

void
Simple2DTransform::
rotation(const std::complex<float> &r)
{
  float m;

  m=std::abs(r);
  if(m>=0.00001)
    {
      m_rotation=r/m;
    }
}

QTransform
Simple2DTransform::
transformation(void) const
{
  QTransform R(m_scale*m_rotation.real(), m_scale*m_rotation.imag(), 0.0f,
               -m_scale*m_rotation.imag(), m_scale*m_rotation.real(), 0.0f,
               m_translation.real(), m_translation.imag(), 1.0f);

  return R;
}
