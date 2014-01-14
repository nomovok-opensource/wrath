/*! 
 * \file wobbly.frag.glsl
 * \brief file wobbly.frag.glsl
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


#ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
shader_in mediump vec3 wobbly_values;
#endif

shader_in mediump vec2 brush_linear_position;

void
shader_main(void)
{


  mediump vec4 color;
  
  discard_if_clipped(); 

  /*
    WRATHShaderBrushSourceHoard defines the macros:
    - WRATH_LINEAR_BRUSH_PRESENT if the brush is present and mapping is linear
      (thus position of brush is determined by vertex shader)
    - WRATH_NONLINEAR_BRUSH_PRESENT if the brush is present and mapping is
      to be non-linear in that the position is set in the
      fragment shader.

    In both cases, the function wrath_shader_brush_color() is 
    provided to get the color of the brush; 
    for WRATH_NONLINEAR_BRUSH_PRESENT the brush position is passed,
    for WRATH_LINEAR_BRUSH_PRESENT no values is passed (because
    the brush position was determined at the vertex shader)
   */

  #if defined(WRATH_LINEAR_BRUSH_PRESENT)
  {
    color=wrath_shader_brush_color();
  }
  #elif defined(WRATH_NONLINEAR_BRUSH_PRESENT)
  {
    mediump float amplitude, speed, phase;

    /*
      Not all node packers allow for fetching per-node
      values from the fragment shader. The macro
      WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
      (see the description to \ref WRATHItemDrawerFactory)
      is defined if fetching node values from fragmenet
      shader is possible, if it is not then we need
      to fetch them from the vertex shader and forward
      them to the fragment shader.
    */
    #ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    {
      speed=fetch_node_value(wobbly_angular_speed);
      amplitude=fetch_node_value(wobbly_magnitude);
      phase=fetch_node_value(wobbly_phase);
    }
    #else
    {
      speed=wobbly_values.x;
      amplitude=wobbly_values.y;
      phase=wobbly_values.z;
    }
    #endif

    mediump vec2 wobbled;

    wobbled=brush_linear_position;
    wobbled.y+= amplitude*sin( brush_linear_position.x*speed + phase);
    
    color=wrath_shader_brush_color(wobbled);
  }
  #else
  {
    color=vec4(1.0, 0.0, 0.0, 1.0);
  }
  #endif      
    
  
  #ifdef WRATH_BRUSH_PREMULTIPLY_ALPHA
  {
    gl_FragColor=vec4(color.xyz, 1.0)*color.w;
  }
  #else
  {
    gl_FragColor=color;
  }
  #endif
  
}
