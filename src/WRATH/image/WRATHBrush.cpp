/*! 
 * \file WRATHBrush.cpp
 * \brief file WRATHBrush.cpp
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
#include "WRATHBrush.hpp"

/////////////////////////////////////////////////
// WRATHShaderBrush methods
bool
WRATHShaderBrush::
operator<(const WRATHShaderBrush &rhs) const
{
#define EASY(X) if (X!=rhs.X) { return X<rhs.X; }

  EASY(m_bits);
  EASY(m_gradient_source);
  EASY(m_texture_coordinate_source);
  EASY(m_color_value_source);
  EASY(m_custom_bits);

  return false;

#undef EASY
}
