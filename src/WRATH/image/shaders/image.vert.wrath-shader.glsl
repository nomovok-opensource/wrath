/*! 
 * \file image.vert.wrath-shader.glsl
 * \brief file image.vert.wrath-shader.glsl
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


void
shader_main(void)
{
  highp vec2 frag_pos;
  highp vec2 clipped_normalized, offset;


  clipped_normalized=compute_clipped_normalized_coordinate(normalized_coordinate,
                                                           vec2(0.0, 0.0), size_and_z.xy);
 

  frag_pos.xy=clipped_normalized*size_and_z.xy;
  gl_Position=compute_gl_position_and_apply_clipping(vec3(frag_pos.xy, size_and_z.z));

  #ifdef APPLY_BRUSH
  wrath_shader_brush_prepare(frag_pos.xy*brush.zw + brush.xy);
  #endif
}
