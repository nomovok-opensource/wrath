/*! 
 * \file distance_field_draw_distance_points.frag.wrath-shader.glsl
 * \brief file distance_field_draw_distance_points.frag.wrath-shader.glsl
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



#ifdef GL_ES
  #ifdef NV_shader_framebuffer_fetch
    #extension GL_NV_shader_framebuffer_fetch: require
  #else  
    #extension GL_EXT_frag_depth: require
    #define fragdepth gl_FragDepthEXT
  #endif
#else
  #define fragdepth gl_FragDepth
#endif


shader_in mediump vec2 st;
uniform mediump float distance_sign;

void
main(void)
{
  mediump float v, d, hd;

  d=min(1.0, length(st));
  hd=0.5*d;

  #ifdef NV_shader_framebuffer_fetch
    hd=min(hd, abs(gl_LastFragColor-0.5));
  #else
    fragdepth=hd;
  #endif

  v=0.5 + distance_sign*hd;
  wrath_FragColor=vec4(v, v, v, v);
}
               
