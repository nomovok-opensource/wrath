// -*- C++ -*-

/*! 
 * \file linear-gradient-values.pre-compute.wrath-shader.glsl
 * \brief file linear-gradient-values.pre-compute.wrath-shader.glsl
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
    WRATH_LINEAR_GRADIENT_PREC precision qualifier 

    WRATH_LINEAR_GRADIENT_VS: correpsonds to WRATHGradientSource::linear_computation,
                              if the symbol is not defined, then computation is in
                              fragment shader
    
    WRATH_LINEAR_GRADIENT_varying name of varying to transmit
    per node values from vertex shader to fragment shader, only needed
    if both WRATH_LINEAR_GRADIENT_VS and WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    are not defined.

    
    
 */

#ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
  void pre_compute_gradient(void) {} 
#else
  shader_out WRATH_LINEAR_GRADIENT_PREC vec4 WRATH_LINEAR_GRADIENT_varying;
  void pre_compute_gradient(void)
  {
    
    WRATH_LINEAR_GRADIENT_varying=vec4(fetch_node_value(WRATH_LINEAR_GRADIENT_p0_x),
                                       fetch_node_value(WRATH_LINEAR_GRADIENT_p0_y),
                                       fetch_node_value(WRATH_LINEAR_GRADIENT_delta_x),
                                       fetch_node_value(WRATH_LINEAR_GRADIENT_delta_y));   
  }

  void pre_compute_gradient(in WRATH_LINEAR_GRADIENT_PREC vec2 p)
  {
    pre_compute_gradient();
  }
#endif


