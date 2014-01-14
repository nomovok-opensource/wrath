/*! 
 * \file radial-gradient-values.pre_compute.wrath-shader.glsl
 * \brief file radial-gradient-values.pre_compute.wrath-shader.glsl
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
  The computation:

  C0= (p0, r0)
  C1= (p1, r1)

  C(t)= ( p(t), r(t) )

  r(t)= r0 + (r1-r0)*t
  p(t)= p0 + (p1-p0)*t

  Let delta_r=r1-r0, delta_p=p1-p0

  

  find t so that p is an element of C(t), i.e

  ||p-p(t)||^2 = r(t)^2

  i.e.

  <p-p(t), p-p(t)> = r(t)^2

  < p - p0 - t*delta_p, p - p0 - t*delta_p>           = (r0 + t*delta_r)^2
  ||p-p0||^2 - 2t*<p-p0, delta_p> + ||delta_p||^2 t^2 = (r0)^2 + 2t*delta_r*r0 + t^2 delta_r^2

  t^2 ( ||delta_p||^2 - delta_r^2 ) + t( - 2*<p-p0, delta_p> - 2*delta_r*r0 ) + ( ||p-p0||^2 - r0^2) = 0

  thus:

  a= ||delta_p||^2 - delta_r^2 (constant over p)
  b(p) = - 2<p-p0, delta_p> - 2*delta_r*r0 (linear over p)
  c(p) = ||p-p0||^2 - r0^2 (quadratic over p)

  t = ( - b(p) +/- sqrt( b(p)^2 - 4*a*c(p) ) )/(2*a)

   =   -b(p)/2a +/- sqrt( (b(p)/2a)^2 - c(p)/a )

  so we store:

  A := 1/a
  B(p) := - b(p)/(2*a) = ( <p-p0, delta_p> + delta_r*r0) * A
  C(p) := c(p)/a  = ( ||p-p0||^2 - r0^2) * A

  then

   t = B +/- sqrt( B*B - C)

  thus we need the per-node values:

  A:= 1/( ||delta_p||^2 - delta_r^2 ) [float]
  A_delta_p := A*delta_p [vec2]
  A_r0_delta_r := A*r0*delta_r [float]
  p0 [vec2]
  r0_r0:= r0^2 [float]
  

  then:

    B(p) = <p-p0, A_delta_p> + A_r0_delta_r
    C(p) = (||p-p0||^2 - r0_r0)*A

  We only support partially non-linear if fragment fetch is not possible.
  In that case, we transmit B(p) from the vertex shader along with
  p-p0, r0_r0 and A
 */


/*
  expected macros:
    WRATH_RADIAL_GRADIENT_PREC precision of computation
    WRATH_RADIAL_GRADIENT_PARTIAL_LINEAR if defined do some computation in wrath_pre_compute_gradient
 */


#ifdef WRATH_RADIAL_GRADIENT_PARTIAL_LINEAR
  
  #ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    void wrath_pre_compute_gradient(in WRATH_RADIAL_GRADIENT_PREC vec2 p) {}
  #else
    shader_out WRATH_RADIAL_GRADIENT_PREC float WRATH_RADIAL_GRADIENT_varying0;
    shader_out WRATH_RADIAL_GRADIENT_PREC vec4 WRATH_RADIAL_GRADIENT_varying1;  
    void wrath_pre_compute_gradient(in WRATH_RADIAL_GRADIENT_PREC vec2 p)
    {
      WRATH_RADIAL_GRADIENT_PREC vec2 pminusp0;
      WRATH_RADIAL_GRADIENT_PREC float B;
      vec2 A_delta_p;

      pminusp0=p - vec2(fetch_node_value(WRATH_RADIAL_GRADIENT_p0_x),
                        fetch_node_value(WRATH_RADIAL_GRADIENT_p0_y));

      A_delta_p=vec2(fetch_node_value(WRATH_RADIAL_GRADIENT_A_delta_p_x),
                     fetch_node_value(WRATH_RADIAL_GRADIENT_A_delta_p_y));

      B=dot(pminusp0, A_delta_p) + fetch_node_value(WRATH_RADIAL_GRADIENT_A_r0_delta_r);
      
      WRATH_RADIAL_GRADIENT_varying0=B;
      WRATH_RADIAL_GRADIENT_varying1=vec4(pminusp0.x, pminusp0.y,
                                          fetch_node_value(WRATH_RADIAL_GRADIENT_A),
                                          fetch_node_value(WRATH_RADIAL_GRADIENT_r0_r0));
    }
  #endif

#else

  #ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK

    void wrath_pre_compute_gradient(void) {}

  #else
    shader_out WRATH_RADIAL_GRADIENT_PREC vec3 WRATH_RADIAL_GRADIENT_varying0;
    shader_out WRATH_RADIAL_GRADIENT_PREC vec4 WRATH_RADIAL_GRADIENT_varying1;

    void wrath_pre_compute_gradient(void) 
    {

      WRATH_RADIAL_GRADIENT_varying0=vec3(fetch_node_value(WRATH_RADIAL_GRADIENT_A),
                                          fetch_node_value(WRATH_RADIAL_GRADIENT_A_r0_delta_r),
                                          fetch_node_value(WRATH_RADIAL_GRADIENT_r0_r0));

      WRATH_RADIAL_GRADIENT_varying1=vec4(fetch_node_value(WRATH_RADIAL_GRADIENT_p0_x),
                                          fetch_node_value(WRATH_RADIAL_GRADIENT_p0_y),
                                          fetch_node_value(WRATH_RADIAL_GRADIENT_A_delta_p_x),
                                          fetch_node_value(WRATH_RADIAL_GRADIENT_A_delta_p_y));

      
                                          
    }

  #endif

#endif



