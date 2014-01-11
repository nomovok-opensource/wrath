/*! 
 * \file repeat-gradient.pre-compute.wrath-shader.glsl
 * \brief file repeat-gradient.pre-compute.wrath-shader.glsl
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
  expected macros:
    WRATH_REPEAT_GRADIENT_PREC specifies precision
    WRATH_REPEAT_GRADIENT_VARYING_LABEL specifier label for the varying
    WRATH_REPEAT_GRADIENT_UNDERLYING_GRADIENT_pre_compute underlying gradient precompute function
    WRATH_REPEAT_GRADIENT_pre_compute name to use for pre-compute function
 */
#ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
  void WRATH_REPEAT_GRADIENT_pre_compute(void) 
  {
    WRATH_REPEAT_GRADIENT_UNDERLYING_GRADIENT_pre_compute();
  } 
#else
  shader_out WRATH_REPEAT_GRADIENT_PREC vec4 WRATH_REPEAT_GRADIENT_VARYING_LABEL;
  void WRATH_REPEAT_GRADIENT_pre_compute(void) 
  {
    WRATH_REPEAT_GRADIENT_UNDERLYING_GRADIENT_pre_compute();
    WRATH_REPEAT_GRADIENT_VARYING_LABEL=vec4(fetch_node_value(WRATH_GRADIENT_window_x),
                                             fetch_node_value(WRATH_GRADIENT_window_y),
                                             fetch_node_value(WRATH_GRADIENT_window_delta_x),
                                             fetch_node_value(WRATH_GRADIENT_window_delta_y));
  }
#endif
