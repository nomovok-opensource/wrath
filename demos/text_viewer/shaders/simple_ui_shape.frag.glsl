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

#ifdef LINEAR_GRADIENT
shader_in mediump vec2 tex_coord;
uniform mediump sampler2D gradientTexture;
#endif

#ifdef NON_LINEAR_GRADIENT

shader_in mediump vec2 frag_pos;
uniform mediump sampler2D gradientTexture;

  #ifdef WRATH_SHAPE_Y_GRAD_TEXTURE_UNIFORM
    uniform mediump float gradient_y_coordinate;
  #else
    shader_in mediump float varying_gradient_y_coordinate;
  #endif
#endif


shader_in mediump vec4 tex_color;



void
shader_main(void)
{

  mediump vec4 color;

  #ifdef NON_LINEAR_GRADIENT
    mediump vec2 tex_coord;
  #endif

    //#ifdef CLIPPING_USES_DISCARD
  discard_if_clipped();
    //#endif

  color=tex_color;

  #if defined(LINEAR_GRADIENT)
  {
    color*=texture2D(gradientTexture, tex_coord); 
  }
  #elif defined(NON_LINEAR_GRADIENT)
  {
    tex_coord.x=compute_gradient(frag_pos);
    #ifdef WRATH_SHAPE_Y_GRAD_TEXTURE_UNIFORM
      tex_coord.y=gradient_y_coordinate;
    #else
      tex_coord.y=varying_gradient_y_coordinate;
    #endif

    color*=texture2D(gradientTexture, tex_coord);
  }
  #endif

  #ifdef GRADIENT_INTERPOLATE_ENFORCE_BLEND
  {
      #ifdef GRADIENT_INTERPOLATE_RANGE_ENFORCE_POSITIVE
        if(tex_coord.x<0.0)
          color=vec4(0.0, 0.0, 0.0, 0.0);
      #endif

      #ifdef GRADIENT_INTERPOLATE_RANGE_ENFORCE_LESS_THAN_ONE
        if(tex_coord.x>1.0)
          color=vec4(0.0, 0.0, 0.0, 0.0);
      #endif
  }
  #else
  {
      #ifdef GRADIENT_INTERPOLATE_RANGE_ENFORCE_POSITIVE
        if(tex_coord.x<0.0)
          discard;
      #endif

      #ifdef GRADIENT_INTERPOLATE_RANGE_ENFORCE_LESS_THAN_ONE
        if(tex_coord.x>1.0)
          discard;
      #endif
  }
  #endif


  #if defined(AA_HINT) && defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump float alpha, abs_aa, dd;
    abs_aa=1.0 - abs(aa_hint);
    dd=fwidth(abs_aa);
    alpha=color.w*min(1.0, abs_aa/dd);
    
    
    #if defined(IS_OPAQUE_PASS)
      if(alpha<float(TRANSLUCENT_THRESHOLD))
        discard;
    #elif defined(IS_TRANSLUCENT_PASS)
      if(alpha>=float(TRANSLUCENT_THRESHOLD))
        alpha=0.0;
    #endif

    gl_FragColor=vec4(color.xyz*alpha, alpha);
  }
  #else

    gl_FragColor=vec4(color.xyz, 1.0)*color.w;
  
  #endif
}
