/*! 
 * \file simple_ui_shape.vert.glsl
 * \brief file simple_ui_shape.vert.glsl
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



//pos meanings:
// .xy: location in UI co-ordinates (2D)
shader_in mediump vec2 pos;

uniform mediump float animation_fx_interpol;
uniform mediump mat2 animation_matrix;


#ifdef AA_HINT
shader_in mediump float in_aa_hint;
shader_out mediump float aa_hint;
#endif



shader_out mediump vec4 tex_color;


void
shader_main(void)
{
  
  tex_color=vec4(fetch_node_value(color_red),
		 fetch_node_value(color_green), 
		 fetch_node_value(color_blue), 
                 fetch_node_value(color_alpha));
  tex_color*=(1.0-2.0*animation_fx_interpol);

  #ifdef AA_HINT
  {
    aa_hint=in_aa_hint;
  }
  #endif
  
  mediump vec2 tpos;
  mediump vec4 clip_pos;

  tpos=animation_matrix*pos.xy;
  clip_pos=compute_gl_position_and_apply_clipping(vec3(tpos, -1.0));

  gl_Position=vec4(clip_pos.xy,
		   (1.0-animation_fx_interpol)*clip_pos.zw);
  
}
