/*! 
 * \file font_generic_aa.frag.wrath-shader.glsl
 * \brief file font_generic_aa.frag.wrath-shader.glsl
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

  /*
    for the rendering situation of lay down depth first,
    there is no transparent pass, thus the shader reduces
    to:
    - non-color pass: check coverage with wrath_glyph_is_covered() test
    - color pass: just set wrath_FragColor to final_color
   */
  #if defined(WRATH_COVER_DRAW)
  {
    wrath_FragColor=final_color;
  }
  #elif defined(WRATH_NON_COLOR_DRAW) 
  {
    mediump float d;
    d=wrath_glyph_is_covered();
    if(d<0.5)
      discard;
    wrath_FragColor=final_color;    
  }
  #else
  {
    mediump float d;
    d=wrath_glyph_compute_coverage();

    /*
      make d one of:
        0.0, 0.5, 1.0
     
    if(d<0.25)
      d=0.0;
    else if(d<0.75)
      d=0.5;
    else
      d=1.0;
    */

    d*=final_color.a;

    #if defined(WRATH_IS_OPAQUE_PASS)
    {
      //if texel is too translucent, then the opaque pass discarded the texel
      if(d<float(WRATH_TRANSLUCENT_THRESHOLD))
        discard;
    }
    #elif defined(WRATH_IS_TRANSLUCENT_PASS)
    {
      if(d>=float(WRATH_TRANSLUCENT_THRESHOLD))
        d=0.0;
    }
    #endif

    wrath_FragColor=vec4(final_color.xyz*d, d);
  }
  #endif
}
