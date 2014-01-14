/*! 
 * \file image-value-normalized-coordinate.compute.wrath-shader.glsl
 * \brief file image-value-normalized-coordinate.compute.wrath-shader.glsl
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

#if defined(WRATH_IMAGE_REPEAT_MODE_VS) || defined(WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK)
#else
  shader_in WRATH_IMAGE_REPEAT_MODE_PREC vec4 WRATH_IMAGE_VALUE_NORMALIZED_varying;
#endif


WRATH_IMAGE_REPEAT_MODE_PREC vec2
wrath_compute_texture_coordinate(in WRATH_IMAGE_REPEAT_MODE_PREC vec2 p)
{
  WRATH_IMAGE_REPEAT_MODE_PREC vec2 start;
  WRATH_IMAGE_REPEAT_MODE_PREC vec2 size, q;
  
  #if defined(WRATH_IMAGE_REPEAT_MODE_VS) || defined(WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK)
  {
    start=vec2(fetch_node_value(WRATH_TEXTURE_subrect_x),
               fetch_node_value(WRATH_TEXTURE_subrect_y));
    size=vec2(fetch_node_value(WRATH_TEXTURE_subrect_w),
              fetch_node_value(WRATH_TEXTURE_subrect_h));
  }
  #else
  {
    start=WRATH_IMAGE_VALUE_NORMALIZED_varying.xy;
    size=WRATH_IMAGE_VALUE_NORMALIZED_varying.zw;
  }
  #endif

  WRATH_IMAGE_REPEAT_MODE_PREC vec2 return_value;
  
  q=p/size;

  return_value.x=WRATH_IMAGE_REPEAT_MODE_X(q.x);
  return_value.y=WRATH_IMAGE_REPEAT_MODE_Y(q.y);
  
  #ifdef WRATH_BRUSH_FLIP_IMAGE_Y
  {
    return_value.y = 1.0 - return_value.y;
  }
  #endif

  return return_value*size + start;

}
