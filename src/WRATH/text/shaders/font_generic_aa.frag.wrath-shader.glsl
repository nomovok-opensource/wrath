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
  #if defined(APPLY_BRUSH_RELATIVE_TO_LETTER) || defined(APPLY_BRUSH_RELATIVE_TO_ITEM)
  {
    tex_color*=wrath_shader_brush_color();
  }
  #endif

  /*
    for the rendering situation of lay down depth first,
    there is no transparent pass, thus the shader reduces
    to:
    - non-color pass: check coverage with is_covered() test
    - color pass: just set gl_FragColor to tex_color
   */
  #if defined(WRATH_COVER_DRAW)
  {
    gl_FragColor=tex_color;
  }
  #elif defined(WRATH_NON_COLOR_DRAW) 
  {
    mediump float d;
    d=is_covered();
    if(d<0.5)
      discard;
    gl_FragColor=tex_color;    
  }
  #else
  {
    mediump float d;
    d=compute_coverage();

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

    d*=tex_color.a;

    #if defined(IS_OPAQUE_PASS)
    {
      //if texel is too translucent, then the opaque pass discarded the texel
      if(d<float(TRANSLUCENT_THRESHOLD))
        discard;
    }
    #elif defined(IS_TRANSLUCENT_PASS)
    {
      if(d>=float(TRANSLUCENT_THRESHOLD))
        d=0.0;
    }
    #endif

    gl_FragColor=vec4(tex_color.xyz*d, d);
  }
  #endif
}
