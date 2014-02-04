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



/*
  tex_v=texture2D(wrath_AnalyticNormalTexture, tex_coord);
  tex_o=texture2D(wrath_AnalyticPositionTexture, tex_coord);
 */
mediump float
wrath_analytic_font_compute_distance(in mediump vec2 GlyphTextureCoordinate,
                                     in mediump vec2 GlyphCoordinate)
{
  mediump vec4 tex_v, X_Y;  
  mediump vec2 p, Cv, dL_dM;
  mediump float d;
  mediump vec2 tex_o;

#define offsetL_M tex_o
#define X X_Y.xy
#define Y X_Y.zw
#define Lv X_Y.xz
#define Mv X_Y.yw
#define offsetL offsetL_M.x
#define offsetM offsetL_M.y
#define dL dL_dM.x
#define dM dL_dM.y

  tex_v=texture2D(wrath_AnalyticNormalTexture, GlyphTextureCoordinate);
  #if defined(WRATH_FONT_USE_LA_LOOKUP)
    tex_o=texture2D(wrath_AnalyticPositionTexture, GlyphTextureCoordinate).ra;
  #else
    tex_o=texture2D(wrath_AnalyticPositionTexture, GlyphTextureCoordinate).rg;
  #endif


  p=GlyphCoordinate - tex_o;
  X_Y=(2.0)*(255.0/254.0)*tex_v.xzyw - vec4(1.0, 1.0, 1.0, 1.0);
  Cv= Lv.xy*Mv.yx;

  dL_dM=p.x*X + p.y*Y;
  d=(Cv.y<Cv.x)?
    max(dL, dM):
    min(dL, dM);

  return d;


#undef offsetL_M 
#undef X 
#undef Y 
#undef Lv 
#undef Mv 
#undef offsetL 
#undef offsetM 
#undef dL 
#undef dM 


}

