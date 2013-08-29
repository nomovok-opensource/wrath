/*! 
 * \file WRATHglShaderBits.hpp
 * \brief file WRATHglShaderBits.hpp
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


#ifndef __WRATH_GL_SHADER_BITS_HPP__
#define __WRATH_GL_SHADER_BITS_HPP__

#include "WRATHConfig.hpp"
#include "WRATHgl.hpp"

/*
  Depending on the version of GL and it's headers,
  the shader_bit values (usually defined by GL's
  separate shader object interface) may or maynot
  be defined. WRATH uses these values, so go ahead
  and define them as necessary.
 */

#ifndef GL_VERTEX_SHADER_BIT
#define GL_VERTEX_SHADER_BIT 0x00000001
#endif

#ifndef GL_FRAGMENT_SHADER_BIT
#define GL_FRAGMENT_SHADER_BIT            0x00000002
#endif

#ifndef GL_GEOMETRY_SHADER_BIT
#define GL_GEOMETRY_SHADER_BIT            0x00000004
#endif

#ifndef GL_TESS_CONTROL_SHADER_BIT
#define GL_TESS_CONTROL_SHADER_BIT        0x00000008
#endif

#ifndef GL_TESS_EVALUATION_SHADER_BIT
#define GL_TESS_EVALUATION_SHADER_BIT     0x00000010
#endif

#ifndef GL_ALL_SHADER_BITS
#define GL_ALL_SHADER_BITS                0xFFFFFFFF
#endif


#endif
