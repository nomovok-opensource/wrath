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
  position:
    .xy=width height of rect
    .z=z coordinate to feed to the pvm matrix
       value is common to all corners of the quad.
 */
shader_in mediump vec3 size_and_z;

/*
  brush:
    .xy=offset to apply for brush
    .zw=strech to apply for brush
 */
shader_in mediump vec4 brush;

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
shader_out mediump vec3 wobbly_values;
#endif

shader_out mediump vec2 brush_linear_position;

void
shader_main(void)
{
  highp vec2 frag_pos;
  highp vec2 clipped_normalized, offset;


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
  }
  #endif

  /*
    some node types provide per-node clipping against a quad
    which is parrallel to the item coordinate axis. For
    these, the functions compute_clipped_normalized_coordinate
    is provided to allow for the vertex shader to provide
    clipping a quad against a quad.
   */
  clipped_normalized=compute_clipped_normalized_coordinate(normalized_coordinate,
                                                           vec2(0.0, 0.0), size_and_z.xy);
 

  frag_pos.xy=clipped_normalized*size_and_z.xy;
  brush_linear_position=frag_pos.xy*brush.zw + brush.xy;

  gl_Position=compute_gl_position_and_apply_clipping(vec3(frag_pos.xy, size_and_z.z));

  /*
    WRATHShaderBrushSourceHoard defines the macros:
    - LINEAR_BRUSH_PRESENT if the brush is present and mapping is linear
      (thus position of brush is determined by vertex shader)
    - NONLINEAR_BRUSH_PRESENT if the brush is present and mapping is
      to be non-linear in that the position is set in the
      fragment shader.

    In both cases, the function wrath_shader_brush_prepare is provided
    to initialize the brush; for LINEAR_BRUSH_PRESENT the brush
    position is passed, for NONLINEAR_BRUSH_PRESENT no arguments
    are to be given.
   */
  #if defined(LINEAR_BRUSH_PRESENT)
  {
    wrath_shader_brush_prepare(brush_linear_position);
  }
  #elif defined(NONLINEAR_BRUSH_PRESENT)
  {
    wrath_shader_brush_prepare();
  }
  #endif
}
