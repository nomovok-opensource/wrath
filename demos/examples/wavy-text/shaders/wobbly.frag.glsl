/*! 
 * \file wobbly.frag.glsl
 * \brief file wobbly.frag.glsl
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


#ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
shader_in mediump vec3 wobbly_values;
#endif

shader_in mediump vec4 glyph_linear_position_and_size;
shader_in mediump vec4 tex_color;

void
shader_main(void)
{
  mediump vec2 wobbly;
  mediump float in_glyph;
  mediump float amplitude, speed, phase;
  mediump vec4 final_color;

  final_color=tex_color;

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
  #ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
  {
    speed=fetch_node_value(wobbly_angular_speed);
    amplitude=fetch_node_value(wobbly_magnitude);
    phase=fetch_node_value(wobbly_phase);
  }
  #else
  {
    speed=wobbly_values.x;
    amplitude=wobbly_values.y;
    phase=wobbly_values.z;
  }
  #endif

  wobbly=glyph_linear_position_and_size.xy;
  wobbly.x += amplitude*glyph_linear_position_and_size.z
    * sin( wobbly.y*speed/glyph_linear_position_and_size.w + phase);

  /*
    make sure the wobbly is still within the glyph
   */
  in_glyph=step(0.0, wobbly.x) * step(wobbly.x, glyph_linear_position_and_size.z);
  wobbly.x=clamp(wobbly.x, 0.0, glyph_linear_position_and_size.z);

  #if defined(WRATH_FONT_IMPLEMENT_SIGNED_DISTANCE) && defined(DRAW_OUTLINE)
  {
    mediump float sd;
    sd=wrath_glyph_signed_distance(wobbly);
    final_color.r *= step(0.5, sd);
    final_color.g *= step(sd, 0.5);
    final_color.b *= step(-0.4, sd) * step(0.0, sd);
    final_color.a *= step(-0.6, sd);
  }
  #else
  {
    mediump float coverage;
    /*
      get the coverage, then also multiply it by
      if the fragment wobbled is within the glyph
    */
    coverage=wrath_glyph_compute_coverage(wobbly.xy)*in_glyph;
    final_color.a*=coverage;
  }
  #endif

  /*
    font shaders are two pass drawers, see WRATHTwoPassDrawer
    to meaning and use of below macros.
   */
  #if defined(WRATH_IS_OPAQUE_PASS)
  {
    //if texel is too translucent, then the opaque pass discarded the texel
    if(final_color.a<float(WRATH_TRANSLUCENT_THRESHOLD))
      discard;
  }
  #elif defined(WRATH_IS_TRANSLUCENT_PASS)
  {
    if(final_color.a>=float(WRATH_TRANSLUCENT_THRESHOLD))
      final_color.a=0.0;
  }
  #endif

  wrath_FragColor=vec4(final_color.xyz*final_color.a, final_color.a);
  
}
