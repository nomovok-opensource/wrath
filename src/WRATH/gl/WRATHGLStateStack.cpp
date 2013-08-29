/*! 
 * \file WRATHGLStateStack.cpp
 * \brief file WRATHGLStateStack.cpp
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
#include "WRATHGLStateStack.hpp"
#include "WRATHglGet.hpp"
#include "WRATHNew.hpp"
#include "vectorGL.hpp"

namespace
{
  class enable_disable_bit
  {
  public:
    enable_disable_bit(GLenum enumeration);
    ~enable_disable_bit();

  private:
    GLboolean m_value;
    GLenum m_enumeration;

  };

  class generic_action:
    public WRATHReferenceCountedObjectT<generic_action>
  {};

  class color_buffer_action:public generic_action
  {
  public:
    color_buffer_action(void);
    ~color_buffer_action();
  private:
    enable_disable_bit m_blend_enable;
    GLint m_blend_src_rgb, m_blend_src_a;
    GLint m_blend_dest_rgb, m_blend_dest_a;
    GLint m_blend_equation_rgb, m_blend_equation_a;
    vec4 m_blend_color;  
    vecN<GLboolean, 4> m_color_mask;
    vec4 m_clear_color;
  };

  class depth_buffer_action:public generic_action
  {
  public:
    depth_buffer_action(void);
    ~depth_buffer_action();

  private:
    enable_disable_bit m_enable;
    GLint m_func;
    GLfloat m_clear;
    GLboolean m_mask;
  };

  class stencil_buffer_action:public generic_action
  {
  public:
    stencil_buffer_action(void);
    ~stencil_buffer_action();

  private:

    class per_face
    {
    public:
      explicit
      per_face(GLenum face);
      ~per_face();

    private:
      GLenum m_face;
      GLint m_mask;
      GLint m_func, m_func_mask, m_func_ref;
      GLint m_sfail, m_dpfail, m_dppass;
    };

    enable_disable_bit m_enable;
    per_face m_front, m_back;
    GLint m_clear_value;
  };

  class rendering_target_action:public generic_action
  {
  public:
    rendering_target_action(void);
    ~rendering_target_action();

  private:
    GLint m_fbo;
    vecN<GLint,4> m_viewport;
    vec2 m_depth_range;
    enable_disable_bit m_scissor_enable;
    vecN<GLint,4> m_scissor;
    
  };

  class rendering_action_action:public generic_action
  {
  public:
    rendering_action_action(void);
    ~rendering_action_action();

  private:
    enable_disable_bit m_polygon_offset;
    float m_polygon_offset_factor, m_polygon_offset_units;
    enable_disable_bit m_culling_enabled;
    GLint m_culling_mode;
    GLint m_front_face;
  };

  class action_packet:public generic_action
  {
  public:
    explicit
    action_packet(uint32_t flags);

  private:
    std::vector<handle> m_actions;
  };
}


/////////////////////////////////////
//WRATHGLStateStack methods
WRATHGLStateStack::
~WRATHGLStateStack()
{
  while(!m_actions.empty())
    {
      m_actions.pop_back();
    }
}

void
WRATHGLStateStack::
push(uint32_t flags)
{
  handle H;

  H=WRATHNew action_packet(flags);
  m_actions.push_back(H);
}

void
WRATHGLStateStack::
pop(void)
{
  m_actions.pop_back();
}


///////////////////////////////////////////
// action_packet methods
action_packet::
action_packet(uint32_t flags)
{
  if(flags&WRATHGLStateStack::color_buffer_bit)
    {
      m_actions.push_back(WRATHNew color_buffer_action());
    }

  if(flags&WRATHGLStateStack::depth_buffer_bit)
    {
      m_actions.push_back(WRATHNew depth_buffer_action());
    }

  if(flags&WRATHGLStateStack::stencil_buffer_bit)
    {
      m_actions.push_back(WRATHNew stencil_buffer_action());
    }

  if(flags&WRATHGLStateStack::rendering_target_bit)
    {
      m_actions.push_back(WRATHNew rendering_target_action());
    }

  if(flags&WRATHGLStateStack::rendering_action_bit)
    {
      m_actions.push_back(WRATHNew rendering_action_action());
    }
}

//////////////////////////////////////////
//enable_disable_bit methods
enable_disable_bit::
enable_disable_bit(GLenum enumeration):
  m_enumeration(enumeration)
{
  m_value=glIsEnabled(enumeration);
}

enable_disable_bit::
~enable_disable_bit()
{
  if(m_value==GL_TRUE)
    {
      glEnable(m_enumeration);
    }
  else
    {
      glDisable(m_enumeration);
    }
}

/////////////////////////////////////////////
// color_buffer_action methods
color_buffer_action::
color_buffer_action(void):
  m_blend_enable(GL_BLEND),
  m_blend_src_rgb(WRATHglGet<GLint>(GL_BLEND_SRC_RGB)),
  m_blend_src_a(WRATHglGet<GLint>(GL_BLEND_SRC_ALPHA)),
  m_blend_dest_rgb(WRATHglGet<GLint>(GL_BLEND_DST_RGB)),
  m_blend_dest_a(WRATHglGet<GLint>(GL_BLEND_DST_ALPHA)),
  m_blend_equation_rgb(WRATHglGet<GLint>(GL_BLEND_EQUATION_RGB)),
  m_blend_equation_a(WRATHglGet<GLint>(GL_BLEND_EQUATION_ALPHA)),
  m_blend_color(WRATHglGet<vec4>(GL_BLEND_COLOR)),
  m_color_mask(WRATHglGet<vecN<GLboolean, 4> >(GL_COLOR_WRITEMASK)),
  m_clear_color(WRATHglGet<vec4>(GL_COLOR_CLEAR_VALUE))
{}

color_buffer_action::
~color_buffer_action()
{
  glBlendFuncSeparate(m_blend_src_rgb, m_blend_dest_rgb,
                      m_blend_src_a, m_blend_dest_a);
  glBlendEquationSeparate(m_blend_equation_rgb, m_blend_equation_a);
  glBlendColor(m_blend_color[0], m_blend_color[1], 
               m_blend_color[2], m_blend_color[3]);
  
  glClearColor(m_clear_color[0], m_clear_color[1],
               m_clear_color[2], m_clear_color[3]);
}

///////////////////////////////////////////////
// depth_buffer_action methods
depth_buffer_action::
depth_buffer_action():
  m_enable(GL_DEPTH_TEST),
  m_func(WRATHglGet<GLint>(GL_DEPTH_FUNC)),
  m_clear(WRATHglGet<GLfloat>(GL_DEPTH_CLEAR_VALUE)),
  m_mask(WRATHglGet<GLboolean>(GL_DEPTH_WRITEMASK))
{}

depth_buffer_action::
~depth_buffer_action()
{
  glDepthFunc(m_func);
  glDepthMask(m_mask);

  #if defined(WRATH_GL_VERSION)
    glClearDepth(m_clear);
  #else
    glClearDepthf(m_clear);
  #endif
}

//////////////////////////////////
// stencil_buffer_action::per_face methods
stencil_buffer_action::per_face::
per_face(GLenum face):
  m_face(face),
  m_mask(WRATHglGet<GLint>( face==GL_FRONT?GL_STENCIL_VALUE_MASK:GL_STENCIL_BACK_VALUE_MASK)),
  m_func(WRATHglGet<GLint>( face==GL_FRONT?GL_STENCIL_FUNC:GL_STENCIL_BACK_FUNC)),
  m_func_mask(WRATHglGet<GLint>( face==GL_FRONT?GL_STENCIL_VALUE_MASK:GL_STENCIL_BACK_VALUE_MASK)),
  m_func_ref(WRATHglGet<GLint>( face==GL_FRONT?GL_STENCIL_REF:GL_STENCIL_BACK_REF)),
  m_sfail(WRATHglGet<GLint>( face==GL_FRONT?GL_STENCIL_FAIL:GL_STENCIL_BACK_FAIL)),
  m_dpfail(WRATHglGet<GLint>( face==GL_FRONT?
                              GL_STENCIL_PASS_DEPTH_FAIL:
                              GL_STENCIL_BACK_PASS_DEPTH_FAIL)),
  m_dppass(WRATHglGet<GLint>( face==GL_FRONT?
                              GL_STENCIL_PASS_DEPTH_PASS:
                              GL_STENCIL_BACK_PASS_DEPTH_PASS))
{}
        
stencil_buffer_action::per_face::
~per_face()
{
  glStencilOpSeparate(m_face, m_sfail, m_dpfail, m_dppass);
  glStencilFuncSeparate(m_face, m_func, m_func_ref, m_func_mask);
}


/////////////////////////////////////////////
// stencil_buffer_action methods
stencil_buffer_action::
stencil_buffer_action():
  m_enable(GL_STENCIL_TEST),
  m_front(GL_FRONT),
  m_back(GL_BACK),
  m_clear_value(WRATHglGet<GLint>(GL_STENCIL_CLEAR_VALUE))
{}

stencil_buffer_action::
~stencil_buffer_action()
{
  glClearStencil(m_clear_value);
}

/////////////////////////////////////
// rendering_target_action methods
rendering_target_action::
rendering_target_action(void):
  m_fbo(WRATHglGet<GLint>(GL_FRAMEBUFFER_BINDING)),
  m_viewport(WRATHglGet<vecN<GLint,4> >(GL_VIEWPORT)),
  m_depth_range(WRATHglGet<vec2>(GL_DEPTH_RANGE)),
  m_scissor_enable(GL_SCISSOR_TEST),
  m_scissor(WRATHglGet<vecN<GLint,4> >(GL_SCISSOR_BOX))
{}

rendering_target_action::
~rendering_target_action()
{
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
  glScissor(m_scissor[0], m_scissor[1], m_scissor[2], m_scissor[3]);
}

///////////////////////////////////
// rendering_action_action methods
rendering_action_action::
rendering_action_action(void):
  m_polygon_offset(GL_POLYGON_OFFSET_FILL),
  m_polygon_offset_factor(WRATHglGet<float>(GL_POLYGON_OFFSET_FACTOR)),
  m_polygon_offset_units(WRATHglGet<float>(GL_POLYGON_OFFSET_UNITS)),
  m_culling_enabled(GL_CULL_FACE),
  m_culling_mode(WRATHglGet<int>(GL_CULL_FACE_MODE)),
  m_front_face(WRATHglGet<int>(GL_FRONT_FACE))
{}

rendering_action_action::
~rendering_action_action()
{
  glPolygonOffset(m_polygon_offset_factor, m_polygon_offset_units);
  glCullFace(m_culling_mode);
  glFrontFace(m_front_face); 
}
