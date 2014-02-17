/*! 
 * \file WRATHGLStateStack.hpp
 * \brief file WRATHGLStateStack.hpp
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


#ifndef WRATH_HEADER_GL_STATE_STACK_HPP_
#define WRATH_HEADER_GL_STATE_STACK_HPP_

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include <stdint.h>
#include <vector>
#include "WRATHReferenceCountedObject.hpp"

/*! \addtogroup GLUtility
 * @{
 */

/*!\class WRATHGLStateStack
  A WRATHGLStateStack purpose is to allow
  for pushing and popping GL state, under
  OpenGL one has glPushAttrib/glPopAttib,
  but that does not exist under GLES2.
  Do NOT use WRATHGLStateStack in an
  innerloop of rendering, in particular
  do NOT use WRATHGLStateStack in a
  WRATHGLStateChange::state_change object's
  methods. Rather the use pattern is for
  doing one-time-ish actions, such as
  generating texture data via FBO rarely
  (for example distance field generation
  via GPU).

  On desconstruction, all state of a
  WRATHGLStateStack is popped, thus
  one can safely push state to
  a WRATHGLStateStack and just let the
  WRATHGLStateStack go out of scope to
  restore the state.
 */
class WRATHGLStateStack:boost::noncopyable
{
public:
  enum
    {
      /*!
        Indicates to save the state associated to color and blending
        - enable/disable of blending
        - blending source and destination functions
        - blend equation
        - blend color
        - color write mask
        - clear color
       */
      color_buffer_bit=1,

      /*!
        Indicates to save the state assoicated to depth buffer
        - enable/disable of the depth test
        - depth func
        - depth clear value
        - depth write mask
       */
      depth_buffer_bit=2,

      /*! 
        Indicates to save the state assoicated to stencil buffer
        - enable/disable stencil test
        - stencil func and reference values
        - stenicl op values
        - stencil clear value
        - stencil write mask
       */
      stencil_buffer_bit=4,

      /*!
        Indicates to save the state associated to rendering location
        - bound FBO
        - viewport
        - depth range
        - scissor values and if enabled
       */
      rendering_target_bit=8,

      /*!
        Indicates to save the state associated to how rendering is performed
        - polygon offset
        - culling enable
        - culling mode (i.e. glCullFace)
        - front face value (i.e glFrontFace)
       */
      rendering_action_bit=16
    };

  virtual
  ~WRATHGLStateStack(void);

  /*!\fn void push
    Push current GL state to this
    WRATHGLStateStack. The GL state 
    pushed is controlled by the
    bits of flags.                              
    \param flags bit field indicating what GL state
                 to push, see \ref color_buffer_bit,
                 \ref depth_buffer_bit, \ref stencil_buffer_bit,
                 \ref rendering_target_bit and \ref rendering_action_bit.
   */
  void
  push(uint32_t flags);
  
  /*!\fn void pop
    Restores the GL state saved with the 
    last \ref push(uint32_t) call.
   */
  void
  pop(void);

private:
  typedef WRATHReferenceCountedObject::handle_t<WRATHReferenceCountedObject> handle; 
  std::vector<handle> m_actions;

};
/*! @} */

#endif
