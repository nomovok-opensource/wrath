/*! 
 * \file WRATHShapeDistanceFieldGPUutil.cpp
 * \brief file WRATHShapeDistanceFieldGPUutil.cpp
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



#include "WRATHConfig.hpp"
#include "WRATHShapeDistanceFieldGPUutil.hpp"
#include "WRATHGLExtensionList.hpp"
#include "WRATHStaticInit.hpp"

/*
  N900 buggage: using glCopyTexSubImage2D with an
  FBO bound that uses a texture as the color
  attachment does not work, but using a renderbuffer
  attachment does work.
 */
#if defined(N900BUILD)
#define FBO_COLOR_USE_RENDERBUFFER
#endif

namespace
{
  class gl_resource_deleter
  {
  public:
    GLuint m_fbo;
    GLuint m_texture;
    GLuint m_depth;
    GLuint m_stencil;
    bool m_depth_is_renderbuffer;

    void
    operator()(void) const;
  };  

#if defined(FBO_COLOR_USE_RENDERBUFFER)
  GLenum
  gles2_render_buffer_format(GLenum fm)
  {
    #if !defined(WRATH_GL_VERSION)
      {
        WRATHStaticInit();
        static WRATHGLExtensionList R;
        static bool supports_rgb_rgba8(R.extension_supported("GL_OES_rgb8_rgba8"));

        switch(fm)
          {
          case GL_RGBA:
            return supports_rgb_rgba8?GL_RGBA8_OES:GL_RGBA4;
          case GL_RGB:
            return supports_rgb_rgba8?GL_RGB8_OES:GL_RGB565;
          default:
            return fm;
          }
      }
    #endif
   
    return fm;
  }
#endif
}

void
gl_resource_deleter::
operator()(void) const
{
  if(m_fbo)
    {
      glDeleteFramebuffers(1, &m_fbo);
    }

  if(m_texture)
    {
#ifdef FBO_COLOR_USE_RENDERBUFFER
      glDeleteRenderbuffers(1, &m_texture);
#else
      glDeleteTextures(1, &m_texture);
#endif
    }

  if(m_stencil)
    {
      glDeleteRenderbuffers(1, &m_stencil);
    }

  if(m_depth)
    {
      if(m_depth_is_renderbuffer)
        {
          glDeleteRenderbuffers(1, &m_depth);
        }
      else
        {
          glDeleteTextures(1, &m_depth);
        }
    }
}


////////////////////////////////////////////
// WRATHShapeGPUDistanceFieldCreator::ScratchPadFBO methods
WRATHShapeGPUDistanceFieldCreator::ScratchPadFBO::
ScratchPadFBO(const WRATHTripleBufferEnabler::handle &tr,
             GLenum texture_format):
  m_tr(tr),
#if defined(FBO_COLOR_USE_RENDERBUFFER)
  m_format(gles2_render_buffer_format(texture_format)),
#else
  m_format(texture_format),
#endif

  m_max_dim(0, 0),
  m_current_dim(0, 0),
  m_fbo(0),
  m_texture(0),
  m_depth(0),
  m_stencil(0),
  m_depth_is_renderbuffer(false)
{}

WRATHShapeGPUDistanceFieldCreator::ScratchPadFBO::
~ScratchPadFBO()
{
  gl_resource_deleter R;

  R.m_fbo=m_fbo;
  R.m_texture=m_texture;
  R.m_depth=m_depth;
  R.m_stencil=m_stencil;
  R.m_depth_is_renderbuffer=m_depth_is_renderbuffer;

  m_tr->schedule_rendering_action(R);
}

enum return_code
WRATHShapeGPUDistanceFieldCreator::ScratchPadFBO::
init_and_bind_fbo(ivec2 pdims, bool)
{
  int max_dim(WRATHglGet<int>(GL_MAX_TEXTURE_SIZE));

  if(pdims.x()>max_dim or pdims.y()>max_dim or pdims.x()<=0 or pdims.y()<=0)
    {
      return routine_fail;
    }

  if(pdims.x()>m_max_dim.x() or pdims.y()>m_max_dim.y())
    {
      //delete old fbo and image stff: 
      gl_resource_deleter R;
      R.m_fbo=0; //setting this as 0 makes it so that R does not delete the FBO. 
      R.m_texture=m_texture;
      R.m_depth=m_depth;
      R.m_stencil=m_stencil;
      R.m_depth_is_renderbuffer=m_depth_is_renderbuffer;
      R();
      m_max_dim=pdims;

      //and create a new one:
      if(m_fbo==0)
        {
          glGenFramebuffers(1, &m_fbo);
          if(m_fbo==0)
            {
              /*
                epiccally bad, cannot make an fbo,
                we fake it by reading/drawing to/from the screen!
              */
              WRATHwarning("Cannot create FBO: GL implementation out of spec, faking via drawing to screen");
              m_current_dim=pdims;
              glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
              glViewport(0, 0, m_current_dim.x(), m_current_dim.y());
              return routine_success;
            }
        }

      glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

      #ifdef FBO_COLOR_USE_RENDERBUFFER
      {
        glGenRenderbuffers(1, &m_texture);
        WRATHassert(m_texture!=0);
        glBindRenderbuffer(GL_RENDERBUFFER, m_texture);
        glRenderbufferStorage(GL_RENDERBUFFER, m_format, 
                              m_max_dim.x(), m_max_dim.y());
        
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                  GL_RENDERBUFFER, m_texture);
      }
      #else
      {
        glGenTextures(1, &m_texture);
        WRATHassert(m_texture!=0);
        
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        //TODO: under GL3 if m_format is an integer format
        //then the 7'th argument should also reflect that,
        //for example GL_RGB8I --> GL_RGB_INTEGER, etc.
        glTexImage2D(GL_TEXTURE_2D, 0, 
                     m_format, 
                     m_max_dim.x(), m_max_dim.y(), 0,
                     m_format, GL_UNSIGNED_BYTE, NULL);
        
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                               GL_TEXTURE_2D, m_texture, 0);
      }
      #endif

        /*
          TODO: for GLES2, if depth_stencil texture is supported,
          then use that
         */
      #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
      {
        m_depth_is_renderbuffer=true;
        m_depth=0;
        
        glGenRenderbuffers(1, &m_stencil);
        WRATHassert(m_stencil!=0);
        glBindRenderbuffer(GL_RENDERBUFFER, m_stencil);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, 
                              m_max_dim.x(), m_max_dim.y());
        
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, 
                                  GL_RENDERBUFFER, m_stencil);
      }
      #else
      {
        m_depth_is_renderbuffer=false;
        m_stencil=0;
        
        glGenTextures(1, &m_depth);
        WRATHassert(m_depth!=0);
        
        glBindTexture(GL_TEXTURE_2D, m_depth);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_DEPTH24_STENCIL8,
                     m_max_dim.x(), m_max_dim.y(), 0,
                     GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                               GL_TEXTURE_2D, m_depth, 0);
      }
      #endif
    }

  #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
  {
    if(m_depth==0)
      {
        glGenRenderbuffers(1, &m_depth);
        WRATHassert(m_depth!=0);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 
                              m_max_dim.x(), m_max_dim.y());
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                                  GL_RENDERBUFFER, m_depth);
      }
  }
  #endif

  m_current_dim=pdims;
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glViewport(0, 0, m_current_dim.x(), m_current_dim.y());

  return routine_success;
}

ivec4
WRATHShapeGPUDistanceFieldCreator::ScratchPadFBO::
viewport_parameters(void)
{
  return ivec4(0, 0, m_current_dim.x(), m_current_dim.y());
}


/////////////////////////////////////////////
//WRATHShapeGPUDistanceFieldCreator::DistanceFieldTarget_WRATHImage methods
WRATHShapeGPUDistanceFieldCreator::DistanceFieldTarget_WRATHImage::
DistanceFieldTarget_WRATHImage(WRATHImage *pImage, ivec2 offset):
  m_image(pImage),
  m_offset(offset)
{}

enum return_code
WRATHShapeGPUDistanceFieldCreator::DistanceFieldTarget_WRATHImage::
copy_results(const ScratchPad::handle &H)
{
  ivec4 rect;
  
  if(!H.valid() or m_image==NULL)
    {
      return routine_fail;
    }

  rect=H->viewport_parameters();
  m_image->copy_from_framebuffer(m_offset,
                                 ivec2(rect[0], rect[1]),
                                 ivec2(rect[2], rect[3]));
  
  return routine_success;

  
}
