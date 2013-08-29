/*! 
 * \file simple_ui_line.vert.glsl
 * \brief file simple_ui_line.vert.glsl
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


shader_in mediump vec3 pos;
shader_in mediump vec4 color;


shader_out mediump vec4 tex_color;
shader_out mediump float reciprocal_scale_factor;


uniform mediump float animation_fx_interpol;
uniform mediump mat2 animation_matrix;

void
shader_main(void)
{
  highp vec4 clip_pos;
  highp vec2 rotated_pos;

  tex_color=color;

  rotated_pos=animation_matrix*pos.xy;
 
  clip_pos=compute_gl_position_and_apply_clipping(vec3(rotated_pos, pos.z));

  gl_Position=vec4(clip_pos.xy, 
		   (1.0-animation_fx_interpol)*clip_pos.zw); 
}
