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
  We use WRATHDefaultTextAttributePacker to
  pack the attributes, the description of the
  class names the attributes it packs
  and their function
 */



/*pos meanings, , attribute from
  WRATHDefaultTextAttributePacker:

 .xy: location before transformation of bottom left of glyph
 .z : z-transformation the z value to feed to transformation matrix
 .w : scaling factor that created the position.
*/
shader_in highp vec4 pos;


/*
  additional stretching to apply to glyph, attribute from
  WRATHDefaultTextAttributePacker
  .x stretching in x
  .y stretching in y
  Note that thus the total stretch in x is glyph_stretch.x*pos.w
  Note that thus the total stretch in y is glyph_stretch.y*pos.w
 */
shader_in highp vec2 glyph_stretch;


/*
  size of glyph in _pixels_ on the texture holding the glyph, 
  attribute from WRATHDefaultTextAttributePacker.
  .xy: glyph size in texels
  .zw: bottom left corner in texel of glyph on texture page
 */
shader_in highp vec4 glyph_size_and_bottom_left;

/*
  normalized coordinate within the glyph, attribute from
  WRATHDefaultTextAttributePacker
 */
shader_in highp vec2 glyph_normalized_coordinate;

/*
  Color of the glyph, attribute from
  WRATHDefaultTextAttributePacker
 */
shader_in mediump vec4 color;

/*
  color
 */
shader_out mediump vec4 tex_color;





/*
  if fragment node value fetch is not supported,
  we need to forward the values to the fragment shader
 */
#ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
shader_out mediump vec3 wobbly_values;
#endif

/*
  forward the glyph_linear_position to the fragment
  shader, that is the value from we will distort
 */
shader_out mediump vec4 glyph_linear_position_and_size;

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
    clipping a quad against a quad;

    note that we inflate glyph_normalized_coordinate, this
    is so that we have additional room on the quad
    to distort the glyph, we inflate from [0,1] to
    [-0.5, 1.5]
   */
  vec2 inflated_normalized, sz;

  inflated_normalized=2.0*glyph_normalized_coordinate - vec2(0.5, 0.5);
  sz=glyph_size_and_bottom_left.xy*glyph_stretch.xy*pos.w;
  clipped_normalized=compute_clipped_normalized_coordinate(glyph_normalized_coordinate,
                                                           pos.xy - 0.5*sz, 
                                                           sz);
  /*
    forward the glyph size and linear position to
    the fragment shader
   */
  glyph_linear_position_and_size.xy=clipped_normalized*glyph_size_and_bottom_left.xy;
  glyph_linear_position_and_size.zw=glyph_size_and_bottom_left.xy;

  gl_Position=compute_gl_position(vec3(pos.xy + glyph_linear_position_and_size.xy, 
                                       pos.z));
  tex_color=color;
}
