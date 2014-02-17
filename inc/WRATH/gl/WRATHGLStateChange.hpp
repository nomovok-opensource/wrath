/*! 
 * \file WRATHGLStateChange.hpp
 * \brief file WRATHGLStateChange.hpp
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




#ifndef WRATH_HEADER_GL_STATE_HPP_
#define WRATH_HEADER_GL_STATE_HPP_

#include "WRATHConfig.hpp"
#include <set>
#include <vector>
#include "WRATHassert.hpp" 
#include "WRATHgl.hpp"
#include "vectorGL.hpp"
#include "WRATHReferenceCountedObject.hpp"

/*! \addtogroup GLUtility
 * @{
 */

class WRATHGLProgram;

/*!\class WRATHGLStateChange

  A WRATHGLStateChange represents a set
  of functions to affect GL state.

  One can use WRATHGLStateChange to modify 
  the following global GL state:\n

  - Polygon culling (GL_CULL_FACE, glCullFace, glFrontFace) [restore suggested] 
  - Polygon offset (GL_POLYGON_OFFSET_FILL, glPolygonOffset) [restore suggested] 
  - Viewport and DepthRange (glViewport, glDepthRangef) [restore suggested]
  - Blending State values [restore not suggested]
              -# glBlendEquation[Separate]
              -# glBlendFunc[Separate]
  - Change constant vertex attrib values: glVertexAttrib[1234]f[v]  [restore not suggested]
  - Color mask (glColorMask) [restore suggested]
  
  Do NOT modify the following GL state with a WRATHGLStateChange:\n

  - Bound FBO (glBindFramebuffer) [different FBO's should be different WRATHRawDrawData]
  - Blending Enabled (GL_BLEND) [blended vs solid items should be different WRATHRawDrawData]  
  - Scissor test (GL_SCISSOR_TEST, glScissors) [used by clipping algorithms] 
  - Stencil test  [used by clipping algorithms] 
              -# GL_STENCIL_TEST
              -# glStencilFunc[Separate]
              -# glStencilOp[Separate]
  - Depth test (GL_DEPTH_TEST, glDepthFunc)  [used by clipping algorithms] 
  - Depth and stencil masks  (glDepthMask, glStencilMask)  [used by clipping algorithms] 
  - Bound GLSL program (glUseProgram)  [handled by WRATHMultiGLProgram \ref WRATHDrawCallSpec::m_program] 
  - Index and Attribute sources  [handled by WRATHDrawCallSpec::attribute_array_params \ref WRATHDrawCallSpec::m_attribute_format_location, WRATHBufferObject \ref WRATHDrawCallSpec::m_data_source and WRATHDrawCommand \ref WRATHDrawCallSpec::m_draw_command]:
              -# glEnadle/DisableVertexAttribArray, 
              -# glVertexAttribPointer, 
              -# glBindBuffer
  - Bound textures (glBindTexture) [handled by WRATHTextureChoice \ref WRATHDrawCallSpec::m_bind_textures]
  - Uniform values (glUniform[1234][fi][v]) [handled by WRATHGLUniformData \ref WRATHDrawCallSpec::m_uniform_data]
 */
class WRATHGLStateChange:
  public WRATHReferenceCountedObjectT<WRATHGLStateChange>
{
public:

  /*!\class state_change
    A state_change represents a state
    change of GL to something, i.e.
    calling GL functions that directly
    affect GL state.
   */
  class state_change:
    public WRATHReferenceCountedObjectT<state_change>
  {
  public:  
    virtual
    ~state_change()
    {}

    /*!\fn void set_state(WRATHGLProgram*)
      To be implemented by a derived class
      to change GL state when this state_change
      becomes active.
      \param program currently bound program.
     */
    virtual
    void
    set_state(WRATHGLProgram *program)=0;

    /*!\fn void restore_state(WRATHGLProgram*)
      To be implemented by a derived class
      to restore GL state.
      \param program currently bound program.      
     */
    virtual
    void
    restore_state(WRATHGLProgram *program)=0;
  };

  /*!\class blend_state
    Represents setting the blending function,
    i.e. calling glBlendFunc.
   */
  class blend_state:public state_change
  {
  public:

    /*!\fn blend_state
      Ctor.
      \param a1 first argument to pass to glBlendFunc
      \param a2 second argument to pass to glBlendFunc

      a1 represents the source co-efficient
      and a2 represents the destination co-efficient
     */
    blend_state(GLenum a1, GLenum a2):
      state_change(),
      m_arg1(a1), m_arg2(a2)
    {}

    virtual
    void
    set_state(WRATHGLProgram*)
    {
      glBlendFunc(m_arg1, m_arg2);
    }

    virtual
    void
    restore_state(WRATHGLProgram*)
    {}

    /*!\var m_arg1
      First argument fed to glBlendFunc
     */
    GLenum m_arg1;

    /*!\var m_arg2
      Second agument fed to glBlendFunc
     */
    GLenum m_arg2;
  };

  /*!\typedef element_type
    Conveniance typedef for an element
    of a \ref element_type_collection
   */
  typedef state_change::handle element_type;

  /*!\typedef element_type_collection
    Conveniance typedef for a collection of handles
    to \ref state_change objects
   */ 
  typedef std::set<state_change::handle> element_type_collection;

  
  /*!\fn void add_state_change
    Add a state_change to this WRATHGLStateChange.
    The calling order of setting and restoring
    states is not defined. As such, for each
    GL state, no more than one state_change
    should be added to a WRATHGLStateChange 
    which affects that GL state.
    \param st state_change to add.
   */
  void
  add_state_change(state_change::handle st);

  /*!\fn void add_state_changes
    Add a group of state changes given
    as an STL iterator range, the range,
    as always with STL iterator is half open.
    \tparam iterator iterator type to a \ref state_change::handle
    \param begin iterator to first element to add
    \param end iterator to one past last element to add.
   */
  template<typename iterator>
  void
  add_state_changes(iterator begin, iterator end)
  {
    for(;begin!=end; ++begin)
      {
        add_state_change(*begin);
      }
  }

  /*!\fn add(iterator, iterator)
    Equivalent to add_state_changes(),
    provided for template programming
    conveniance.
    \tparam iterator iterator type to a \ref state_change::handle
    \param begin iterator to first element to add
    \param end iterator to one past last element to add.
   */
  template<typename iterator>
  void
  add(iterator begin, iterator end)
  {
    add_state_changes(begin, end);
  }

  /*!\fn void remove_state_change
    Remove a state_change from this WRATHGLStateChange.
    \param st state_change to remove.
   */
  void
  remove_state_change(state_change::handle st);

  /*!\fn void set_state(const const_handle&, WRATHGLProgram*) const
    Changes the state as according to the state 
    changes held in this WRATHGLStateChange. The 
    order in which states are set and restored is 
    not defined. Those states that are only defined
    in prev_value will have their state_change::restore_state() 
    methods called, those states that are only present 
    in this WRATHGLStateChange will have their 
    state_change::set_state()  methods called. Those state 
    objects common to both this WRATHGLStateChange and 
    prev_value will have neither their state_change::restore_state()
    and state_change::set_state() methods called. 
    \param prev_value state changes transitioning
           from. If the handle is invalid indicates
           that the call is the first state change
           call for the draw call.
    \param program currently bound program.

    Returns the number of actions called.
   */
  int
  set_state(const const_handle &prev_value, WRATHGLProgram *program) const;

  /*!\fn const element_type_collection& elements
    Returns the state_change objects of this
    WRATHGLStateChange
   */
  const element_type_collection&
  elements(void) const
  {
    return m_state_changes;
  }

  /*!\fn bool different(const WRATHGLStateChange::handle&,
                        const WRATHGLStateChange::handle&)
    Returns true if the contents of
    two WRATHGLStateChange differ from
    each other.
    \param v0 handle to a WRATHGLStateChange.
    \param v1 handle to a WRATHGLStateChange.
   */
  static
  bool
  different(const WRATHGLStateChange::const_handle &v0,
            const WRATHGLStateChange::const_handle &v1);

  /*!\fn bool compare
    Comparison function for two WRATHUniformData
    objects. Invalid handles are sorted first,
    and otherwise sorted by contents of the objects
    (i.e. elements()).
    \param lhs handle to left hand side of comparison op
    \param rhs handle to right hand side of comparison op
   */ 
  static
  bool
  compare(const WRATHGLStateChange::const_handle &lhs,
          const WRATHGLStateChange::const_handle &rhs);

private:
  std::set<state_change::handle> m_state_changes;
};
/*! @} */

#endif
