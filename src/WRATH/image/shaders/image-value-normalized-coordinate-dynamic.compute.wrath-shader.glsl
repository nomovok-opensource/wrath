/*! 
 * \file image-value-normalized-coordinate-dynamic.compute.wrath-shader.glsl
 * \brief file image-value-normalized-coordinate-dynamic.compute.wrath-shader.glsl
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
  requires the symbols WRATH_IMAGE_REPEAT_MODE_X
  and WRATH_IMAGE_REPEAT_MODE_Y which is a function
  that consumes normalized coordinate and gives
  normalized coordinates

  if the symbol WRATH_IMAGE_REPEAT_MODE_VS is defined
  all computation can be done in the vertex shader
 */

#if defined(WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK)
#else
  shader_in WRATH_IMAGE_REPEAT_MODE_PREC vec4 WRATH_IMAGE_VALUE_NORMALIZED_varying0;
  shader_in WRATH_IMAGE_REPEAT_MODE_PREC float WRATH_IMAGE_VALUE_NORMALIZED_varying1;
#endif

#define between(x, minv, maxv) step(minv, x)*step(x,maxv)
#define vec2_between(x, minv, maxv) between(x, vec2(minv, minv), vec2(maxv, maxv))

WRATH_IMAGE_REPEAT_MODE_PREC vec2
wrath_compute_texture_coordinate(in WRATH_IMAGE_REPEAT_MODE_PREC vec2 p)
{
  WRATH_IMAGE_REPEAT_MODE_PREC vec2 start;
  WRATH_IMAGE_REPEAT_MODE_PREC vec2 size;
  WRATH_IMAGE_REPEAT_MODE_PREC float mode;
  
  #if defined(WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK)
  {
    start=vec2(fetch_node_value(WRATH_TEXTURE_subrect_x),
               fetch_node_value(WRATH_TEXTURE_subrect_y));
    size=vec2(fetch_node_value(WRATH_TEXTURE_subrect_w),
              fetch_node_value(WRATH_TEXTURE_subrect_h));
    mode=fetch_node_value(WRATH_IMAGE_repeat_mode);
  }
  #else
  {
    start=WRATH_IMAGE_VALUE_NORMALIZED_varying0.xy;
    size=WRATH_IMAGE_VALUE_NORMALIZED_varying0.zw;
    mode=WRATH_IMAGE_VALUE_NORMALIZED_varying1;
  }
  #endif

  WRATH_IMAGE_REPEAT_MODE_PREC vec2 repeat_applied;
  WRATH_IMAGE_REPEAT_MODE_PREC vec2 mode_xy, q;

  q=p/size;
  mode_xy=vec2(0.1*floor(mode), fract(mode));
  
  repeat_applied=
    + step(mode_xy, vec2(0.5, 0.5))*wrath_compute_clamp(q)
    + vec2_between(mode_xy, 0.5, 0.7)*wrath_compute_repeat(q)
    + vec2_between(mode_xy, 0.7, 1.0)*wrath_compute_mirror_repeat(q);
  
  #ifdef WRATH_BRUSH_FLIP_IMAGE_Y
  {
    repeat_applied.y = 1.0 - repeat_applied.y;
  }
  #endif

  return repeat_applied*size + start;

}


#undef vec2_between
#undef between


