/*! 
 * \file simple_ui_shape.frag.glsl
 * \brief file simple_ui_shape.frag.glsl
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



shader_in mediump vec4 tex_color;



void
shader_main(void)
{

  mediump vec4 color;

  #ifdef WRATH_NON_LINEAR_GRADIENT
    mediump vec2 tex_coord;
  #endif

    //#ifdef CLIPPING_USES_DISCARD
  discard_if_clipped();
    //#endif

  color=tex_color;

   


  #if defined(AA_HINT) && defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump float alpha, abs_aa, dd;
    abs_aa=1.0 - abs(aa_hint);
    dd=fwidth(abs_aa);
    alpha=color.w*min(1.0, abs_aa/dd);
    
    
    #if defined(WRATH_IS_OPAQUE_PASS)
      if(alpha<float(WRATH_TRANSLUCENT_THRESHOLD))
        discard;
    #elif defined(WRATH_IS_TRANSLUCENT_PASS)
      if(alpha>=float(WRATH_TRANSLUCENT_THRESHOLD))
        alpha=0.0;
    #endif

    wrath_FragColor=vec4(color.xyz*alpha, alpha);
  }
  #else

    wrath_FragColor=vec4(color.xyz, 1.0)*color.w;
  
  #endif
}
