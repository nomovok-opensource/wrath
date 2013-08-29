/*! 
 * \file WRATHItemDrawState.hpp
 * \brief file WRATHItemDrawState.hpp
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

#ifndef __WRATH_ITEM_DRAW_STATE_HPP__
#define __WRATH_ITEM_DRAW_STATE_HPP__


/*! \addtogroup Group
 * @{
 */


#include "WRATHConfig.hpp"
#include <limits>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include "WRATHRawDrawData.hpp"
#include "WRATHDrawType.hpp"
#include "WRATHItemDrawer.hpp"


/*!\class WRATHSubItemDrawState
  A WRATHSubItemDrawState is not a genuine 
  WRATHItemDrawState, it only holds 
  - a set of GL state changes
  - textures to use
  - a set of uniforms
  - Buffer object usage hint for index data.
  It is used to modify an existing WRATHItemDrawState
  via WRATHItemDrawState::absorb().
 */
class WRATHSubItemDrawState
{
public:
  /*!\fn WRATHSubItemDrawState
    Ctor, initializes \ref m_buffer_object_hint
    as GL_STATIC_DRAW
   */
  WRATHSubItemDrawState(void):
    m_buffer_object_hint(GL_STATIC_DRAW)
  {}

  /*!\var m_textures
    Texture choices/bindings. Keyed by GL texture
    units (i.e. GL_TEXTURE0, GL_TEXTURE1, etc),
    see \ref WRATHTextureChoice and \ref 
    WRATHTextureChoice::texture_base.
   */
  std::map<GLenum, WRATHTextureChoice::texture_base::handle> m_textures;

  /*!\var m_gl_state_change 
    GL state changes/setters invoked, see 
    \ref WRATHGLStateChange and \ref WRATHGLStateChange::state_change.
   */
  std::set<WRATHGLStateChange::state_change::handle> m_gl_state_change;

  /*!\var m_uniforms  
    List of uniform "setters",
    see \ref WRATHUniformData and \ref 
    WRATHUniformData::uniform_setter_base.
   */
  std::set<WRATHUniformData::uniform_setter_base::handle> m_uniforms;

  /*!\var m_buffer_object_hint
    Used to dertermine if to use a buffer object
    for index data and if so what the usage hint 
    for the buffer object, a value of GL_INVALID_ENUM 
    indicates to not use a GL buffer object and any
    other value is the usage hint. Default value is
    GL_STATIC_DRAW.
   */
  GLenum m_buffer_object_hint;

  /*!\fn WRATHSubItemDrawState& buffer_object_hint
    Set the buffer object hint, \ref m_buffer_object_hint.   
    \param v new value to use
   */
  WRATHSubItemDrawState&
  buffer_object_hint(GLenum v)
  {
    m_buffer_object_hint=v;
    return *this;
  }

  /*!\fn WRATHSubItemDrawState& no_buffer_object
    Use to indicate to not back WRATHBufferObjects
    by a GL buffer object, provided as a readablity
    conveniance, equivalent to 
    \code
    buffer_object_hint(GL_INVALID_ENUM)
    \endcode
   */
  WRATHSubItemDrawState&
  no_buffer_object(void)
  {
    m_buffer_object_hint=GL_INVALID_ENUM;
    return *this;    
  }

  /*!\fn WRATHSubItemDrawState& add_uniform 
    Add a uniform (setter) to \ref m_uniforms.
    \param v handle to uniform setter to add.
   */
  WRATHSubItemDrawState&
  add_uniform(const WRATHUniformData::uniform_setter_base::handle &v)
  {
    if(v.valid())
      {
        m_uniforms.insert(v);
      }
    return *this;
  }
  
  /*!\fn WRATHSubItemDrawState& add_uniforms
    Add serval uniforms (setter) to \ref m_uniforms.
    \tparam iterator needs to be an iterator to a WRATHUniformData::uniform_setter_base::handle.
    \param begin iterator to 1st uniform to add
    \param end iterator to one past the last uniform to add
   */
  template<typename iterator>
  WRATHSubItemDrawState&
  add_uniforms(iterator begin, iterator end)
  {
    for(;begin!=end;++begin)
      {
        if(begin->valid())
          {
            m_uniforms.insert(*begin);
          }
      }
    return *this;
  }

  /*!\fn WRATHSubItemDrawState& remove_uniform 
    Remove a uniform (setter) to \ref m_uniforms.
    \param v handle to uniform setter to remove.
   */
  WRATHSubItemDrawState&
  remove_uniform(const WRATHUniformData::uniform_setter_base::handle &v)
  {
    m_uniforms.erase(v);
    return *this;
  }

  /*!\fn WRATHSubItemDrawState& add_gl_state_change
    Add a GL state change to \ref m_gl_state_change.
    \param v handle to state change to add.
   */
  WRATHSubItemDrawState&
  add_gl_state_change(const WRATHGLStateChange::state_change::handle &v)
  {
    if(v.valid())
      {
        m_gl_state_change.insert(v);
      }
    return *this;
  }

  /*!\fn WRATHSubItemDrawState& remove_gl_state_change
    Remove a GL state change to \ref m_gl_state_change.
    \param v handle to state change to remove.
   */
  WRATHSubItemDrawState&
  remove_gl_state_change(const WRATHGLStateChange::state_change::handle &v)
  {
    m_gl_state_change.erase(v);
    return *this;
  }

  /*!\fn WRATHSubItemDrawState& add_gl_state_changes
    Add serval uniforms (setter) to \ref m_uniforms.
    \tparam iterator needs to be an iterator to a WRATHGLStateChange::state_change::handle
    \param begin iterator to 1st state change to add
    \param end iterator to one past the last state change to add
   */
  template<typename iterator>
  WRATHSubItemDrawState&
  add_gl_state_changes(iterator begin, iterator end)
  {
    for(;begin!=end;++begin)
      {
        if(begin->valid())
          {
            m_gl_state_change.insert(*begin);
          }
      }
    return *this;
  }

  /*!\fn WRATHSubItemDrawState& add_texture 
    Set a texture to be bounded to the named texture unit.
    An invalid handle indicates to set so that no texture
    is bound to the named texture unit.
    \param tex_unit which texture unit as in WRATHTextureChoice::add_texture
    \param ptex handle to texture to bind to tex_unit, for example GL_TEXTURE0
   */
  WRATHSubItemDrawState&
  add_texture(GLenum tex_unit, WRATHTextureChoice::texture_base::handle ptex)
  {
    if(ptex.valid())
      {
        m_textures[tex_unit]=ptex;
      }
    else
      {
        m_textures.erase(tex_unit);
      }
    return *this;
  }

  /*!\fn WRATHSubItemDrawState& add_textures 
    Conveniance function to add many texture 
    binds.
    \tparam iterator is an iterator to std::pair<GLenum,texture_base::handle>
    \param begin iterator to first element to add
    \param end iterator to one past last element to add.
   */
  template<typename iterator>
  WRATHSubItemDrawState&
  add_textures(iterator begin, iterator end)
  {
    for(;begin!=end;++begin)
      {
        if(begin->second.valid())
          {
            m_textures[begin->first]=begin->second;
          }
        else
          {
            m_textures.erase(begin->first);
          }
      }
    return *this;
  }

  /*!\fn WRATHSubItemDrawState& absorb
    Include all GL state changes, texture bindings
    and uniforms from another WRATHSubItemDrawState.
    Additionaly, takes the value of \ref m_buffer_object_hint.
    If a texture binding point is already used,
    the one from the WRATHSubItemDrawState subkey is used.
    \param subkey WRATHSubItemDrawState for whcih to absorb elements.
   */
  WRATHSubItemDrawState&
  absorb(const WRATHSubItemDrawState &subkey)
  {
    add_textures(subkey.m_textures.begin(), subkey.m_textures.end());
    add_gl_state_changes(subkey.m_gl_state_change.begin(), subkey.m_gl_state_change.end());
    add_uniforms(subkey.m_uniforms.begin(), subkey.m_uniforms.end());
    m_buffer_object_hint=subkey.m_buffer_object_hint;
    return *this;
  }

};



 
/*!\class WRATHItemDrawState
   A WRATHItemDrawState is the interface to specify how
   a UI item is drawn. It is roughly comprised of:
   - a GL state vector on how to draw (\ref m_textures, \ref m_gl_state_change, \ref m_uniforms, \ref m_drawer, \ref m_primitive_type)
   - if (and how) index data resides in a buffer object (\ref m_buffer_object_hint)
   - meta data to specify when to draw (\ref m_force_draw_order and \ref m_draw_type)

  See also \ref WRATHCompiledItemDrawState and WRATHCompiledItemDrawStateCollection
 */
class WRATHItemDrawState
{
public:
  /*!\fn WRATHItemDrawState(void)
    Empty ctor, initializes value
    to indicate an invalid key, and
    also inits \ref m_buffer_object_hint
    as GL_STATIC_DRAW.
   */
  WRATHItemDrawState(void):
    m_primitive_type(GL_INVALID_ENUM),
    m_drawer(NULL),
    m_buffer_object_hint(GL_STATIC_DRAW)
  {}

  /*!\fn WRATHItemDrawState(WRATHItemDrawer *pdrawer,
                             GLenum pprimitive_type=GL_TRIANGLES)
    Ctor. Initializes the WRATHItemDrawState.
    \param pdrawer WRATHItemDrawer used to draw batches of elements.   
    \param pprimitive_type primitive type, passed to glDrawElements,
                           typically GL_TRIANGLES
  
   */
  explicit
  WRATHItemDrawState(WRATHItemDrawer *pdrawer,
                      GLenum pprimitive_type=GL_TRIANGLES):
    m_primitive_type(pprimitive_type),
    m_drawer(pdrawer),
    m_buffer_object_hint(GL_STATIC_DRAW)
  {}
   
  /*!\var m_force_draw_order 
    The handle m_force_draw_order gives a way
    to force drawing order, different values
    of m_force_draw_order give rise to different
    WRATHItemGroup objects, hence a different value of
    \ref m_force_draw_order breaks batching.
    This value is used directly in the key
    WRATHRawDrawDataElement generated by
    a WRATHItemGroup. The defualt value of \ref 
    m_force_draw_order is an invalid handle, see
    also \ref WRATHDrawOrder.
  */
  WRATHDrawOrder::const_handle m_force_draw_order;

  /*!\var m_textures 
    Texture choices/bindings. Keyed by GL texture
    units (i.e. GL_TEXTURE0, GL_TEXTURE1, etc),
    see \ref WRATHTextureChoice and \ref 
    WRATHTextureChoice::texture_base.
   */
  std::map<GLenum, WRATHTextureChoice::texture_base::handle> m_textures;

  /*!\var m_gl_state_change
    GL state changes/setters invoked, see 
    \ref WRATHGLStateChange and \ref WRATHGLStateChange::state_change.
   */
  std::set<WRATHGLStateChange::state_change::handle> m_gl_state_change;

  /*!\var m_uniforms
    List of uniform "setters",
    see \ref WRATHUniformData and \ref 
    WRATHUniformData::uniform_setter_base.
   */
  std::set<WRATHUniformData::uniform_setter_base::handle> m_uniforms;

  /*!\var m_primitive_type
    Primitive type, i.e. GL_TRIANGLES, GL_LINES, GL_POINTS, etc
   */
  GLenum m_primitive_type;
  
  /*!\var m_drawer
    Drawer of data
   */
  WRATHItemDrawer *m_drawer;

  /*!\var m_buffer_object_hint
    Used to dertermine if to use a buffer object,
    and if so what the usage hint for the buffer
    object for the index buffer of a WRATHItemGroup.
    A value of GL_INVALID_ENUM indicates
    to not use a GL buffer object and any other
    value is the usage hint. Default value is
    GL_STATIC_DRAW.
   */
  GLenum m_buffer_object_hint;

  /*!\var m_draw_type
    WRATHDrawType meta-data used by an implementation
    of \ref WRATHCanvas to determine at what phase 
    of drawing to do the drawing. 
   */
  WRATHDrawType m_draw_type;

  /*!\fn WRATHItemDrawState& draw_type
    Set the WRATHDrawType, \ref m_draw_type.
    Default value is WRATHDrawType().
    \param v new value to use
   */
  WRATHItemDrawState&
  draw_type(WRATHDrawType v)
  {
    m_draw_type=v;
    return *this;
  }
  
  /*!\fn bool operator<(const WRATHItemDrawState &) const
    Comparison operator to sort WRATHItemDrawState,
    sorted by:
    - 1) \ref m_force_draw_order
    - 2) WRATHItemDrawer of \ref m_drawer
    - 3) \ref m_buffer_object_hint
    - 4) \ref m_primitive_type
    - 5) \ref m_draw_type
    - 6) WRATHTextureChoice::texture_base 's \ref m_textures
    - 7) WRATHGLStateChange::state_change 's \ref m_gl_state_change
    - 8) WRATHUniformData::uniform_setter_base 's \ref m_uniforms
    \param rhs object to which to compare
   */
  bool
  operator<(const WRATHItemDrawState &rhs) const;

  /*!\fn bool compare_GL_state_vector
    Similar to operator<(), except that the 
    field \ref m_force_draw_order is ignored
    \param rhs object to which to compare
   */
  bool
  compare_GL_state_vector(const WRATHItemDrawState &rhs) const;

  /*!\fn WRATHItemDrawState& force_draw_order(const WRATHDrawOrder::const_handle&)
    Set the value for \ref m_force_draw_order, the defualt
    value of \ref m_force_draw_order is an invalid handle. 
    \param v value to which to set \ref m_force_draw_order
   */
  WRATHItemDrawState&
  force_draw_order(const WRATHDrawOrder::const_handle &v)
  {
    m_force_draw_order=v;
    return *this;
  }

  /*!\fn WRATHItemDrawState& add_texture
    Set a texture to be bounded to the named texture unit.
    \param tex_unit which texture unit as in WRATHTextureChoice::add_texture
    \param ptex handle to texture to bind to tex_unit
   */
  WRATHItemDrawState&
  add_texture(GLenum tex_unit, WRATHTextureChoice::texture_base::handle ptex)
  {
    if(ptex.valid())
      {
        m_textures[tex_unit]=ptex;
      }
    else
      {
        m_textures.erase(tex_unit);
      }
    return *this;
  }

  /*!\fn WRATHItemDrawState& add_textures
    Conveniance function to add many texture 
    binds
    \tparam iterator is an iterator to std::pair<GLenum,texture_base::handle>.
    \param begin iterator to first element to add
    \param end iterator to one past last element to add.
   */
  template<typename iterator>
  WRATHItemDrawState&
  add_textures(iterator begin, iterator end)
  {
    for(;begin!=end;++begin)
      {
        if(begin->second.valid())
          {
            m_textures[begin->first]=begin->second;
          }
        else
          {
            m_textures.erase(begin->first);
          }
      }
    return *this;
  }

  /*!\fn WRATHItemDrawState& primitive_type
    Set the primititive type, \ref m_primitive_type.
   */
  WRATHItemDrawState&
  primitive_type(GLenum v)
  {
    m_primitive_type=v;
    return *this;
  }

  /*!\fn WRATHItemDrawState& drawer
    Set the drawer, \ref m_drawer.
    \param v new value to use
   */
  WRATHItemDrawState&
  drawer(WRATHItemDrawer *v)
  {
    m_drawer=v;
    return *this;
  }
  
  /*!\fn WRATHItemDrawState& buffer_object_hint
    Set the buffer object hint, \ref m_buffer_object_hint.
    A value of GL_INVALID_ENUM indicates
    to not use a GL buffer object and any other
    value is the usage hint. Default value
    is GL_STATIC_DRAW.
    \param v new value to use
   */
  WRATHItemDrawState&
  buffer_object_hint(GLenum v)
  {
    m_buffer_object_hint=v;
    return *this;
  }

  /*!\fn WRATHItemDrawState& no_buffer_object
    Set to indicate to not back WRATHBufferObjects
    by a GL buffer object, provided as a readablity
    conveniance, equivalent to 
    \code
    buffer_object_hint(GL_INVALID_ENUM)
    \endcode
   */
  WRATHItemDrawState&
  no_buffer_object(void)
  {
    m_buffer_object_hint=GL_INVALID_ENUM;
    return *this;    
  }

  /*!\fn WRATHItemDrawState& add_uniform
    Add a uniform (setter) to \ref m_uniforms.
    \param v handle to uniform setter to add.
   */
  WRATHItemDrawState&
  add_uniform(const WRATHUniformData::uniform_setter_base::handle &v)
  {
    if(v.valid())
      {
        m_uniforms.insert(v);
      }
    return *this;
  }
  
  /*!\fn WRATHItemDrawState& add_uniforms
    Add serval uniforms (setter) to \ref m_uniforms.
    \tparam iterator needs to be an iterator to a WRATHUniformData::uniform_setter_base::handle.
    \param begin iterator to 1st uniform to add
    \param end iterator to one past the last uniform to add
   */
  template<typename iterator>
  WRATHItemDrawState&
  add_uniforms(iterator begin, iterator end)
  {
    for(;begin!=end;++begin)
      {
        if(begin->valid())
          {
            m_uniforms.insert(*begin);
          }
      }
    return *this;
  }

  /*!\fn WRATHItemDrawState& remove_uniform
    Remove a uniform (setter) to \ref m_uniforms.
    \param v handle to uniform setter to remove.
   */
  WRATHItemDrawState&
  remove_uniform(const WRATHUniformData::uniform_setter_base::handle &v)
  {
    m_uniforms.erase(v);
    return *this;
  }

  /*!\fn WRATHItemDrawState& add_gl_state_change
    Add a GL state change to \ref m_gl_state_change.
    \param v handle to state change to add.
   */
  WRATHItemDrawState&
  add_gl_state_change(const WRATHGLStateChange::state_change::handle &v)
  {
    if(v.valid())
      {
        m_gl_state_change.insert(v);
      }
    return *this;
  }

  /*!\fn WRATHItemDrawState& remove_gl_state_change
    Remove a GL state change to \ref m_gl_state_change.
    \param v handle to state change to remove.
   */
  WRATHItemDrawState&
  remove_gl_state_change(const WRATHGLStateChange::state_change::handle &v)
  {
    m_gl_state_change.erase(v);
    return *this;
  }

  /*!\fn WRATHItemDrawState& add_gl_state_changes
    Add serval GL state changes to \ref m_gl_state_change.
    \tparam iterator needs to be an iterator to a WRATHGLStateChange::state_change::handle
    \param begin iterator to 1st state change to add
    \param end iterator to one past the last state change to add
   */
  template<typename iterator>
  WRATHItemDrawState&
  add_gl_state_changes(iterator begin, iterator end)
  {
    for(;begin!=end;++begin)
      {
        if(begin->valid())
          {
            m_gl_state_change.insert(*begin);
          }
      }
    return *this;
  }

  /*!\fn WRATHItemDrawState& absorb
    Include all GL state changes, texture bindings
    and uniforms from a WRATHSubItemDrawState.
    Additionaly, takes the value of \ref m_buffer_object_hint.
    If a texture binding point is already used,
    the one from the WRATHSubItemDrawState subkey is used.
    \param subkey WRATHSubItemDrawState to absorb elements from.
   */
  WRATHItemDrawState&
  absorb(const WRATHSubItemDrawState &subkey)
  {
    add_textures(subkey.m_textures.begin(), subkey.m_textures.end());
    add_gl_state_changes(subkey.m_gl_state_change.begin(), subkey.m_gl_state_change.end());
    add_uniforms(subkey.m_uniforms.begin(), subkey.m_uniforms.end());
    m_buffer_object_hint=subkey.m_buffer_object_hint;
    return *this;
  }

};


/*!\class WRATHCompiledItemDrawState
  A WRATHCompiledItemDrawState represents a 
  \ref WRATHItemDrawState ignoring \ref WRATHItemDrawState::m_force_draw_order
  compiled into a simpler object that is much faster to perform comparisons.
  To that end it uses:
  - a \ref WRATHUniformData in place of a set of WRATHUniformData::uniform_setter_base
  - a \ref WRATHGLStateChange in place of a set of WRATHGLStateChange::state_change 
  - a \ref WRATHTextureChoice in place of a set of WRATHTextureChoice::texture_base

  See also \ref WRATHItemDrawState and WRATHCompiledItemDrawStateCollection
 */
class WRATHCompiledItemDrawState
{
public:
  /*!\typedef uniform_setter_collection
    Conveniance typedef to a set of handles to
    \ref WRATHUniformData::uniform_setter_base objects
   */
  typedef std::set<WRATHUniformData::uniform_setter_base::handle> uniform_setter_collection; 

  /*!\typedef state_change_collection
    Conveniance typedef to a set of handles to
    \ref WRATHGLStateChange::state_change objects
   */
  typedef std::set<WRATHGLStateChange::state_change::handle> state_change_collection; 

  /*!\typedef texture_bind_collection
    Conveniance typedef to a map of handles to
    \ref WRATHTextureChoice::texture_base objects
    keyed by GL texture unit (i.e. GL_TEXTURE0, GL_TEXTURE1, etc)
   */
  typedef std::map<GLenum, WRATHTextureChoice::texture_base::handle> texture_bind_collection;

  /*!\fn WRATHCompiledItemDrawState(void)
    Ctor. Initializes the \ref WRATHCompiledItemDrawState
    with value of a freshly consructed
    \ref WRATHItemDrawState object, i.e. 
    - \ref m_drawer NULL
    - \ref m_primitive_type as GL_INVALID_ENUM
    - \ref m_buffer_object_hint as GL_STATIC_DRAW
    - all other handles are invalid handles
   */
  WRATHCompiledItemDrawState(void):
    m_drawer(NULL),
    m_buffer_object_hint(GL_STATIC_DRAW),
    m_primitive_type(GL_INVALID_ENUM)
  {}

  /*!\fn WRATHCompiledItemDrawState(const WRATHItemDrawState&)
    Ctor. Initializes the \ref WRATHCompiledItemDrawState from
    a \ref WRATHItemDrawState
    - \ref m_drawer as \ref WRATHItemDrawState::m_drawer
    - \ref m_buffer_object_hint as \ref WRATHItemDrawState::m_buffer_object_hint
    - \ref m_primitive_type as \ref WRATHItemDrawState::m_primitive_type
    - \ref m_draw_type as \ref WRATHItemDrawState::m_draw_type
    - \ref m_uniforms as fetch_compiled_uniform() applied to \ref WRATHItemDrawState::m_uniforms
    - \ref m_gl_state_change as fetch_compiled_state_change() applied to \ref WRATHItemDrawState::m_gl_state_change
    - \ref m_textures as fetch_compiled_texture() applied to \ref WRATHItemDrawState::m_textures
   */
  WRATHCompiledItemDrawState(const WRATHItemDrawState &obj):
    m_drawer(obj.m_drawer),
    m_buffer_object_hint(obj.m_buffer_object_hint),
    m_primitive_type(obj.m_primitive_type),
    m_draw_type(obj.m_draw_type),
    m_uniforms(fetch(obj.m_uniforms)),
    m_gl_state_change(fetch(obj.m_gl_state_change)),
    m_textures(fetch(obj.m_textures))
  {}

  /*!\var m_drawer
    Same role as in \ref WRATHItemDrawState::m_drawer
   */
  WRATHItemDrawer *m_drawer;

  /*!\var m_buffer_object_hint
    Same role as in \ref WRATHItemDrawState::m_buffer_object_hint
   */
  GLenum m_buffer_object_hint;

  /*!\var m_primitive_type
    Same role as in \ref WRATHItemDrawState::m_primitive_type
   */
  GLenum m_primitive_type;

  /*!\var m_draw_type
    Same role as in \ref WRATHItemDrawState::m_draw_type
   */
  WRATHDrawType m_draw_type;

  /*!\var m_uniforms
    Same role as in \ref WRATHItemDrawState::m_uniforms,
    value is generated via fetch_compiled_uniform()
   */
  WRATHUniformData::const_handle m_uniforms;

  /*!\var m_gl_state_change
    Same role as in \ref WRATHItemDrawState::m_gl_state_change,
    value is generated via fetch_compiled_state_change()
   */
  WRATHGLStateChange::const_handle m_gl_state_change;

  /*!\var m_textures
    Same role as in \ref WRATHItemDrawState::m_textures,
    value is generated via fetch_compiled_texture()
   */
  WRATHTextureChoice::const_handle m_textures;

  /*!\fn bool operator<(const WRATHCompiledItemDrawState &) const
    Comparison operator.
    \param rhs value to which to compare
   */
  bool
  operator<(const WRATHCompiledItemDrawState &rhs) const;

  /*!\fn WRATHUniformData::const_handle fetch_compiled_uniform
    Given a set of handles to \ref WRATHUniformData::uniform_setter_base
    objects, returns a handle to a single \ref WRATHUniformData object.
    It is guaranteed that passing the same set produces the exact same
    value.
    \param p set of handles to compile into a single handle
   */
  static
  WRATHUniformData::const_handle
  fetch_compiled_uniform(const uniform_setter_collection &p);

  /*!\fn WRATHGLStateChange::const_handle fetch_compiled_state_change
    Given a set of handles to \ref WRATHGLStateChange::state_change
    objects, returns a handle to a single WRATHGLStateChange object.
    It is guaranteed that passing the same set produces the exact same
    value.
    \param p set of handles to compile into a single handle
   */
  static
  WRATHGLStateChange::const_handle
  fetch_compiled_state_change(const state_change_collection &p);

  /*!\fn WRATHTextureChoice::const_handle fetch_compiled_texture
    Given a set of handles to \ref WRATHTextureChoice::texture_base
    objects, returns a handle to a single WRATHTextureChoice object.
    It is guaranteed that passing the same map produces the exact same
    value.
    \param p map of handles keyed by texture unit to compile into a single handle
   */
  static
  WRATHTextureChoice::const_handle
  fetch_compiled_texture(const texture_bind_collection &p);

  /*!\fn WRATHUniformData::const_handle fetch(const uniform_setter_collection&) 
    Provided as a conveniance, equivalent to
    \code
    fetch_compiled_uniform(p)
    \endcode
    \param p set of handles to compile into a single handle
   */
  static
  WRATHUniformData::const_handle
  fetch(const uniform_setter_collection &p)
  {
    return fetch_compiled_uniform(p);
  }

  /*!\fn WRATHGLStateChange::const_handle fetch(const state_change_collection&) 
    Provided as a conveniance, equivalent to
    \code
    fetch_compiled_state_change(p)
    \endcode
    \param p set of handles to compile into a single handle
   */
  static
  WRATHGLStateChange::const_handle
  fetch(const state_change_collection &p)
  {
    return fetch_compiled_state_change(p);
  }

  /*!\fn WRATHTextureChoice::const_handle fetch(const texture_bind_collection&) 
    Provided as a conveniance, equivalent to
    \code
    fetch_compiled_texture(p)
    \endcode
    \param p map of handles keyed by texture unit to compile into a single handle
   */
  static
  WRATHTextureChoice::const_handle
  fetch(const texture_bind_collection &p)
  {
    return fetch_compiled_texture(p);
  }
  
};


/*!\class WRATHCompiledItemDrawStateCollection
  A \ref WRATHCompiledItemDrawStateCollection is the 
  data of a set of \ref WRATHCompiledItemDrawState objects
  presented as pair of arrays of vectors.
 */
class WRATHCompiledItemDrawStateCollection
{
public:
  /*!\fn WRATHCompiledItemDrawStateCollection(void)
    Ctor. Initializes the \ref WRATHCompiledItemDrawStateCollection
    as empty.
   */
  WRATHCompiledItemDrawStateCollection(void)
  {}

  /*!\fn WRATHCompiledItemDrawStateCollection(const std::set<WRATHItemDrawState>&)
    Ctor. Initializes the \ref WRATHCompiledItemDrawStateCollection
    from a set of \ref WRATHItemDrawState objects. Each element
    of the passed set creates an element in \ref WRATHCompiledItemDrawStateCollection.
    The fields \ref WRATHItemDrawState::m_buffer_object_hint and
    \ref WRATHItemDrawState::m_primitive_type must be the same
    value throughout the set.
    \param p set of WRATHItemDrawState values from which to initialize
   */
  WRATHCompiledItemDrawStateCollection(const std::set<WRATHItemDrawState> &p);

  /*!\fn WRATHCompiledItemDrawStateCollection(const WRATHItemDrawState&)
    Ctor. Initializes the \ref WRATHCompiledItemDrawStateCollection
    from a single \ref WRATHItemDrawState object. As such, the created
    object will have only one element.
    \param p WRATHItemDrawState from which to initialize
   */
  WRATHCompiledItemDrawStateCollection(const WRATHItemDrawState &p);

  /*!\fn WRATHCompiledItemDrawStateCollection(const_c_array<WRATHCompiledItemDrawState>,
                                              const_c_array<WRATHDrawOrder::const_handle>)
    Ctor. Initializes the \ref WRATHCompiledItemDrawStateCollection
    from an array of \ref WRATHCompiledItemDrawState objects and
    an array of WRATHDrawOrder handles. The fields 
    \ref WRATHCompiledItemDrawState::m_buffer_object_hint and
    \ref WRATHCompiledItemDrawState::m_primitive_type must be the same
    value throughout the array. If pforce_draw_orders is larger, then additional 
    elements are ignored. If it is smaller invalid handles are added until it is 
    the same size.
    \param pdraw_states draw states, the ordering is copied from the array
    \param pforce_draw_orders force draw order objects, the ordering is copied from the array
   */
  WRATHCompiledItemDrawStateCollection(const_c_array<WRATHCompiledItemDrawState> pdraw_states,
                                       const_c_array<WRATHDrawOrder::const_handle> pforce_draw_orders=
                                       const_c_array<WRATHDrawOrder::const_handle>());
  /*!\fn unsigned int size
    Returns the number of draw elements of the
    WRATHCompiledItemDrawStateCollection object.
   */
  unsigned int
  size(void) const
  {
    return m_draw_states.size();
  }

  /*!\fn const std::vector<WRATHCompiledItemDrawState>& draw_states
    Returns the \ref WRATHCompiledItemDrawState
    array of this WRATHCompiledItemDrawStateCollection 
    object. The i'th element of the array is to
    use the \ref WRATHDrawOrder object given by
    the i'th element of force_draw_orders().
   */
  const std::vector<WRATHCompiledItemDrawState>&
  draw_states(void) const 
  {
    return m_draw_states;
  }

  /*!\fn const WRATHCompiledItemDrawState& draw_state
    Provided as a conveniance, equivalent to
    \code
    draw_states()[i]
    \endcode
   */
  const WRATHCompiledItemDrawState&
  draw_state(unsigned int i) const 
  {
    return m_draw_states[i];
  }

  /*!\fn const std::vector<WRATHDrawOrder::const_handle>& force_draw_orders
    Returns the \ref WRATHDrawOrder handle
    array of this WRATHCompiledItemDrawStateCollection 
    object. The i'th element of the array is to
    use the \ref WRATHCompiledItemDrawState object given by
    the i'th element of draw_states().
   */
  const std::vector<WRATHDrawOrder::const_handle>&
  force_draw_orders(void) const 
  {
    return m_force_draw_orders;
  }

  /*!\fn const WRATHDrawOrder::const_handle& force_draw_order
    Provided as a conveniance, equivalent to
    \code
    force_draw_orders()[i]
    \endcode
   */
  const WRATHDrawOrder::const_handle&
  force_draw_order(unsigned int i) const 
  {
    return m_force_draw_orders[i];
  }

  /*!\fn GLenum buffer_object_hint
    Provided as a conveniance, equivalent to
    \code
    draw_state(0).m_buffer_object_hint
    \endcode
    If size() returns 0, then returns GL_INVALID_ENUM
   */
  GLenum
  buffer_object_hint(void) const
  {
    return m_draw_states.empty()?
      GL_INVALID_ENUM:
      m_draw_states[0].m_buffer_object_hint;
  }

  /*!\fn GLenum primitive_type
    Provided as a conveniance, equivalent to
    \code
    draw_state(0).m_primitive_type
    \endcode
    If size() returns 0, then returns GL_INVALID_ENUM
   */
  GLenum
  primitive_type(void) const
  {
    return m_draw_states.empty()?
      GL_INVALID_ENUM:
      m_draw_states[0].m_primitive_type;
  }

  /*!\fn bool operator<(const WRATHCompiledItemDrawStateCollection&) const
    Comparison operator.
    \param rhs value to which to compare
   */
  bool
  operator<(const WRATHCompiledItemDrawStateCollection &rhs) const;

private:
  std::vector<WRATHCompiledItemDrawState> m_draw_states;
  std::vector<WRATHDrawOrder::const_handle> m_force_draw_orders;
};


/*! @} */



#endif
