/*! 
 * \file shape.frag.wrath-shader.glsl
 * \brief file shape.frag.wrath-shader.glsl
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



#ifdef AA_HINT
shader_in mediump float aa_hint;
#endif




void
shader_main(void)
{
  mediump vec4 color;
  
  discard_if_clipped(); 

  #ifdef WRATH_APPLY_BRUSH
   color=wrath_shader_brush_color();
  #else
   color=vec4(1.0, 0.0, 0.0, 1.0);
  #endif

  #if defined(AA_HINT) && defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump float alpha, abs_aa, dd;
    #if defined(AA_HINT_BOUNDARY_AT_ZERO)
    {
      /*
        in this mode, 
             aa_hint>0 <--> interior of shape
             aa_hint<0 <--> exterior of shape 
       */
      abs_aa=aa_hint;
    }
    #else
    {
      /*
        near the boundary it has absolute value 1.0.
        Thus abs_aa= 1.0 - abs(aa_hint) is zero
        near the boundary.
       */
      abs_aa=1.0 - abs(aa_hint);
    }
    #endif
    dd=fwidth(abs_aa);
    alpha=min(1.0, abs_aa/dd);

    color.w*=alpha;
  }
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
