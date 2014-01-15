/*! 
 * \file font_generic.frag.wrath-shader.glsl
 * \brief file font_generic.frag.wrath-shader.glsl
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


shader_in mediump vec4 tex_color;

void
shader_main(void)
{
  mediump vec4 final_color;

  final_color=tex_color;
  
  #if defined(WRATH_APPLY_BRUSH_RELATIVE_TO_LETTER) || defined(WRATH_APPLY_BRUSH_RELATIVE_TO_ITEM)
  {
    final_color*=wrath_shader_brush_color();
  }
  #endif

  #if defined(WRATH_COVER_DRAW)
  {
    //a previous pass guarantees that the
    //fragment is covered by the glyph, thus
    //we can skip the test and just draw the color
    gl_FragColor=final_color;
  }
  #else
  {
    mediump float d;
    d=wrath_glyph_is_covered();

    #if defined(WRATH_IS_OPAQUE_PASS) 
      if(d<0.5)
        discard;
      gl_FragColor=final_color;
    #else
      if(d<0.5)
        gl_FragColor=vec4(0.0, 0.0, 0.0, 0.0);
      else
        gl_FragColor=final_color;
    #endif
  }
  #endif
}
