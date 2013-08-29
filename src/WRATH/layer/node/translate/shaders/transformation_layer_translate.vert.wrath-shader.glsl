/*! 
 * \file transformation_layer_translate.vert.wrath-shader.glsl
 * \brief file transformation_layer_translate.vert.wrath-shader.glsl
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
  Per item uniforms:

  -  fetch_node_value(WRATH_LAYER_TRANSLATE_X) translate-x
  -  fetch_node_value(WRATH_LAYER_TRANSLATE_Y) translate-y
  -  fetch_node_value(WRATH_LAYER_TRANSLATE_Z) normalized-z
  -  fetch_node_value(WRATH_LAYER_TRANSLATE_SCALE) scaling factor
  -  fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MIN_X) x-min clip window
  -  fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MAX_X) x-max clip window
  -  fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MIN_Y) y-min clip window
  -  fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MAX_Y) y-max clip window

 */


/*
  "pvm" matrix, i.e. from after node
  transformation to gl_Position
 */
uniform mediump mat4 clip_matrix_layer;


#ifdef CLIP_VIA_DISCARD
shader_out mediump vec4 clipping_via_discard;
#endif




vec2
apply_2d_transformation(in highp vec2 p)
{
  vec2 tr;
  float sc;

  tr=vec2(fetch_node_value(WRATH_LAYER_TRANSLATE_X),
          fetch_node_value(WRATH_LAYER_TRANSLATE_Y));
  sc=fetch_node_value(WRATH_LAYER_TRANSLATE_SCALE);
  return p*abs(sc) + tr.xy;
}


vec4
compute_gl_position(in highp vec3 pos)
{
  highp vec2 tpos;
  highp vec4 clip_pos;

  tpos=apply_2d_transformation(pos.xy);
  clip_pos=clip_matrix_layer*vec4(tpos, pos.z, 1.0);
  return vec4(clip_pos.xy, 
              fetch_node_value(WRATH_LAYER_TRANSLATE_Z)*clip_pos.w, 
              clip_pos.w);
}





vec4
compute_gl_position_and_apply_clipping(in highp vec3 pos)
{
  
#if defined(CLIP_VIA_DISCARD) || defined(CLIP_VIA_CLIP_VERTEX) || defined(CLIP_VIA_CLIP_DISTANCE)
  vec4 values;
  

  if(fetch_node_value(WRATH_LAYER_TRANSLATE_SCALE)>0.0)
    {

      values.x = pos.x - fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MIN_X);
      values.y = fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MAX_X) - pos.x;
      values.z = pos.y - fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MIN_Y);
      values.w = fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MAX_Y) - pos.y;
    }
  else
    {
      values=vec4(1.0, 1.0, 1.0, 1.0);
    }
#endif


#ifdef CLIP_VIA_DISCARD
  clipping_via_discard=values;
#elif defined(CLIP_VIA_CLIP_VERTEX)
  gl_ClipVertex=values;
#elif defined(CLIP_VIA_CLIP_DISTANCE)
  gl_ClipDistance[0]=values.x;
  gl_ClipDistance[1]=values.y;
  gl_ClipDistance[2]=values.z;
  gl_ClipDistance[3]=values.w;
#endif

  return compute_gl_position(pos);
}

/*
  the nice way to do clipping is as follows:
  vertex shader processes shader_in data that has the form:
    1) top left corner of a quad, quad is drawn parallel to clipping window
    2) size of quad
    3) normalized coordinate (a,b) with 0<=a<=1, 0<=b<=1

  this routine computes the normalized coordinate
  to use after clipping.
 */
vec2
compute_clipped_normalized_coordinate(in highp vec2 normalized_coordinate,
                                      in highp vec2 quad_top_left,
                                      in highp vec2 quad_size)
{
  highp float scale_jazz;

  scale_jazz=fetch_node_value(WRATH_LAYER_TRANSLATE_SCALE);

  if(scale_jazz>0.0)
    {
      
      normalized_coordinate*=quad_size;
      normalized_coordinate+=quad_top_left;
      
      normalized_coordinate.x=clamp(normalized_coordinate.x,
                                    fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MIN_X),
                                    fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MAX_X));

      normalized_coordinate.y=clamp(normalized_coordinate.y,
                                    fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MIN_Y),
                                    fetch_node_value(WRATH_LAYER_TRANSLATE_CLIP_WINDOW_MAX_Y));
      
      normalized_coordinate-=quad_top_left;
      return normalized_coordinate/quad_size;
    }
  else
    {
      return normalized_coordinate;
    }

}
