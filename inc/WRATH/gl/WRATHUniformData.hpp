/*! 
 * \file WRATHUniformData.hpp
 * \brief file WRATHUniformData.hpp
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




#ifndef __WRATH_UNIFORM_DATA_HPP__
#define __WRATH_UNIFORM_DATA_HPP__

#include "WRATHConfig.hpp"
#include <map>
#include "WRATHReferenceCountedObject.hpp"
#include "WRATHgl.hpp"
#include "WRATHNew.hpp"
#include "WRATHgluniform.hpp"
#include "WRATHGLProgram.hpp"
#include "WRATHTripleBufferEnabler.hpp"

/*! \addtogroup GLUtility
 * @{
 */

/*!\class WRATHUniformData
  A WRATHUniformData represents a collection
  of uniforms for uniform data stored as a
  set of handles to WRATHUniformData::uniform_setter_base
  objects.
 */
class WRATHUniformData:
  public WRATHReferenceCountedObjectT<WRATHUniformData>
{
public:

  /*!\class uniform_setter_base
    Base class for setting uniform values
    of GLSL programs. 
   */
  class uniform_setter_base:
    public WRATHReferenceCountedObjectT<uniform_setter_base>
  {
  public:

    virtual
    ~uniform_setter_base()
    {}

    /*!\fn void gl_command
      To be implemented by a derived
      class to make the necessary GL
      commands to set the uniform.
      \param pr WRATHGLProgram of the uniform(s) to set
     */    
    virtual
    void
    gl_command(WRATHGLProgram *pr)=0;
  };

  /*!\class uniform_by_name_base
    The class uniform_by_name_base is a pure
    virtual class that fetches the location
    of the GLSL uniform. A fixed uniform_by_name_base
    derived object can be used with multiple
    WRATHGLProgram objects. A uniform_by_name_base
    will fetch the location (and store) the location
    of the uniform when it is to be used with a
    different WRATHGLProgram object than previously.    
   */
  class uniform_by_name_base:public uniform_setter_base
  {
  public:
    typedef handle_t<uniform_by_name_base> handle;
    typedef const_handle_t<uniform_by_name_base> const_handle;

    /*!\fn uniform_by_name_base
      Ctor.
      \param uniform_name _name_ of GLSL uniform that the object is to set
     */
    explicit
    uniform_by_name_base(const std::string &uniform_name);
    
    /*!\fn void set_uniform_value
      To be implemented by a derived class to make
      the actual GL call to set the value of a 
      uniform in a GLSL program. Only called
      if uniform_by_name_base succesfully fetched
      the location of the uniform.
      \param location the GL location of the uniform, as in
                      glGetUniformLocation
     */
    virtual
    void
    set_uniform_value(GLint location)=0;

    /*!\fn const std::string& uniform_name
      Returns the name of the GLSL uniform
     */
    const std::string&
    uniform_name(void) const
    {
      return m_uniform_name;
    }

    virtual
    void
    gl_command(WRATHGLProgram *pr);

  private:
    std::map<WRATHGLProgram*, GLint> m_location_map;
    WRATHGLProgram *m_pr;
    std::string m_uniform_name;
    GLint m_location;
  };

  /*!\class uniform_by_name
    Template class which holds the
    value to which to set the uniform.
    \tparam T type of value for which there
              exists a WRATHglUniform call to accept.
   */
  template<typename T>
  class uniform_by_name:public uniform_by_name_base
  {
  public:
    typedef handle_t<uniform_by_name> handle;
    typedef const_handle_t<uniform_by_name> const_handle;

    /*!\var m_value
      Value used in the WRATHglUniform call
      to set the value of the GLSL uniform.
      Once this object is in use for drawing,
      \ref m_value may only be set from the 
      rendering thread.
     */
    T m_value;

    /*!\fn uniform_by_name
      Ctor.
      \param uniform_name _name_ of GLSL uniform that the object is to set
      \param v initial value of m_value.
     */
    explicit
    uniform_by_name(const std::string &uniform_name,
                    const T &v=T()):
      uniform_by_name_base(uniform_name),
      m_value(v)
    {}

    
    void
    set_uniform_value(GLint location) 
    {
      WRATHassert(location!=-1);
      WRATHglUniform(location, m_value);
    }
  };

  /*!\class uniform_by_name_ref
    Template class which holds a _pointer_
    to the value to which to set the uniform.
    \param T type of value for which there
             exists a WRATHglUniform call to accept.
   */
  template<typename T>
  class uniform_by_name_ref:public uniform_by_name_base
  {
  public:
    typedef handle_t<uniform_by_name_ref> handle;
    typedef const_handle_t<uniform_by_name_ref> const_handle;

    /*!\fn uniform_by_name_ref
      Ctor.
      \param uniform_name _name_ of GLSL uniform that the object is to set
      \param value_ptr pointer to value to use, the
                       T pointed to by value_ptr must
                       be in scope whenever this
                       uniform_by_name_ref's gl_command()
                       is called. NULL is an acceptable
                       value, in which case this uniform_by_name_ref
                       will not set a uniform when it's
                       gl_command() method is called.
                       If T is non-null, then the value pointed
                       to by T must only be set in the rendering
                       thread once this object is in use for
                       drawing.
     */
    explicit
    uniform_by_name_ref(const std::string &uniform_name,
                        const T *value_ptr):
      uniform_by_name_base(uniform_name),
      m_value_ptr(value_ptr)
    {}

    /*!\fn void change_reference
      Change the reference to use as the value.
      \param value_ptr pointer to value to use, the
                       T pointed to by value_ptr must
                       be in scope whenever this
                       uniform_by_name_ref's gl_command()
                       is called. NULL is an acceptable
                       value, in which case this uniform_by_name_ref
                       will not set a uniform when it's
                       gl_command() method is called.
                       If T is non-null, then the value pointed
                       to by T must only be set in the rendering
                       thread once this object is in use for
                       drawing.
     */
    void
    change_reference(const T *value_ptr)
    {
      m_value_ptr=value_ptr;
    }
  
    void
    set_uniform_value(GLint location) 
    {
      WRATHassert(location!=-1);
      WRATHglUniform(location, *m_value_ptr);
    }

  private:
    const T *m_value_ptr;
  };

  /*!\class uniform_by_name_triple_buffered
    A triple buffered analogue of \ref uniform_by_name.
    The value can be changed from the simulation thread.
    Changing the value is a lock-free operation.
    \param T type of value for which there
             exists a WRATHglUniform call to accept.
   */
  template<typename T>
  class uniform_by_name_triple_buffered:public uniform_by_name_base
  {
  public:
    typedef handle_t<uniform_by_name_triple_buffered> handle;
    typedef const_handle_t<uniform_by_name_triple_buffered> const_handle;

    /*!\fn uniform_by_name_triple_buffered
      Ctor. 
      \param uniform_name _name_ of GLSL uniform that the object is to set
      \param tr handle to WRATHTripleBufferEnabler for lock free setting 
      \param v initial value to use for the uniform
     */
    uniform_by_name_triple_buffered(const std::string &uniform_name,
                                    const WRATHTripleBufferEnabler::handle &tr,
                                    const T &v=T()):
      uniform_by_name_base(uniform_name),
      m_value(v, v, v),
      m_tr(tr)
    {
      m_connection=m_tr->connect(WRATHTripleBufferEnabler::on_complete_simulation_frame,
                                 WRATHTripleBufferEnabler::post_update_no_lock,
                                 boost::bind(&uniform_by_name_triple_buffered::on_end_simulation_frame, this));
    }

    ~uniform_by_name_triple_buffered(void)
    {
      m_connection.disconnect();
    }

    /*!\fn void value(const T &v)
      Set the value to send to GL as the value of the 
      uniform, may only be called from the simulation
      thread.
      \param v new value for the uniform to send to GL
     */
    void
    value(const T &v)
    {
      m_value[m_tr->current_simulation_ID()]=v;
    }

    void
    set_uniform_value(GLint location) 
    {
      WRATHassert(location!=-1);
      WRATHglUniform(location, *m_value[m_tr->present_ID()]);
    }
  private:

    void
    on_end_simulation_frame(void)
    {
      int from(m_tr->last_simulation_ID());
      int to(m_tr->current_simulation_ID());

      m_value[to]=m_value[from];
    }

    WRATHTripleBufferEnabler::connect_t m_connection;
    vecN<T, 3> m_value;
    WRATHTripleBufferEnabler::handle m_tr;
  };

  /*!\typedef element_type
    Conveniance typedef for an element
    of a \ref element_type_collection
   */
  typedef uniform_setter_base::handle element_type;

  /*!\typedef element_type_collection
    Conveniance typedef for a collection of handles
    to \ref uniform_setter_base objects
   */ 
  typedef std::set<uniform_setter_base::handle> element_type_collection;


  /*!\fn void add_uniform
    Adds the passed uniform_setter_base.
    \param p handle to uniform_setter_base 
             to add to this WRATHUniformData
   */
  void
  add_uniform(const uniform_setter_base::handle &p);
  
  /*!\fn void add_uniforms
    Conveniance function to add many uniform 
    setters.
    \tparam iterator is an iterator to uniform_setter_base::handle.
    \param begin handle to first uniform_setter_base to add
    \param end one past the last uniform_setter_base handle to add
   */
  template<typename iterator>
  void
  add_uniforms(iterator begin, iterator end)
  {
    for(;begin!=end; ++begin)
      {
        add_uniform(*begin);
      }
  }

  /*!\fn void add
    Equivalent to add_uniforms(),
    provided for template programming
    conveniance.
    \param begin iterator to first element to add
    \param end iterator to one past last element to add.
   */
  template<typename iterator>
  void
  add(iterator begin, iterator end)
  {
    add_uniforms(begin, end);
  }

  /*!\fn void execute_gl_commands
    For each uniform_setter_base within 
    this WRATHUniformData, execute it's 
    uniform_setter_base::gl_command() 
    method to set the uniform value.
   */
  void
  execute_gl_commands(WRATHGLProgram *pr) const; 

  /*!\fn enum return_code remove_uniform
    Removes the uniform_setter_base object.
    If the object was not in the set of 
    uniform_setter_base objects used by
    this, returns routine_fail.

    \param h handle to WRATHUniformData
   */
  enum return_code
  remove_uniform(const uniform_setter_base::handle &h);

  /*!\fn const element_type_collection& elements
    Returns the uniform objects of this.
   */
  const element_type_collection&
  elements(void) const
  {
    return m_uniforms;
  }

  /*!\fn bool different
    Returns true if the contents of
    two WRATHUniformData differ from
    each other.
    \param v0 handle to a WRATHUniformData.
    \param v1 handle to a WRATHUniformData.
   */
  static
  bool
  different(const WRATHUniformData::const_handle &v0,
            const WRATHUniformData::const_handle &v1);

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
  compare(const WRATHUniformData::const_handle &lhs,
          const WRATHUniformData::const_handle &rhs);


private:
  std::set<uniform_setter_base::handle> m_uniforms;
};

/*! @} */

#endif
