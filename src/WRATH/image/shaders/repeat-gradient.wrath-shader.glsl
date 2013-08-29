/*! 
 * \file repeat-gradient.wrath-shader.glsl
 * \brief file repeat-gradient.wrath-shader.glsl
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
    WRATH_REPEAT_GRADIENT_UNDERLYING_GRADIENT_compute underlying gradient compute function
    WRATH_REPEAT_GRADIENT_compute name to use for compute function
 */
       
#ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
  WRATH_REPEAT_GRADIENT_PREC float WRATH_REPEAT_GRADIENT_compute(in WRATH_REPEAT_GRADIENT_PREC vec2 p)
  {

    WRATH_REPEAT_GRADIENT_PREC vec2 q, f, dt, dp;

    dp=vec2(fetch_node_value(WRATH_GRADIENT_window_x),
	    fetch_node_value(WRATH_GRADIENT_window_y));

    dt=vec2(fetch_node_value(WRATH_GRADIENT_window_delta_x),
	    fetch_node_value(WRATH_GRADIENT_window_delta_y));

    f=fract( (p-dp)/dt );
    q=dp + f*dt;
    return WRATH_REPEAT_GRADIENT_UNDERLYING_GRADIENT_compute(q);
  }
#else
  shader_in WRATH_REPEAT_GRADIENT_PREC vec4 WRATH_REPEAT_GRADIENT_VARYING_LABEL;
  WRATH_REPEAT_GRADIENT_PREC float WRATH_REPEAT_GRADIENT_compute(in WRATH_REPEAT_GRADIENT_PREC vec2 p)
  {
    WRATH_REPEAT_GRADIENT_PREC vec2 q, f;

    f=fract( (p-WRATH_REPEAT_GRADIENT_VARYING_LABEL.xy)/WRATH_REPEAT_GRADIENT_VARYING_LABEL.zw );
    q=WRATH_REPEAT_GRADIENT_VARYING_LABEL.xy + f*WRATH_REPEAT_GRADIENT_VARYING_LABEL.zw;
    return WRATH_REPEAT_GRADIENT_UNDERLYING_GRADIENT_compute(q);
  }
#endif
