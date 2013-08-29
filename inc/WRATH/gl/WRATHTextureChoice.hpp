/*! 
 * \file WRATHTextureChoice.hpp
 * \brief file WRATHTextureChoice.hpp
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



#ifndef __WRATH_TEXTURE_CHOICE_HPP__
#define __WRATH_TEXTURE_CHOICE_HPP__

#include "WRATHConfig.hpp"
#include <map>
#include "WRATHassert.hpp" 
#include <boost/utility.hpp>
#include "WRATHReferenceCountedObject.hpp"
#include "WRATHgl.hpp"
#include "WRATHglGet.hpp"
#include "WRATHNew.hpp"
#include "WRATHUniformData.hpp"

/*! \addtogroup GLUtility
 * @{
 */

/*!\class WRATHTextureChoice
  A WRATHTextureChoice represents what texture to bind
  to different texture units.
 */
class WRATHTextureChoice:
  public WRATHReferenceCountedObjectT<WRATHTextureChoice>
{
public:

  /*!\class texture_base
    A texture_base represents a base
    class for texture binds. 
    The bind call itself is
    to be provided by the 
    method bind_texture().
   */
  class texture_base:
    public WRATHReferenceCountedObjectT<texture_base>
  {
  public:    
    texture_base(void)
    {}

    virtual
    ~texture_base()
    {}

    /*!\fn void bind_texture(GLenum)
      To be implemented by a derived class
      to bind the texture. When called, GL
      will have the correct texture unit
      passed already active via glActiveTexture.
      \param texture_unit the texture unit to which to bind
                          the texture, the unit is already
                          active via glActiveTexture.
     */
    virtual
    void
    bind_texture(GLenum texture_unit)=0;

    /*!\fn void unbind_texture
      To be optionally implemented by a 
      derived class to unbind the texture.
      The use case is for syncing across
      contexts of different threads (or
      possibly processes) to "unlock" the
      texture to allow for another thread
      (or process) to modify the texture
      data. In contrast to bind_texture(),
      the active texture unit may or may
      not be active. Each call to bind_texture()
      has a matching call to unbind_texture()
      [with the same value to texture_unit],
      but it is possible for the calls to
      not be stacked, i.e. it is possible
      for example to have: bind_texture(u0),
      bind_texture(u1), bind_texture(u2),
      unbind_texture(u0), unbind_texture(u2),
      unbind_texture(u1). I.e. there is 
      no order matching or stacking for the
      bind/unbind calls.
      \param texture_unit the texture unit from which the 
                          texture is to be unbound. The texture
                          unit may or may not be the active 
                          texture via glActiveTexture.
     */
    virtual
    void
    unbind_texture(GLenum texture_unit);

    /*!\fn WRATHUniformData::uniform_setter_base::handle texture_size
      To be optionally implemented to return a uniform
      that holds the size of the texture. The default
      implementation is to return an invalid handle.
      For those GL/GLES implementations that support
      textureSize() functions, this method is 
      superfluous and unnecessary.
      \param pname _prefix_ for the uniform name, the name
                   for the uniform will be pname + "Size".
     */
    virtual
    WRATHUniformData::uniform_setter_base::handle
    texture_size(const std::string &pname);
  };
    
  /*!\class texture
    A texture object holds a binding point,
    a texture unit and a texture name.
    The binding point determines the type
    of texture (for example GL_TEXTURE_2D),
    the texture unit determines which
    texture unit the texture is bound to
    and the texture name determines
    what texture to use. 
   */
  class texture:public texture_base
  {
  public:
    /*!\fn texture
      Ctor. 
      \param tex_name name of texture, i.e. value passed to glBindTexture to
                           name the texture to be bound
      \param binding_pt binding point of texture; specifies the texture type
                        for core GLES2, must be GL_TEXTURE_2D or GL_TEXTURE_CUBE_MAP
     */
    texture(GLuint tex_name, GLenum binding_pt=GL_TEXTURE_2D):
      m_texture_name(tex_name),
      m_binding_point(binding_pt)
    {}

    virtual
    ~texture()
    {}

    /*!\fn binding_point
      Returns the binding point (for example
      GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP) that
      specifies the texture type.
     */
    GLenum
    binding_point(void) const
    {
      return m_binding_point;
    }

    virtual
    void
    bind_texture(GLenum texture_unit);

  private:
    GLuint m_texture_name;
    GLenum m_binding_point;
  };

  /*!\typedef element_type_collection
    Conveniance typedef for a collection of handles
    to texture_base objects keyed by texture unit
    to which they are to be bound. The key values
    are of the form GL_TEXTURE0, GL_TEXTURE1, etc.
   */ 
  typedef std::map<GLenum, texture_base::handle> element_type_collection;

  /*!\typedef element_type
     Conveniance typedef for an element
     of a \ref element_type_collection
   */ 
  typedef std::map<GLenum, texture_base::handle>::value_type element_type;

  /*!\fn void add_texture
    Adds a texture to this WRATHTextureChoice.
    \param tex_unit enumeration of which texture unit,
                    thus, \code GL_TEXTURE0 <= tex_unit \endcode and
                    \code tex_unit < GL_TEXTURE0 + glGet<GLint>(GL_MAX_TEXTURE_IMAGE_UNITS)
                    \endcode

    \param ptex texture to add    
   */
  void
  add_texture(GLenum tex_unit, texture_base::handle ptex);

  /*!\fn void add_textures
    Conveniance function to add many texture 
    binds.
    \tparam iterator is an iterator to std::pair<GLenum,texture_base::handle>
    \param begin iterator to first element to add
    \param end iterator to one past last element to add.
   */
  template<typename iterator>
  void
  add_textures(iterator begin, iterator end)
  {
    for(;begin!=end;++begin)
      {
        add_texture(begin->first, begin->second);
      }
  }

  /*!\fn void add
    Equivalent to add_textures(),
    provided for template programming
    conveniance.
    \param begin iterator to first element to add
    \param end iterator to one past last element to add.
   */
  template<typename iterator>
  void
  add(iterator begin, iterator end)
  {
    add_textures(begin, end);
  }

  /*!\fn void remove_texture
    Removes the texture  at the named texture unit.
    \param tex_unit which GL texture unit,
           is GL_TEXTURE0, GL_TEXTURE1, etc.
   */
  void
  remove_texture(GLenum tex_unit);

  /*!\fn int bind_textures
    Binds all textures of this WRATHTextureChoice
    to their texture units (by calling texture_base::bind_texture). 
    Avoids rebinding those textures already bound. 
    Returns the number of textures binded. Calls 
    texture_base::unbind_texture on those
    textures in h that are not in, or not on the same
    texture unit, as this.
    \param h previously bound WRATHTextureChoice
   */
  int
  bind_textures(const const_handle &h) const;

  /*!\fn void unbind_textures
    Calls texture_base::unbind_texture on all textures
    of this WRATHTextureChoice.
   */
  void
  unbind_textures(void) const;

  /*!\fn const element_type_collection& elements
    Returns the texture bindings encoded by this,
    keyed by texture unit with value as
    handles to a texture_base object.
   */
  const element_type_collection&
  elements(void) const
  {
    return m_values;
  }

  /*!\fn bool different
    Returns true if the contents of
    two WRATHTextureChoice differ from
    each other.
    \param v0 handle to a WRATHTextureChoice.
    \param v1 handle to a WRATHTextureChoice.
   */
  static
  bool
  different(const WRATHTextureChoice::const_handle &v0,
            const WRATHTextureChoice::const_handle &v1);

  /*!\fn bool compare
    Comparison function for two WRATHTextureChoice
    objects. Invalid handles are sorted first,
    and otherwise sorted by contents of the objects
    (i.e. elements()).
    \param lhs handle to left hand side of comparison op
    \param rhs handle to right hand side of comparison op
   */ 
  static
  bool
  compare(const WRATHTextureChoice::const_handle &lhs,
          const WRATHTextureChoice::const_handle &rhs);

private:
  std::map<GLenum, texture_base::handle> m_values;
};

/*! @} */
#endif
