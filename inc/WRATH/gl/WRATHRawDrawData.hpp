/*! 
 * \file WRATHRawDrawData.hpp
 * \brief file WRATHRawDrawData.hpp
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



#ifndef __WRATH_RAW_DRAW_DATA_HPP__
#define __WRATH_RAW_DRAW_DATA_HPP__

#include "WRATHConfig.hpp"
#include <set>
#include <boost/utility.hpp>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include <boost/utility.hpp>


#include "WRATHNew.hpp"
#include "WRATHBufferObject.hpp"
#include "WRATHMultiGLProgram.hpp"
#include "WRATHUniformData.hpp"
#include "WRATHTextureChoice.hpp"
#include "WRATHGLStateChange.hpp"
#include "WRATHDrawCommand.hpp"
#include "WRATHTripleBufferEnabler.hpp"
#include "opengl_trait.hpp"
#include "vecN.hpp"



/*! \addtogroup Kernel
 * @{
 */

/*!\class WRATHDrawOrder

  A WRATHDrawOrder is an abstract interface
  to specifying a drawing order (as such
  elements using different WRATHDrawOrder's 
  are drawn in separate draw calls). The
  order in which elements are drawn can 
  change. To that end a WRATHDrawOrder has
  a "time" associated to it that can incremented.
  It should be incremented whenever it's 
  state has changed in such a way that it's
  drawing order to other element might change.
 */
class WRATHDrawOrder:
  public WRATHReferenceCountedObjectT<WRATHDrawOrder>
{
public:

  /*!\class print_t
    Conveniance class that wraps a handle 
    to a WRATHDrawOrder object that has 
    overrided operator<< so that it calls
    print_stats() on the object passing
    the stream.
   */
  class print_t
  {
  public:
    /*!\fn print_t
      Ctor.
      \param h handle to \ref WRATHDrawOrder to execute
               WRATHDrawOrder::print_stats() upon.
     */
    explicit
    print_t(const WRATHDrawOrder::const_handle &h):
      m_h(h)
    {}

    /*!\fn std::ostream& operator<<(std::ostream &, const print_t &)
      Call WRATHDrawOrder::print_stats() on the WRATHDrawOrder
      object specified at the ctor of a \ref print_t
      passing an std::ostream.
      \param ostr std::ostream to which to print
      \param obj holds reference to WRATHDrawOrder whose
                 WRATHDrawOrder::print_stats() method will
                 be called
     */
    friend 
    std::ostream&
    operator<<(std::ostream &ostr, const print_t &obj);
    
  private:
    WRATHDrawOrder::const_handle m_h;
  };

  virtual
  ~WRATHDrawOrder()
  {}

  /*!\fn void print_stats
    To be optionally implemented by a derived
    class to print information about the object
    to an std::ostream
    \param ostr std::ostream to which to print
   */
  virtual
  void
  print_stats(std::ostream& ostr) const
  {
    WRATHunused(ostr);
  }

protected:
  /*!\fn note_change(void)  

    A derived class should call note_change() 
    on itself whenever it's internal state 
    has changed in such a way that it's drawing 
    sorting order may have changed.
   */
  void
  note_change(void)
  {
    m_signal();
  }

private:
  friend class WRATHRawDrawData;
  mutable boost::signals2::signal<void ()> m_signal;
};



/*!\class WRATHDrawOrderComparer
  A WRATHDrawOrderComparer is used by a 
  WRATHRawDrawData to sort the WRATHRawDrawDataElement
  which it is to draw. It is a pure virtual
  class that implement a comparison operation.
 */
class WRATHDrawOrderComparer:
  public WRATHReferenceCountedObjectT<WRATHDrawOrderComparer>
{
public:

  /*!\enum draw_sort_order_type  
    A draw_sorder_order_type is used
    to specify the comparison between
    two valid WRATHDrawOrder::const_handle 's
   */
  enum draw_sort_order_type
    {
      /*!
        First argument (the left hand side)
        is viewed as to be drawn before
        the second argument(the right hand side)
       */
      less_draw_sort_order,

      /*!
        Arguments have "equal" draw order,
        i.e. can be drawn in either order.
       */
      equal_draw_sort_order,

      /*!
        First argument (the left hand side)
        is viewed as to be drawn after
        the second argument(the right hand side)
       */
      greater_draw_sort_order,

    };

  /*!\fn WRATHDrawOrderComparer  
    Default empty ctor.
   */
  WRATHDrawOrderComparer(void)
  {}

  virtual
  ~WRATHDrawOrderComparer()
  {}

  /*!\fn draw_sort_order_type compare_objects  
    To be implemented by a derived class to
    specify the relative draw order between 
    two objects. It is NOT guaranteed that both
    arguments will be valid handles.
    \param lhs left hand side of comparison operation
    \param rhs right hand side of comparison operation
   */
  virtual
  enum draw_sort_order_type
  compare_objects(WRATHDrawOrder::const_handle lhs, 
                  WRATHDrawOrder::const_handle rhs) const=0;
};

/*!\class WRATHDrawCallSpec
  A WRATHDrawCallSpec specifies a single GL draw call:
  - GL state vector
  - attribute sources
  - draw command
  - draw order
 */
class WRATHDrawCallSpec
{
public:

  enum
    {
      /*!
        Number attributes supported,
        tweak as according to one's platform.
      */
      #if defined(WRATH_GL_VERSION) || WRATH_GLES_VERSION>=3
        attribute_count=16
      #else
        attribute_count=8
      #endif
    };

  /*!\typedef attribute_array_params
    Typedef for format and location for attribute data
    of draw call.
   */
  typedef vecN<opengl_trait_value, attribute_count> attribute_array_params;

  /*!\fn WRATHDrawCallSpec(void)
    Note the WRATHDrawCallSpec does NOT
    own any of data pointed to by any
    of the pointers.
   */
  explicit 
  WRATHDrawCallSpec(void):
    m_program(NULL),
    m_data_source(NULL),
    m_uniform_data(NULL),
    m_draw_command(NULL),
    m_gl_state_change(NULL)
  {}

  /*!\var m_force_draw_order
    m_force_draw_order is a handle to a
    WRATHDrawOrder to give a dynamic ordering
    to draw commands. The class WRATHDrawOrder
    provides an interface so that draw ordering
    of different elements can be changed at
    rum time with the only overhead being
    a resort.
  */
  WRATHDrawOrder::const_handle m_force_draw_order;
  
  /*!\var m_attribute_format_location
    Attributes formats for active attribute, also
    see attribute_format_location(). The i'th index
    of m_attribute_format_location determine the
    argument's for the i'th attribute, i.e.
    the arguments for glVertexAttribPointer(i, ...).
  */
  attribute_array_params m_attribute_format_location;
  
  /*!\var m_program
    WRATHMultiGLProgram that does the drawing.
   */
  WRATHMultiGLProgram *m_program;
  
  /*!\var m_bind_textures
    Handle to a WRATHTextureChoice to change
    what texture bindings, an invalid handle
    indicates that no texture bindings are to
    be affected
   */
  WRATHTextureChoice::const_handle m_bind_textures;
  
  /*!\var m_data_source
    WRATHBufferObject's holding the attribute data that 
    is drawn by the \ref m_program.
   */
  vecN<WRATHBufferObject*, attribute_count> m_data_source;
  
  /*!\var m_uniform_data
    Handle to a WRATHUniformData indicating
    setting uniform values of the \ref
    m_program. An invalid handle indicates
    to not change state.
   */
  WRATHUniformData::const_handle m_uniform_data;
  
  /*!\var m_draw_command
    A WRATHDrawCommand holds the actual GL
    call to do the drawing (for example
    glDrawElements), typically a
    WRATHDrawCommand holds a buffer object
    to source the indices from as well.
   */
  WRATHDrawCommand *m_draw_command;
  
  /*!\var m_gl_state_change
    Handle to a WRATHGLStateChange for
    expensive state changes (for example
    setting blend factors).
   */
  WRATHGLStateChange::const_handle m_gl_state_change;
  
  /*!\fn bool valid
    Returns true if and only if all of the 
    following conditions are met:
    - \ref m_program is not NULL
    - \ref m_draw_command is not NULL
    - \ref m_data_source[i] is non-NULL whenever m_attribute_format_location[i] is valid
   */
  bool
  valid(void) const;
    
  /*!\fn WRATHDrawCallSpec& program
    Sets \ref  m_program, the default 
    value for \ref m_program is NULL.
    \param v value to which to set \ref m_program 
   */
  WRATHDrawCallSpec&
  program(WRATHMultiGLProgram* v)
  {
    m_program=v;
    return *this;
  }
    
  /*!\fn WRATHDrawCallSpec& bind_textures
    Sets \ref m_bind_textures, the default 
    value for \ref m_bind_textures is an
    invalid handle.
    \param v value to which to set \ref m_bind_textures
   */
  WRATHDrawCallSpec&
  bind_textures(const WRATHTextureChoice::const_handle &v)
  {
    m_bind_textures=v;
    return *this;
  }
     
  /*!\fn WRATHDrawCallSpec& gl_state_change
    Sets \ref m_gl_state_change, the default 
    value for \ref m_gl_state_change is an
    invalid handle.
    \param v value to which to set \ref m_gl_state_change
   */
  WRATHDrawCallSpec&
  gl_state_change(const WRATHGLStateChange::const_handle &v)
  {
    m_gl_state_change=v;
    return *this;
  }
     
  /*!\fn WRATHDrawCallSpec& uniform_data
    Sets \ref m_uniform_data, the default 
    value for \ref m_uniform_data is an
    invalid handle.
    \param v value to which to set \ref m_uniform_data
   */
  WRATHDrawCallSpec&
  uniform_data(const WRATHUniformData::const_handle &v)
  {
    m_uniform_data=v;
    return *this;
  }  
    
  /*!\fn WRATHDrawCallSpec& data_source(WRATHBufferObject*, int)
    Sets a specified entry \ref m_data_source, the default 
    value for all entries of \ref m_data_source is NULL.
    \param v value to which to set \ref m_data_source[I]
    \param I entry of \ref m_data_source  to set
   */
  WRATHDrawCallSpec&
  data_source(WRATHBufferObject* v, int I)
  {
    m_data_source[I]=v;
    return *this;
  }

  /*!\fn WRATHDrawCallSpec& data_source(WRATHBufferObject*)
    Sets all entries \ref m_data_source, the default 
    value for each entry of \ref m_data_source is 
    NULL.
    \param v value to which to set \ref m_data_source 
   */
  WRATHDrawCallSpec&
  data_source(WRATHBufferObject* v)
  {
    m_data_source=vecN<WRATHBufferObject*, attribute_count>(v);
    return *this;
  }

  /*!\fn WRATHDrawCallSpec& draw_command
    Sets \ref m_draw_command, the default 
    value for \ref m_draw_command is NULL.
    \param v value to which to set \ref m_draw_command 
   */
  WRATHDrawCallSpec&
  draw_command(WRATHDrawCommand* v)
  {
    m_draw_command=v;
    return *this;
  }
  
  /*!\fn WRATHDrawCallSpec& attribute_format_location
    Sets the named attribute of 
    \ref m_attribute_format_location, the
    default value for each indicates
    that the attribute index is not used.
    \param attr which attribute to set
    \param v value to which to set the named attribute
   */
  WRATHDrawCallSpec&
  attribute_format_location(int attr, 
                            const opengl_trait_value &v)
  {
    m_attribute_format_location[attr]=v;
    return *this;
  }

  /*!\fn WRATHDrawCallSpec& force_draw_order
    Set \ref m_force_draw_order, the default
    value is an invalid handle.
    \param v value to which to set \ref m_force_draw_order
  */
  WRATHDrawCallSpec&
  force_draw_order(const WRATHDrawOrder::const_handle &v)
  {
    m_force_draw_order=v;
    return *this;
  }

  /*!\fn void reset
    Reintialize the WRATHDrawCallSpec 
    to be empty, i.e. all elements set to 
    NULL or an invalid handle.
   */
  void
  reset(void)
  {
    m_force_draw_order=NULL;
    m_attribute_format_location=attribute_array_params();
    m_program=NULL;
    m_data_source=vecN<WRATHBufferObject*, attribute_count>(NULL);
    m_uniform_data=NULL;
    m_draw_command=NULL;
    m_gl_state_change=NULL;
  }
};


class WRATHRawDrawData;

/*!\class WRATHRawDrawDataElement
  A WRATHRawDrawDataElement represents a
  single draw call within a WRATHRawDrawData
  object. The values of the draw call are
  immutable for the lifetime of the object.
 */
class WRATHRawDrawDataElement:boost::noncopyable
{
public:

  /*!\fn WRATHRawDrawDataElement
    Ctor. 
    \param spec values for the draw call
  */
  explicit
  WRATHRawDrawDataElement(const WRATHDrawCallSpec &spec):
    m_spec(spec),
    m_raw_draw_data(NULL),
    m_location_in_raw_draw_data(-1)
  {}

  ~WRATHRawDrawDataElement()
  {
    WRATHassert(m_raw_draw_data==NULL);
    m_draw_order_dirty.disconnect();
  }

  /*!\fn WRATHRawDrawData* raw_draw_data
    Returns the WRATHRawDrawData (if any) that
    this WRATHRawDrawDataElement has been added
    to via \ref WRATHRawDrawData::add_element().
   */
  WRATHRawDrawData*
  raw_draw_data(void) const
  {
    return m_raw_draw_data;
  }
    
  /*!\fn const WRATHDrawCallSpec&draw_spec 
    Returns the WRATHDrawCallSpec that specifies
    the draw call.
   */
  const WRATHDrawCallSpec&
  draw_spec(void) const
  {
    return m_spec;
  }

private:
  friend class WRATHRawDrawData;

  WRATHDrawCallSpec m_spec;

  /*
    used by WRATHRawDrawData book-keeping.
   */
  WRATHRawDrawData *m_raw_draw_data;
  int m_location_in_raw_draw_data;
  boost::signals2::connection m_draw_order_dirty;
};

/*!\class WRATHRawDrawData
  A WRATHRawDrawData is a collection of a
  pointers to const WRATHRawDrawDataElement 's.
  It is assumed that the objects are constant
  when they are used by a WRATHRawDrawData. If one
  needs to change the value, it is required to
  first remove it from the WRATHRawDrawData and
  then re-add it.  
 */
class WRATHRawDrawData:
  public WRATHTripleBufferEnabler::PhasedDeletedObject
{
public:

  /*!\fn WRATHRawDrawData
    Ctor. The WRATHRawDrawData will store the 
    WRATHRawDrawDataElement 's pointers sorted as follows:
    - 0) \ref WRATHDrawCallSpec::m_force_draw_order, this sorting
         is as follows: first those elements for which 
         WRATHDrawCallSpec::m_force_draw_order is not a valid
         handle comes first, followed by those with a valid handle
         sorted by draw_order_sorter()
    - 1) WRATHMultiGLProgram \ref WRATHDrawCallSpec::m_program 
    - 2) WRATHTextureChoice \ref WRATHDrawCallSpec::m_bind_textures
    - 3) WRATHGLStateChange \ref WRATHDrawCallSpec::m_gl_state_change
    - 4) WRATHBufferObject \ref WRATHDrawCallSpec::m_data_source 
    - 5) \ref WRATHDrawCallSpec::attribute_array_params \ref WRATHDrawCallSpec::m_attribute_format_location
    - 6) WRATHUniformData \ref WRATHDrawCallSpec::m_uniform_data
    - 7) WRATHBufferObject of WRATHDrawCommand \ref WRATHDrawCallSpec::m_draw_command
    - 8) WRATHDrawCommand \ref WRATHDrawCallSpec::m_draw_command

    \param ptriple_buffer_enabler handle to a WRATHTripleBufferEnabler to
                                  which the created WRATHRawDrawData will
                                  sync to. In particular it will update
                                  it's drawing list data for the rendering
                                  thread according to the signaling
                                  of ptriple_buffer_enabler. It is an
                                  error if the handle is not valid.
                                 
    \param pdraw_order_sorter handle to a WRATHDrawOrderComparer to
                              sort drawing order, an invalid handle
                              indicates to ignore WRATHDrawCallSpec::m_force_draw_order
                              in the sorting.
   */
  explicit
  WRATHRawDrawData(const WRATHTripleBufferEnabler::handle &ptriple_buffer_enabler,
                   const WRATHDrawOrderComparer::const_handle &pdraw_order_sorter=
                   WRATHDrawOrderComparer::const_handle());

  virtual
  ~WRATHRawDrawData(void);

  /*!\fn const WRATHDrawOrderComparer::const_handle& draw_order_sorter(void) const
    Returns a handle to the draw order sorting object
    used by this WRATHRawDrawData. 
    May only be called from the simulation thread.
   */
  const WRATHDrawOrderComparer::const_handle&
  draw_order_sorter(void) const;

  /*!\fn void draw_order_sorter(const WRATHDrawOrderComparer::const_handle&)
    Change the sorting object used by this
    WRATHRawDrawData to sort draw orders.
    May only be called from the simulation thread.
  */
  void
  draw_order_sorter(const WRATHDrawOrderComparer::const_handle &v);

  /*!\fn void add_element
    Adds a draw "command" to the WRATHRawDrawData
    It is an error to have the same WRATHRawDrawDataElement
    added to multiple WRATHRawDrawData objects.
    It is an error to change the contents
    of a WRATHRawDrawDataElement while it is on
    a WRATHRawDrawData object. Additionally, a single
    WRATHRawDrawDataElement may only be on one
    WRATHRawDrawData at a time.

    May only be called from the simulation thread.

    \param b a WRATHRawDrawDataElement to add, the
             object pointed to by b must stay in scope
             until it is removed from the WRATHRawDrawData,
             via \ref remove_element() or until the
             WRATHRawDrawData goes out of scope.
   */
  void
  add_element(WRATHRawDrawDataElement *b);

  /*!\fn void remove_element
    Removes a draw "command" from the WRATHRawDrawData
    on which it was added.
    May only be called from the simulation thread.
    \param b pointer to element to remove
   */
  static
  void
  remove_element(WRATHRawDrawDataElement *b);

  /*!\class draw_information
    An instance of a draw_information
    stores statistics on drawing such
    as number of draw calls, number
    of texture switches, etc.
   */
  class draw_information
  {
  public:
    /*!\var m_draw_count
      Number of draw calls.
     */
    int m_draw_count;

    /*!\var m_program_count
      Number of GLSL program _changes_
     */
    int m_program_count;

    /*!\var m_texture_choice_count
      Number of texture choice _changes_
     */
    int m_texture_choice_count;

    /*!\var m_gl_state_change_count
      Number of GL state _change_
     */
    int m_gl_state_change_count;

    /*!\var m_attribute_change_count
      Number of attribute format changes.
     */
    int m_attribute_change_count;

    /*!\var m_buffer_object_bind_count
      Number of buffer object binds.
     */
    int m_buffer_object_bind_count;
    
    draw_information(void):
      m_draw_count(0),
      m_program_count(0),
      m_texture_choice_count(0),
      m_gl_state_change_count(0),
      m_attribute_change_count(0),
      m_buffer_object_bind_count(0)
    {}
      
  };

  /*!\class DrawState
    DrawState object is used to track GL state
    during the drawing of data from a WRATHRawDrawData.
    It's main use case is that if one wishes to
    chain drawing multiple WRATHRawDrawData objects
    together that might share some common GL state
    vector values.
   */
  class DrawState
  {
  public:

    enum
      {
        /*!
          Bring into local scope WRATHDrawCallSpec::attribute_count
         */
        attribute_count=WRATHDrawCallSpec::attribute_count
      };
    
    /*!
      Bring into local scope WRATHDrawCallSpec::attribute_array_params
    */
    typedef WRATHDrawCallSpec::attribute_array_params attribute_array_params;

    /*!\fn DrawState(WRATHMultiGLProgram::Selector, draw_information*)
      Ctor.
      \param selector initial selector for choosing WRATHGLProgram
                      from WRATHGLMultiProgram of WRATHRawDrawDataElement
                      that are drawn
      \param pdraw_information pointer to \ref draw_information object
                               to record GL state change statitistics.
                               Values of the object are _incremented_.
                               If the value is NULL, an internal object
                               is used
    */
    DrawState(WRATHMultiGLProgram::Selector selector=WRATHMultiGLProgram::Selector(),
              draw_information *pdraw_information=NULL):
      m_prog(NULL),
      m_attr_source(NULL),
      m_currently_bound(NULL),
      m_indx_source(NULL),
      m_init_attributes(true),
      m_primitive_type(GL_INVALID_ENUM),
      m_index_type(GL_INVALID_ENUM),
      m_active(false),
      m_selector(selector),
      m_draw_information_ptr(pdraw_information!=NULL?
                             pdraw_information:
                             &m_draw_information)
    {}

    virtual
    ~DrawState()
    {}
    
    /*!\fn void texture(const WRATHTextureChoice::const_handle &)
      Bind a specified WRATHTextureChoice and have
      the DrawState be aware of that binding.
      \param hnd handle to WRATHTextureChoice to bind
     */
    void
    texture(const WRATHTextureChoice::const_handle &hnd);

    /*!\fn WRATHTextureChoice::const_handle texture(void) const
      Returns the WRATHTextureChoice that the DrawState
      has tracked to be active
     */
    WRATHTextureChoice::const_handle
    texture(void) const
    {
      return m_tex;
    }
    
    /*!\fn void uniform(const WRATHUniformData::const_handle &)
      Bind a specified WRATHUniformData and have
      the DrawState be aware of that binding.
      \param hnd handle to WRATHUniformData to bind
     */
    void
    uniform(const WRATHUniformData::const_handle &hnd);

    /*!\fn WRATHUniformData::const_handle uniform(void) const
      Returns the WRATHUniformData that the DrawState
      has tracked to be active
     */
    WRATHUniformData::const_handle
    uniform(void) const
    {
      return m_uniform;
    }
    
    /*!\fn void gl_state_change(const WRATHGLStateChange::const_handle&)
      Bind a specified WRATHGLStateChange and have
      the DrawState be aware of that binding.
      \param hnd handle to WRATHGLStateChange to bind
     */
    void
    gl_state_change(const WRATHGLStateChange::const_handle &hnd);

    /*!\fn WRATHGLStateChange::const_handle gl_state_change(void) const
      Returns the WRATHGLStateChange that the DrawState
      has tracked to be active
     */
    WRATHGLStateChange::const_handle
    gl_state_change(void) const
    {
      return m_gl_state_source;
    }

    /*!\fn void program(WRATHMultiGLProgram*)
      Sets the WRATHMultiGLProgram that the 
      DrawState is viewed as active. In contrast 
      to the behavior of texture(), uniform() 
      and gl_state_change(), does NOT 
      affect the actual GL state, i.e. does NOT
      call \ref WRATHGLProgram::use_program().
      \param pr WRATHMultiGLProgram to view as active
     */
    void
    program(WRATHMultiGLProgram *pr);

    /*!\fn WRATHMultiGLProgram program(void)
      Returns the WRATHMultiGLProgram that the 
      DrawState is viewed as active. 
     */
    WRATHMultiGLProgram*
    program(void)
    {
      return m_prog;
    }

    /*!\fn WRATHMultiGLProgram::Selector selector(void) const
      Returns the active WRATHMultiGLProgram::Selector
      used for selecting which WRATHGLProgram to use
      from each WRATHMultiGLProgram.
     */
    WRATHMultiGLProgram::Selector
    selector(void) const
    {
      return m_selector;
    }

    /*!\fn void selector(WRATHMultiGLProgram::Selector) 
      Sets the active WRATHMultiGLProgram::Selector
      used for selecting which WRATHGLProgram to use
      from each WRATHMultiGLProgram. Does NOT call
      \ref  WRATHGLProgram::use_program() on changes
      to the selector value.
      \param s WRATHMultiGLProgram::Selector to use
     */
    void
    selector(WRATHMultiGLProgram::Selector s);

    /*!\fn void make_program_active()
      Because program() and selector() do NOT call
      WRATHGLProgram::use_program() even when the
      the WRATHGLProgram to be used changes, the
      GL state vector (and internal tracking of
      DrawState) do not have the GL program as
      in use. Calling make_program_active() makes
      the correct WRATHGLProgram in use and viewed
      as in use by DrawState.
     */
    void
    make_program_active(void);

    /*!\fn bool valid_program_active 
      Calls make_program_active(). Returns true
      if both the current WRATHMultiGLProgram
      is non-NULL and the WRATHGLProgram selected
      is useable (i.e. \ref WRATHGLProgram::link_success())
      is true.
     */
    bool
    valid_program_active(void);

    /*!\fn void set_attribute_sources
      Set the attribute format and location values.
      \param p_attr_source source of attribute data
      \param p_attr_fmt format and location within source of attribute data
     */
    void
    set_attribute_sources(const vecN<WRATHBufferObject*, attribute_count> &p_attr_source,
                          const attribute_array_params &p_attr_fmt);

    /*!\fn void queue_drawing
      Queue a draw command, draw command is NOT necessarily
      executed immediately.
     */
    void
    queue_drawing(WRATHDrawCommand *draw_command);

    /*!\fn bool draw_active
      Returns trus if thw DrawState object is active,
      i.e. draw_begin() has been called but draw_end()
      has not yet.
     */
    bool
    draw_active(void) { return m_active; }

    /*!\fn draw_information& recorder
      Returns the \ref draw_information to which
      GL state change counts, draw call counts, etc
      to which are incremented.
     */
    draw_information&
    recorder(void) 
    {
      return *m_draw_information_ptr;
    }

    /*!\fn void draw_begin(draw_information&, WRATHMultiGLProgram::Selector)
      Reinitializes this DrawState object for drawing and
      sets the DrawState as active.
      \param out_stats draw statistics to which to record, values are incremented
      \param pselector Selector object that chooses which WRATHGLProgram
                       to use from each WRATHGLMultiProgram. The value
                       is stored in the DrawState object
     */
    void
    draw_begin(draw_information &out_stats, 
               WRATHMultiGLProgram::Selector pselector);

    /*!\fn void draw_begin(WRATHMultiGLProgram::Selector)
      Provided as a conveniance, equivalent to
      \code
        draw_begin(recorder(), pselector);
      \endcode
      \param pselector Selector object that chooses which WRATHGLProgram
                       to use from each WRATHGLMultiProgram. The value
                       is stored in the DrawState object
     */
    void
    draw_begin(WRATHMultiGLProgram::Selector pselector)
    {
      draw_begin(recorder(), pselector);
    }

    /*!\fn void draw_begin(draw_information&)
      Provided as a conveniance, equivalent to
      \code
        draw_begin(out_stats, selector());
      \endcode
      \param out_stats draw statistics to which to record, values are incremented
     */
    void
    draw_begin(draw_information &out_stats)
    {
      draw_begin(out_stats, selector());
    }

    /*!\fn void draw_begin(void)
      Provided as a conveniance, equivalent to
      \code
        draw_begin(recorder(), selector());
      \endcode
     */
    void
    draw_begin(void)
    {
      draw_begin(recorder(), selector());
    }

    /*!\fn void draw_end(void)
      Signal end of drawing. Flushes all queued
      drawing as well. Must be called before
      draw_begin() is called again on the
      DrawState object.
    */
    void
    draw_end(void);

    /*!\fn void flush_draws(void)
      Flush queued drawing commands.
      Needed if one changes the GL state vector
      without telling the DrawState object.
     */
    void
    flush_draws(void);
      
  private:

    void
    index_buffer(WRATHDrawCommand *draw_command);
    
    
    WRATHMultiGLProgram *m_prog;
    WRATHGLProgram *m_current_glsl;
    WRATHUniformData::const_handle m_uniform;
    WRATHTextureChoice::const_handle m_tex;
    vecN<WRATHBufferObject*, attribute_count> m_attr_source;
    std::set<WRATHBufferObject*> m_locked_bos;
    WRATHBufferObject *m_currently_bound;
    WRATHBufferObject *m_indx_source;
    WRATHGLStateChange::const_handle m_gl_state_source;
    DrawState::attribute_array_params m_attr_format;
    bool m_init_attributes;

    std::vector<WRATHDrawCommand::index_range> m_draw_ranges;
    GLenum m_primitive_type;
    GLenum m_index_type;

    std::vector<uint8_t> m_temp_bytes;

    bool m_active;
    WRATHMultiGLProgram::Selector m_selector;

    draw_information m_draw_information;
    draw_information *m_draw_information_ptr;
  };

  /*!\fn void draw(draw_information&, WRATHMultiGLProgram::Selector)
    Draw the WRATHRawDrawDataElement of this
    WRATHRawDrawData, assuming no current
    GL state, recording the draw
    statistics by incrementing a draw_information.

    May only be called from the rendering thread.
    Equivalent to
    \code
    DrawState draw_state() 
    draw_state.draw_begin(selector, out_stats);
    draw(out_stats, draw_state);
    draw_state.draw_end();
    \endcode

    \param out_stats values of out_stats are_incremented_
    \param selector Selector object that chooses which WRATHGLProgram
                    to use from each WRATHGLMultiProgram of each
                    WRATHRawDrawDataElement in drawing
   */
  void
  draw(draw_information &out_stats, 
       WRATHMultiGLProgram::Selector selector=WRATHMultiGLProgram::Selector());


  
  /*!\fn void draw(DrawState&)
    Draw the WRATHRawDrawDataElement of this
    WRATHRawDrawData, recording the draw
    statistics by incrementing a draw_information.
    The GL state is assumed to be that as 
    stored in the DrawState object passed.
    May only be called within a DrawState::draw_begin()
    DrawState::draw_end() pair of the passed 
    \ref DrawState object.

    May only be called from the rendering thread.

    \param draw_state DrawState left over from a previous
                      call to draw()
   */
  void
  draw(DrawState &draw_state);


  /*!\fn bool render_empty
    Returns true if there are no element to
    draw. May only be called from the rendering
    thread.
   */
  bool
  render_empty(void);

protected:
  

  virtual
  void
  on_place_on_deletion_list(void);

  virtual
  void
  phase_simulation_deletion(void);
  
private:

  class sorter
  {
  public:
    
    sorter(const WRATHDrawOrderComparer::const_handle &cmp):
      m_comparer(cmp)
    {}

    bool
    operator()(const WRATHRawDrawDataElement *lhs,
               const WRATHRawDrawDataElement *rhs);

    WRATHDrawOrderComparer::const_handle m_comparer;
    
  };

  void
  check_sort_elements(void);

  void
  post_copy_elements(void);

  void
  remove_element_implement(WRATHRawDrawDataElement *b);
  
  void
  mark_list_dirty(void);

  /*
    Sorting occurs only in the simulation thread.
    The strategy is as follows:
    1) on signal (on_complete_simulation_frame, pre_update_no_lock)
       if m_list_dirty sort the buffers of list of m_buffers[current_simulation_ID()]
    2) on signal (on_complete_simulation_frame, post_update_no_lock)
       copy the contents of m_buffers[last_simulation_ID()] to m_buffers[current_simulation_ID()].
    3) ordering changes fire a signal
   */
  sorter m_sorter;
  bool m_list_dirty;

  vecN<std::vector<WRATHRawDrawDataElement*>, 3> m_buffers;
  vecN<WRATHTripleBufferEnabler::connect_t, 2> m_connections;
};

/*! @} */


#endif
