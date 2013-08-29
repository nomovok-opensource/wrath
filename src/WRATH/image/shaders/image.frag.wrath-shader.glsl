/*! 
 * \file image.frag.wrath-shader.glsl
 * \brief file image.frag.wrath-shader.glsl
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





void
shader_main(void)
{
  mediump vec4 color;
  
  discard_if_clipped(); 

  #ifdef APPLY_BRUSH
   color=wrath_shader_brush_color();
  #else
   color=vec4(1.0, 0.0, 0.0, 1.0);
  #endif      
    
  #if defined(IS_OPAQUE_PASS)
  {
    if(color.w<float(TRANSLUCENT_THRESHOLD))
      discard;
  }
  #elif defined(IS_TRANSLUCENT_PASS)
  {
    if(color.w>=float(TRANSLUCENT_THRESHOLD))
      color.w=0.0;
  }
  #endif

  #ifdef PREMULTIPLY_ALPHA
  {
    gl_FragColor=vec4(color.xyz, 1.0)*color.w;
  }
  #else
  {
    gl_FragColor=color;
  }
  #endif
  
}
