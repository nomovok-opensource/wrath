/*! 
 * \file transformation_layer_rotate_translate.vert.wrath-shader.glsl
 * \brief file transformation_layer_rotate_translate.vert.wrath-shader.glsl
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
  Holds the translation, scaling and rotation as:
    scale=s
    Rotation= A -B
              B  A
    translation = (a,b)

  fetch_node_value(WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_RX) = sA
  fetch_node_value(WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_RY) = sB
  fetch_node_value(WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_TX) = a
  fetch_node_value(WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_TY) = b
  fetch_node_value(WRATH_LAYER_ROTATE_TRANSLATE_Z) = z normalized-z
 */


/*
  "pvm" matrix, i.e. from after node
  transformation to gl_Position
 */
uniform mediump mat4 clip_matrix_layer;




vec2
apply_2d_transformation(in highp vec2 p)
{
  vec4 tr;
  vec2 pp;

  tr=vec4(fetch_node_value(WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_RX),
          fetch_node_value(WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_RY),
          fetch_node_value(WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_TX),
          fetch_node_value(WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_TY));
  pp=vec2(p.x*tr.x - p.y*tr.y, p.x*tr.y+p.y*tr.x);
  pp+=tr.zw;

  return pp;
}


vec4
compute_gl_position(in highp vec3 pos)
{
  highp vec2 tpos;
  highp float effective_z;
  highp vec4 clip_pos;

  tpos=apply_2d_transformation(pos.xy);
  clip_pos=clip_matrix_layer*vec4(tpos, pos.z, 1.0);
  return vec4(clip_pos.xy, 
              clip_pos.w*fetch_node_value(WRATH_LAYER_ROTATE_TRANSLATE_Z), 
              clip_pos.w);
}

vec4
compute_gl_position_and_apply_clipping(in highp vec3 pos)
{
  return compute_gl_position(pos);
}


vec2
compute_clipped_normalized_coordinate(in highp vec2 normalized_coordinate,
                                      in highp vec2 quad_top_left,
                                      in highp vec2 quad_size)
{
  return normalized_coordinate;
}
