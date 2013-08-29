/*! 
 * \file WRATHMultiGLProgram.hpp
 * \brief file WRATHMultiGLProgram.hpp
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




#ifndef __WRATH_MULTI_GL_PROGRAM_HPP__
#define __WRATH_MULTI_GL_PROGRAM_HPP__

#include "WRATHConfig.hpp"
#include "WRATHGLProgram.hpp"
#include "WRATHReferenceCountedObject.hpp"
#include <boost/utility.hpp>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>

/*! \addtogroup GLUtility
 * @{
 */

/*!\class WRATHMultiGLProgram
  A WRATHMultiGLProgram represents common shader code
  use to generate different WRATHGLProgram objects
  with different defines added, analogous to having
  different executables by passing different "-D"
  options to a compiler. 
 */
class WRATHMultiGLProgram:public boost::noncopyable
{
public:
  
  /*!\class macro_collection
    A macro_collection is essentially a wrapper over an 
    std::map keyed by macros with values as to what the macros
    expand together with a collection of conveniance
    functions to make it easier to create the collection
   */
  class macro_collection
  {
  public:
    /*!\var m_macros
      The actual macros of the macro_collection
     */
    std::map<std::string, std::string> m_macros;

    /*!\fn macro_collection& add_macro(const std::string&, const std::string &)
      Conveniance function to add an entry to
      \ref m_macros.
      \param macro key to entry to add
      \param macro_value value of entry to add
     */
    macro_collection&
    add_macro(const std::string &macro, const std::string &macro_value="")
    {
      m_macros[macro]=macro_value;
      return *this;
    }

    /*!\fn macro_collection& add_macro(const std::string&, const T&)
      Conveniance function to add an entry to
      \ref m_macros.
      \param macro key to entry to add
      \param macro_value value of entry to add
     */
    template<typename T>
    macro_collection&
    add_macro(const std::string &macro, const T &macro_value)
    {
      std::ostringstream ostr;
      ostr << macro_value;
      m_macros[macro]=ostr.str();
      return *this;
    }
  };

  /*!\class Selector
    A Selector is the key to select
    a WRATHGLProgram from a WRATHMultiGLProgram.
    A Selector is a small, copyable object
    (it is essentially a wrapper over an int).
  */
  class Selector
  {
  public:
    /*!\fn Selector(void)
      Ctor. Creates a selector for having no additional macros.
     */
    Selector(void):
      m_ID(0)
    {}

    /*!\fn Selector(const std::map<std::string, std::string> &)
      Create a selector from a set of macros and their definitions.
      \param macros set of defined macros and their definitions, keyed by macro
                    with value as definition of macros.
     */
    Selector(const std::map<std::string, std::string> &macros);

    /*!\fn Selector(const macro_collection &)
      Create a selector from a set of macros and their definitions.
      \param macros set of defined macros and their definitions
     */
    Selector(const macro_collection &macros);

    /*!\fn const std::map<std::string, std::string>& macro_list
      Returns the macros of the Selector as an std::map
      with keys as macro and value as the macro expansion
     */
    const std::map<std::string, std::string>&
    macro_list(void) const;
     
    /*!\fn bool operator==()
      Comparison operator
      \param rhs value to which to compare
     */
    bool
    operator==(Selector rhs) const
    {
      return m_ID==rhs.m_ID;
    }
     
    /*!\fn bool operator!=()
      Comparison operator
      \param rhs value to which to compare
     */
    bool
    operator!=(Selector rhs) const
    {
      return m_ID!=rhs.m_ID;
    }
     
    /*!\fn bool operator<()
      Comparison operator
      \param rhs value to which to compare
     */
    bool
    operator<(Selector rhs) const
    {
      return m_ID<rhs.m_ID;
    }

  private:
    friend class WRATHMultiGLProgram;
    unsigned int m_ID;
  };

  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHMultiGLProgram, std::string);
  /// @endcond

  /*!\fn WRATHMultiGLProgram(const std::string &,
                             const std::map<GLenum, WRATHGLShader::shader_source> &,
                             const WRATHGLPreLinkActionArray &,
                             const WRATHGLProgramInitializerArray &,
                             const WRATHGLProgramOnBindActionArray &)
     Ctor.  
     \param presource_name resource name to assign to the WRATHMultiGLProgram
     \param shaders shader source of each stage to use for the WRATHMultiGLProgram
     \param actions specifies actions to perform before and after linking of each 
                    WRATHGLProgram of the WRATHMultiGLProgram
     \param initers one-time initialization actions to perform the first time each 
                    WRATHGLProgram of the WRATHMultiGLProgram is used
     \param bind_actions array of actions to be executed each time each
                         WRATHGLProgram of the WRATHMultiGLProgram is bound                           
   */
  WRATHMultiGLProgram(const std::string &presource_name,
                      const std::map<GLenum, WRATHGLShader::shader_source> &shaders,
                      const WRATHGLPreLinkActionArray &actions=WRATHGLPreLinkActionArray(),
                      const WRATHGLProgramInitializerArray &initers=WRATHGLProgramInitializerArray(),
                      const WRATHGLProgramOnBindActionArray &bind_actions=WRATHGLProgramOnBindActionArray()):
    m_shader_source_code(shaders),
    m_actions(actions),
    m_initers(initers),
    m_bind_actions(bind_actions)
  {
    register_resource(presource_name);
  }
                         
  
  /*!\fn WRATHMultiGLProgram(const std::string &,
                             const WRATHGLShader::shader_source &,
                             const WRATHGLShader::shader_source &,
                             const WRATHGLPreLinkActionArray &,
                             const WRATHGLProgramInitializerArray &,
                             const WRATHGLProgramOnBindActionArray &)
     Ctor.  
     \param presource_name resource name to assign to the WRATHMultiGLProgram
     \param vertex_source shader source of vertex shader for the WRATHMultiGLProgram
     \param fragment_source shader source of fragment shader for the WRATHMultiGLProgram
     \param actions specifies actions to perform before and after linking of each 
                    WRATHGLProgram of the WRATHMultiGLProgram
     \param initers one-time initialization actions to perform the first time each 
                    WRATHGLProgram of the WRATHMultiGLProgram is used
     \param bind_actions array of actions to be executed each time each
                         WRATHGLProgram of the WRATHMultiGLProgram is bound                           
   */
  WRATHMultiGLProgram(const std::string &presource_name,
                      const WRATHGLShader::shader_source &vertex_source,
                      const WRATHGLShader::shader_source &fragment_source,
                      const WRATHGLPreLinkActionArray &actions=WRATHGLPreLinkActionArray(),
                      const WRATHGLProgramInitializerArray &initers=WRATHGLProgramInitializerArray(),
                      const WRATHGLProgramOnBindActionArray &bind_actions=WRATHGLProgramOnBindActionArray()):
    m_actions(actions),
    m_initers(initers),
    m_bind_actions(bind_actions)
  {
    m_shader_source_code[GL_VERTEX_SHADER]=vertex_source;
    m_shader_source_code[GL_FRAGMENT_SHADER]=fragment_source;
    register_resource(presource_name);
  }

  ~WRATHMultiGLProgram();

  /*!\fn boost::signals2::connection connect_dtor
    The dtor of a WRATHMultiGLProgram emit's a signal, use this function
    to connect to that signal. The signal is emitted just before
    the WRATHMultiGLProgram is removed from the resource manager.
   */
  boost::signals2::connection 
  connect_dtor(const boost::signals2::signal<void () >::slot_type &slot)
  {
    return m_dtor_signal.connect(slot);
  }

  /*!\fn WRATHGLProgram* fetch_program
    Returns the WRATHGLProgram created with the source
    and functors specified in the ctor with the source
    pre-pended by the macros specified by the Selector.
    \param selector specifies what macros by which to prepend 
                    the shader source code
   */
  WRATHGLProgram*
  fetch_program(Selector selector) const;

  /*!\fn const std::string& resource_name
    Returns the resource name of this WRATHMultiGLProgram
   */ 
  const std::string&
  resource_name(void) const
  {
    return m_resource_name;
  }

private:
  typedef std::pair<WRATHGLProgram*, boost::signals2::connection> per_program;

  void
  register_resource(const std::string &pname);

  void
  on_program_delete(unsigned int ID) const;

  mutable WRATHMutex m_mutex;
  /*
    it would be tempting to have an std::map keyed
    by a handle to a selector, but the fetching
    of a program is to be fast, so instead we have
    a vector of pointers, indexed by the ID of
    a selector. That ID is guaranteed to be unique,
    unless some idiot makes atleast 2^32 different
    selectors.
   */
  mutable std::vector<per_program> m_programs;  
  std::map<GLenum, WRATHGLShader::shader_source> m_shader_source_code;
  WRATHGLPreLinkActionArray m_actions;
  WRATHGLProgramInitializerArray m_initers;
  WRATHGLProgramOnBindActionArray m_bind_actions;
  std::string m_resource_name;
  boost::signals2::signal<void () > m_dtor_signal;
};

/*! @} */

#endif
