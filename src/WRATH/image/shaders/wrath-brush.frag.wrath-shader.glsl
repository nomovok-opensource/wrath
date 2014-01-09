/*! 
 * \file wrath-brush.frag.wrath-shader.glsl
 * \brief file wrath-brush.frag.wrath-shader.glsl
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



/*
  "control flow" via #if macros.
  
  Some notes:
   if NONWRATH_LINEAR_BRUSH_PRESENT is defined, then 
   both LINEAR_GRADIENT and LINEAR_TEXTURE_COORDINATE
   are NOT defined.
 */
#ifdef NONWRATH_LINEAR_BRUSH_PRESENT
  #ifdef LINEAR_GRADIENT
  #error "LINEAR_GRADIENT defined with NONWRATH_LINEAR_BRUSH_PRESENT defined"
  #endif

  #ifdef NON_LINEAR_GRADIENT
    #ifndef FULLY_NON_LINEAR_GRADIENT
      #error "NON_LINEAR_GRADIENT defined but FULLY_NON_LINEAR_GRADIENT not defined with NONWRATH_LINEAR_BRUSH_PRESENT defined"
    #endif
  #endif   

  #ifdef LINEAR_TEXTURE_COORDINATE
  #error "LINEAR_TEXTURE_COORDINATE defined with NONWRATH_LINEAR_BRUSH_PRESENT defined"
  #endif
#endif

#ifdef LINEAR_GRADIENT
  uniform mediump sampler2D wrath_brush_gradientTexture;
  #ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
     shader_in mediump float wrath_brush_tex_coord;
  #else
     shader_in mediump vec2 wrath_brush_tex_coord;
  #endif
#endif

#ifdef NON_LINEAR_GRADIENT
  uniform mediump sampler2D wrath_brush_gradientTexture;
  #ifndef NONWRATH_LINEAR_BRUSH_PRESENT
    #ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
       shader_in mediump vec2 wrath_brush_frag_pos;
    #else
       shader_in mediump vec3 wrath_brush_frag_pos;
       #define wrath_brush_grad_tex_y wrath_brush_frag_pos.z
    #endif
  #else
    #ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
       shader_in mediump float wrath_brush_grad_tex_y;
    #endif
  #endif

#endif




#if defined(LINEAR_TEXTURE_COORDINATE) || defined(NON_LINEAR_TEXTURE_COORDINATE)
  uniform mediump sampler2D wrath_brush_imageTexture;
  uniform mediump vec2 wrath_brush_imageTextureSize;
  #ifndef NONWRATH_LINEAR_BRUSH_PRESENT
    shader_in mediump vec2 wrath_brush_image_tex_coord;
  #endif
#endif

#ifdef CONST_COLOR_VS
  shader_in mediump vec4 wrath_brush_const_color;
#endif


#ifdef NONWRATH_LINEAR_BRUSH_PRESENT
mediump vec4 wrath_shader_brush_color(in mediump vec2 wrath_brush_frag_pos, out mediump float valid)
#else
mediump vec4 wrath_shader_brush_color(out mediump float valid)
#endif
{
  mediump vec4 color=vec4(1.0, 1.0, 1.0, 1.0);
  mediump vec4 grad_color=vec4(1.0, 1.0, 1.0, 1.0);
  mediump vec2 grad_tex=vec2(0.5, 0.5);


  valid=1.0;

  #if defined(NONWRATH_LINEAR_BRUSH_PRESENT) && defined(NON_LINEAR_TEXTURE_COORDINATE)

  mediump vec2 wrath_brush_image_tex_coord;
  wrath_brush_image_tex_coord=wrath_brush_frag_pos.xy/wrath_brush_imageTextureSize;

  #endif

  #if defined(CONST_COLOR_FS)
  {  
    color=const_color_value();
    #if defined(CONST_COLOR_ALPHA_TEST)
    {
      if(color.w<0.5)
        valid=0.0;
    }  
    #endif
  }
  #elif defined(CONST_COLOR_VS)
  {
    color=wrath_brush_const_color;
    #if defined(CONST_COLOR_ALPHA_TEST)
    {
      if(color.w<0.5)
        valid=0.0;
    }  
    #endif
  }
  #endif

  #if defined(LINEAR_TEXTURE_COORDINATE) || defined(NON_LINEAR_TEXTURE_COORDINATE)
  {
    mediump vec4 image_color;

    /*
      ISSUE:
        if the repeate mode is repeat, the mipmap
        to choose is utterly borked because neighboring
        texels on a repeat boundary have radically 
        different values. Everything is fine in all 
        modes except repeat though.

        The correct thing to do is to use texture2DGrad
        giving the gradient as 
         dx = dFdx(image_tex_coord.xy)
         dy = dFdy(image_tex_coord.xy)
        this will do the right thing even if repeat comes
        up in terms of selecting the correct mipmap.

        One needs GL_EXT_shader_texture_lod 
        (and GL_OES_standard_derivatives) 
        for those functions (in EXT form).
     */
    #if defined(LINEAR_TEXTURE_COORDINATE)
    {
      /*
      image_color=texture2DGrad(wrath_brush_imageTexture, wrath_brush_image_tex_coord,
                                dFdx(wrath_brush_image_tex_coord.xy),
                                dFdy(wrath_brush_image_tex_coord.xy));
      */
      image_color=texture2D(wrath_brush_imageTexture,
                            wrath_brush_image_tex_coord);
    }
    #else
    {
      /*
      image_color=texture2DGrad(wrath_brush_imageTexture, 
                                compute_texture_coordinate(wrath_brush_image_tex_coord.xy),
                                dFdx(wrath_brush_image_tex_coord.xy),
                                dFdy(wrath_brush_image_tex_coord.xy));
      */
      image_color=texture2D(wrath_brush_imageTexture, 
                            compute_texture_coordinate(wrath_brush_image_tex_coord));
    }
    #endif

    #ifdef IMAGE_ALPHA_TEST
    {
      if(image_color.w<0.5)
        valid=0.0;
    }
    #endif

    color*=image_color;
  }
  #endif

  #if defined(LINEAR_GRADIENT)  
  {
    #ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    {
      grad_tex=vec2(wrath_brush_tex_coord, fetch_node_value(WRATH_GRADIENT_y_coordinate));
    }
    #else
    {
      grad_tex=wrath_brush_tex_coord;
    }
    #endif
    #define HAVE_GRADIENT 
  }
  #elif defined(NON_LINEAR_GRADIENT)
  {
    mediump vec2 vv;

    vv=compute_gradient(wrath_brush_frag_pos.xy);

    grad_tex.x=vv.x;
    valid*=vv.y;

    #ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    {
      grad_tex.y=fetch_node_value(WRATH_GRADIENT_y_coordinate);
    }
    #else
    {
      grad_tex.y=wrath_brush_grad_tex_y;
    }
    #endif    
    #define HAVE_GRADIENT 
  }
  #endif


  #ifdef HAVE_GRADIENT
  {
    #ifdef GRADIENT_INTERPOLATE_ENFORCE_BLEND
    {
      #ifdef GRADIENT_INTERPOLATE_RANGE_ENFORCE_POSITIVE
      {
        if(grad_tex.x<0.0)
          color=vec4(0.0, 0.0, 0.0, 0.0);
      }
      #endif

      #ifdef GRADIENT_INTERPOLATE_RANGE_ENFORCE_LESS_THAN_ONE
      {
        if(grad_tex.x>1.0)
          color=vec4(0.0, 0.0, 0.0, 0.0);
      }
      #endif
    }
    #else
    {
      #ifdef GRADIENT_INTERPOLATE_RANGE_ENFORCE_POSITIVE
      {
        if(grad_tex.x<0.0)
          valid=0.0;
      }
      #endif

      #ifdef GRADIENT_INTERPOLATE_RANGE_ENFORCE_LESS_THAN_ONE
      {
        if(grad_tex.x>1.0)
          valid=0.0;
      }
      #endif
    }
    #endif

    grad_color=texture2D(wrath_brush_gradientTexture, grad_tex);
  
    #ifdef GRADIENT_ALPHA_TEST
    {
      if(grad_color.w<0.5)
        valid=0.0;
    }
    #endif

    color*=grad_color;
  }
  #endif

  #if defined(FINAL_ALPHA_TEST)
  {
    if(color.w<0.5)
      valid=0.0;
  }
  #endif

  return color;
}

#ifdef NONWRATH_LINEAR_BRUSH_PRESENT

  mediump vec4 wrath_shader_brush_color(in mediump vec2 wrath_brush_frag_pos)
  {
    mediump vec4 C;
    mediump float v;
    C=wrath_shader_brush_color(wrath_brush_frag_pos, v);

    #if !defined(WRATH_COVER_DRAW) && defined(WRATH_BRUSH_ISSUES_DISCARD)
    {
      if(v<0.5)
        discard;
    }
    #endif

    return C;
  }

#else

  mediump vec4 wrath_shader_brush_color()
  {
    mediump vec4 C;
    mediump float v;
    C=wrath_shader_brush_color(v);
    
    #if !defined(WRATH_COVER_DRAW) && defined(WRATH_BRUSH_ISSUES_DISCARD)
    {
      if(v<0.5)
        discard;
    }
    #endif

    return C;    
  }
#endif
