/*! 
 * \file WRATHStateStreamManipulators.hpp
 * \brief file WRATHStateStreamManipulators.hpp
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





#ifndef WRATH_HEADER_STATE_STREAM_MANIPULATOR_HPP_
#define WRATH_HEADER_STATE_STREAM_MANIPULATOR_HPP_

#include "WRATHConfig.hpp"
#include "WRATHStateStream.hpp"
#include "WRATHWrapper.hpp"


/*! \addtogroup Utility
 * @{
 */

/*!\namespace WRATHStateStreamManipulators
  Namespace to encapsulate various types
  for easier use and manipulation of 
  \ref WRATHStateStream objects.
 */
namespace WRATHStateStreamManipulators
{
  /*!\class set_state_type
    A set_state_type is a conveniance class
    to specify a state change in a WRATHStateStream.
   */
  template<typename S>
  class set_state_type
  {
  public:
    /*!\fn set_state_type
      Ctor.
      \param pdata to which to set the state
      \param pID optional ID in the case one uses
                 the same type for differnt changes,
                 the ID can be used to differentiate.
     */
    set_state_type(S pdata, int pID):
      m_ID(pID), m_data(pdata)
    {}

    /*!\fn int ID
      returns the ID as set by the ctor.
    */
    int
    ID(void) const
    {
      return m_ID;
    }

    /*!\fn S data
      returns the data as set by the ctor.
     */
    S
    data(void) const
    {
      return m_data;
    }
  private:
    int m_ID;
    S m_data;
  };
  

  /*!\class get_state_type
    A get_state_type is a conveniance class
    to get a state in a WRATHStateStream.
   */
  template<typename S>
  class get_state_type
  {
  public:
    /*!\fn get_state_type(S&, enum return_code*, int)
      Ctor. 
      \param ptarget reference to variable to put the return value
      \param R pointer to error return code if the query fails
      \param pId tag of query
     */
    get_state_type(S &ptarget, enum return_code *R, int pId):
      m_target(ptarget), m_return(R), m_ID(pId)
    {}

    /*!\fn get_state_type(S&, int)
      Ctor. Sets to not write the return code of a
      query, i.e return_value() returns NULL.
      \param ptarget reference to variable to put the return value
      \param pId tag of query
     */
    get_state_type(S &ptarget, int pId):
      m_target(ptarget), m_return(NULL), m_ID(pId)
    {}

    /*!\fn int ID
      returns the ID as set by the ctor.
     */
    int
    ID(void) const
    {
      return m_ID;
    }

    /*!\fn S& target
      returns the target as set by the ctor.
     */
    S&
    target(void) const
    {
      return m_target;
    }

    /*!\fn enum return_code* return_value
      Returns the location to place the return code 
     */
    enum return_code*
    return_value(void) const
    {
      return m_return;
    }

  private:
    S &m_target;
    enum return_code *m_return;
    int m_ID;
  };

  /*!\class get_state_cast_type
    A get_state_type is a conveniance class
    to get a state in a WRATHStateStream.
    \tparam T the type of the state stream
    \tparam S the type to which to write the state stream value
   */
  template<typename T, typename S>
  class get_state_cast_type
  {
  public:
    /*!\fn get_state_cast_type(S&, enum return_code*, int)
      Ctor. 
      \param ptarget reference to variable to put the return value
      \param R pointer to error return code if the query fails
      \param pId tag of query
     */
    get_state_cast_type(S &ptarget, enum return_code *R, int pId):
      m_target(ptarget), m_return(R), m_ID(pId)
    {}

    /*!\fn get_state_cast_type(S&, int)
      Ctor.
      \param ptarget reference to variable to put the return value
      \param pId tag of query
     */
    get_state_cast_type(S &ptarget, int pId):
      m_target(ptarget), m_return(NULL), m_ID(pId)
    {}

    /*!\fn int ID
      returns the ID as set by the ctor.
     */
    int
    ID(void) const
    {
      return m_ID;
    }

    /*!\fn S& target
      returns the target as set by the ctor.
     */
    S&
    target(void) const
    {
      return m_target;
    }

    /*!\fn enum return_code* return_value
      Returns the pointer to the return code 
     */
    enum return_code*
    return_value(void) const
    {
      return m_return;
    }

  private:
    S &m_target;
    enum return_code *m_return;
    int m_ID;
  };

  /*!\class push_state_type
    A push_state_type is a conveniance class
    to push a state change in a WRATHStateStream.
   */
  template<typename S>
  class push_state_type
  {
  public:
    /*!\fn push_state_type
      Ctor.
      \param pdata value to which to set the state
      \param pID ID in the case one uses
                 the same type for differnt changes,
                 the ID can be used to differentiate.
     */
    push_state_type(S pdata, int pID):
      m_ID(pID), m_data(pdata)
    {}

    /*!\fn int ID
      returns the ID as set by the ctor.
     */
    int
    ID(void) const
    {
      return m_ID;
    }

    /*!\fn S data
      returns the data as set by the ctor.
     */
    S
    data(void) const
    {
      return m_data;
    }

  private:
    int m_ID;
    S m_data;
  };
  
  /*!\class pop_state_type
    A pop_state_type is a conveniance class
    to pop a state change in a WRATHStateStream.
   */
  template<typename S>
  class pop_state_type
  {
  public:    
    /*!\fn pop_state_type
      Ctor.
      \param pID ID in the case one uses
                 the same type for differnt changes,
                 the ID can be used to differentiate.
     */
    explicit
    pop_state_type(int pID):
      m_ID(pID)
    {}

    /*!\fn int ID
      returns the ID as set by the ctor.
     */
    int
    ID(void) const
    {
      return m_ID;
    }
    
  private:
    int m_ID;
  };

  /*!\class generic_state
    A generic_state is a template conveniance class
    for walking the state streams of a WRATHStateStream
    more easily. It is a wrapper over a type that
    is castable to that type and constructable from
    that type. In addition, it has a template parameter
    "tag" that allows one to use the same type T but
    still produde distinct types from the template class
    generic_state. Inherits from \ref WRATHUtil::wrapper_type\<T\>
    thus can be casted into type T and if T is a pointer
    type, provides operator-> and operator*.
    \param T the actual type that generic_state wraps (for example float)
    \param Tag a tag to distinguish to create different types 
    that wrap the same type T
 
    The use pattern for generic_state is as follows.
    Setting the value is done with:
    \code
    class TagForExample {}; 
    typedef generic_state<TypeForExample, TagForExample> example;
    WRATHStateStream MyStateStream;
    .
    .
    TypeForExample v, push_value;
    //set example, can also use can also use example::set_type(v) 
    MyStateSteam << WRATHStateStreamManipulators::set_state<example>(v);
    .
    .
    //push example, can also use can also use example::push_type(v) 
    MyStateSteam << WRATHStateStreamManipulators::push_state<example>(v);
    .
    .
    .
    //pop example, can also use example::pop_type()
    MyStateSteam << WRATHStateStreamManipulators::pop_state<example>();
    \endcode

    Reading the state stream values is done as follows:
    \code    
    example::stream_iterator iter;
    example default_value, current_value;
    
    //init iter to walk the range [begin, end) of time
    //in MyStateSteam and to set current_value as the 
    //value "at" start, if no value is there then to use
    //default_value to init current_value
    current_value=example::init_stream_iterator(MyStateSteam, begin, default_value, iter); 
    for(int i=begin; i<end; ++i)
      {
        if(example::update_value_from_change(i, current_value, iter))
          {
            //value changed at location i, current_value now holds the new value
          }
      }
    \endcode
  */
  template<typename T, typename Tag>
  class generic_state:public WRATHUtil::wrapper_type<T>
  {
  public:
    /*!\typedef set_type
      Conveniance typedef.
     */
    typedef set_state_type<generic_state> set_type;

    /*!\typedef push_type
      Conveniance typedef.
     */
    typedef push_state_type<generic_state> push_type;

    /*!\typedef pop_type
      Conveniance typedef.
     */
    typedef pop_state_type<generic_state> pop_type;

    /*!\typedef get_type
      Conveniance typedef.
     */
    typedef get_state_cast_type<generic_state, T> get_type;

    /*!\typedef get_uncasted_type
      Conveniance typedef.
     */
    typedef get_state_type<generic_state> get_uncasted_type;

    /*!\typedef iterator
      Conveniance typedef.
     */
    typedef typename const_c_array< std::pair<int, generic_state> >::iterator iterator;
 
    /*!\typedef stream_iterator
      Conveniance typedef.
     */
    typedef range_type<iterator> stream_iterator;

    /*!\fn generic_state(void)
      Empty ctor, initializing T with empty ctor,
      or for POD types, T value is uninitialized.
     */
    generic_state(void)
    {}

    /*!\fn generic_state(const T &)
      Ctor, initializing T with passed value
      \param v value to which to initialize \ref WRATHUtil::wrapper_type_base<T>::m_value
     */
    generic_state(const T &v):
      WRATHUtil::wrapper_type<T>(v)
    {}

    /*!\fn T sub_range(int, T, stream_iterator&)
      Conveniance function, equivalent to
      \code
      WRATHStateStream::sub_range<generic_state>(start_index, generic_state(default_value), R);
      \endcode
      See \ref WRATHStateStream::sub_range()
     */
    static
    T
    sub_range(int start_index, T default_value, stream_iterator &R)
    {
      return WRATHStateStream::sub_range<generic_state>(start_index, generic_state(default_value), R);
    }

    /*!\fn bool update_value_from_change(int, T&, stream_iterator&)
      Conveniance function, equivalent to
      \code
      WRATHStateStream::update_value_from_change_cast<generic_state, T>(current_index, out_value, R);
      \endcode
      see \ref WRATHStateStream::update_value_from_change()
     */
    static
    bool
    update_value_from_change(int current_index, T &out_value, stream_iterator &R)
    {
      return WRATHStateStream::update_value_from_change_cast<generic_state, T>(current_index, out_value, R);
    }

    /*!\fn bool update_value_from_change(int, stream_iterator&)
      Conveniance function, equivalent to
      \code
      WRATHStateStream::update_value_from_change<generic_state>(current_index, R);
      \endcode
      see \ref WRATHStateStream::update_value_from_change()
     */
    static
    bool
    update_value_from_change(int current_index, stream_iterator &R)
    {
      return WRATHStateStream::update_value_from_change<generic_state>(current_index, R);
    }

    /*!\fn T init_stream_iterator(const WRATHStateStream&, int, T, stream_iterator&)
      Conveniance function, equivalent to
      \code
      state_stream.get_iterator_range<generic_state>(start_index, 
                                                     generic_state(default_value),
                                                     R, -1);
      \endcode
      see \ref WRATHStateStream::get_iterator_range()
     */
    static
    T
    init_stream_iterator(const WRATHStateStream &state_stream,
                         int start_index, T default_value,
                         stream_iterator &R)
    {
      return state_stream.get_iterator_range<generic_state>(start_index, 
                                                            generic_state(default_value),
                                                            R, -1);
    }
                         
  };

  

  /*!\fn set_state_type<S> set_state(S, int)
    Provided as a syntatic conveniance 
    (so that one does not need to 
    explicitely name the type S)
    to return a set_state_type\<S\>
    \tparam S (template parameter) type of data
    \param pdata value
    \param pID tag
   */
  template<typename S>
  set_state_type<S>
  set_state(S pdata, int pID=0)
  {
    return set_state_type<S>(pdata, pID);
  }

  /*!\fn push_state_type<S> push_state(S, int)
    Provided as a syntatic conveniance 
    (so that one does not need to 
    explicitely name the type S)
    to return a push_state_type\<S\>
    \tparam S (template parameter) type of data
    \param pdata value
    \param pID tag
   */
  template<typename S>
  push_state_type<S>
  push_state(S pdata, int pID=0)
  {
    return push_state_type<S>(pdata, pID);
  }

  /*!\fn pop_state_type<S> pop_state(int)
    Provided as a syntatic conveniance 
    (so that one does not need to 
    explicitely named the type S)
    to return a pop_state_type\<S\>
    \tparam S (template parameter) type of data
    \param pID tag
   */
  template<typename S>
  pop_state_type<S>
  pop_state(int pID=0)
  {
    return pop_state_type<S>(pID);
  }

  /*!\fn get_state_type<S> get_state(S&, int)
    Provided as a syntatic conveniance 
    (so that one does not need to 
    explicitely name the type S)
    to return a \ref get_state_type\<S\>
    \tparam S (template parameter) type of data
    \param pdata value
    \param pID tag
   */
  template<typename S>
  get_state_type<S>
  get_state(S &pdata, int pID=0)
  {
    return get_state_type<S>(pdata, pID);
  }

  /*!\fn get_state_type<S> get_state(S&, enum return_code&, int)
    Provided as a syntatic conveniance 
    (so that one does not need to 
    explicitely name the type S)
    to return a get_state_type\<S\>
    \tparam S (template parameter) type of data
    \param ptarget location to place value
    \param R valid reference to set if retrieved succeeded.
    \param pID tag
   */
  template<typename S>
  get_state_type<S>
  get_state(S &ptarget, enum return_code &R, int pID=0)
  {
    return get_state_type<S>(ptarget, &R, pID);
  }

  /*!\class get_stream_size_type
    A get_stream_size_type is just a wrapper
    over a pointer to an integer.
    It is used to get the current size 
    of a WRATHStateStream or WRATHPolygonPoints. 
    The use for such is to get a range between such
    markers.
   */
  class get_stream_size_type
  {
  public:
    /*!\var m_target
      Address of location to place the value.
     */
    int *m_target;
    
    /*!\fn get_stream_size_type
       Ctor.
      \param ptarget address to place the value
     */
    explicit
    get_stream_size_type(int *ptarget):
      m_target(ptarget)
    {}
  };

  /*!\fn get_stream_size_type stream_size(int&)
    "Manipulator" to get the current size of
    the data stream
    \param ptarget location to place value
   */
  inline
  get_stream_size_type
  stream_size(int &ptarget)
  {
    return get_stream_size_type(&ptarget);
  }
 
};


/*!\fn WRATHStateStream& operator<<(WRATHStateStream&, const WRATHStateStreamManipulators::set_state_type<S>&)
  Overload operator<< to set a value of a WRATHStateStream. 
  Equivalent to
  \code
  target.set_state(type_tag<S>(), obj.data(), obj.ID());
  \endcode
  See also \ref WRATHStateStream::set_state().
  \param target WRATHStateStream to manipulate
  \param obj holds the value to set to the stream
 */ 
template<typename S>
WRATHStateStream&
operator<<(WRATHStateStream &target, const WRATHStateStreamManipulators::set_state_type<S> &obj)
{
  target.set_state(type_tag<S>(), obj.data(), obj.ID());
  return target;
}

/*!\fn WRATHStateStream& operator<<(WRATHStateStream&, const WRATHStateStreamManipulators::get_state_cast_type<T,S>&)
  Overload operator<< to get a value of a WRATHStateStream. 
  Equivalent to
  \code
  enum return_code R;
  R=target.get_state_cast(type_tag<T>(), type_tag<S>(), obj.target(), obj.ID());
  if(obj.return_value()!=NULL)
    {
      *obj.return_value()=R;
    }
  \endcode
  See also \ref WRATHStateStream::get_state_cast().
  \param target WRATHStateStream to manipulate
  \param obj holds the targets to which to write the query results
 */ 
template<typename T, typename S>
WRATHStateStream&
operator<<(WRATHStateStream &target, const WRATHStateStreamManipulators::get_state_cast_type<T,S> &obj)
{
  enum return_code R;
  R=target.get_state_cast(type_tag<T>(), type_tag<S>(), obj.target(), obj.ID());
  if(obj.return_value()!=NULL)
    {
      *obj.return_value()=R;
    }
  return target;
}

/*!\fn WRATHStateStream& operator<<(WRATHStateStream&, const WRATHStateStreamManipulators::get_state_type<S>&)
  Overload operator<< to get a value of a WRATHStateStream. 
  Equivalent to
  \code
  enum return_code R;
  R=target.get_state(type_tag<S>(), obj.target(), obj.ID());
  if(obj.return_value()!=NULL)
    {
      *obj.return_value()=R;
    } 
  \endcode
  See also \ref WRATHStateStream::get_state().
  \param target WRATHStateStream to manipulate
  \param obj  holds the targets to which to write the query results
 */ 
template<typename S>
WRATHStateStream&
operator<<(WRATHStateStream &target, const WRATHStateStreamManipulators::get_state_type<S> &obj)
{
  enum return_code R;
  R=target.get_state(type_tag<S>(), obj.target(), obj.ID());
  if(obj.return_value()!=NULL)
    {
      *obj.return_value()=R;
    } 
  return target;
}

/*!\fn WRATHStateStream& operator<<(WRATHStateStream&, const WRATHStateStreamManipulators::push_state_type<S>&)
  Overload operator<< to push a value of a WRATHStateStream. 
  Equivalent to
  \code
  target.push_state(type_tag<S>(), obj.data(), obj.ID());
  \endcode
  See also \ref WRATHStateStream::push_state().
  \param target WRATHStateStream to manipulate
  \param obj holds the value to push to the stream
 */ 
template<typename S>
WRATHStateStream&
operator<<(WRATHStateStream &target, const WRATHStateStreamManipulators::push_state_type<S> &obj)
{
  target.push_state(type_tag<S>(), obj.data(), obj.ID());
  return target;
}

/*!\fn WRATHStateStream& operator<<(WRATHStateStream&, const WRATHStateStreamManipulators::pop_state_type<S>&)
  Overload operator<< to pop a value of a WRATHStateStream.
  Equivalent to
  \code
  target.pop_state(type_tag<S>(), obj.ID());
  \endcode
  See also \ref WRATHStateStream::pop_state().
  \param target WRATHStateStream to manipulate
  \param obj holds the ID to pop
 */ 
template<typename S>
WRATHStateStream&
operator<<(WRATHStateStream &target, const WRATHStateStreamManipulators::pop_state_type<S> &obj)
{
  target.pop_state(type_tag<S>(), obj.ID());
  return target;
}

/*!\def WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
  Provides a safe way to declare/create properties so that
  the named property is wrapped in a unique type (thus
  one does not need to track stream ID's). Will create
  the classes/typedefs
  - class tag_property_name_tag {};
  - typedef WRATHStateStreamManipulators::generic_state<property_type, tag_property_name_tag> property_name

  and the following non-member functions:
  - property_name::set_type set_property_name(property_type) to set the value, see \ref WRATHStateStreamManipulators::generic_state::set_type
  - property_name::push_type push_property_name(property_type) to push the value, see \ref WRATHStateStreamManipulators::generic_state::push_type
  - property_name::pop_type pop_property_name(void) to pop the value, see \ref WRATHStateStreamManipulators::generic_state::pop_type()
  - property_name::get_type get_property_name(property_type &v) to get the value see \ref WRATHStateStreamManipulators::generic_state::get_type
  - property_name::get_uncasted_type get_property_name(property_name &v) to get the value see \ref WRATHStateStreamManipulators::generic_state::get_uncasted_type

  which can be using within a WRATHStateStream to set, push, pop and get
  the state value. The two get functions, one takes a reference to a
  property_name object (not likely to used often) and the other takes a
  reference to a property_type object (most likely to used)

  Note that the typedef of \ref WRATHStateStreamManipulators::generic_state
  then gives the declared class the static methods
  - sub_range, see  WRATHStateStreamManipulators::generic_state<T, Tag>::sub_range()
  - update_value_from_change, see  WRATHStateStreamManipulators::generic_state<T, Tag>::update_value_from_change()
  - init_stream_iterator, see  WRATHStateStreamManipulators::generic_state<T, Tag>::init_stream_iterator()
  
  Hopefully, the above can make one's code easier to follow and
  track what state is being set and read.

  Macro's use is typically to be within a namespace (for example
  WRATHText has a number of these).

  \param property_name The name to give the property
  \param property_type The underlying type wrapped by the property
*/
#define WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(property_name, property_type) \
  class tag_##property_name##_tag {};\
  typedef ::WRATHStateStreamManipulators::generic_state<property_type, tag_##property_name##_tag> property_name;\
  inline \
  property_name::set_type  \
  set_##property_name(property_type v) \
  {\
    return  property_name::set_type(property_name(v), -1);      \
  }\
  inline \
  property_name::push_type \
  push_##property_name(property_type v) \
  {\
    return property_name::push_type(property_name(v), -1);     \
  }\
  inline \
  property_name::pop_type  \
  pop_##property_name(void) \
  {\
  return property_name::pop_type(-1);\
  }\
  inline \
  property_name::get_type \
  get_##property_name(property_type &v) \
  {\
  return property_name::get_type(v, -1);\
  }\
  inline\
  property_name::get_uncasted_type\
  get_##property_name(property_name &v) \
  {\
  return property_name::get_uncasted_type(v, -1); \
  }
     
  

/*! @} */

#endif
