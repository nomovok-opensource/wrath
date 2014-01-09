// -*- C++ -*-

/*! 
 * \file linear-gradient-values.compute.wrath-shader.glsl
 * \brief file linear-gradient-values.compute.wrath-shader.glsl
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

#ifdef WRATH_GL_VERTEX_SHADER
#define WRATH_LINEAR_GRADIENT_RETURN_TYPE float
#define WRATH_MAKE_WRATH_LINEAR_GRADIENT_RETURN_TYPE(X) X
#else
#define WRATH_LINEAR_GRADIENT_RETURN_TYPE vec2
#define WRATH_MAKE_WRATH_LINEAR_GRADIENT_RETURN_TYPE(X) vec2(X, 1.0)
#endif


#if defined(WRATH_LINEAR_GRADIENT_VS) || defined(WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK)

    WRATH_LINEAR_GRADIENT_PREC WRATH_LINEAR_GRADIENT_RETURN_TYPE compute_gradient(in WRATH_LINEAR_GRADIENT_PREC vec2 p)
    {
      vec2 pt, dt;
      
      pt=vec2(fetch_node_value(WRATH_LINEAR_GRADIENT_p0_x),
              fetch_node_value(WRATH_LINEAR_GRADIENT_p0_y));

      dt=vec2(fetch_node_value(WRATH_LINEAR_GRADIENT_delta_x),
              fetch_node_value(WRATH_LINEAR_GRADIENT_delta_y));

      return WRATH_MAKE_WRATH_LINEAR_GRADIENT_RETURN_TYPE(dot(p - pt, dt));
    }

#else
  
   shader_in WRATH_LINEAR_GRADIENT_PREC vec4 WRATH_LINEAR_GRADIENT_varying;
   
   WRATH_LINEAR_GRADIENT_PREC WRATH_LINEAR_GRADIENT_RETURN_TYPE compute_gradient(in WRATH_LINEAR_GRADIENT_PREC vec2 p)
   {
     WRATH_LINEAR_GRADIENT_PREC vec2 pt, dt;

     pt=WRATH_LINEAR_GRADIENT_varying.xy;
     dt=WRATH_LINEAR_GRADIENT_varying.zw;

     return WRATH_MAKE_WRATH_LINEAR_GRADIENT_RETURN_TYPE(dot(p-pt, dt));
   }

#endif


#undef WRATH_MAKE_WRATH_LINEAR_GRADIENT_RETURN_TYPE
#undef WRATH_LINEAR_GRADIENT_RETURN_TYPE


