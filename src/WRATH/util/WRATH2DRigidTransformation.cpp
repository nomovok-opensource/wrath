/*! 
 * \file WRATH2DRigidTransformation.cpp
 * \brief file WRATH2DRigidTransformation.cpp
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


#include "WRATHConfig.hpp"
#include "WRATH2DRigidTransformation.hpp"

#define epsilon 0.00001f

void
WRATH2DRigidTransformation::
rotation(enum rotation_enum_t r)
{
  switch(r)
    {
    default:
    case no_rotation:
      m_rotation=std::complex<float>(1.0f, 0.0f);
      break;

    case rotate_90_degrees:
      m_rotation=std::complex<float>(0.0f, 1.0f);
      break;

    case rotate_180_degrees:
      m_rotation=std::complex<float>(-1.0f, 0.0f);
      break;

    case rotate_270_degrees:
      m_rotation=std::complex<float>(0.0f, -1.0f);
      break;
    }
}


enum return_code
WRATH2DRigidTransformation::
rotation(const std::complex<float> &r)
{
  float m;

  m=std::abs(r);
  if(m<epsilon)
    {
      return routine_fail;
    }
  else
    {
      m_rotation=r/m;
      return routine_success;
    }
  
}
