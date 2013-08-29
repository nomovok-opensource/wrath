/*! 
 * \file item.vert.glsl
 * \brief file item.vert.glsl
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


//pos meanings:
// .xy: location in UI co-ordinates (2D)
shader_in mediump vec2 pos;

void
shader_main(void)
{
  // WRATHShaderBrushSourceHoard defines the macro 
  // LINEAR_BRUSH_PRESENT if an donly if it defines 
  // a brush where the brush input coordinates are 
  // determined by the vertex shader.
  #ifdef LINEAR_BRUSH_PRESENT
  {
    wrath_shader_brush_prepare(pos.xy);
  }
  #endif

  gl_Position= compute_gl_position_and_apply_clipping( vec3(pos, -1.0) );
}
