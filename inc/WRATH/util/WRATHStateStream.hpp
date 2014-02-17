/*! 
 * \file WRATHStateStream.hpp
 * \brief file WRATHStateStream.hpp
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




#ifndef WRATH_HEADER_STATE_STREAM_HPP_
#define WRATH_HEADER_STATE_STREAM_HPP_

#include "WRATHConfig.hpp"
#include <vector>
#include <map>
#include <typeinfo>
#include <boost/utility.hpp>
#include <algorithm>
#include "WRATHNew.hpp"
#include "c_array.hpp"
#include "type_tag.hpp"


/*! \addtogroup Utility
 * @{
 */

/*!\class WRATHStateStream
  A WRATHStateStream represents a collection 
  of streams of state changes. A state change consists
  of a "when" (stored as an integer),
  a "value" (any type) and a tag (an integer
  and the typeinfo of the type of the value).
  A WRATHStateStream allows one to add state
  changes and to then get the state changes.
  The change state streams themselves are 
  arrays of std::pair<int,S> where S is
  the data type and the .first field is
  the "when" of the state change. 
 */
class WRATHStateStream:boost::noncopyable
{
public:
  /*!\fn WRATHStateStream
    Ctor.
   */
  WRATHStateStream(void):
    m_time_location(0)
  {}
  
  ~WRATHStateStream(void);

  /*!\fn int time_value
    Returns the "local time" od
    this WRATHStateStream. At construction
    the value is initialized as 0.
   */
  int
  time_value(void) const
  {
    return m_time_location;
  }

  /*!\fn void increment_time_to_value
    Increment the "local time" of
    this WRATHStateStream TO the indicated
    value, that value must be atleast
    as large as the last value passed
    \ref increment_time_to_value() and must
    also be non-negative.
    \param v new value to set "local time" to.
   */
  void
  increment_time_to_value(int v)
  {
    WRATHassert(m_time_location<=v);
    m_time_location=v;
  }

  /*!\fn const_c_array< std::pair<int,S> > state_stream
    Returns the state change stream array associated 
    to a type and ID. If there is no state change stream
    associated to the type and ID, returns an empty
    array. The entries of the array are pairs of when 
    the state changed and to what value it changed. 
    \tparam S value type of the state stream to maipulate
    \param pID ID needed if there are multiple 
               independent arrays of the same type.
   */
  template<typename S>
  const_c_array< std::pair<int,S> >
  state_stream(int pID=0) const
  {
    key_type K(typeid(S), pID);
    typename std::map<key_type, array_holder_base*>::const_iterator iter;

    iter=m_runtime_arrays.find(K);
    if(iter==m_runtime_arrays.end())
      {
        return const_c_array< std::pair<int,S> >();
      }

    array_holder<S> *arr;
    arr=dynamic_cast<array_holder<S>*>(iter->second);
    WRATHassert(arr!=NULL);
    
    return const_c_array< std::pair<int,S> >(arr->m_data);
  }

  /*!\fn void set_state(type_tag<S>, const S&, int)
    "Set" the state associated to a type
    and ID. If the "local time" has changed since
    the last call to setting that state (or if
    that state was never set), set_state() adds
    the change state array, otherwise it modifies
    the last value of that array.
    \tparam S value type of the state stream to maipulate
    \param value value to set state change to.
    \param pID ID of state, in case that the type S
               is used for different states.
   */
  template<typename S>
  void
  set_state(type_tag<S>, const S &value, int pID)
  {
    get_array_holder<S>(pID).set_state(m_time_location, value);
  }

  /*!\fn void push_state
    Pushes a state onto the state
    stream stack and sets the value
    of the state stream to that value.
    Each stream of state changes also
    maintains a stack, this stack can
    be pushed and popped. The top of
    the stack is always the current
    state value.
    \tparam S value type of the state stream to maipulate
    \param value value to push onto the stack.
    \param pID ID of state, in case that the type S
               is used for different states.
   */
  template<typename S>
  void
  push_state(type_tag<S>, const S &value, int pID)
  {
    get_array_holder<S>(pID).push_state(m_time_location, value);
  }

  /*!\fn enum return_code pop_state
    Pops the a state of the state
    stream stack changing the value
    to the new top most element
    of the stack. If the stack cannot
    be popped, returns \ref routine_fail.
    \tparam S value type of the state stream to maipulate
    \param pID ID of state, in case that the type S
               is used for different states.
   */
  template<typename S>
  enum return_code
  pop_state(type_tag<S>, int pID)
  {
    return get_array_holder<S>(pID).pop_state(m_time_location);
  }

  /*!\fn enum return_code get_state
    "Get" the state associated to a type and ID.
    Returns \ref routine_success and sets target to
    the current state value if there is state,
    otherwise returns \ref routine_fail.
    \tparam S value type of the state stream to maipulate
    \param target location to which to write the current state,
                  only written to if there is a state stream
                  of the type S with ID pID.
    \param pID the ID of state
   */
  template<typename S>
  enum return_code
  get_state(type_tag<S>, S &target, int pID) const
  {
    const array_holder<S> *arr;

    arr=get_array_holder_ptr<S>(pID);
    if(arr==NULL or arr->m_data.empty())
      {
        return routine_fail;
      }

    target=arr->m_data.back().second;
    return routine_success;
  }

  /*!\fn void get_state_cast
    "Get" the state associated to a type and ID.
    Returns \ref routine_success and sets target to
    the current state value if there is state,
    otherwise returns \ref routine_fail.
    \tparam state_type value type of the state stream to maipulate
    \tparam target_type value type to which to cast the state stream value
    \param target location to which to write the current state,
                  only written to if there is a state stream
                  of the type S with ID pID.
    \param pID the ID of state
   */
  template<typename state_type, typename target_type>
  enum return_code
  get_state_cast(type_tag<state_type>, 
                 type_tag<target_type>,
                 target_type &target, int pID) const
  {
    const array_holder<state_type> *arr;

    arr=get_array_holder_ptr<state_type>(pID);
    if(arr==NULL or arr->m_data.empty())
      {
        return routine_fail;
      }

    target=arr->m_data.back().second;
    return routine_success;
  }

  /*!\fn void reset
    Clears and resets this WRATHStateStream, i.e.
    erasing all state change streams and reseting
    the "local time" of this WRATHStateStream to 0.
   */
  void
  reset(void);

  /*!\fn void set_state(const WRATHStateStream&, bool)
    For each state stream of a passed
    WRATHStateStream that is non-empty,
    set the state of this WRATHStateStream
    to it's value.
    \param obj WRATHStateStream from which to copy state
    \param copy_stacks if true the stacks are also copied
                       from obj.
   */
  void
  set_state(const WRATHStateStream &obj, bool copy_stacks=false);


  /*!\fn void sub_range
    Conveniance function to get the value to start
    a property at a start_index for a given interval 
    of iterators and to also increment a range iterator 
    to where it state changes of the passed iterator
    begin that are past the given start_index.
    \param default_value default value to return if one cannot
                         fetch an appropiate value from the 
                         iterator range of R.
    \param start_index first index to "deal" with
    \param R range iterator that is modified as follows:
             R.m_begin is incremented to the first element
             that indicates a change past start_index,
             i.e R.m_begin->first > start_index.

    if there is a state change at or before start_index
    returns that value, otherwise returns default_value.
   */
  template<typename S>
  static
  S
  sub_range(int start_index, S default_value, 
            range_type<typename const_c_array< std::pair<int, S> >::iterator> &R) 
  {
    typename const_c_array< std::pair<int, S> >::iterator iter;
    
    /*
      std::upper_bound does a binary search, upper
      bound returns the last iterator, iter, so that
      for each j before iter, start_index<j->second
      is false. Eqivalently, the last iterator iter,
      so that for each j before iter, start_index>=j->second. 
      In particular, start_index<iter->second.

      Note that: given the array A={ (0,A0), (1,A1), (5,A5), (10,A10) }
      passing start_index=0 will give an iterator to (1,A1)

      since (start_index=0) < (0,A0).second is false
      and (start_index=1) < (1,A1).second is true.
    */
    iter=std::upper_bound(R.m_begin, R.m_end, start_index, 
                          comparison_search_functor<S>());

    

    if(iter!=R.m_begin)
      {
        typename const_c_array< std::pair<int, S> >::iterator prev_iter(iter);
        --prev_iter;

        WRATHassert(prev_iter->first<=start_index);
        default_value=prev_iter->second;
      }

    WRATHassert(iter==R.m_end or iter->first>start_index);
      

    R.m_begin=iter;
    return default_value;
  } 
  
  /*!\fn bool update_value_from_change_cast
    Analogue of \ref update_value_from_change but where
    the type of the location to write the value is not
    the same as the type of the state stream to manipulate.
    \tparam S value type of the state stream to maipulate
    \tparam Scast value type to which to write the value
    \param current_index "the current local time"
    \param out_value location to write the current state value
                     IF it has changed. If it has not changed,
                     out_value is left unchanged
    \param R iterator range that has been initialized with
             sub_range()
   */
  template<typename S, typename Scast>
  static
  bool
  update_value_from_change_cast(int current_index, Scast &out_value, 
                                range_type<typename const_c_array< std::pair<int, S> >::iterator> &R) 
  {
    bool return_value(false);
    typename const_c_array< std::pair<int, S> >::iterator last_iterator;

    /*
      The expectation is that R.m_begin will only need to be 
      incremented at most once (see the doxy notes), the while
      loop is here to make it more robust in the case that
      .first portion of the array has repeated values.
     */
    while(R.m_begin!=R.m_end and R.m_begin->first<=current_index)
      {
        last_iterator=R.m_begin;
        return_value=true;
        ++R.m_begin;
      }

    if(return_value)
      {
        out_value=last_iterator->second;
      }
    WRATHassert(R.m_begin==R.m_end or R.m_begin->first>current_index);
    
    return return_value;
  }

  /*!\fn bool update_value_from_change(int, S&, 
                                       range_type<typename const_c_array< std::pair<int, S> >::iterator>&)
    Conveniance function to update values from a
    stream of state changes. 

    return true if the state value has changed.
    The usage of update_value_from_change is as follows,
    any other use patterns will be incorrect. One wishes
    to increment through a range of values and also
    know the state at each index within the range, the example
    code is as follows:

    \code
      //wish to increment over range of indices [begin, end)
      S current_value;
      const_c_array< std::pair<int,S> > array;

      //obj is a WRATHStateStream object
      array=obj.state_stream<S>(pID);
      range_type<const_c_array< std::pair<int, S> >::iterator> R(array.begin(), array.end());

      //default value is the default value for the state of type S of ID pID.
      current_value=sub_range(begin, defualt_value, R);
      for(int I=begin; I<end; ++I)
      {
         if(update_value_from_change(I, current_value, R))
         {
           //the value in the state stream has changed.
           //the new value has been written to current_value
         }
      }
    \endcode
    \tparam S value type of the state stream to maipulate
    \param current_index "the current local time"
    \param out_value location to write the current state value
                     IF it has changed. If it has not changed,
                     out_value is left unchanged
    \param R iterator range that has been initialized with
             sub_range()
   */
  template<typename S>
  static
  bool
  update_value_from_change(int current_index, S &out_value, 
                           range_type<typename const_c_array< std::pair<int, S> >::iterator> &R) 
  {
    return update_value_from_change_cast<S, S>(current_index, out_value, R);
  }

  /*!\fn bool update_value_from_change(int, 
                                       range_type<typename const_c_array< std::pair<int, S> >::iterator>&)
    Analgous role as update_value_from_change(), but does
    not take a reference of which to write a new value 
    \tparam S value type of the state stream to maipulate
    \param current_index "the current local time"
    \param R iterator range that has been initialized with
             sub_range()
   */
  template<typename S>
  static
  bool
  update_value_from_change(int current_index, 
                           range_type<typename const_c_array< std::pair<int, S> >::iterator> &R)
  {
    bool return_value(false);

    while(R.m_begin!=R.m_end and R.m_begin->first<=current_index)
      {
        return_value=true;
        ++R.m_begin;
      }  
    WRATHassert(R.m_begin==R.m_end or R.m_begin->first>current_index);  

    return return_value;
  }
  
  /*!\fn S get_iterator_range
    Conveniance function to get the value at
    a particular start_index for a named state
    (i.e. type and id) and to update an iterator 
    range.

    If there is a state change at or before start_index
    returns that value, otherwise returns default_value.

    The function is equivalent (indeed it actually just is)
    the following:
       \code
          const_c_array< std::pair<int,S> > array;
          array=state_stream<S>(pID);
          R.m_begin=array.begin();
          R.m_end=array.end();
          
          return sub_range(start_index, default_value, R);
       \endcode

    \tparam S value type of the state stream to maipulate    
    \param default_value default value to return if one cannot
                         fetch an appropiate value from the
                         return value of state_stream<S>(pID)
    \param start_index first index to "deal" with
    \param R range iterator that is modified as follows:
             R.m_begin is incremented to the first element
             that indicates a change past start_index,
             i.e R.m_begin->first > start_index and R.m_end
             is changed to the end of the named state stream.
    \param pID ID of state
   */
  template<typename S>
  S
  get_iterator_range(int start_index, S default_value, 
                     range_type<typename const_c_array< std::pair<int, S> >::iterator> &R,
                     int pID) const
  {
    const_c_array< std::pair<int,S> > array;
    array=state_stream<S>(pID);
    R.m_begin=array.begin();
    R.m_end=array.end();

    return sub_range(start_index, default_value, R);
  }

private:

  class key_type
  {
  public:
    key_type(const std::type_info &tp, int id):
      m_type(tp), m_id(id)
    {}

    bool
    operator<(const key_type &rhs) const
    {
      if(m_id!=rhs.m_id)
        {
          return m_id<rhs.m_id;
        }

      return m_type.before(rhs.m_type);

    }

    int
    id(void) const
    {
      return m_id;
    }

  private:
    const std::type_info &m_type;
    int m_id;
  };

  class array_holder_base:boost::noncopyable
  {
  public:
    virtual
    ~array_holder_base(){}

    virtual
    void
    copy_state(int loc, const array_holder_base*,
               bool copy_stacks)=0;

    virtual
    void
    create_copy(int loc, WRATHStateStream *target, 
                int pID, bool copy_stacks) const=0;
  };

  template<typename S>
  class array_holder:public array_holder_base
  {
  public:
    array_holder(void):
      m_stack(1)
    {}

    virtual
    void
    copy_state(int loc, const array_holder_base *psource,
               bool copy_stacks)
    {
      const array_holder *src;

      src=dynamic_cast<const array_holder*>(psource);
      WRATHassert(src!=NULL);

      if(!src->m_data.empty())
        {
          set_state(loc, src->m_data.back().second);
          if(copy_stacks)
            {
              m_stack=src->m_stack;
            }
        }
    }

    virtual
    void
    create_copy(int loc, WRATHStateStream *target, int pID,
                bool copy_stacks) const
    {
      target->get_array_holder<S>(pID).copy_state(loc, this, copy_stacks);
    }

    void
    set_state(int time_location, const S &value)
    {
      set_state_implement(time_location, value);
      m_stack.back()=value;
    }

    void
    push_state(int time_location, const S &value)
    {
      set_state_implement(time_location, value);
      m_stack.push_back(value);
    }

    enum return_code
    pop_state(int time_location)
    {
      if(m_stack.size()>1)
        {
          m_stack.pop_back();
          set_state_implement(time_location, m_stack.back());
          return routine_success;
        }
      return routine_fail;
    }

    
    std::vector< std::pair<int,S> > m_data;
    std::vector<S> m_stack;

  private:
    
    void
    set_state_implement(int time_location, const S &value)
    {
      if(m_data.empty())
        {
          m_data.push_back( std::make_pair(time_location, value) );
        }
      else if(m_data.back().second!=value)
        {
          if(time_location!=m_data.back().first)
            {
              m_data.push_back( std::make_pair(time_location, value) );
            }
          else 
            {
              m_data.back().second=value;
            }
        }
    }

  };

  template<typename S>
  array_holder<S>&
  get_array_holder(int I)
  {
    key_type K(typeid(S), I);
    std::map<key_type, array_holder_base*>::iterator iter;

    iter=m_runtime_arrays.find(K);
    if(iter==m_runtime_arrays.end())
      {
        m_runtime_arrays[K]=WRATHNew array_holder<S>();
        iter=m_runtime_arrays.find(K);
      }

    WRATHassert(iter!=m_runtime_arrays.end());
    array_holder<S> *arr;

    arr=dynamic_cast<array_holder<S>*>(iter->second);
    WRATHassert(arr!=NULL);

    return *arr;
  }

  template<typename S>
  const array_holder<S>*
  get_array_holder_ptr(int I) const
  {
    key_type K(typeid(S), I);
    std::map<key_type, array_holder_base*>::const_iterator iter;

    iter=m_runtime_arrays.find(K);
    if(iter==m_runtime_arrays.end())
      {
        return NULL;
      }
    else
      {
        const array_holder<S> *ptr;

        ptr=dynamic_cast<const array_holder<S>*>(iter->second);
        WRATHassert(ptr!=NULL);

        return ptr;
      }
  }

  template<typename S>
  class comparison_search_functor
  {
  public:
    bool
    operator()(const std::pair<int,S> &lhs,
               const std::pair<int,S> &rhs) const
    {
      return lhs.first<rhs.first;
    }

    bool
    operator()(const std::pair<int,S> &lhs,
               int rhs) const
    {
      return lhs.first<rhs;
    }

    bool
    operator()(int lhs,
               const std::pair<int,S> &rhs) const
    {
      return lhs<rhs.first;
    }
  };

  std::map<key_type, array_holder_base*> m_runtime_arrays;
  int m_time_location;
};

/*! @} */


#endif

