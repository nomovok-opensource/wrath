/*! 
 * \file font_analytic_base.frag.wrath-shader.glsl
 * \brief file font_analytic_base.frag.wrath-shader.glsl
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
#define WRATH_FONT_IMPLEMENT_SIGNED_DISTANCE


uniform mediump sampler2D wrath_AnalyticNormalTexture;
uniform mediump sampler2D wrath_AnalyticPositionTexture;


mediump float
wrath_analytic_font_compute_distance(in mediump vec2 GlyphTextureCoordinate,
                                     in mediump vec2 GlyphCoordinate)
{
  mediump vec4 tex_v, X_Y;  
  mediump vec2 p, Cv, dL_dM;
  mediump float d;
  mediump vec2 q;

#define X X_Y.xy
#define Y X_Y.zw
#define Lv X_Y.xz
#define Mv X_Y.yw
#define dL dL_dM.x
#define dM dL_dM.y

  tex_v=texture2D(wrath_AnalyticNormalTexture, GlyphTextureCoordinate);
  #if defined(WRATH_FONT_USE_LA_LOOKUP)
    q=texture2D(wrath_AnalyticPositionTexture, GlyphTextureCoordinate).ra;
  #else
    q=texture2D(wrath_AnalyticPositionTexture, GlyphTextureCoordinate).rg;
  #endif


  p=GlyphCoordinate - q;
  /*
    a little tricker we are doing, we want to compute:

      dL = dot(p, Lv) = p.x*Lv.x + p.y*Lv.y
      dM = dot(p, Mv) = p.x*Mv.x + p.y*Mv.y

    where L is stored in U8 format on tex_v.xy
    and M is stored in U8 format on tex_v.zw.
    Note that:

      vec2(dL, dM) = p.x*vec2(Lv.x, Mv.x) + p.y*vec2(Lv.y, Mv.y)

    thus we let X_Y=(Lv.x, Mv.x, Lv.y, Mv.y),
    and then we have that 

      vec2(dL, dM) = p.x*X + p.y*Y

    and we #define Lv as X_Y.xy and Mv as X_Y.zw
   */
  X_Y=(2.0)*(255.0/254.0)*tex_v.xzyw - vec4(1.0, 1.0, 1.0, 1.0);
  Cv= Lv.xy*Mv.yx;

  dL_dM=p.x*X + p.y*Y;
  d=(Cv.y<Cv.x)?
    max(dL, dM):
    min(dL, dM);

  return d;


#undef X 
#undef Y 
#undef Lv 
#undef Mv 
#undef dL 
#undef dM 


}

