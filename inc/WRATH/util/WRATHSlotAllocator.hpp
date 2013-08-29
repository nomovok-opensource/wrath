/*! 
 * \file WRATHSlotAllocator.hpp
 * \brief file WRATHSlotAllocator.hpp
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



#ifndef __WRATH_SLOT_ALLOCATOR_HPP__
#define __WRATH_SLOT_ALLOCATOR_HPP__

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include <map>
#include <set>
#include <vector>
#include "WRATHMutex.hpp"

/*! \addtogroup Utility
 * @{
 */


/*!\class WRATHSlotAllocator
  A WRATHSlotAllocator is a tracker to
  allocate slots by a value. Typically,
  the value type T is a pointer type
  and the values are pointer to objects.
  A fixed object when repeatedly added 
  does NOT take more slots, rather it 
  increments a reference count. When an 
  object is removed, it decrements that 
  reference  count, and when that reference 
  count reaches zero, then the slot that the
  object occupies is removed.
  \tparam T value type used to key slot allocations,
            typically a pointer type with the values
            being pointers to objects.
*/
template<typename T>
class WRATHSlotAllocator:boost::noncopyable
{
public:
  /*!\class per_node_data
    A per_node_data holds the number
    of references and the location
    for an object (of type T).
   */
  class per_node_data
  {
  public:
    /*!\fn per_node_data(void)
      Ctor. Initializes a per_node_data instance with no references
      and target slow location of 0.
     */
    per_node_data(void):
      m_reference_count(0),
      m_location(0)
    {}
   
    /*!\fn per_node_data(int)
      Ctor. Initializes a per_node_data for an existing slot location
      of an object of type T.
      \param location object slot location
     */
    explicit
    per_node_data(int location):
      m_reference_count(1),
      m_location(location)
    {}

    /*!\var m_reference_count
      Number of times the object is
      referenced.
     */
    int m_reference_count;

    /*!\var m_location
      The slot location for the object.
     */
    int m_location;
  };

  /*!\typedef map_type
    Slot allocation is stored as an std::map
    keyed by T with values as per_node_data.
   */
  typedef std::map<T, per_node_data> map_type;

  /*!\fn WRATHSlotAllocator
    Ctor. Create a WRATHSlotAllocator with
    the specified number of slots. 
   */
  explicit
  WRATHSlotAllocator(unsigned int max_size):
    m_max_size(max_size)
  {}
  
  ~WRATHSlotAllocator(void)
  {}

  /*!\fn const std::vector<T>& active_elements_as_array
    Returns an array with index I giving
    what value is using slot I. A value
    of NULL indicates that the slot is
    free.
    The contents of the returned reference are
    changed whenever add_element(T),
    or remove_element(T) is called.
    Accessing the value of the
    returned array should be within
    locking the WRATHMutex, mutex().
   */
  const std::vector<T>&
  active_elements_as_array(void) const
  {
    return m_active_as_nodes;
  }

  /*!\fn const map_type& active_elements
    Returns an std::map keyed by T with values of
    per_node_data of tracked values.
    The contents of the returned reference are
    changed whenever add_element(T),
    or remove_element(T) is called.
    Accessing the value of the
    returned array should be within
    locking the WRATHMutex, mutex().
   */
  const map_type&
  active_elements(void) const
  {
    return m_active;
  }

  /*!\fn enum return_code accepts_element(T) const
    Returns \ref routine_success if the value is already
    tracked or if there is a slot free to track it.
    
    Is thread safe by locking \ref mutex().

    \param v value to query for tracking.
   */
  enum return_code
  accepts_element(T v) const
  {
    enum return_code r;

    WRATHLockMutex(m_mutex);
    r=accepts_element_nolock(v);
    WRATHUnlockMutex(m_mutex);

    return r;
  }

  /*!\fn int slot_location(T) const
    Returns the slot location for an value,
    if the value is not tracked, then returns
    -1.
    
    Is thread safe by locking \ref mutex().

    \param v value to query.
   */
  int
  slot_location(T v) const
  {
    int I;

    WRATHLockMutex(m_mutex);
    I=slot_location_nolock(v);
    WRATHUnlockMutex(m_mutex);

    return I;
  }

  /*!\fn int add_element(T)
    Adds a reference count to an value if it is
    already tracked, if the value is not tracked
    and a slot is free, then the value is tracked
    and it's reference count is initialized as 1.
    Returns the slot number for the value if it
    tracked or added to tracking, -1 if the value
    is not tracked and there are no free slots.
    
    Is thread safe by locking \ref mutex().

    \param v value to track or add reference count 
   */
  int
  add_element(T v)
  {
    int R;

    WRATHLockMutex(m_mutex);
    R=add_element_nolock(v);
    WRATHUnlockMutex(m_mutex);

    return R;
  }

  /*!\fn enum return_code remove_element(T)
    Decrement the reference count associated to an value.
    if the reference count is decremented to 0, then stop
    tracking the value and free it's slot for another value.
    Returns \ref routine_fail if the value was not being actively 
    tracked and \ref routine_success otherwise.
    
    Is thread safe by locking \ref mutex().

    \param v value to decrement the reference count
   */
  enum return_code
  remove_element(T v)
  {
    enum return_code r;

    WRATHLockMutex(m_mutex);
    r=remove_element_nolock(v);
    WRATHUnlockMutex(m_mutex);

    return r;
  }

  /*!\fn T element_at_slot(int) const
    Returns the value occupying the named slot.
    
    Is thread safe by locking \ref mutex().

    \param slot slot number to query.
   */
  T
  element_at_slot(int slot) const
  {
    T v;

    WRATHLockMutex(m_mutex);
    v=element_at_slot_nolock(slot);
    WRATHUnlockMutex(m_mutex);

    return v;
  }
  
  /*!\fn bool free_slots_available
    Returns true if there are still free slots available.
    Is thread safe by locking \ref mutex().   
   */
  bool
  free_slots_available(void) const
  {
    bool r;
    WRATHLockMutex(m_mutex);
    r=free_slots_available_nolock();
    WRATHUnlockMutex(m_mutex);
    return r;
  }

  /*!\fn int total_slots
    Returns the number of slots that this WRATHSlotAllocator
    has, i.e. the maximum number of unique values that can 
    be tracked.
    Is thread safe by locking \ref mutex().
   */
  int
  total_slots(void) const
  {
    int r;

    WRATHLockMutex(m_mutex);
    r=total_slots_nolock();
    WRATHUnlockMutex(m_mutex);

    return r;
  }

  /*!\fn bool slot_allocated_for_value(T) const
    Returns true if the value is tracked. 
    Is thread safe by locking \ref mutex().
    \param v value to query if it is tracked or not.
   */
  bool
  slot_allocated_for_value(T v) const
  {
    bool r;

    WRATHLockMutex(m_mutex);
    r=slot_allocate_for_value_nolock(v);
    WRATHUnlockMutex(m_mutex);

    return r;
  }

  /*!\fn WRATHMutex& mutex
    Return the WRATHMutex used for locking.
   */
  WRATHMutex&
  mutex(void) const
  {
    return m_mutex;
  }

  /*!\fn void clear
    Clears all slots marking them as free
   */
  void
  clear(void) 
  {
    WRATHAutoLockMutex(m_mutex);
    m_active.clear();
    m_free_slots.clear();
    m_active_as_nodes.clear();
  }

  /*!\fn int highest_slot_allocated
    Returns the highest slot number
    currently allocated.
   */
  int
  highest_slot_allocated(void) const
  {
    WRATHAutoLockMutex(m_mutex);
    return m_active_as_nodes.size() - 1;
  }

private:
  
  bool
  free_slots_available_nolock(void) const
  {
    return static_cast<int>(m_active_as_nodes.size())<m_max_size
      or !m_free_slots.empty();
  }

  
  bool
  slot_allocate_for_value_nolock(T v) const
  {
    return m_active.find(v)!=m_active.end();
  }
  
  int
  total_slots_nolock(void) const
  {
    return m_max_size;
  }

  int
  allocate_slot_nolock(T v)
  {
    if(!m_free_slots.empty())
      {
        int return_value;
        
        return_value=*m_free_slots.begin();
        m_active_as_nodes[return_value]=v;
        
        m_free_slots.erase(m_free_slots.begin());
        
        WRATHassert(return_value<m_max_size);
        return return_value;
      }
    else
      {
        int return_value;
        
        return_value=m_active_as_nodes.size();
        m_active_as_nodes.push_back(v);
        
        WRATHassert(return_value<m_max_size);
        return return_value;
      }
  }
  
  void
  free_slot_nolock(int slot)
  {
    WRATHassert(slot>=0);

    unsigned int sl(slot);
    
    WRATHassert(sl<m_active_as_nodes.size());
    WRATHassert(m_free_slots.find(slot)==m_free_slots.end());
    
    if(sl==m_active_as_nodes.size() - 1)
      {
        m_active_as_nodes.pop_back();
      }
    else
      {
        m_active_as_nodes[sl]=NULL;
        m_free_slots.insert(sl);
      }
  }  

  enum return_code
  remove_element_nolock(T v)
  {
    typename std::map<T, per_node_data>::iterator iter;

    WRATHassert(v!=NULL);

    iter=m_active.find(v);
    if(iter!=m_active.end())
      {
        WRATHassert(iter->second.m_reference_count>0);
        
        --iter->second.m_reference_count;
        if(iter->second.m_reference_count==0)
          {
            free_slot_nolock(iter->second.m_location);
            m_active.erase(iter);
          }
        return routine_success;
      }
    else
      {
        return routine_fail;
      }
  }

  
  T
  element_at_slot_nolock(int slot) const
  {

    WRATHassert(slot>=0);
    WRATHassert(static_cast<int>(m_active_as_nodes.size())>slot);
    return m_active_as_nodes[slot];
  }

  int
  add_element_nolock(T v)
  {
    typename std::map<T, per_node_data>::iterator iter;

    WRATHassert(v!=NULL);
  
    iter=m_active.find(v);
    if(iter!=m_active.end())
      {
        ++iter->second.m_reference_count;
        return iter->second.m_location;
      }
    else
      {
        if(m_max_size > static_cast<int>(m_active.size()))
          {
            int return_value;
            
            return_value=allocate_slot_nolock(v);
            m_active[v]=per_node_data(return_value);
            return return_value;
          }
        else
          {
            WRATHassert(0);
            return -1;
          }
      }
  }


  
  enum return_code
  accepts_element_nolock(T v) const
  {
    WRATHassert(v!=NULL);

    if(m_max_size > static_cast<int>(m_active.size())
       or m_active.find(v)!=m_active.end())
      {
        return routine_success;
      }
    else
      {
        return routine_fail;
      }
  }

  int
  slot_location_nolock(T v) const
  {
    typename std::map<T, per_node_data>::const_iterator iter;

   
    iter=m_active.find(v);
    if(iter!=m_active.end())
      {
        return iter->second.m_location;
      }
    else
      {
        return -1;
      }
  }

  
  int m_max_size;
  std::map<T, per_node_data> m_active;
  std::vector<T> m_active_as_nodes;
  std::set<int> m_free_slots;
  mutable WRATHMutex m_mutex;
};

/*! @} */


#endif
