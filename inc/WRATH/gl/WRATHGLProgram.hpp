/*! 
 * \file WRATHGLProgram.hpp
 * \brief file WRATHGLProgram.hpp
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


#ifndef __WRATH_GL_PROGRAM_HPP__
#define __WRATH_GL_PROGRAM_HPP__

#include "WRATHConfig.hpp"
#include <string>
#include <map>
#include <vector>
#include <list>
#include <sstream>
#include <typeinfo>
#include <iterator>
#include <boost/utility.hpp>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include <stdint.h>
#include "vectorGL.hpp"
#include "WRATHgl.hpp"
#include "WRATHgluniform.hpp"
#include "WRATHNew.hpp"
#include "WRATHResourceManager.hpp"
#include "WRATHReferenceCountedObject.hpp"
#include "WRATHGPUConfig.hpp"

/*! \addtogroup GLUtility
 * @{
 */

class WRATHGLProgram;

/*!\def WRATH_REMOVE_PRECISION_QUALIFIERS
  If WRATH_REMOVE_PRECISION_QUALIFIERS 
  is defined then WRATHGLShader will prefix 
  all shader source
  code with:\n
  \#define lowp\n
  \#define mediump\n
  \#define highp\n
  thus effectively killing the precsision qualifiers
  in shader. Also note that WRATHGLShader also
  has this behavior for GL desktop builds, i.e.
  when WRATH_GL_VERSION is defined.
 */
//#define WRATH_REMOVE_PRECISION_QUALIFIERS

/*!\def WRATH_TEGRA_PREC_HACK
  If WRATH_TEGRA_PREC_HACK is defined, precision qualifiers are
  defined as empty AND defualt precision qualifiers are given.
  Default for fragment shader stage is mediump and for
  vertex shader stage is highp. This is only needed for
  TEGRA2 models with older drivers where the GLSL implementation
  expected the precision qualifies in the wrong order.
 */

/*!\class WRATHGLShader
  Simple WRATHGLShader utility class,
  providing a simple interface to build
  shader source code from mutliple files,
  resources and strings. In addition
  the actual GL object creation is defferred
  to layer, in doing so, one can create 
  WRATHGLShader objects from outside the 
  main GL thread. Each of the following commands
    - compile_success()
    - compile_log()
    - name()

  triggers the GL commands to compile the shader
  if the shader has not been yet attmpeted to
  compile. Hence one may only call these from
  outside the rendering thread if shader_ready()
  returns true. Moreover, a WRATHGLShader may only
  be delete from the GL rendering thread.

  WRATHGLShader allows for appending
  multiple files and strings into
  one source blob, also supports
  a very, very minimal include
  directive: it just dumbly includes
  the file, not a true preprocessor.
  The file is included only once,
  all other inclusions of the same
  file are ignored. The file is included
  by adding:\n\n

  \code
  @file_to_include@
  \endcode\n\n

  If the shader source is from a file
  (\ref WRATHGLShader::from_file),
  the file will be taken from the current 
  working directory. For string sources
  (\ref WRATHGLShader::from_file)
  and resource sources (\ref WRATHGLShader::from_resource),
  the shader source is taken from 
  a resource.

  the file will be taken from the current 
  working directory for string sources and 
  from  the directory of the include for file
  sources.

  WRATH will also add the define 
  \code
  #define WRATH_XXX
  \endcode

  where XXX is the name of the shader of 
  the shader stage as given by 
  \ref gl_shader_type_label(), i.e.
  a vertex shader will have the symbol
  WRATH_GL_VERTEX_SHADER defined.

  The values returned by various functions
  in \ref WRATHGPUConfig, and macros 
  \ref WRATH_REMOVE_PRECISION_QUALIFIERS 
  and \ref WRATH_TEGRA_PREC_HACK affect how
  a WRATHGLShader construct shader source
  code.
*/
class WRATHGLShader:public boost::noncopyable
{
public:

  /*!\enum shader_source_type
    Enumeration to indiciate
    the source for a shader.
   */
  enum shader_source_type
    {
      /*!
        Shader source code is taken
        from the file whose name
        is the passed string.
       */
      from_file,

      /*!
        The passed string is the
        shader source code.
       */
      from_string,
      /*!
        The passed string is label
        for a string of text stored
        in \ref WRATHShaderSourceResource system.
       */
      from_resource,
    };
  
  /*!\enum add_source_location_type
    Enumeration to determine if source
    code or a macro 
   */
  enum add_source_location_type
    {
      /*!
        add the source code or macro
        to the back.
       */
      push_back,
      /*!
        add the source code or macro
        to the front.
       */
      push_front
    };

  /*!\enum shader_extension_enable_type
    Enumeration to indicate extension
    enable flags.
   */
  enum shader_extension_enable_type
    {
      /*!
        Requires the named GLSL extension,
        i.e. will add \#extension extension_name: require
        to GLSL source code.
       */
      require_extension,
      /*!
        Enables the named GLSL extension,
        i.e. will add \#extension extension_name: enable
        to GLSL source code.
       */
      enable_extension,
      /*!
        Enables the named GLSL extension,
        but request that the GLSL compiler
        issues warning when the extension
        is used, i.e. will add
        \#extension extension_name: warn
        to GLSL source code.
       */
      warn_extension,
      /*!
        Disables the named GLSL extension,
        i.e. will add \#extension extension_name: disable
        to GLSL source code.
       */
      disable_extension
    };
    
  /*!\class shader_source
    A shader_source represents the source code
    to a GLSL shader, specifying sets of source
    code and macros to use.
   */
  class shader_source
  {
  public:
    /*!\typedef source_code_type
      Conveniance typedef for shader source code,
      .first gives a string which is either a filename
      or shader source code as a string, the interpretation
      of .first is determined by the value of .second.
     */
    typedef std::pair<std::string, enum shader_source_type> source_code_type;
    
    shader_source(void):
      m_wrath_FragColor(true),
      m_force_highp(false),
      m_version(WRATHGPUConfig::default_shader_version())
    {}

    /*!\var m_values
      List of shader source codes of this
      shader_source.
     */
    std::list<source_code_type> m_values;

    /*!\var m_extensions
      Map of extensions, keyed by the name of an
      extension with values of \ref shader_extension_enable_type.
     */
    std::map<std::string, enum shader_extension_enable_type> m_extensions;

    /*!\var m_wrath_FragColor
      If the shader source is for a Fragment shader,
      then if the value is true, then assempled source
      code will insert the symbol <b>wrath_FragColor</b>
      as follows:
      - if \ref WRATH_GL_GLES_VERSION is 2, then 
        <b>wrath_FragColor</b> is realized as a macro to
        <b>gl_FragColor</b>
      - if \ref WRATH_GL_GLES_VERSION is 3 or higher,
        then <b>wrath_FragColor</b> is declared
        as a <b>out mediump vec4</b>.
      The purpose of the macro is to allow for an
      application to more easily use GL3 core profile
      GLSL and GLES3 GLSL. Default value is true.
     */
    bool m_wrath_FragColor;

    /*!\var m_force_highp
      Only has affect for GLES2 shaders.
      If true adds defines so that mediump and lowp
      become highp.
     */
    bool m_force_highp;

    /*!\var m_version
      Specifies the version of GLSL to which to
      declare the shader. An empty string indicates
      to not have a "#version" directive in the shader.
      Default value is given by
      WRATHGPUConfig::default_shader_version().
     */
    std::string m_version;

    /*!\fn force_highp
      Sets m_force_highp.
      \param v value to which to set m_force_highp
     */
    shader_source&
    force_highp(bool v)
    {
      m_force_highp=v;
      return *this;
    }

    /*!\fn wrath_FragColor
      Sets m_wrath_FragColor.
      \param v value to which to set m_wrath_FragColor 
     */
    shader_source&
    wrath_FragColor(bool v)
    {
      m_wrath_FragColor=v;
      return *this;
    }

    /*!\fn shader_source& add_source
      Add shader source code to this shader_source.
      \param str string, either a file name or GLSL source
      \param tp interpretation of str, i.e. determines if
                str is a filename or raw GLSL source code.
      \param loc location to add source
     */
    shader_source&
    add_source(const std::string &str, enum shader_source_type tp=from_file,
               enum add_source_location_type loc=push_back)
    {
      if(!str.empty())
        {
          if(loc==push_front)
            {
              m_values.push_front( source_code_type(str, tp));
            }
          else
            {
              m_values.push_back( source_code_type(str, tp));
            }
        }
      return *this;
    }

    /*!\fn shader_source& add_macro(const std::string&, const std::string&, enum add_source_location_type)
      Add a macro to this shader_source.      
      Functionally, will insert \#define macro_name macro_value
      in the GLSL source code.
      \param macro_name name of macro
      \param macro_value value to which macro is given
      \param loc location to add macro within code
     */
    shader_source&
    add_macro(const std::string &macro_name, const std::string &macro_value="",
              enum add_source_location_type loc=push_back)
    {
      std::ostringstream ostr;

      if(!macro_name.empty())
        {
          ostr << "\n#define " << macro_name << " " << macro_value << "\n";

          if(loc==push_front)
            {
              m_values.push_front(source_code_type(ostr.str(), from_string) );
            }
          else
            {
              m_values.push_back(source_code_type(ostr.str(), from_string) );
            }
        }
      return *this;
    }

    /*!\fn shader_source& add_macro(const std::string&, const T&, enum add_source_location_type)
      Template version of add a macro to this shader_source.
      Macros and source code are placed in the order they
      are added.
      Functionally, will insert \#define macro_name macro_value
      in the GLSL source code.

      \param macro_name name of macro
      \param macro_value value to which macro is given.
      \param loc location to add macro within code
     */
    template<typename T>
    shader_source&
    add_macro(const std::string &macro_name, const T &macro_value,
              enum add_source_location_type loc=push_back)
    {
      std::ostringstream ostr;

      if(!macro_name.empty())
        {
          ostr << "\n#define " << macro_name << " " << macro_value << "\n";
          if(loc==push_front)
            {
              m_values.push_front(source_code_type(ostr.str(), from_string) );
            }
          else
            {
              m_values.push_back(source_code_type(ostr.str(), from_string) );
            }
        }
      return *this;
    }

    /*!\fn shader_source& add_macro(const std::pair<S,T>&, enum add_source_location_type)
      Template version of add a macro to this shader_source.
      Macros and source code are placed in the order they
      are added. The type S must be be able to used
      to passed into the constructor of std::string
      implicitely. Equivalent to:
      \code
      add_macro(macro_pair.first, macro_pair.second, loc)
      \endcode
     */
    template<typename S, typename T>
    shader_source&
    add_macro(const std::pair<S,T> &macro_pair,
              enum add_source_location_type loc=push_back)
    {
      return add_macro(macro_pair.first, macro_pair.second, loc);
    } 

    /*!\fn shader_source& add_macros
      Template iterator version for add a sequence of
      macros. 
      \tparam iterator iterator must derefernce
                       into either a type that can be used passed
                       to the constructor std::string implicitely
                       or into an std::pair<S,T> where S can be 
                       can be used passed to the constructor 
                       std::string implicitely.
      \param begin iterator to first macro to add
      \param end iterator one past the last macro to add
      \param loc location to which to add the source code
     */
    template<typename iterator>
    shader_source&
    add_macros(iterator begin, iterator end,
               enum add_source_location_type loc=push_back)
    {
      for(;begin!=end;++begin)
        {
          add_macro(*begin, loc);
        }
      return *this;
    }

    /*!\fn shader_source& remove_macro
      Adds the string
      \code
      #undef X
      \endcode
      where X is the passed macro name
      \param macro_name name of macro
     */
    shader_source&
    remove_macro(const std::string &macro_name)
    {
      if(!macro_name.empty())
        {
          std::ostringstream ostr;
          ostr << "\n#undef " << macro_name << "\n";
          m_values.push_back(source_code_type(ostr.str(), from_string) );
        }
      return *this;
    }

    /*!\fn shader_source& absorb(const shader_source&)
      Absorb all shader source code (source files and macros)
      from another shader_source object. Added sources and
      macros are added at the end and in the order they appear
      in the source shader_source object. Additionally,
      if the field m_force_highp of either this or obj is
      true, then m_force_highp is set as true. Additionally,
      the value of \ref m_version is also copied from obj
      if obj's \ref m_version value is non-empty.

      \param obj shader_source object from which to absorb
     */
    shader_source&
    absorb(const shader_source &obj);

    /*!\fn shader_source& absorb(iterator, iterator)
      Absorb a range of other shader source objects.
      \tparam iterator must dereference to a shader_source
      \param begin iterator to 1st shader_source object to absorb
      \param end iterator to one past the last shader_source object to absorb
     */
    template<typename iterator>
    shader_source&
    absorb(iterator begin, iterator end)
    {
      for(;begin!=end; ++begin)
        {
          absorb(*begin);
        }
      return *this;
    }
    
    /*!\fn shader_source& specify_extension
      Specifiy an extension and usage.
      \param ext_name name of GL extension
      \param tp usage of extension
     */
    shader_source&
    specify_extension(const std::string &ext_name, 
                      enum shader_extension_enable_type tp=enable_extension)
    {
      m_extensions[ext_name]=tp;
      return *this;
    }

    /*!\fn shader_source& specify_version
      Sets \ref m_version.
     */
    shader_source&
    specify_version(const std::string &v)
    {
      m_version=v;
      return *this;
    };

    /*!\fn void build_source_code(std::ostream&, GLenum shader_type) const
      Stream the GLSL source of this \ref shader_source
      into an std::ostream. In addition to the
      GLSL source code specified in this \ref shader_source
      object, the following GLSL code will be prefixed:
      - version directive set to \ref m_version if \ref m_version is non-empty
      - if WRATHGPUConfig::old_glsl_texture_functions_deprecated() returns true 
        a set of macros to allow using those symbols too. This is provided so
        that shaders mostly targetted for GLES2 will work in GL3+ core profile
      - The macro shader_in, defined either as in, varying or attribute depending
        on the shader type and the value of WRATHGPUConfig::use_in_out_in_shaders().
        Specifically, if WRATHGPUConfig::use_in_out_in_shaders() is true,
        then shader_in is defined as in and shader_out as out. Otherwise,
        shader_in is defined as attribute for vertex shaders and varying for all other
        stages and shader_out is defined as varying out for fragment shaders and
        as varying for all other stages
      - The macro WRATH_DERIVATIVES_SUPPORTED is defined if the fragment shader
        supports the derivative functions dFdx(), dFdy() and fwidth()
      - A macro of the from WRATH_XXX where XXX is the string return value from
        WRATHGLShader::gl_shader_type_label(shader_type) to identify the shader type
      - The symbols lowp, mediump and highp are defined as empty macros if WRATH
        is targetting desktop OpenGL or if WRATH_REMOVE_PRECISION_QUALIFIERS is
        defined
      - If WRATH is not targetting desktop OpenGL, then lowp and mediump are defined
        as highp if \ref m_force_highp is set to true. Conversely, if \ref m_force_highp 
        is false and if the shader stage is GL_FRAGMENT_SHADER, then will insert
        additional macro code so that highp is defined as mediump if GL_FRAGMENT_PRECISION_HIGH
        is not supported
      - defines for a fragment shader the symbol "wrath_FragColor" if \ref 
        m_wrath_FragColor is true, see \ref m_wrath_FragColor to what it is defined
      - The following macros are defined if the corresponding function returns true:
      -- WRATHGPUConfig::dependent_texture_lookup_requires_LOD() WRATH_GPU_CONFIG_DEPENDENT_TEXTURE_LOOKUP_REQUIRES_LOD
      -- WRATHGPUConfig::fragment_shader_poor_branching() WRATH_GPU_CONFIG_FRAGMENT_SHADER_POOR_BRANCHING
      -- WRATHGPUConfig::fragment_shader_texture_LOD_supported() WRATH_GPU_CONFIG_FRAGMENT_SHADER_TEXTURE_LOD

      \param output_glsl_source_code stream to which to
                                     print the actual
                                     GLSL source code.
      \param shader_type shader type (i.e. GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc)
     */
    void
    build_source_code(std::ostream &output_glsl_source_code, GLenum shader_type) const;

    /*!\fn void build_source_code(std::string&, GLenum) const
      Place the GLSL source code into a string.
      \param output_glsl_source_code string to which to
                                     print the actual
                                     GLSL source code.
      \param shader_type shader type (i.e. GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc)
     */
    void
    build_source_code(std::string &output_glsl_source_code, GLenum shader_type) const
    {
      std::ostringstream ostr;

      build_source_code(ostr, shader_type);
      output_glsl_source_code=ostr.str();
    }
  };

  /*!\class shader_source_collection
    A wrapper over std::map<GLenum, shader_source>
   */
  class shader_source_collection:public std::map<GLenum, shader_source>
  {
  public:
    /*!\fn shader_source_collection& absorb_shader_stage()
      Adds an entry.
      \param shader_stage key of entry
      \param src value of entry
     */ 
    shader_source_collection&
    absorb_shader_stage(GLenum shader_stage, const shader_source &src)
    {
      operator[](shader_stage).absorb(src);
      return *this;
    }
  };

  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHGLShader, std::string);            
  /// @endcond

  /*!\fn WRATHGLShader
    Ctor. Construct a WRATHGLShader.
    \param presource_name resource name of WRATHGLShader
    \param src GLSL source code of the shader
    \param pshader_type type of shader, i.e. GL_VERTEX_SHADER
                        for a vertex shader, etc.
   */
  WRATHGLShader(const std::string &presource_name,
                const shader_source &src, 
                GLenum pshader_type);

  ~WRATHGLShader();

  /*!\fn bool shader_ready(void)
    The actual GL shader is NOT built at constructor,
    rather it is built if any of
    - compile_success()
    - compile_log()
    - name()
    are called. This way, one can construct WRATHGLShader
    objects from outside the GL thread. The functions
    return true if and only if the shader has been built.
   */
  bool
  shader_ready(void)
  {
    return m_shader_ready;
  }

  /*!\fn const std::string& source_code
    Returns the GLSL source string fed to GL
    to create the GLSL shader.
   */
  const std::string&
  source_code(void)
  {
    return m_source_code;
  }

  /*!\fn const std::string& compile_log
    Returns the GLSL compile log of
    the GLSL source code.
    If the shader source has not yet
    been sent to GL for compiling, will
    trigger those commands. Hence, should
    only be called from the GL rendering 
    thread or if shader_ready() returns true.
   */
  const std::string&
  compile_log(void);

  /*!\fn bool compile_success(void) const
    Returns true if and only if GL
    successfully compiled the shader.
    If the shader source has not yet
    been sent to GL for compiling, will
    trigger those commands. Hence, should
    only be called from the GL rendering 
    thread or if shader_ready() returns true.
   */
  bool
  compile_success(void);

  /*!\fn GLuint name
    Returns the GL name (i.e. ID assigned by GL)
    of this WRATHGLShader. 
    If the shader source has not yet
    been sent to GL for compiling, will
    trigger those commands. Hence, should
    only be called from the GL rendering 
    thread or if shader_ready() returns true.
   */
  GLuint
  name(void);

  /*!\fn const std::string& resource_name
    Returns the resource name of this
    WRATHGLShader as set by it's constructor.
   */
  const std::string&
  resource_name(void) 
  {
    return m_resource_name;
  }

  /*!\fn GLenum shader_type
    Returns the shader type of this
    WRATHGLShader as set by it's constructor.
   */
  GLenum 
  shader_type(void) 
  {
    return m_shader_type;
  }

  /*!\fn std::string gl_shader_type_label
    Provided as a conveniance to return a string
    from a GL enumeration naming a shader type.
    For example <B>GL_VERTEX_SHADER</B> will
    return the string GL_VERTEX_SHADER.
    Unreconized shader types will have the label:
    \code
    UNKNOWN_SHADER_STAGE_XXXX
    \endcode
    where XXXX is the enumeration value in hex.
   */
  static
  std::string
  gl_shader_type_label(GLenum shader_type);

  /*!\fn uint32_t gl_shader_bit
    Returns a bit mask with the bit up for the
    specified shader type, for example,
    GL_VERTEX_SHADER will return GL_VERTEX_SHADER_BIT.
    Unreconized shader types will return 0.
   */
  static
  uint32_t
  gl_shader_bit(GLenum shader_type);

private:

  void
  compile(void);

  bool m_shader_ready;
  GLuint m_name;
  GLenum m_shader_type;

  std::string m_resource_name;
  std::string m_source_code;
  std::string m_compile_log;
  bool m_compile_success;
};



/*!\class WRATHGLPreLinkAction
  A WRATHGLPreLinkAction is an action for a WRATHGLProgram
  to perform after attaching shader but before linking.
 */
class WRATHGLPreLinkAction:
  public WRATHReferenceCountedObjectT<WRATHGLPreLinkAction>
{
public:
  
  virtual
  ~WRATHGLPreLinkAction()
  {}

  /*!\fn void action
    To be implemented by a derived class
    to perform an action _before_ the GLSL
    program is linked. Default implementation
    does nothing.
    \param glsl_program WRATHGLProgram on
                        which to perform
                        the action.
   */
  virtual
  void
  action(WRATHGLProgram *glsl_program) const;

  /*!\fn bool post_action
    To be implemented by a derived class
    to perform an action _after_ the GLSL
    program is linked. To return true if
    and only if an error or warning should
    be reported. Default implementation
    does nothing and returns false.
    \param ostr std::ostream to which to stream errors and warnings.
    \param pr WRATHGLProgram
   */
  virtual
  bool
  post_action(std::ostream &ostr, WRATHGLProgram* pr) const;
};


/*!\class WRATHGLBindAttribute
  A WRATHGLBindAttribute inherits from WRATHGLPreLinkAction,
  it's purpose is to bind named attributes to named
  locations.
 */
class WRATHGLBindAttribute:public WRATHGLPreLinkAction
{
public:
  /*!\fn WRATHGLBindAttribute(const std::string &, int)
    Ctor.
    \param pname name of attribute in GLSL code
    \param plocation location to which to place the attribute
   */
  WRATHGLBindAttribute(const std::string &pname, int plocation):
    m_label(pname),
    m_location(plocation)
  {}

  ~WRATHGLBindAttribute()
  {}

  virtual
  void
  action(WRATHGLProgram *glsl_program) const;

  virtual
  bool
  post_action(std::ostream &str, WRATHGLProgram *program) const;

private:
  std::string m_label;
  int m_location;
};


/*!\class WRATHGLPreLinkActionArray
  A WRATHGLPreLinkActionArray is a conveniance class
  wrapper over an array of WRATHGLPreLinkAction handles.
 */
class WRATHGLPreLinkActionArray
{
public:

  /*!\fn WRATHGLPreLinkActionArray(void)
    Ctor, initialize \ref m_values as empty
   */
  WRATHGLPreLinkActionArray(void)
  {}

  /*!\fn WRATHGLPreLinkActionArray(const WRATHGLPreLinkAction::const_handle &h)
    Ctor, initialize \ref m_values with one specified element.
   */
  WRATHGLPreLinkActionArray(const WRATHGLPreLinkAction::const_handle &h):
    m_values(1, h)
  {}

  /*!\fn WRATHGLPreLinkActionArray& add(const WRATHGLPreLinkAction::const_handle&)
    Add a prelink action to \ref m_values.
    \param h handle to action to add
   */
  WRATHGLPreLinkActionArray&
  add(const WRATHGLPreLinkAction::const_handle &h)
  {
    m_values.push_back(h);
    return *this;
  }

  /*!\fn WRATHGLPreLinkActionArray& add_binding
    Provided as a conveniance, equivalent to
    \code
    add(WRATHNew WRATHGLBindAttribute(pname, plocation))
    \endcode
    \param pname name of the attribute
    \param plocation location to which to bind the attribute.
   */
  WRATHGLPreLinkActionArray&
  add_binding(const std::string &pname, int plocation)
  {
    return add(WRATHNew WRATHGLBindAttribute(pname, plocation));
  }

  /*!\fn WRATHGLPreLinkActionArray& absorb
    Add all entries of another into \ref m_values.
    \param obj from which to copy
   */
  WRATHGLPreLinkActionArray&
  absorb(const WRATHGLPreLinkActionArray &obj);

  /*!\fn void execute_actions
    Provided as a conveniance, executes each of those
    actions listed in \ref m_values.
   */
  void
  execute_actions(WRATHGLProgram *glsl_program) const;

  /*!\fn bool execute_post_actions
    Provided as a conveniance, executes each of those
    post actions listed in \ref m_values.
   */
  bool
  execute_post_actions(std::ostream &ostr, WRATHGLProgram *glsl_program) const;

  /*!\var m_values
    Values of the array
   */
  std::vector<WRATHGLPreLinkAction::const_handle> m_values;
};




/*!\class WRATHGLProgramInitializer
  A WRATHGLProgramInitializer is a functor object called the first time
  a WRATHGLProgram is bound (i.e. the first
  time WRATHGLProgram::use_program() is called).
  It's main purpose is to facilitate initializing
  uniform values.
 */
class WRATHGLProgramInitializer:
  public WRATHReferenceCountedObjectT<WRATHGLProgramInitializer>
{
public:
  /*!\fn perform_initialization
    To be implemented by a derived class to
    perform additional one-time actions.
    Function is called the first time the 
    GLSL program of the WRATHGLProgram is 
    bound.
    \param pr WRATHGLProgram to initialize
   */
  virtual
  void
  perform_initialization(WRATHGLProgram *pr) const=0;
};

/*!\class WRATHGLUniformInitializer
  Initialize a uniform via the templated
  overloaded function WRATHglUniform.
 */
template<typename T>
class WRATHGLUniformInitializer:public WRATHGLProgramInitializer
{
public:

  /*!\fn WRATHGLUniformInitializer
    Ctor. 
    \param uniform_name name of uniform in GLSL to initialize
    \param value value with which to set the uniform
   */
  WRATHGLUniformInitializer(const std::string &uniform_name,
                            const T &value):
    m_uniform_name(uniform_name),
    m_value(value)
  {}

  virtual
  void
  perform_initialization(WRATHGLProgram *pr) const;

private:
  std::string m_uniform_name;
  T m_value;
};

/*!\typedef WRATHGLSamplerInitializer
  Conveniance typedef to initialize samplers.
 */
typedef WRATHGLUniformInitializer<int> WRATHGLSamplerInitializer;

/*!\class WRATHGLProgramInitializerArray
  Conveniance class to hold an array of handles
  of WRATHGLProgramInitializer objects
 */
class WRATHGLProgramInitializerArray
{
public:

  /*!\fn add(const WRATHGLProgramInitializer::const_handle&)
    Add an initializer to \ref m_values.
    \param h handle to initializer to add
   */
  WRATHGLProgramInitializerArray&
  add(const WRATHGLProgramInitializer::const_handle &h)
  {
    m_values.push_back(h);
    return *this;
  }

  /*!\fn absorb
    Add all entries of another WRATHGLProgramInitializerArray into \ref m_values.
    \param obj WRATHGLProgramInitializerArray from which to copy
   */
  WRATHGLProgramInitializerArray&
  absorb(const WRATHGLProgramInitializerArray &obj);

  /*!\fn add(const std::string&, const T&)
    Provided as a conveniance, creates
    a WRATHGLUniformInitializer object
    and adds that to \ref m_values.
    \param uniform_name name of uniform in GLSL to initialize
    \param value value with which to set the uniform
   */
  template<typename T>
  WRATHGLProgramInitializerArray&
  add(const std::string &uniform_name, const T &value)
  {
    return add(WRATHNew WRATHGLUniformInitializer<T>(uniform_name, value));
  }

  /*!\fn add_sampler_initializer
    Provided as a conveniance, creates
    a WRATHGLSamplerInitializer object
    and adds that to \ref m_values.
    \param uniform_name name of uniform in GLSL to initialize
    \param value value with which to set the uniform, in this
                 case specifies the texture unit as follows:
                 a value of n means to use GL_TEXTUREn texture
                 unit.
  */
  WRATHGLProgramInitializerArray&
  add_sampler_initializer(const std::string &uniform_name, int value)
  {
    return add(WRATHNew WRATHGLSamplerInitializer(uniform_name, value));
  }

  /*!\var m_values
    Values of the array
   */
  std::vector<WRATHGLProgramInitializer::const_handle> m_values;
};

/*!\class WRATHGLProgramOnBindAction
  A WRATHGLProgramOnBindAction represents an action
  that is performed _everytime_ a WRATHGLProgram
  is bound.
 */
class WRATHGLProgramOnBindAction:
  public WRATHReferenceCountedObjectT<WRATHGLProgramOnBindAction>
{
public:
  
  /*!\fn void perform_action
    To be implemented by a derived class
    to perform an action when the WRATHGLProgram
    is bound.
   */
  virtual
  void
  perform_action(WRATHGLProgram *pr) const=0;
};

/*!\class WRATHGLProgramOnBindActionArray
  A WRATHGLProgramOnBindActionArray is simply an array
  of handles to WRATHGLProgramOnBindAction objects.
 */
class WRATHGLProgramOnBindActionArray
{
public:

  /*!\fn add(const WRATHGLProgramOnBindAction::const_handle&)
    Add a WRATHGLProgramOnBindAction to \ref m_values.
    \param h handle to action to add
   */
  WRATHGLProgramOnBindActionArray&
  add(const WRATHGLProgramOnBindAction::const_handle &h)
  {
    m_values.push_back(h);
    return *this;
  }

  /*!\fn absorb
    Add all entries of another WRATHGLProgramOnBindAction into \ref m_values.
    \param obj WRATHGLProgramOnBindAction from which to copy
   */
  WRATHGLProgramOnBindActionArray&
  absorb(const WRATHGLProgramOnBindActionArray &obj);

  /*!\fn execute_actions
    Provided as a conveniance, executes each of those
    actions listed in \ref m_values.
   */
  void
  execute_actions(WRATHGLProgram *pr) const;

  /*!\var m_values
    Values of the array
   */
  std::vector<WRATHGLProgramOnBindAction::const_handle> m_values;
};


/*!\class WRATHGLProgram
  Class for creating and using GLSL programs.
  A WRATHGLProgram delays the GL commands to
  create the actual GL program until the first time
  it is bound with use_program(). In addition to
  proving the GL code to create the GLSL code,
  WRATHGLProgram also provides queries GL for all
  active uniforms and attributes (see active_attributes(), 
  active_uniforms(), find_uniform() and find_attribute()).
  Also, provides an interface so that a sequence of 
  GL commands are executed the first time it is bound
  and also an interface so a sequence of actions is
  executed every time it is bound.
  WRATHGLProgram's are considered a resource,
  as such have a resource manager. 
 */
class WRATHGLProgram:public boost::noncopyable
{
public:

  /*!\class parameter_info
    A parameter_info holds the type,
    size and name of a uniform or an attribute
    of a GL program. This data is fetched from GL
    via glGetActiveAttrib/glGetAttribLocation
    for attributes and glGetActiveUniform/glGetUniformLocation
    for uniforms. Note that, depending on the GL
    implementation, arrays may or may
    not be listed with an appended '[0]' and
    that usually elements of an array ARE
    not listed individually.
  */
  class parameter_info
  {
  public:
    parameter_info(void):
      m_type(GL_INVALID_ENUM),
      m_count(0),
      m_index(-1),
      m_location(-1)
    {}
    
    /*!\var m_name
      Name of the parameter within
      the GL API.
    */
    std::string m_name;
    
    /*!\var m_type
      GL enumeration stating the
      parameter's type.
    */
    GLenum m_type;
    
    /*!\var m_count
      If parameter is an array, holds
      the legnth of the array, otherwise
      is 1.
    */
    GLint m_count;
    
    /*!\var m_index 
      GL API index for the parameter, NOT the
      ID of the parameter as used in uniform_parameter
      or attribute_parameter. The member m_index
      is used in calls to GL to query about
      the parameter, such as glGetActiveUniform
      and glGetActiveUniformsiv.
    */
    GLuint m_index;

    /*!\var m_location
      "Location" of the uniform or attribute
      as returned by glGetUniformLocation
      or glGetAttriblocation
     */
    GLint m_location;
  };

  /*!\class attribute_uniform_query_result
    An attribute_uniform_query_result is a
    tuple holding a const pointer to a
    parameter_info object and an integer
    holding the GL-location of uniform
    or attribute requested. The purpose
    of attribute_uniform_query_result is
    to allow one to request the location
    of a uniform that is an element of an
    array. For example if GLSL code has
    \code
     vec4 some_uniforms[20]
    \endcode
    then (typically) the table of uniforms
    of attributes (see \ref active_uniforms()),
    does NOT hold a value for each element
    of the array some_uniforms. Rather it
    holds an entry for the first element of the
    array. However, GL enforces that the 
    uniform location of some_uniforms[N]
    is N+locaion of some_uniforms. 
   */
  class attribute_uniform_query_result
  {
  public:
    /*!\fn attribute_uniform_query_result(int plocation, 
                                          const parameter_info *)
      Ctor.
      \param plocation value to which to initialize \ref m_location
      \param pv value to which to initialize \ref m_info
     */
    attribute_uniform_query_result(int plocation, 
                                   const parameter_info *pv):
      m_location(plocation),
      m_info(pv)
    {}

    /*!\fn attribute_uniform_query_result(void)
      Ctor. \ref m_location is initialized as -1 and
      \ref m_info as NULL.
    */
    attribute_uniform_query_result(void):
      m_location(-1),
      m_info(NULL)
    {}

    /*!\var m_location
      The location of the uniform or attribute
      queried. For non-array element this is 
      the same as \code m_info->m_location. 
      \endcode For
      an array element of index N, this
      is \code m_info->m_location+N. \endcode
     */
    GLint m_location;

    /*!\var m_info
      A pointer to the parameter_info object 
     */
    const parameter_info *m_info;
  }; 

  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHGLProgram, std::string);
  /// @endcond

  /*!\fn WRATHGLProgram(const std::string&,
                        const std::vector<WRATHGLShader*>&,
                        const WRATHGLPreLinkActionArray&,
                        const WRATHGLProgramInitializerArray&,
                        const WRATHGLProgramOnBindActionArray&)
  
    Ctor. 
    \param presource_name resource name to assign to the WRATHGLProgram
    \param pshaders shaders used to create the WRATHGLProgram
    \param action specifies actions to perform before linking of the WRATHGLProgram
    \param initers one-time initialization actions to perform the first time the
                   WRATHGLProgram is used
    \param bind_actions array of actions to be executed each time the program is bound
   */
  WRATHGLProgram(const std::string &presource_name,
                 const std::vector<WRATHGLShader*> &pshaders,
                 const WRATHGLPreLinkActionArray &action=WRATHGLPreLinkActionArray(),
                 const WRATHGLProgramInitializerArray &initers=WRATHGLProgramInitializerArray(),
                 const WRATHGLProgramOnBindActionArray &bind_actions=WRATHGLProgramOnBindActionArray()):
    m_shaders(pshaders),
    m_initializers(initers.m_values),
    m_bind_actions(bind_actions)
  {
    pre_assemble(presource_name, action);
  }
  
  /*!\fn WRATHGLProgram(const std::string&,
                        WRATHGLShader*,
                        WRATHGLShader*,
                        const WRATHGLPreLinkActionArray&,
                        const WRATHGLProgramInitializerArray&,
                        const WRATHGLProgramOnBindActionArray&)
  
    Ctor. 
    \param presource_name resource name to assign to the WRATHGLProgram
    \param vert_shader pointer to vertex shader to use for the WRATHGLProgram
    \param frag_shader pointer to fragment shader to use for the WRATHGLProgram
    \param action specifies actions to perform before and 
                  after linking of the WRATHGLProgram.
    \param initers one-time initialization actions to perform the first time the
                   WRATHGLProgram is used
    \param bind_actions array of actions to be executed each time the program is bound
   */
  WRATHGLProgram(const std::string &presource_name,
                 WRATHGLShader *vert_shader,
                 WRATHGLShader *frag_shader,
                 const WRATHGLPreLinkActionArray &action=WRATHGLPreLinkActionArray(),
                 const WRATHGLProgramInitializerArray &initers=WRATHGLProgramInitializerArray(),
                 const WRATHGLProgramOnBindActionArray &bind_actions=WRATHGLProgramOnBindActionArray()):
    m_initializers(initers.m_values),
    m_bind_actions(bind_actions)
  {
    m_shaders.push_back(vert_shader);
    m_shaders.push_back(frag_shader);
    pre_assemble(presource_name, action);
  }

  /*!\fn WRATHGLProgram(const std::string&,
                        const WRATHGLShader::shader_source&,
                        const WRATHGLShader::shader_source&,
                        const WRATHGLPreLinkActionArray&,
                        const WRATHGLProgramInitializerArray&,
                        const WRATHGLProgramOnBindActionArray&)
  
    Ctor. 
    \param presource_name resource name to assign to the WRATHGLProgram
    \param vert_shader shader source of vertex shader to use for the WRATHGLProgram
    \param frag_shader shader source of fragment shader to use for the WRATHGLProgram
    \param action specifies actions to perform before and 
                  after linking of the WRATHGLProgram.
    \param initers one-time initialization actions to perform the first time the
                   WRATHGLProgram is used
    \param bind_actions array of actions to be executed each time the program is bound
   */
  WRATHGLProgram(const std::string &presource_name,
                 const WRATHGLShader::shader_source &vert_shader,
                 const WRATHGLShader::shader_source &frag_shader,
                 const WRATHGLPreLinkActionArray &action=WRATHGLPreLinkActionArray(),
                 const WRATHGLProgramInitializerArray &initers=WRATHGLProgramInitializerArray(),
                 const WRATHGLProgramOnBindActionArray &bind_actions=WRATHGLProgramOnBindActionArray()):
    m_initializers(initers.m_values),
    m_bind_actions(bind_actions)
  {
    m_shaders.push_back(WRATHNew WRATHGLShader(presource_name+".vert", vert_shader, GL_VERTEX_SHADER) );
    m_shaders.push_back(WRATHNew WRATHGLShader(presource_name+".frag", frag_shader, GL_FRAGMENT_SHADER) );
    pre_assemble(presource_name, action);
  }

   /*!\fn WRATHGLProgram(const std::string&,
                         const std::map<GLenum, WRATHGLShader::shader_source>&,
                         const WRATHGLPreLinkActionArray&,
                         const WRATHGLProgramInitializerArray&,
                         const WRATHGLProgramOnBindActionArray&)
  
    Ctor. 
    \param presource_name resource name to assign to the WRATHGLProgram
    \param shaders shader source of each stage to use for the WRATHGLProgram
    \param action specifies actions to perform before and 
                  after linking of the WRATHGLProgram.
    \param initers one-time initialization actions to perform the first time the
                   WRATHGLProgram is used
    \param bind_actions array of actions to be executed each time the program is bound
   */
  WRATHGLProgram(const std::string &presource_name,
                 const std::map<GLenum, WRATHGLShader::shader_source> &shaders,
                 const WRATHGLPreLinkActionArray &action=WRATHGLPreLinkActionArray(),
                 const WRATHGLProgramInitializerArray &initers=WRATHGLProgramInitializerArray(),
                 const WRATHGLProgramOnBindActionArray &bind_actions=WRATHGLProgramOnBindActionArray());

  /*!\fn WRATHGLProgram(const std::string&,
                        const WRATHGLShader::shader_source&,
                        WRATHGLShader*,
                        const WRATHGLPreLinkActionArray&,
                        const WRATHGLProgramInitializerArray&,
                        const WRATHGLProgramOnBindActionArray&)
  
    Ctor. 
    \param presource_name resource name to assign to the WRATHGLProgram
    \param vert_shader shader source of vertex shader to use for the WRATHGLProgram
    \param frag_shader pointer to fragment shader to use for the WRATHGLProgram
    \param action specifies actions to perform before and 
                  after linking of the WRATHGLProgram.
    \param initers one-time initialization actions to perform the first time the
                   WRATHGLProgram is used
    \param bind_actions array of actions to be executed when bound
   */  
  WRATHGLProgram(const std::string &presource_name,
                 const WRATHGLShader::shader_source &vert_shader,
                 WRATHGLShader *frag_shader,
                 const WRATHGLPreLinkActionArray &action=WRATHGLPreLinkActionArray(),
                 const WRATHGLProgramInitializerArray &initers=WRATHGLProgramInitializerArray(),
                 const WRATHGLProgramOnBindActionArray &bind_actions=WRATHGLProgramOnBindActionArray()):
    m_initializers(initers.m_values),
    m_bind_actions(bind_actions)
  {
    m_shaders.push_back(WRATHNew WRATHGLShader(presource_name+".vert", vert_shader, GL_VERTEX_SHADER) );
    m_shaders.push_back(frag_shader);
    pre_assemble(presource_name, action);
  }

  /*!\fn WRATHGLProgram(const std::string&,
                        WRATHGLShader*,
                        const WRATHGLShader::shader_source&,
                        const WRATHGLPreLinkActionArray&,
                        const WRATHGLProgramInitializerArray&,
                        const WRATHGLProgramOnBindActionArray&)
  
    Ctor. 
    \param presource_name resource name to assign to the WRATHGLProgram
    \param vert_shader pointer to vertex shader to use for the WRATHGLProgram
    \param frag_shader shader source of fragment shader to use for the WRATHGLProgram
    \param action specifies actions to perform before and 
                  after linking of the WRATHGLProgram.
    \param initers one-time initialization actions to perform the first time the
                   WRATHGLProgram is used
    \param bind_actions array of actions to be executed when bound
   */    
  WRATHGLProgram(const std::string &presource_name,
                 WRATHGLShader *vert_shader,
                 const WRATHGLShader::shader_source &frag_shader,
                 const WRATHGLPreLinkActionArray &action=WRATHGLPreLinkActionArray(),
                 const WRATHGLProgramInitializerArray &initers=WRATHGLProgramInitializerArray(),
                 const WRATHGLProgramOnBindActionArray &bind_actions=WRATHGLProgramOnBindActionArray()):
    m_initializers(initers.m_values),
    m_bind_actions(bind_actions)
  {
    m_shaders.push_back(vert_shader);
    m_shaders.push_back(WRATHNew WRATHGLShader(presource_name+".frag", frag_shader, GL_FRAGMENT_SHADER) );
    pre_assemble(presource_name, action);
  }

  ~WRATHGLProgram(void);

  /*!\fn connect_dtor connect_dtor
    The dtor of a WRATHGLProgram emit's a signal, use this function
    to connect to that signal. The signal is emitted just before
    the WRATHGLProgram is removed from the resource manager.
   */
  boost::signals2::connection 
  connect_dtor(const boost::signals2::signal<void () >::slot_type &slot)
  {
    return m_dtor_signal.connect(slot);
  }

  /*!\fn void use_program
    Call to set GL to use the GLSLProgram
    of this WRATHGLProgram. The GL context
    must be current.
   */
  void
  use_program(void);

  /*!\fn GLuint name
    Returns the GL name (i.e. ID assigned by GL,
    for use in glUseProgram) of this WRATHGLProgram. 
    This function should
    only be called either after use_program() has
    been called or only when the GL context is
    current.
  */
  GLuint 
  name(void);

  /*!\fn const std::string& resource_name
    Returns the resource name of this
    WRATHGLProgram as set by it's constructor.
   */
  const std::string&
  resource_name(void)
  {
    return m_resource_name;
  }

  /*!\fn const std::string& link_log
    Returns the link log of this WRATHGLProgram,
    essentially the value returned by 
    glGetProgramInfoLog. This function should
    only be called either after use_program() has
    been called or only when the GL context is
    current.
   */
  const std::string&
  link_log(void);

  /*!\fn const std::string& action_log
    Returns the result of post action
    of the WRATHGLPreLinkActionArray passed
    at the ctor. This function should
    only be called either after use_program() has
    been called or only when the GL context is
    current.
   */
  const std::string&
  action_log(void);

  /*!\fn link_success
    Returns true if and only if the
    WRATHGLProgram successfully linked.
    This function should
    only be called either after use_program() has
    been called or only when the GL context is
    current.
   */
  bool
  link_success(void);

  /*!\fn const std::vector<WRATHGLShader*>& shaders
    Returns the WRATHGLShaders used to create
    this WRATHGLProgram.
   */
  const std::vector<WRATHGLShader*>&
  shaders(void) 
  {
    return m_shaders;
  }

  /*!\fn void log_contents
    Stream the log of this WRATHGLProgram
    to an std::ostream. This function should
    only be called either after use_program() has
    been called or only when the GL context is
    current.
    \param ostr std::ostream to which to stream
                the log of this WRATHGLProgram.
   */
  void
  log_contents(std::ostream &ostr);
  
  /*!\fn const std::map<std::string, parameter_info>& active_uniforms
    Returns a const reference of a std::map,
    keyed by name, of the active attributes 
    as listed by glGetActiveUniform().
    This function should
    only be called either after use_program() has
    been called or only when the GL context is
    current.
   */
  const std::map<std::string, parameter_info>&
  active_uniforms(void);

  /*!\fn const std::map<std::string, parameter_info>& active_attributes
    Returns a const reference of a std::map,
    keyed by name, of the active attributes 
    as listed by glGetActiveAttrib().  
    This function should
    only be called either after use_program() has
    been called or only when the GL context is
    current.  
   */
  const std::map<std::string, parameter_info>&
  active_attributes(void);

  /*!\fn attribute_uniform_query_result find_uniform
    Searches active_uniforms() to find the named
    uniform, also performs additional searches
    for workarounds that various GL implementations
    have in listing their uniforms. This function should
    only be called either after use_program() has
    been called or only when the GL context is
    current. Returns value
    will have the field attribute_uniform_query_result::m_info
    with the value NULL (and attribute_uniform_query_result::m_lcoation as
    -1) if unable to find a uniform of the stated name.

    \param uniform_name name of uniform to find
   */
  attribute_uniform_query_result
  find_uniform(const std::string &uniform_name);

  /*!\fn GLint uniform_location
    Provided as a conveniance to fetch the location
    of a named uniform, equivalent to:
    \code
    find_uniform(uniform_name).m_location
    \endcode
    This function should
    only be called either after use_program() has
    been called or only when the GL context is
    current.
    \param uniform_name name of uniform to find
   */
  GLint
  uniform_location(const std::string &uniform_name) 
  {
    return find_uniform(uniform_name).m_location;
  }
  
  /*!\fn attribute_uniform_query_result find_attribute
    Searches active_attributes() to find the named
    attribute, also performs additional searches
    for workarounds that various GL implementations
    have in listing their attributes. Returns NULL
    if unable to find a uniform of the stated
    name. This function should
    only be called either after use_program() has
    been called or only when the GL context is
    current.

    \param attribute_name name of attribute to find
   */
  attribute_uniform_query_result
  find_attribute(const std::string &attribute_name);
  
  /*!\fn GLint attribute_location
    Provided as a conveniance to fetch the location
    of a named attribute, equivalent to:
    \code
    find_attribute(attribute_name).m_location
    \endcode
    This function should
    only be called either after use_program() has
    been called or only when the GL context is
    current.

    \param attribute_name name of attribute to find
   */
  GLint
  attribute_location(const std::string &attribute_name)
  {
    return find_attribute(attribute_name).m_location;
  }

private:
  friend class WRATHGLBindAttribute;

  void
  pre_assemble(const std::string &presource_name, 
               const WRATHGLPreLinkActionArray &action);

  void
  assemble(void);

  std::vector<WRATHGLShader*> m_shaders;

  GLuint m_name;
  bool m_link_success, m_assembled;
  std::string m_link_log, m_resource_name;
  std::string m_action_log;

  std::set<std::string> m_binded_attributes;

  boost::signals2::signal<void () > m_dtor_signal;

  std::map<std::string, parameter_info> m_uniform_list;
  std::map<std::string, parameter_info> m_attribute_list;
  std::vector<WRATHGLProgramInitializer::const_handle> m_initializers;
  WRATHGLProgramOnBindActionArray m_bind_actions;
  WRATHGLPreLinkActionArray m_pre_link_actions;
};


/*! @} */

///////////////////////////////////
//WRATHGLUniformInitializer methods
template<typename T>
void
WRATHGLUniformInitializer<T>::
perform_initialization(WRATHGLProgram *pr) const
{
  GLint loc;
  
  loc=pr->uniform_location(m_uniform_name);
  if(loc!=-1)
    {
      WRATHglUniform(loc, m_value);
    }
  else
    {
      #ifdef WRATHDEBUG
        WRATHwarning("Failed to init \"" << m_uniform_name
                     << "\" in program \"" << pr->resource_name() << "\""
                     << "GL ID=" << pr->name());
      #endif
    }
}



#endif
