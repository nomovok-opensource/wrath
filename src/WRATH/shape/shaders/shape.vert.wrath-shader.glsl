/*! 
 * \file shape.vert.wrath-shader.glsl
 * \brief file shape.vert.wrath-shader.glsl
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



/*
  TODO:
  - option to allow for image to modulate
  - option for discard on small alpha color
 */

//pos meanings:
// .xy: location in UI co-ordinates (2D)
shader_in mediump vec2 pos;

#ifdef WRATH_BRUSH_AA_HINT
shader_in mediump float in_aa_hint;
shader_out mediump float aa_hint;
#endif

void
shader_main(void)
{
  #ifdef WRATH_BRUSH_AA_HINT
  {
    aa_hint=in_aa_hint;
  }
  #endif
  
  gl_Position=compute_gl_position_and_apply_clipping(vec3(pos, -1.0));
  #ifdef WRATH_APPLY_BRUSH
    wrath_shader_brush_prepare(pos.xy); 
  #endif
}
