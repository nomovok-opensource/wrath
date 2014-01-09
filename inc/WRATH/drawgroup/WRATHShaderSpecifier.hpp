/*! 
 * \file WRATHShaderSpecifier.hpp
 * \brief file WRATHShaderSpecifier.hpp
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




#ifndef __WRATH_SHADER_SPECIFIER_HPP__
#define __WRATH_SHADER_SPECIFIER_HPP__

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include <typeinfo>
#include "WRATHGLProgram.hpp"
#include "WRATHItemDrawer.hpp"
#include "WRATHResourceManager.hpp"
#include "WRATHAttributePacker.hpp"
#include "WRATHItemDrawerFactory.hpp"
#include "WRATHTextureFontDrawer.hpp"
#include "WRATHBufferBindingPoint.hpp"
#include "WRATHBaseSource.hpp"

/*! \addtogroup Group
 * @{
 */


/*!\class WRATHShaderSpecifier
  A WRATHShaderSpecifier provides an interface
  for a user of WRATH to specify shader source
  code. A WRATHShaderSpecifier will then assemble
  shader source code to send to GL comprising of:
  - user shader source code
  - source code to fetch node values
  - source code to provide transformation functions

  In contrast to "vanilla GLSL" shaders, user
  source code does NOT have a main(), instead
  it is to have a shader_main(). The main created
  by a WRATHShaderSpecifier calls the needed
  initialization code required for node fetching
  and transformation code to work.

  For the conventions that user shader source
  code should follow, see WRATHItemDrawerFactory::generate_drawer().
 */
class WRATHShaderSpecifier:boost::noncopyable
{
public:

  /*!\typedef ResourceKey
    Resource key type for WRATHAttributePacker 
    resource manager.
   */
  typedef std::string ResourceKey;

  /*!\class ReservedBindings
    A ReservedBindings specifies those binding points 
    that are taken into use by a WRATHShaderSpecifier.
    These binding points cannot be used by a
    WRATHItemDrawerFactory. For example if a
    WRATHItemDrawerFactory dictates to use
    a texture it must not use any of the texture
    units named in the field \ref m_texture_binding_points
   */
  class ReservedBindings
  {
  public:
    /*!\fn ReservedBindings& add_texture_binding
      Reserve a texture unit, i.e. adds
      to \ref m_texture_binding_points.
      \param v texture unit to reserve
     */
    ReservedBindings&
    add_texture_binding(GLenum v)
    {
      m_texture_binding_points.insert(v);
      return *this;
    }

    /*!\fn ReservedBindings& add_buffer_binding(WRATHBufferBindingPoint)
      Reserve a buffer binding point, i.e. adds
      to \ref m_buffer_binding_points.
      \param v buffer binding point to reserve
     */
    ReservedBindings&
    add_buffer_binding(WRATHBufferBindingPoint v)
    {
      m_buffer_binding_points.insert(v);
      return *this;
    }

    /*!\fn ReservedBindings& add_buffer_binding(GLenum, int)
      Provided as a conveninace, equivalent to
      \code
      add_buffer_binding(WRATHBufferBindingPoint(v, idx));
      \endcode
     */
    ReservedBindings&
    add_buffer_binding(GLenum v, int idx)
    {
      return add_buffer_binding(WRATHBufferBindingPoint(v, idx));
    }

    /*!\fn ReservedBindings& absorb
      Add all entries of another ReservedBindings into this.
      \param obj ReservedBindings from which to copy
    */
    ReservedBindings&
    absorb(const ReservedBindings &obj);

    /*!\var m_texture_binding_points
      Specifies the texture units occupied
      by the WRATHShaderSpecifier
     */
    std::set<GLenum> m_texture_binding_points;
    
    /*!\var m_buffer_binding_points
      Specifies the resered buffer binding points.
     */
    std::set<WRATHBufferBindingPoint> m_buffer_binding_points;
  };

  /*!\class Initializer
    Inspirit, a \ref WRATHGLProgramInitializerArray
    and a \ref ReservedBindings with the addition that
    add_sampler_initializer() affects both
    the \ref ReservedBindings and \ref 
    WRATHGLProgramInitializerArray
    object.
   */
  class Initializer
  {
  public:
    /*!\fn Initializer& absorb
      Absorb the enries of another Initializer
      into this.                                
      \param obj Initializer from which to absorb
     */
    Initializer&
    absorb(const Initializer &obj)
    {
      m_initializers.absorb(obj.m_initializers);
      m_bindings.absorb(obj.m_bindings);
      return *this;
    }
    
    /*!\fn Initializer& add(const WRATHGLProgramInitializer::const_handle &)
      Add a WRATHGLProgramInitializer to \ref m_initializers
      \param h handle WRATHGLProgramInitializer to add
     */
    Initializer&
    add(const WRATHGLProgramInitializer::const_handle &h)
    {
      m_initializers.add(h);
      return *this;
    }
    
    /*!\fn Initializer& add(const std::string &, const T &)
      Add a constant uniform initilazation to 
      \ref m_initializers.
      Equivalent to
      \code
      m_initializers.add(uniform_name, value);
      \endcode
      \param uniform_name name of uniform in GLSL code
      \param value value to assigned to uniform
     */ 
    template<typename T>
    Initializer&
    add(const std::string &uniform_name, const T &value)
    {
      m_initializers.add(uniform_name, value);
      return *this;
    }

    /*!\fn Initializer& add_sampler_initializer
      Add a sampler initializer and reserve the texture unit
      to be used by the sampler. Equivalent to:
      \code
      m_initializers.add_sampler_initializer(uniform_name, value);
      m_bindings.add_texture_binding(GL_TEXTURE0+value);
      \endcode
      \param uniform_name name of the sampler in GLSL code
      \param value texture unit value, i.e. 0, 1, 2, ...
     */
    Initializer&
    add_sampler_initializer(const std::string &uniform_name, int value)
    {
      m_initializers.add_sampler_initializer(uniform_name, value);
      m_bindings.add_texture_binding(GL_TEXTURE0+value);
      return *this;
    }

    /*!\var m_initializers
      WRATHGLProgramInitializerArray passed to the ctor
      of WRATHGLProgram on creating a WRATHGLProgram
     */
    WRATHGLProgramInitializerArray m_initializers;

    /*!\var m_bindings
      Set of bindings reserved by the user shader source
      code.
     */   
    ReservedBindings m_bindings;
  };
  
  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHShaderSpecifier, ResourceKey);
  /// @endcond

  /*!\fn WRATHShaderSpecifier(const ResourceKey &pname,
                              const WRATHGLShader::shader_source &vs=WRATHGLShader::shader_source(),
                              const WRATHGLShader::shader_source &fs=WRATHGLShader::shader_source(),
                              const Initializer &initers=Initializer(),
                              const WRATHGLProgramOnBindActionArray &on_bind_actions=WRATHGLProgramOnBindActionArray())
    Ctor.
    \param pname resource name of the WRATHShaderSpecifier
    \param vs initial value to give the vertex shader source (see \ref vertex_shader_source())
    \param fs initial value to give the fragment shader source (see \ref fragment_shader_source())
    \param initers initial value to give to the initializers (see \ref initializers())
    \param on_bind_actions list of actions called each time drawer made by this are put into use
   */
  explicit
  WRATHShaderSpecifier(const ResourceKey &pname,
                       const WRATHGLShader::shader_source &vs=WRATHGLShader::shader_source(),
                       const WRATHGLShader::shader_source &fs=WRATHGLShader::shader_source(),
                       const Initializer &initers=Initializer(),
                       const WRATHGLProgramOnBindActionArray &on_bind_actions=WRATHGLProgramOnBindActionArray());


  /*!\fn WRATHShaderSpecifier(const WRATHGLShader::shader_source &vs=WRATHGLShader::shader_source(),
                              const WRATHGLShader::shader_source &fs=WRATHGLShader::shader_source(),
			      const Initializer &initers=Initializer(),
			      const WRATHGLProgramOnBindActionArray &on_bind_actions=WRATHGLProgramOnBindActionArray())
    Ctor. Will not be tracked by the resource manager of WRATHShaderSpecifier.
    \param vs initial value to give the vertex shader source (see \ref vertex_shader_source())
    \param fs initial value to give the fragment shader source (see \ref fragment_shader_source())
    \param initers initial value to give to the initializers (see \ref initializers())
    \param on_bind_actions list of actions called each time drawer made by this are put into use
   */
  explicit
  WRATHShaderSpecifier(const WRATHGLShader::shader_source &vs=WRATHGLShader::shader_source(),
                       const WRATHGLShader::shader_source &fs=WRATHGLShader::shader_source(),
                       const Initializer &initers=Initializer(),
                       const WRATHGLProgramOnBindActionArray &on_bind_actions=WRATHGLProgramOnBindActionArray());

  ~WRATHShaderSpecifier();

  /*!\fn const ResourceKey& resource_name
    returns the resource name of this WRATHShaderSpecifier.
   */
  const ResourceKey&
  resource_name(void) const
  {
    return m_resource_name;
  }

  /*!\fn WRATHGLProgramOnBindActionArray& append_bind_actions
    Returns a reference to the WRATHGLProgramOnBindActionArray
    object of this WRATHShaderSpecifier. Modify the returned
    object to specify actions to be executed on the _each_ 
    time a GLSL program created with this WRATHShaderSpecifier 
    is bound. It is an error to add (or remove) bind actions
    after the first call \ref fetch_drawer().
   */
  WRATHGLProgramOnBindActionArray&
  append_bind_actions(void)
  {
    WRATHassert(m_modifiable);
    return m_bind_actions;
  }

  /*!\fn const WRATHGLProgramOnBindActionArray& bind_actions
    Returns a const reference of the bind actions of this
    WRATHShaderSpecifier. 
   */
  const WRATHGLProgramOnBindActionArray&
  bind_actions(void) const
  {
    return m_bind_actions;
  }

  /*!\fn WRATHGLProgramInitializerArray& append_initializers
    Returns a reference to the WRATHGLProgramInitializerArray
    object of this WRATHShaderSpecifier. Modify the returned
    object to specify actions (typically setting of uniforms)
    to be executed on the _first_ time a GLSL program created
    with this WRATHShaderSpecifier is bound. It is an error to 
    add (or remove) initializers after the first call \ref 
    fetch_drawer().
   */
  WRATHGLProgramInitializerArray&
  append_initializers(void)
  {
    WRATHassert(m_modifiable);
    return m_initializers;
  }

  /*!\fn const WRATHGLProgramInitializerArray& initializers
    Returns a const reference of the initializes of this
    WRATHShaderSpecifier. 
   */
  const WRATHGLProgramInitializerArray&
  initializers(void) const
  {
    return m_initializers;
  }

  /*!\fn ReservedBindings& append_bindings
    Returns a reference to the ReservedBindings
    object of this WRATHShaderSpecifier. Modify the returned
    object to add bindings to be executed on the _first_ 
    time a GLSL program created with this WRATHShaderSpecifier 
    is bound. It is an error to add (or remove) bindings after 
    the first call \ref fetch_drawer().
   */
  ReservedBindings&
  append_bindings(void) 
  {
    WRATHassert(m_modifiable);
    return m_bindings;
  }

  /*!\fn const ReservedBindings& bindings
    Returns a const reference of the bindings of this
    WRATHShaderSpecifier. 
   */
  const ReservedBindings&
  bindings(void) const
  {
    return m_bindings;
  }

  /*!\fn WRATHGLShader::shader_source& append_shader_source
    Returns a reference for the shader source
    code object for the named shader type.
    Modify the returned object to specify the
    shader source code for the named shader type.
    It is an error to add (or remove) source code 
    after the first call \ref fetch_drawer().
    \param v shader type as a GLenum, for example 
             GL_VERTEX_SHADER for a vertex shader
             and GL_FRAGMENT_SHADER for a fragment
             shader
   */
  WRATHGLShader::shader_source&
  append_shader_source(GLenum v)
  {
    WRATHassert(m_modifiable);
    return m_shader_source_code[v];
  }

  /*!\fn WRATHGLShader::shader_source& append_pre_shader_source
    Returns a reference for the pre-shader source
    code object for the named shader type.
    Modify the returned object to specify the
    shader source code for the named shader type.
    It is an error to add (or remove) source code 
    after the first call \ref fetch_drawer()
    or the first call to \ref fetch_two_pass_drawer().
    \param v shader type as a GLenum, for example 
             GL_VERTEX_SHADER for a vertex shader
             and GL_FRAGMENT_SHADER for a fragment
             shader
   */
  WRATHGLShader::shader_source&
  append_pre_shader_source(GLenum v)
  {
    WRATHassert(m_modifiable);
    return m_pre_shader_source_code[v];
  }
  

  /*!\fn const WRATHGLShader::shader_source& shader_source
    Returns a const refernce to the shader
    source code for the named shader type.
    If the named shader type does not exist,
    returns a const reference to a shader
    source code object that is empty.
    \param v shader type as a GLenum, for example 
             GL_VERTEX_SHADER for a vertex shader
             and GL_FRAGMENT_SHADER for a fragment
             shader
   */
  const WRATHGLShader::shader_source&
  shader_source(GLenum v) const
  {
    std::map<GLenum, WRATHGLShader::shader_source>::const_iterator iter;

    iter=m_shader_source_code.find(v);
    return (iter!=m_shader_source_code.end())?
      iter->second:
      m_empty_source;
  }

  /*!\fn const WRATHGLShader::shader_source& pre_shader_source
    Returns a const refernce to the pre-shader
    source code for the named shader type.
    If the named shader type does not exist,
    returns a const reference to a shader
    source code object that is empty.
    \param v shader type as a GLenum, for example 
             GL_VERTEX_SHADER for a vertex shader
             and GL_FRAGMENT_SHADER for a fragment
             shader
   */
  const WRATHGLShader::shader_source&
  pre_shader_source(GLenum v) const
  {
    std::map<GLenum, WRATHGLShader::shader_source>::const_iterator iter;

    iter=m_pre_shader_source_code.find(v);
    return (iter!=m_pre_shader_source_code.end())?
      iter->second:
      m_empty_source;
  }

  /*!\fn const std::map<GLenum, WRATHGLShader::shader_source>& all_shader_sources
    Returns a const reference of all the
    shader source code object of this
    WRATHShaderSpecifier as an std::map
    with values WRATHGLShader::shader_source
    obects keyed by shader type.
   */
  const std::map<GLenum, WRATHGLShader::shader_source>&
  all_shader_sources(void) const
  {
    return m_shader_source_code;
  }

  /*!\fn const std::map<GLenum, WRATHGLShader::shader_source>& all_pre_shader_sources
    Returns a const reference of all the
    pre-shader source code object of this
    WRATHShaderSpecifier as an std::map
    with values WRATHGLShader::shader_source
    obects keyed by shader type.
   */
  const std::map<GLenum, WRATHGLShader::shader_source>&
  all_pre_shader_sources(void) const
  {
    return m_pre_shader_source_code;
  }

  /*!\fn std::map<GLenum, WRATHGLShader::shader_source>& append_all_shader_sources
    Returns a reference of all the
    shader source code object of this
    WRATHShaderSpecifier as an std::map
    with values WRATHGLShader::shader_source
    obects keyed by shader type.
   */
  std::map<GLenum, WRATHGLShader::shader_source>&
  append_all_shader_sources(void) 
  {
    WRATHassert(m_modifiable);
    return m_shader_source_code;
  }

  /*!\fn std::map<GLenum, WRATHGLShader::shader_source>& append_all_pre_shader_sources
    Returns a reference of all the
    pre-shader source code object of this
    WRATHShaderSpecifier as an std::map
    with values WRATHGLShader::shader_source
    obects keyed by shader type.
   */
  std::map<GLenum, WRATHGLShader::shader_source>&
  append_all_pre_shader_sources(void) 
  {
    WRATHassert(m_modifiable);
    return m_pre_shader_source_code;
  }

  /*!\fn void add_shader_source_code
    Add the shader source code from a WRATHBaseSource
    object. 
    \param src source code to add
    \param prec precision qaulifier to use on the added source code
    \param suffix suffix to which to append to all function, macros, etc
                  added to the code of src
   */
  void
  add_shader_source_code(const WRATHBaseSource *src,
                         enum WRATHBaseSource::precision_t prec,
                         const std::string &suffix="");
                                   
  /*!\fn WRATHGLShader::shader_source& append_vertex_shader_source
    Provided as a convenience, equivalent
    to calling append_shader_source(GLenum) passing
    GL_VERTEX_SHADER as the argument
   */
  WRATHGLShader::shader_source&
  append_vertex_shader_source(void)
  {
    return append_shader_source(GL_VERTEX_SHADER);
  }

  /*!\fn const WRATHGLShader::shader_source& vertex_shader_source
    Provided as a convenience, equivalent
    to calling shader_source(GLenum) const
    passing GL_VERTEX_SHADER as the argument
   */
  const WRATHGLShader::shader_source&
  vertex_shader_source(void) const
  {
    return shader_source(GL_VERTEX_SHADER);
  }

  /*!\fn WRATHGLShader::shader_source& append_pre_vertex_shader_source
    Provided as a convenience, equivalent
    to calling append_pre_shader_source(GLenum) 
    passing GL_VERTEX_SHADER as the argument
   */
  WRATHGLShader::shader_source&
  append_pre_vertex_shader_source(void)
  {
    return append_pre_shader_source(GL_VERTEX_SHADER);
  }

  /*!\fn const WRATHGLShader::shader_source& vertex_pre_shader_source
    Provided as a convenience, equivalent
    to calling pre_shader_source(GLenum) const
    passing GL_VERTEX_SHADER as the argument
   */
  const WRATHGLShader::shader_source&
  vertex_pre_shader_source(void) const
  {
    return pre_shader_source(GL_VERTEX_SHADER);
  }

  /*!\fn WRATHGLShader::shader_source& append_fragment_shader_source
    Provided as a convenience, equivalent
    to calling append_shader_source(GLenum) passing
    GL_FRAGMENT_SHADER as the argument
   */
  WRATHGLShader::shader_source&
  append_fragment_shader_source(void)
  {
    return append_shader_source(GL_FRAGMENT_SHADER);
  }

  /*!\fn const WRATHGLShader::shader_source& fragment_shader_source
    Provided as a convenience, equivalent
    to calling shader_source(GLenum) const
    passing GL_FRAGMENT_SHADER as the argument
   */
  const WRATHGLShader::shader_source&
  fragment_shader_source(void) const
  {
    return shader_source(GL_FRAGMENT_SHADER);
  }

  /*!\fn WRATHGLShader::shader_source& append_pre_fragment_shader_source
    Provided as a convenience, equivalent
    to calling append_pre_shader_source(GLenum)
    passing GL_FRAGMENT_SHADER as the argument
   */
  WRATHGLShader::shader_source&
  append_pre_fragment_shader_source(void)
  {
    return append_pre_shader_source(GL_FRAGMENT_SHADER);
  }

  /*!\fn const WRATHGLShader::shader_source& fragment_pre_shader_source
    Provided as a convenience, equivalent
    to calling pre_shader_source(GLenum) const
    passing GL_FRAGMENT_SHADER as the argument
   */
  const WRATHGLShader::shader_source&
  fragment_pre_shader_source(void) const
  {
    return pre_shader_source(GL_FRAGMENT_SHADER);
  }

  /*!\fn float translucent_threshold(void) const
    Returns the threshold used for two pass
    shaders for opacity. Fragments with a smaller
    opacity that the threshold are discarded
    in the opaque pass and drawn in the 
    transparent pass. Default value is 0.9
   */
  float 
  translucent_threshold(void) const
  {
    return m_translucent_threshold;
  }

  /*!\fn void translucent_threshold(float) 
    Sets the threshold used for two pass
    shaders for opacity. Fragments with a smaller
    opacity that the threshold are discarded
    in the opaque pass and drawn in the 
    transparent pass. Default value is 0.9.
    It is an error to change this value 
    after the first call \ref fetch_drawer()
    or the first call to \ref fetch_two_pass_drawer().
   */
  void
  translucent_threshold(float v)
  {
    WRATHassert(m_modifiable);
    m_translucent_threshold=v;
  }
  
  /*!\fn WRATHItemDrawer* fetch_drawer
    Fetch (and if necessary first create) a WRATHItemDrawer. 
    The determination on weather or not a WRATHItemDrawer
    object has already been generated is done with the key 
    triple(typeid(factory), WRATHAttributePacker*, int).
    I.e. If two different WRATHItemDrawerFactory objects
    implement \ref WRATHItemDrawerFactory::generate_drawer(),
    then they MUST be different types. The actual shader source
    code is generated by concatenating the source code from the
    this WRATHShaderSpecifier with the source code from the
    WRATHItemDrawerFactory in the following order:
    - 1) pre_source_code of the WRATHShaderSpecifier (i.e. \ref pre_shader_source() which is set by append_pre_shader_source())
    - 2) source code of the WRATHItemDrawerFactory
    - 3) source code of the WRATHShaderSpecifier (i.e. \ref shader_source() which is set by append_shader_source())
    \param factory WRATHItemDrawerFactory derived object to  generate a WRATHItemDrawer
    \param attribute_packer WRATHAttributePacker that specifies the attribute names
                            (and locations) of all explicit attributes
    \param sub_drawer_id sub-drawer ID used by the WRATHItemDrawerFactory factory 
   */
  WRATHItemDrawer*
  fetch_drawer(const WRATHItemDrawerFactory &factory,
               const WRATHAttributePacker *attribute_packer,
               int sub_drawer_id) const;


  /*!\fn T* fetch_two_pass_drawer
    Using the shader source of this WRATHShaderSpecifier,
    generate an object of type T where T is derived from
    \ref WRATHTwoPassDrawer and has a contructor with the
    signature:
    \code
    T(WRATHItemDrawer *popaque_drawer,
      WRATHItemDrawer *ptranslucent_drawer,
      WRATHItemDrawer *ptranslucent_drawer_standalone);
    \endcode
    
    Recall that a WRATHTwoPassDrawer has _3_ WRATHItemDrawer objects:
    - WRATHTextureFontDrawer::opaque_pass_drawer() drawer 
      for opaque pass for drawing solid text
    - WRATHTextureFontDrawer::translucent_pass_drawer() drawer 
      for translucent pass for drawing the AA portions of solid text
    - WRATHTextureFontDrawer::translucent_only_drawer() drawer
      for text that is purely transparent

    Each of these drawers has an additional macro added
    to all shaders dependent on the WRATHItemDrawer:
    - WRATH_IS_OPAQUE_PASS for the opaque pass of a solid object
    - WRATH_IS_TRANSLUCENT_PASS for the translucent pass for AA-portions of a solid object
    - WRATH_IS_PURE_TRANSLUCENT_PASS for text the drawer to draw a purely transparent object
    - WRATH_TRANSLUCENT_THRESHOLD is added for all pass types, with value translucent_threshold()
    
    The vertex shader needs to match up with the passed
    WRATHAttributePacker (as usual).

    \tparam T WRATHTwoPassDrawer derived type
    \param factory WRATHItemDrawerFactory derived object to
                   generate a WRATHItemDrawer
    \param attribute_packer  WRATHAttributePacker that specifies the attribute names
                            (and locations) of all explicit attributes
    \param sub_drawer_id sub-drawer ID used by the WRATHItemDrawerFactory factory 
    \param include_transparent_pass if true, two pass drawer will have a transparent
                                    pass, if false does not and as such then draws objects
                                    in one pass
   */
  template<typename T>
  T*
  fetch_two_pass_drawer(const WRATHItemDrawerFactory &factory,
                        const WRATHAttributePacker *attribute_packer,
                        int sub_drawer_id,
                        bool include_transparent_pass=true) const;

  /*!\fn const WRATHShaderSpecifier& fetch_sub_shader
    Returns the WRATHShaderSpecifier associated to a
    particular drawing pass named by a 
    \ref WRATHTwoPassDrawer::drawing_pass_type. These
    WRATHShaderSpecifier objects are the objects
    used to generate the WRATHItemDrawer
    objects used within the WRATHTwoPassDrawer 
    objects returned by fetch_two_pass_drawer().
    Calling \ref fetch_sub_shader(), \ref fetch_two_pass_drawer()
    or \ref fetch_two_pass_drawer() on a WRATHShaderSpecifier
    returned by fetch_sub_shader() returns the value
    as if it was called on the original WRATHShaderSpecifier
    object. 
    \param tp two pass phase 
   */
  const WRATHShaderSpecifier&
  fetch_sub_shader(enum WRATHTwoPassDrawer::drawing_pass_type tp) const;

  /*!\fn bool is_sub_shader
    Returns true if and only if this WRATHShaderSpecifier
    is a sub shader (see \ref fetch_sub_shader()) of a
    WRATHShaderSpecifier. Sub-shader objects are owned
    by the WRATHShaderSpecifier of which they are a sub-shader.
   */
  bool
  is_sub_shader(void) const
  {
    return m_master!=this;
  }

private:

  WRATHShaderSpecifier(const std::string &macro,
                       const WRATHShaderSpecifier*);

  class key_type
  {
  public:
    key_type(const std::type_info &tp,
             const WRATHAttributePacker *pk,
             int pid):
      m_item_group_drawer_type(tp),
      m_attribute_names(pk->all_attribute_names()),
      m_sub_drawer_id(pid)
    {}

    bool
    operator<(const key_type &rhs) const;

  private:
    const std::type_info &m_item_group_drawer_type;
    std::vector<std::string> m_attribute_names;
    int m_sub_drawer_id;
  };

  class multi_pass_key_type
  {
  public:
    multi_pass_key_type(bool has_transparent_pass,
                        const std::type_info &mtp,
                        const std::type_info &tp,
                        const WRATHAttributePacker *pk,
                        int pid):
      m_key(tp, pk, pid),
      m_has_transparent_pass(has_transparent_pass),
      m_multi_draw_type(mtp)
    {}

    bool
    operator<(const multi_pass_key_type &rhs) const;

  private:
    key_type m_key;
    bool m_has_transparent_pass;
    const std::type_info &m_multi_draw_type;
  };

  typedef std::pair<WRATHItemDrawer*, boost::signals2::connection> per_item_drawer;
  typedef std::map<key_type, per_item_drawer> item_drawer_map;

  typedef std::pair<WRATHTwoPassDrawer*, boost::signals2::connection> per_two_pass_drawer;
  typedef std::map<multi_pass_key_type, per_two_pass_drawer> two_pass_drawer_map;

  void
  on_item_draw_dtor(item_drawer_map::iterator iter) const;

  void
  on_two_pass_draw_dtor(two_pass_drawer_map::iterator iter) const;

  void
  ready_sub_shaders(void) const;

  ResourceKey m_resource_name;
  bool m_remove_from_manager;

  std::map<GLenum, WRATHGLShader::shader_source> m_shader_source_code;
  std::map<GLenum, WRATHGLShader::shader_source> m_pre_shader_source_code;

  WRATHGLProgramInitializerArray m_initializers;
  WRATHGLProgramOnBindActionArray m_bind_actions;
  ReservedBindings m_bindings;

  mutable bool m_modifiable;
  float m_translucent_threshold;
  mutable vecN<WRATHShaderSpecifier*, 3> m_sub_shader_specifiers;
  const WRATHShaderSpecifier *m_master;

  WRATHGLShader::shader_source m_empty_source;

  mutable WRATHMutex m_mutex;
  mutable item_drawer_map m_drawers;
  mutable two_pass_drawer_map m_two_pass_drawers;
};


#include "WRATHShaderSpecifierImplement.tcc"

/*! @} */

#endif
