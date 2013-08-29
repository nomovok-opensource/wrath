/*! 
 * \file simple_ui.vert.glsl
 * \brief file simple_ui.vert.glsl
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
  normalized coordinate within the quad, 
  (0,0)=bottom left, (1,1)=top right
 */
shader_in mediump vec2 normalized_coordinate;

shader_out mediump vec4 tex_color;
shader_out mediump vec2 tex_coord;


uniform mediump float animation_fx_interpol;
uniform mediump mat2 animation_matrix;
uniform mediump vec2 imageTextureSize;

void
shader_main(void)
{
  highp vec4 clip_pos;
  highp vec2 rotated_pos;
  highp vec2 clipped_normalized, offset;

  
  clipped_normalized=compute_clipped_normalized_coordinate(normalized_coordinate,
                                                           vec2(0.0, 0.0), size_and_z.xy);

  offset=clipped_normalized*size_and_z.xy;
  rotated_pos=animation_matrix*(offset-size_and_z.xy*0.5) + size_and_z.xy*0.5;

  tex_coord=vec2(fetch_node_value(tex_x), fetch_node_value(tex_y))
    + clipped_normalized*vec2(fetch_node_value(tex_w), fetch_node_value(tex_h));

  tex_color=vec4(fetch_node_value(color_red),
                 fetch_node_value(color_green),
                 fetch_node_value(color_blue),
                 fetch_node_value(color_alpha));
 
  clip_pos=compute_gl_position(vec3(rotated_pos, size_and_z.z));
  gl_Position=vec4(clip_pos.xy, 
		   (1.0-animation_fx_interpol)*clip_pos.zw); 
}
