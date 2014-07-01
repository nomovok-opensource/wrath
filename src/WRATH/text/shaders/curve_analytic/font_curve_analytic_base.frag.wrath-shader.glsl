/*! 
 * \file font_curve_analytic_base.frag.wrath-shader.glsl
 * \brief file font_curve_analytic_base.frag.wrath-shader.glsl
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

/*
  Basic idea is as follows:

   IndexTexture(x, y) = I where (x,y) is a texel within the glyph
   
   Then the pair (I,G) is used to fetch the curve pairs
   where G=GlyphIndex.

   The data for a single curve is:

   "Q": vec2 provides a rotation in QTexture
   "p2": start of curve(or curve corner) in P2Texture.xy (or M_P_Texture.xy)
   "m0_m1": vec2 provides liner coeffiences for the curve after rotation
            in ABTexture (or M_P_Texture)
   "mag_Q": provides quadrative coefficient for the y-corodiante after curve
            in P2Texture.zw (or ScaleTexture)

   where curves meet there is a "rule" texture providing:
    cA: 1 if curve is A is quadratic, 0 else
    cB: 1 if curve is B is quadratic, 0 else
    rule: decide to use AND(min) or OR(max) rule 
    tangle: under certain conditions, defualt value gets flipped

   basic idea is:

    let Q=rotation operation encoded in Q.
    let p=point in glyph coordinate
    let (x,y) = Q(p-p2) [i.e. translate to p2 then rotate by Q]
    the the curve is given by in these coordinate
    
    m0*t
    m1*t + mag_Q*t*t

    Lines get mapped so that m1=0 and mag_Q is 0.

    We get the t via: t=x/m0.
    Now we need to make sure we don't do bad things
    outside of the range t=[0,1].

    Lets say t=0 represents the nearby corner.
    Then, for t<0 we say the curve is not giving good data
    and we use a defualt value determined by the rule.
    The value t=1.0 is viewed as "far" away, so we
    _clamp_ t to be no more than 1.0. This behavior
    of clamping essentially makes it so that in
    rotated coordinates we are extending the curve
    as a horizontal line to inifinity.
    For a quadratic that line is perpindicular to
    the axis of the parobola, and for a linear
    that line is the same as the line segment.
    
    under the case that both curves go past the 
    corner, we do NOT make t no more than 0.0.
    Rather, we do not feed t to the quadratic term 
    but 0. This has the effect of just extending
    the curve before t=0 as a straight line tangent
    to the curve at t=0.
 */

#ifdef WRATH_CURVE_ANALYTIC_SEPARATE_CURVES

  uniform curve_analytic_precision sampler2D wrath_CurveAnalyticIndexTexture;//A8
  uniform curve_analytic_precision sampler2D wrath_CurveAnalyticQTexture; 
  uniform curve_analytic_precision sampler2D wrath_CurveAnalyticM_P_Texture;
  uniform curve_analytic_precision sampler2D wrath_CurveAnalyticScaleTexture;
  uniform curve_analytic_precision sampler2D wrath_CurveAnalyticNextCurveTexture; //L8
  uniform curve_analytic_precision sampler2D wrath_CurveAnalyticRuleTexture; //RGBA4444 (cA, cB, rule, reserved)
 
#else

  uniform curve_analytic_precision sampler2D wrath_CurveAnalyticIndexTexture;//A8
  uniform curve_analytic_precision sampler2D wrath_CurveAnalyticABTexture;   //RGBA_16F (A0,B0,A1,B1)
  uniform curve_analytic_precision sampler2D wrath_CurveAnalyticQTexture;    //RGBA_16F (QaX, QaY, QbX, QbY)
  uniform curve_analytic_precision sampler2D wrath_CurveAnalyticP2Texture;   //LA_16F (p2x, p2y) OR RGBA_16F (p2x, p2y, mag_Qa, mag_Qb)
  uniform curve_analytic_precision sampler2D wrath_CurveAnalyticRuleTexture; //RGBA4444 (cA, cB, rule, tangleXorRule)
  
#endif

/*
    Freaking ATI  wacky drivers.
    If float_texture2D or rule_texture2D
    are defined as just texture2D one gets
    bad render results, but if one uses the Lod
    look up command, all is fine. This is
    inspite of the fact that the textures
    have their minification filter set to
    GL_NEAREST and that the mipmaps are
    not even defined!
   */
#define index_texture2D(X, Y) texture2D(X,Y)

#if defined(WRATH_GPU_CONFIG_DEPENDENT_TEXTURE_LOOKUP_REQUIRES_LOD) && defined(WRATH_GPU_CONFIG_FRAGMENT_SHADER_TEXTURE_LOD)

#define float_texture2D(X,Y) texture2DLod(X, Y, 0.0)
#define rule_texture2D(X,Y) texture2DLod(X, Y, 0.0)
#else
#define float_texture2D(X,Y) texture2D(X, Y)
#define rule_texture2D(X,Y) texture2D(X, Y)
#endif

/*
  Hackage to handle where GLES2 for 2 channel is LA texture
  gut GL is R and RG textures
 */
#ifdef WRATH_USE_LA_LOOKUP
#define float_texture2D_2channel(X, Y) float_texture2D(X,Y).ra
#else
#define float_texture2D_2channel(X, Y) float_texture2D(X,Y).rg
#endif

/*
  - Should the GlyphTextureCoordiante actually be highp for large index textures?
  - Should output and GlyphCoordinate have precision curve_analytic_precision?  
 */
mediump float
wrath_curve_analytic_compute_quasi_distance(in mediump vec2 GlyphCoordinate,
                                            in mediump vec2 GlyphTextureCoordinate,
                                            in mediump float GlyphIndex)
{
  curve_analytic_precision vec2 pp;
  curve_analytic_precision vec4 pa_pb;
  curve_analytic_precision vec4 A0_B0_A1_B1;
  curve_analytic_precision vec4 Qa_Qb;
  curve_analytic_precision vec2 sa_sb, ta_tb, dependent_tex, sigma_ab, omega_ab;
  curve_analytic_precision vec4 ca_cb_rule_tangleXorRule;
  curve_analytic_precision float omega, zeta, sigma, sigma_min, sigma_max;
  curve_analytic_precision vec4 p2_QaScale_QbScale;

#define p2 p2_QaScale_QbScale.xy
#define QaScale p2_QaScale_QbScale.z
#define QbScale p2_QaScale_QbScale.w
#define QaScale_QbScale p2_QaScale_QbScale.zw
#define A0 A0_B0_A1_B1.x
#define B0 A0_B0_A1_B1.y
#define A0_B0 A0_B0_A1_B1.xy
#define A1 A0_B0_A1_B1.z
#define B1 A0_B0_A1_B1.w
#define A1_B1 A0_B0_A1_B1.zw
#define ta ta_tb.x
#define tb ta_tb.y
#define sa sa_sb.x
#define sb sa_sb.y
#define ca ca_cb_rule_tangleXorRule.x
#define cb ca_cb_rule_tangleXorRule.y
#define ca_cb ca_cb_rule_tangleXorRule.xy
#define rule ca_cb_rule_tangleXorRule.z
#define tangleXorRule ca_cb_rule_tangleXorRule.w
#define sigma_a sigma_ab.x
#define sigma_b sigma_ab.y
#define omega_a omega_ab.x
#define omega_b omega_ab.y
#define pa_x pa_pb.x
#define pa_y pa_pb.y
#define pb_x pa_pb.z
#define pb_y pa_pb.w
#define pa pa_pb.xy
#define pb pa_pb.zw
#define pa_pb_x pa_pb.xz
#define pa_pb_y pa_pb.yw
#define Qa Qa_Qb.xy
#define Qb Qa_Qb.zw
#define Qa_Qb_x Qa_Qb.xz
#define Qa_Qb_y Qa_Qb.yw

  dependent_tex.x=index_texture2D(wrath_CurveAnalyticIndexTexture, GlyphTextureCoordinate).r;
  dependent_tex.y=GlyphIndex;

  
  if(dependent_tex.x<0.5/255.0)
    {
      return -1.0;
    }
  
  if(1.0-dependent_tex.x<0.5/255.0)
    {
      return 1.0;
    }

  dependent_tex.x=(255.0*dependent_tex.x + 0.5)/256.0;
  
  #ifdef WRATH_CURVE_ANALYTIC_SEPARATE_CURVES
  {
    curve_analytic_precision vec4 Ma_Pa, Mb_Pb;
    curve_analytic_precision vec4 PP_AB;
    curve_analytic_precision vec2 dependent_tex2;

    dependent_tex2.y=dependent_tex.y;

    dependent_tex2.x=rule_texture2D(wrath_CurveAnalyticNextCurveTexture, dependent_tex).r;
    ca_cb_rule_tangleXorRule=rule_texture2D(wrath_CurveAnalyticRuleTexture, dependent_tex).rgba;

    Ma_Pa=float_texture2D(wrath_CurveAnalyticM_P_Texture, dependent_tex);
    Qa=float_texture2D_2channel(wrath_CurveAnalyticQTexture, dependent_tex);
    QaScale=float_texture2D(wrath_CurveAnalyticScaleTexture, dependent_tex).r;

    dependent_tex2.x=(255.0*dependent_tex2.x + 0.5)/256.0;
    Mb_Pb=float_texture2D(wrath_CurveAnalyticM_P_Texture, dependent_tex2);    
    Qb=float_texture2D_2channel(wrath_CurveAnalyticQTexture, dependent_tex2);
    QbScale=float_texture2D(wrath_CurveAnalyticScaleTexture, dependent_tex2).r;
    
    A0_B0_A1_B1=vec4(Ma_Pa.x, 
                     Mb_Pb.x,
                     Ma_Pa.y,
                     Mb_Pb.y);


    /*
      translate the point to the start
      of each curve (which are different 
      points)
     */
    PP_AB=vec4(GlyphCoordinate-Ma_Pa.zw,
               GlyphCoordinate-Mb_Pb.zw);

    
    /*
      rotate the translated point as according to Q*'s
     */
    pa_pb_x =  PP_AB.xz*Qa_Qb_x + PP_AB.yw*Qa_Qb_y;
    pa_pb_y = -PP_AB.xz*Qa_Qb_y + PP_AB.yw*Qa_Qb_x;
   
    ta_tb= pa_pb_x*A0_B0;


    /*
      ta is coming out of the point of the corner,
      tb is coming into the point of the corner.
      We "ignore" a curve alpha if the time happens
      before curve alpha starts (since it starts
      from the corner, and we "ignore" curve beta
      it is ends after it hits the corner:
     */
    omega_a=step(0.0, ta);
    omega_b=step(tb, 1.0);

    /*
      we need to extent curve a and b correctly
      as a line beyond their domain. We use
      sa_sb inplace of ta_tb*ta_tb, so we
      adjust sa_sb when ta_tb is outside of [0,1].

      if t<0, then making s=0
      is equivalent to extending before t=0 as a line.

      if t>1, then making s=2*t-1 is
      equivalent to extending after t=1 as a line.
     */
    sa_sb=step(0.0, ta_tb)*mix(ta_tb*ta_tb,
                               2.0*ta_tb - 1.0,
                               step(1.0, ta_tb));

    
  }
  #else
  {
    Qa_Qb=float_texture2D(wrath_CurveAnalyticQTexture, dependent_tex);
    A0_B0_A1_B1=float_texture2D(wrath_CurveAnalyticABTexture, dependent_tex);
    p2_QaScale_QbScale=float_texture2D(wrath_CurveAnalyticP2Texture, dependent_tex);
    

    ca_cb_rule_tangleXorRule=rule_texture2D(wrath_CurveAnalyticRuleTexture, dependent_tex).rgba;
    pp=GlyphCoordinate-p2;
    /*
      Qa:= | Qa_x  Qa_y |
           |-Qa_y  Qa_x |
      
      Qb:= | Qb_x  Qb_y |
           |-Qb_y  Qb_x | 
           
      Apply Qa and Qb, the raw values are:
      pa_x=  pp.x*Qa_x + pp.y*Qa_y;
      pa_y= -pp.x*Qa_y + pp.y*Qa_x;
      pb_x=  pp.x*Qb_x + pp.y*Qb_y;
      pb_y= -pp.x*Qb_y + pp.y*Qb_x;
    */
    pa_pb_x =  pp.x*Qa_Qb_x + pp.y*Qa_Qb_y;
    pa_pb_y = -pp.x*Qa_Qb_y + pp.y*Qa_Qb_x;
  
    ta_tb=pa_pb_x*A0_B0;
    /*
      omega_a!=0 <--> t_a>=0
      omega_b!=0 <--> t_b>=0

      sa_sb is used in place of ta_tb*ta_tb.

      if t<0, then making s=0
      is equivalent to extending before t=0 as a line.

      if t>1, then making s=2*t-1 is
      equivalent to extending after t=1 as a line.
    */
    omega_ab=step(0.0, ta_tb);
    sa_sb=omega_ab*mix(ta_tb*ta_tb, 
                       2.0*ta_tb - vec2(1.0, 1.0), 
                       step(1.0, ta_tb) );
  }
  #endif

  
  
  


  /*
    rule:
      0 <---> OR rule  <--> max
      1 <---> AND rule <--> min
   */

  /*
    zeta is the value to use for
    sigma_a or sigma_b if the point
    is not in the curve's "shadow"

    When tangle is not true, zeta is a value
    that does NOT affect the final return value,
    thus for min-rule it is -1 and for max it is +1.

    However when tangle is true, zeta is a value
    to force the test value, thus for min-rule
    it is -1 and for max it is +1.

    TODO:
      there is no need to do the xor here.
      rather than having tangle, store the value
      for zeto in the texture.
   */
  zeta=(tangleXorRule>0.5) ? 1.0 : -1.0;

  



  /*
    omega!=0 <--> one of t_a, t_b positive

    If fragment is not within the shadow 
    of either curve, then we extend
    BOTH curves as lines in a C1 fashion
    and use that as the result.
   */
  omega=max(omega_a, omega_b);

  omega_ab=(omega<0.5)?
    vec2(1.0, 1.0):
    omega_ab;

  /*
    Make:
     sigma_a=-(A1*ta + ta*ta*QaScale - pa_y)*sign(A0);
     sigma_b= (B1*tb + tb*tb*QbScale - pb_y)*sign(B0);
  */
  #ifdef WRATH_CURVE_ANALYTIC_SEPARATE_CURVES
  {
    A0_B0=-A0_B0;
  }
  #else
  {
    A0=-A0;
  }
  #endif
  sigma_ab=(A1_B1*ta_tb + sa_sb*QaScale_QbScale - pa_pb_y)*sign(A0_B0);
  


  sigma_a=(omega_a>0.5)?sigma_a:zeta;
  sigma_b=(omega_b>0.5)?sigma_b:zeta;



  sigma_min=min(sigma_a, sigma_b);
  sigma_max=max(sigma_a, sigma_b);

  sigma=(rule<0.5)?
    sigma_max:
    sigma_min;


  return sigma;


#undef p2
#undef QaScale
#undef QbScale
#undef QaScale_QbScale
#undef A0
#undef B0
#undef A1
#undef B1
#undef A0_B0
#undef A1_B1
#undef ta
#undef tb
#undef sa
#undef sb
#undef ca  
#undef cb   
#undef ca_cb  
#undef rule
#undef tangleXorRule
#undef sigma_a  
#undef sigma_b 
#undef omega_a 
#undef omega_b 
#undef pa_x 
#undef pa_y 
#undef pb_x 
#undef pb_y 
#undef pa 
#undef pb 
#undef pa_pb_x 
#undef pa_pb_y 
#undef Qa 
#undef Qb 
#undef Qa_Qb_x 
#undef Qa_Qb_y 

  
}


#undef index_texture2D
#undef float_texture2D
#undef rule_texture2D



