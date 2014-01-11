/*! 
 * \file simple_ui_shape_translate_layer.vert.glsl
 * \brief file simple_ui_shape_translate_layer.vert.glsl
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


shader_in mediump vec2 tex;


//pos meanings:
// .xy: location in UI co-ordinates (2D)
shader_in mediump vec2 pos;
shader_in mediump vec2 normal;
shader_out mediump vec4 tex_color;

#ifdef AA_HINT
shader_in mediump float in_aa_hint;
shader_out mediump float reciprocal_scale_factor;
shader_out mediump float aa_hint;
#endif

uniform mediump float animation_fx_interpol;
uniform mediump mat2 animation_matrix;



void
shader_main(void)
{
  highp vec2 tpos;
  highp vec4 clip_pos;
  highp vec4 vv;

  vv.x=fetch_node_value(stroke_color_red);
  vv.y=fetch_node_value(stroke_color_green);
  vv.z=fetch_node_value(stroke_color_blue);
  vv.w=fetch_node_value(stroke_width);

  vv.w=max(0.0, vv.w);

  tpos=animation_matrix*(pos.xy + vv.w*normal);
  clip_pos=compute_gl_position_and_apply_clipping(vec3(tpos, -1.0));
  
  #ifdef AA_HINT
    aa_hint=in_aa_hint;
  #endif


  tex_color=vec4(vv.rgb, 1.0);
  tex_color*=(1.0-2.0*animation_fx_interpol);
  gl_Position=vec4(clip_pos.xy, 
                   (1.0-animation_fx_interpol)*clip_pos.zw);
  
}
