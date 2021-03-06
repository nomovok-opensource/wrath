/*! 
 * \file wobbly.vert.glsl
 * \brief file wobbly.vert.glsl
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
  normalized coordinate within the quad, 
  (0,0)=bottom left, (1,1)=top right
 */
shader_in mediump vec2 normalized_coordinate;

/*
  if fragment node value fetch is not supported,
  we need to forward the values to the fragment shader
 */
#ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK

  //needed values for wobbling
  shader_out mediump vec3 wobbly_values;

  //needed values:
  // - .xy = ring outer, ring inner
  shader_out mediump vec2 outer_inner_radii; 
#endif

/*
  the coordinates:
   - .xy = relative to center of the circle
   - .zw = relative to (min,min)-corder of circle
*/
shader_out mediump vec4 circle_coords;

void
shader_main(void)
{
  highp vec2 circle_pos;
  highp vec2 clipped_normalized;
  highp float inner, outer, mm;
  highp vec2 bbox, brush_scale_factor;

  inner=fetch_node_value(inner_radius);
  outer=fetch_node_value(outer_radius);




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
  #ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
  {
    wobbly_values=vec3(fetch_node_value(wobbly_angular_speed),
                       fetch_node_value(wobbly_magnitude),
                       fetch_node_value(wobbly_phase));
    outer_inner_radii=vec2(outer, inner);
  }
  #endif

  /*
    some node types provide per-node clipping against a quad
    which is parrallel to the item coordinate axis. For
    these, the functions compute_clipped_normalized_coordinate
    is provided to allow for the vertex shader to provide
    clipping a quad against a quad.
    
    We are to draw a wobbled ring. The ring unwobbled is
    of outer radius outer, thus unwobbled it's width 
    and height are 2*outer, but it gets wobbled in the
    y-direction, so the bounds are given by:
      
     X: [0, 2*outer]
     Y: [-wobble, 2*outer+wobble]

   */  
  mm=fetch_node_value(wobbly_magnitude);
  bbox=vec2(2.0*outer, 2.0*mm + 2.0*outer); 
  clipped_normalized=compute_clipped_normalized_coordinate(normalized_coordinate,
                                                           vec2(0.0, -mm), bbox);
 

  //the coordinate of the corner in item coordinates
  circle_pos.xy=clipped_normalized*bbox + vec2(0.0, -mm);

  //offset it so that the rect center is (0,0)
  circle_coords.xy=circle_pos.xy - vec2(outer, outer);

  

  gl_Position=compute_gl_position_and_apply_clipping(vec3(circle_pos.xy, -1));

  /*
    if an image is present, we want to rescale
    the brush coordinate so that the image
    maps to the rectangle [0, 2*outer]x[0, 2*outer]
  */
  #if defined(WRATH_BRUSH_IMAGE_PRESENT)
  {
    brush_scale_factor=
      vec2(fetch_node_value(WRATH_TEXTURE_subrect_w), 
           fetch_node_value(WRATH_TEXTURE_subrect_h))
      * wrath_brush_imageTextureSize
      / bbox;
                            
  }
  #else
  {
    brush_scale_factor=vec2(1.0, 1.0);
  }
  #endif

  /*
    circle_coords.zw is to hold the brush position
    before we tweak it in the fragment shader 
    for when the brush is non-linear.
   */
  circle_coords.zw=circle_pos.xy*brush_scale_factor;

  /*
    WRATHShaderBrushSourceHoard defines the macros:
    - WRATH_LINEAR_BRUSH_PRESENT if the brush is present and mapping is linear
      (thus position of brush is determined by vertex shader)
    - WRATH_NONLINEAR_BRUSH_PRESENT if the brush is present and mapping is
      to be non-linear in that the position is set in the
      fragment shader.

    In both cases, the function wrath_shader_brush_prepare is provided
    to initialize the brush; for WRATH_LINEAR_BRUSH_PRESENT the brush
    position is passed, for WRATH_NONLINEAR_BRUSH_PRESENT no arguments
    are to be given.
   */
  #if defined(WRATH_LINEAR_BRUSH_PRESENT)
  {
    wrath_shader_brush_prepare(circle_coords.zw);
  }
  #elif defined(WRATH_NONLINEAR_BRUSH_PRESENT)
  {
    wrath_shader_brush_prepare();
  }
  #endif
}
