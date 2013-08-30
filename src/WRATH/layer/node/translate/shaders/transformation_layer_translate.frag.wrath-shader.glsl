/*! 
 * \file transformation_layer_translate.frag.wrath-shader.glsl
 * \brief file transformation_layer_translate.frag.wrath-shader.glsl
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



#ifdef CLIP_VIA_DISCARD
shader_in mediump vec4 clipping_via_discard;
#endif

/*
  Note: in the case of WRATH_COVER_DRAW,
  the depth or stencil buffer was set 
  in the previous pass, thus we can then 
  avoid the discard 
 */
#if defined(CLIP_VIA_DISCARD) && !defined(WRATH_COVER_DRAW)
#define CLIPPING_USES_DISCARD

void
discard_if_clipped(void)
{
  if(clipping_via_discard.x<0.0 
     || clipping_via_discard.y<0.0 
     || clipping_via_discard.z<0.0 
     || clipping_via_discard.w<0.0)
    {
      discard;
    }
}

mediump float
discard_via_alpha(void)
{
  return step(0.0, clipping_via_discard.x)
    * step(0.0, clipping_via_discard.y)
    * step(0.0, clipping_via_discard.z)
    * step(0.0, clipping_via_discard.w);
    
}

#else

void
discard_if_clipped(void)
{}

mediump float
discard_via_alpha(void)
{
  return 1.0;
}



#endif
