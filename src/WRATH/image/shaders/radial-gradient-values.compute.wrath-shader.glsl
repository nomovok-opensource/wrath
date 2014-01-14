/*! 
 * \file radial-gradient-values.compute.wrath-shader.glsl
 * \brief file radial-gradient-values.compute.wrath-shader.glsl
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

  for partially linear, we can transmit B(p) from the vertex shader to the fragment shader
  if fragment node fetching is not supported we then need to transmit p-p0, r0_r0 and A
  B is located in WRATH_RADIAL_GRADIENT_varying0, and if fragment fetch is not available,
  WRATH_RADIAL_GRADIENT_varying1=(p-p0, A, r0_r0)

  for fully non-linear and if fragment fetch is not available,
  then 
    WRATH_RADIAL_GRADIENT_varying0=(A, A_r0_delta_r, r0_r0)
    WRATH_RADIAL_GRADIENT_varying1=(p0, delta_p) 
  
 */


#define t0 t01.x
#define t1 t01.y
#define good0 good01.x
#define good1 good01.y

#ifdef WRATH_RADIAL_GRADIENT_PARTIAL_LINEAR
  #ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    #define WRATH_RADIAL_GRADIENT_PARTIAL_LINEAR_REALLY
  #endif
#endif


#ifdef WRATH_RADIAL_GRADIENT_PARTIAL_LINEAR_REALLY
  
  shader_in WRATH_RADIAL_GRADIENT_PREC float WRATH_RADIAL_GRADIENT_varying0;
  #define B WRATH_RADIAL_GRADIENT_varying0

  shader_in WRATH_RADIAL_GRADIENT_PREC vec4 WRATH_RADIAL_GRADIENT_varying1;
  #define A WRATH_RADIAL_GRADIENT_varying1.z
  #define pminusp0 WRATH_RADIAL_GRADIENT_varying1.xy
  #define r0_r0 WRATH_RADIAL_GRADIENT_varying1.w
    

  WRATH_RADIAL_GRADIENT_PREC vec2 wrath_compute_gradient(in WRATH_RADIAL_GRADIENT_PREC vec2 p)
  {
    WRATH_RADIAL_GRADIENT_PREC float C, BB_minus_C, sqrt_BB_minus_C, t;
    WRATH_RADIAL_GRADIENT_PREC vec2 t01, good01;
    
    C=A*( dot(pminusp0, pminusp0) - r0_r0);
    BB_minus_C=B*B - C; 

    /*
      cannot take a root, thus the fragment is invalid,
      bail now by returning indicating gradient
      is not valid at the point.
    */
    if(BB_minus_C<0.0)
      return vec2(0.0, 0.0);

    sqrt_BB_minus_C=sqrt(BB_minus_C);
    t0= B + sqrt_BB_minus_C;
    t1= B - sqrt_BB_minus_C;

    /*
      if both are in [0,1] or both are not,
      take the larger of t0 and t1,
      otherwise take the one that is in range.
      
      The value t0*good0 + t1*good1 is exactly
      t0 or t1 if only one of them is good.    
    */
    good01=step(0.0, t01)*step(t01, vec2(1.0, 1.0));
    
    t=(good0==good1)?
      max(t0, t1):
      dot(good01, t01);

    return vec2(t, 1.0);
  }

  #undef B
  #undef A 
  #undef pminusp0
  #undef r0_r0 

#else

  #ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    shader_in WRATH_RADIAL_GRADIENT_PREC vec3 WRATH_RADIAL_GRADIENT_varying0;
    shader_in WRATH_RADIAL_GRADIENT_PREC vec4 WRATH_RADIAL_GRADIENT_varying1;
    #define A WRATH_RADIAL_GRADIENT_varying0.x
    #define A_r0_delta_r WRATH_RADIAL_GRADIENT_varying0.y
    #define r0_r0 WRATH_RADIAL_GRADIENT_varying0.z
    #define p0 WRATH_RADIAL_GRADIENT_varying1.xy
    #define A_delta_p WRATH_RADIAL_GRADIENT_varying1.zw
  #endif

  WRATH_RADIAL_GRADIENT_PREC vec2 wrath_compute_gradient(in WRATH_RADIAL_GRADIENT_PREC vec2 p)
  {
    #ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
      WRATH_RADIAL_GRADIENT_PREC float A=fetch_node_value(WRATH_RADIAL_GRADIENT_A);
      WRATH_RADIAL_GRADIENT_PREC float A_r0_delta_r=fetch_node_value(WRATH_RADIAL_GRADIENT_A_r0_delta_r);
      WRATH_RADIAL_GRADIENT_PREC float r0_r0=fetch_node_value(WRATH_RADIAL_GRADIENT_r0_r0);
      WRATH_RADIAL_GRADIENT_PREC vec2 p0=vec2(fetch_node_value(WRATH_RADIAL_GRADIENT_p0_x),
                                              fetch_node_value(WRATH_RADIAL_GRADIENT_p0_y));
      WRATH_RADIAL_GRADIENT_PREC vec2 A_delta_p=vec2(fetch_node_value(WRATH_RADIAL_GRADIENT_A_delta_p_x),
                                                     fetch_node_value(WRATH_RADIAL_GRADIENT_A_delta_p_y));
    #endif

      WRATH_RADIAL_GRADIENT_PREC float B, C, BB_minus_C, sqrt_BB_minus_C, t;
    WRATH_RADIAL_GRADIENT_PREC vec2 t01, good01, pminusp0;

    pminusp0=p - p0;

    B=dot(pminusp0, A_delta_p) + A_r0_delta_r;
    C=A*( dot(pminusp0, pminusp0) - r0_r0);
    BB_minus_C=B*B - C; 

    /*
      cannot take a root, bail out now
      indicating cannot take the root.
    */
    if(BB_minus_C<0.0)
      return vec2(0.0, 0.0);

    sqrt_BB_minus_C=sqrt(BB_minus_C);
    t0= B + sqrt_BB_minus_C;
    t1= B - sqrt_BB_minus_C;

    /*
      if both are in [0,1] or both are not,
      take the larger of t0 and t1,
      otherwise take the one that is in range.
      
      The value t0*good0 + t1*good1 is exactly
      t0 or t1 if only one of them is good.    
    */
    good01=step(0.0, t01)*step(t01, vec2(1.0, 1.0));
    
    t=(good0==good1)?
      max(t0, t1):
      dot(good01, t01);

    return vec2(t, 1.0);
  }

  #ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    #undef A 
    #undef A_r0_delta_r 
    #undef r0_r0 
    #undef p0 
    #undef A_delta_p 
  #endif


#endif



#undef t0
#undef t1
#undef good0
#undef good1



#ifdef WRATH_RADIAL_GRADIENT_PARTIAL_LINEAR_REALLY
#undef WRATH_RADIAL_GRADIENT_PARTIAL_LINEAR_REALLY
#endif
