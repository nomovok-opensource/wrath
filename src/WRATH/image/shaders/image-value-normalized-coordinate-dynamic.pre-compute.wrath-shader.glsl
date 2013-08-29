/*! 
 * \file image-value-normalized-coordinate-dynamic.pre-compute.wrath-shader.glsl
 * \brief file image-value-normalized-coordinate-dynamic.pre-compute.wrath-shader.glsl
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
  /*
    all computations done in vertex shader
    or can fetch from fragment shader,
    so pre_compute does nothing.
   */
  void pre_compute_texture_coordinate(void) {}
  void pre_compute_texture_coordinate(in WRATH_IMAGE_REPEAT_MODE_PREC vec2 p) {}

#else

  /*
    we need to transmit the fetch values from vertex
    to fragment shader if WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    is not defined
   */
  shader_out WRATH_IMAGE_REPEAT_MODE_PREC vec4 WRATH_IMAGE_VALUE_NORMALIZED_varying0;
  shader_out WRATH_IMAGE_REPEAT_MODE_PREC float WRATH_IMAGE_VALUE_NORMALIZED_varying1;
  void pre_compute_texture_coordinate(void)
  {
    WRATH_IMAGE_VALUE_NORMALIZED_varying0=vec4(fetch_node_value(WRATH_TEXTURE_subrect_x),
                                               fetch_node_value(WRATH_TEXTURE_subrect_y),
                                               fetch_node_value(WRATH_TEXTURE_subrect_w),
                                               fetch_node_value(WRATH_TEXTURE_subrect_h));
    WRATH_IMAGE_VALUE_NORMALIZED_varying1=fetch_node_value(WRATH_IMAGE_repeat_mode);
  }

  void pre_compute_texture_coordinate(in WRATH_IMAGE_REPEAT_MODE_PREC vec2 p)
  {
    pre_compute_texture_coordinate();
  }

#endif



