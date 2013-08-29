/*! 
 * \file WRATHBufferAllocator.hpp
 * \brief file WRATHBufferAllocator.hpp
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




#ifndef __WRATH_BUFFER_ALLOCATOR_HPP__
#define __WRATH_BUFFER_ALLOCATOR_HPP__

#include "WRATHConfig.hpp"
#include <map>
#include <list>
#include <set>
#include <iostream>
#include <string>
#include <boost/utility.hpp>

#include "c_array.hpp"
#include "WRATHNew.hpp"
#include "WRATHTripleBufferEnabler.hpp"
#include "WRATHBufferObject.hpp"
#include "type_tag.hpp"
#include "WRATHAbstractDataSink.hpp"

/*! \addtogroup GLUtility
 * @{
 */

/*!\class WRATHBufferAllocator
  A WRATHBufferAllocator is a book keeper attached
  to a WRATHBufferObject. A WRATHBufferAllocator
  presents an "allocate" and "de-allocate" 
  API. Underneath, it does as follows:
  - When a range is de-allocated it adds
     that range to a list of "free ranges",
     possibly merging the range with a pre-exising
     free-range. If the freed range ends at the end
     of the WRATHBufferObject, then WRATHBufferObject::resize()
     is executed.\n\n
  - When a block is allocated, it first looks
    at it's list of free ranges, if a free range
    is big enough to hold that allocation it is 
    used and the offset is returned, otherwise
    WRATHBufferObject::resize() is called.\n\n
  - For fragmented allocation, the WRATHBufferAllocator
    uses the smallest free blocks and continues
    to use the free blocks until the number of bytes
    needed is consumed or until there are no more
    free blocks remaining. For the latter case,
    the remaining needed to allocate is accomplished
    by resizing the underlying WRATHBufferObject

  One important comment: Given a postive integer N,
  if the allocations are always multiples of N and
  the deallocation marks are always also multiples of
  N, then locations returned by allocate() and 
  fragmented_allocate() are also always multiples 
  of N. Hence, a WRATHBufferAllocator is suitable 
  for allocating for data aligned to the size of a
  data type.

  Lastly, it is an _error_ to resize the WRATHBufferObject
  of a WRATHBufferAllocator directly. Indeed, outside
  of binding and flushing the buffer object, one should
  never access the WRATHBufferObject of a WRATHBufferAllocator.

  Class is thread safe by performing all operations
  of it's public methods behind a WRATHMutex that
  is made public, see \ref mutex(void).
 */
class WRATHBufferAllocator:public WRATHTripleBufferEnabler::PhasedDeletedObject
{
public:

  /*!\class DataSink
    Implementation of DataSink to read and write
    the data of a WRATHBufferAllocator. Uses
    the same mutex as the WRATHBufferAllocator from
    which it was created. Is essentially a wrapper
    of a pointer to a \ref WRATHBufferAllocator and
    is copyable with very light copy operation
    (i.e. opying a pointer).
   */
  class DataSink:public WRATHAbstractDataSink
  {
  public:
    /*!\fn DataSink
      Ctor.       
      \param q WRATHBufferAllocator to which to manipulate data               
     */
    explicit
    DataSink(WRATHBufferAllocator *q=NULL):
      m_buffer(q)
    {}

    virtual
    WRATHMutex*
    mutex(void)
    {
      WRATHassert(m_buffer!=NULL);
      return &m_buffer->mutex();
    }

    virtual
    c_array<uint8_t>
    byte_ptr(int byte_location, int number_bytes)
    {
      WRATHassert(m_buffer!=NULL);
      return m_buffer->pointer<uint8_t>(byte_location, number_bytes);
    }

    virtual
    const_c_array<uint8_t>
    c_byte_ptr(int byte_location, int number_bytes) const
    {
      WRATHassert(m_buffer!=NULL);
      return m_buffer->read_pointer<uint8_t>(byte_location, number_bytes);
    }

  private:
    WRATHBufferAllocator *m_buffer;
  };
  
  /*!\fn WRATHBufferAllocator(const WRATHTripleBufferEnabler::handle &h,
                              GLenum buffer_object_hint,
                              int max_size_in_bytes)
    Create a WRATHBufferAllocator which will
    limit the size of the underlying
    WRATHBufferObject, the underlying
    WRATHBufferObject is owned by this 
    WRATHBufferAllocator. 
    \param h handle to a WRATHTripleBufferEnabler that the created
             WRATHBufferObject will use for coordinating deleting
             the underlying GL buffer object.
    \param buffer_object_hint buffer object hint passed to
                              ctor of WRATHBufferObject.
    \param max_size_in_bytes maximum size in bytes
                             for the underlying WRATHBufferObject
   */
  WRATHBufferAllocator(const WRATHTripleBufferEnabler::handle &h,
                       GLenum buffer_object_hint,
                       int max_size_in_bytes);

  /*!\fn WRATHBufferAllocator(const WRATHTripleBufferEnabler::handle &h,
                              GLenum buffer_object_hint=GL_STATIC_DRAW)
    Create a WRATHBufferAllocator, the underlying
    WRATHBufferObject is owned by this WRATHBufferAllocator.

    \param h handle to a WRATHTripleBufferEnabler that the created
             WRATHBufferObject will use for coordinating deleting
             the underlying GL buffer object.
    \param buffer_object_hint buffer object hint passed to
                              ctor of WRATHBufferObject.
   */
  explicit
  WRATHBufferAllocator(const WRATHTripleBufferEnabler::handle &h,
                       GLenum buffer_object_hint=GL_STATIC_DRAW);


  ~WRATHBufferAllocator();

  /*!\fn WRATHBufferObject* buffer_object
    Returns the underlying buffer object.
    Can be called from a different thread than the GL context.
    Call does not require any locking either.
    This WRATHBufferAllocator *owns* the WRATHBufferObject
    and it is an error to resize the returned WRATHBufferAllocator.
    The WRATHBufferAllocator is the only object allowed to resize
    the WRATHBufferObject it owns.
   */
  WRATHBufferObject*
  buffer_object(void)
  {
    return m_buffer_object;
  }

  /*!\fn int max_buffer_object_size
    Returns the maximum allowed buffer object size.
    Can be called from a different thread than the GL context.
    Call does not require any locking either.
   */
  int
  max_buffer_object_size(void) const
  {
    //TODO: likely we should have a method for
    //checking if the size can actually has
    //a maximum limit, but in the case it is
    //not, m_max_buffer_object_size.second is
    //set to std::numeric_limits<int>::max() which
    //is 2GB.
    return m_max_buffer_object_size.second;
  }
  
  /*!\fn range_type<int> allocated_range
    Returns the range of bytes that are allocated.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
   */
  range_type<int>
  allocated_range(void) const;

  /*!\fn range_type<int> allocated_range_nolock
    Returns the range of bytes that are allocated.
    Can be called from a different thread than the GL context.
    In contrast to \ref allocated_range(void) const ,
    does not lock \ref mutex() for the duration of the call.
  */
  range_type<int>
  allocated_range_nolock(void) const;

  /*!\fn int allocate
    "Allocates" bytes and returns
    an offset(in bytes) to where the 
    block is located, the return
    value -1 indicates allocation
    failure.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
    \param number_bytes number of bytes to allocate
   */
  int
  allocate(int number_bytes);

  /*!\fn enum return_code fragmented_allocate
    "Allocates" bytes, but allows for the allocation
    to be fragmented, i.e. across many blocks. Returns 
    routine_success on success and routine_fail on failure.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
    \param number_bytes number of bytes to allocate
    \param out_allocations on allocation sucess, _appends,
                           the lcoation of the fragments
                           of the allocating as a range_type
                           (i.e. marking the beginning and
                           ending of the fragmented allocation).
   */
  enum return_code
  fragmented_allocate(int number_bytes,
                      std::vector< range_type<int> > &out_allocations);

  /*!\fn enum return_code proxy_allocate
    Tests if the WRATHBufferAllocator can allocate
    the specified number of bytes in one block. 
    Returns routine_success if allocate(int) 
    would succeed and routine_fail if it would
    not.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
    \param number_bytes allocation size to test.
   */
  enum return_code
  proxy_allocate(int number_bytes);

  /*!\fn enum return_code proxy_fragmented_allocate
    Tests if the WRATHBufferAllocator can allocate
    the specified number of bytes in multiple blocks. 
    Returns routine_success if proxy_allocate() 
    would succeed and routine_fail if it would
    not.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
    \param number_bytes allocation size to test.
   */
  enum return_code
  proxy_fragmented_allocate(int number_bytes);

  /*!\fn int max_cts_allocate_possible
    Returns the maximum number of continuous
    bytes that can be alloated *now*, i.e
    allocate() is gauranteed to succeeed
    (and poxy_allocate to return routine_success)
    with the any value less than or equal
    to the return value of max_cts_allocate_possible().
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
   */
  int
  max_cts_allocate_possible(void);

  /*!\fn int max_fragmented_allocate_possible
    Returns the maximum number of bytes
    allocatable by fragmented_allocate().
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
   */
  int
  max_fragmented_allocate_possible(void);
  
  /*!\fn void deallocate
    Mark a range of bytes as de-allocated.
    It is an error (which goes undetected)
    for any block of data to be deallocated
    twice.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
    \param begin_byte first byte to mark as free
    \param end_byte one past the last byte to 
                    mark as free, i.e. mark the 
                    [begin_byte, end_byte) as 
                    free.
   */
  void
  deallocate(int begin_byte, int end_byte);  
  
  /*!\fn const_c_array<T> read_pointer
    Return a const pointer to the named location.
    The pointer is gauranteed to be valid until
    the next allocation or deallocation call to this 
    WRATHBufferAllocator. Notes:
    - this routine is NOT thread safe, but can be made
      thread safe by using \ref mutex() to perform
      locking
    - the return value can become invalid if allocate()
      or fragmented_allocate() are called on this
      WRATHBufferAllocator. As such, a user needs to
      guarantee that does not happen. The easiest way
      in a multi-threaded environment is to lock 
      \ref mutex() before calling read_pointer() and
      to then unlock mutex() once the user is not using
      the return value. Doing so is THE thread safe
      way to use \ref read_pointer().
    \param byte_location location in bytes of the block of memory
    \param number_elements number of elements starting at the named
           location to manipulate through the returned c_array.
   */
  template<typename T>
  const_c_array<T>
  read_pointer(int byte_location, int number_elements) const
  {
    WRATHassert(byte_location>=0);
    const T *ptr;

    #ifdef WRATH_VECTOR_BOUND_CHECK
    {
      int end_byte;
      end_byte=byte_location+sizeof(T)*number_elements;
      WRATHassert(number_elements==0 or block_is_allocated_nolock(byte_location, end_byte));
    }
    #endif
    ptr=reinterpret_cast<const T*>(m_buffer_object->c_ptr(byte_location));
    return const_c_array<T>(ptr, number_elements);
  }

  /*!\fn c_array<T> pointer
    Return a non-const pointer to the named location.
    The pointer is gauranteed to be valid until
    the next allocation or deallocation call to this 
    WRATHBufferAllocator. Requesting the write pointer
    marks the bytes it refers to within the 
    underlying WRATHBufferObject as dirty, as such
    one should not save and re-use the return value
    (that and because the pointer can become invalid
    as soon as any of the allocate() methods are called).
    Some notes:
    - this routine is NOT thread safe, but can be made
      thread safe by using \ref mutex() to perform
      locking  
    - the return value can become invalid if allocate()
      or fragmented_allocate() are called on this
      WRATHBufferAllocator. As such, a user needs to
      guarantee that does not happen. The easiest way
      in a multi-threaded environment is to lock 
      \ref mutex() before calling pointer() and
      to then unlock mutex() once the user is not using
      the return value. Doing so is THE thread safe
      way to use \ref pointer(int, int).

    \param byte_location location in bytes of the block of memory
    \param number_elements number of elements starting at the named
           location to manipulate through the returned c_array.
   */
  template<typename T>
  c_array<T>
  pointer(int byte_location, int number_elements)
  {
    WRATHassert(byte_location>=0);

    T *ptr;
    int end_byte;
    end_byte=byte_location+sizeof(T)*number_elements;
    WRATHassert(number_elements==0 or block_is_allocated_nolock(byte_location, end_byte));
    m_buffer_object->mark_bytes_dirty_no_lock(byte_location, end_byte);
    ptr=reinterpret_cast<T*>(m_buffer_object->c_ptr(byte_location));   
    return c_array<T>(ptr, number_elements);
  }
  
  /*!\fn int freeblocks_total_size
    A WRATHBufferAllocater maintains a list of
    blocks with the WRATHBufferObject that have
    NOT been allocated. When allocation is
    requested, first that free list is consulted
    and if there is an element on that free list
    of atleast the needed size, then the block
    come from that free element. If there
    is not free element of sufficient size 
    available, then the WRATHBufferObject is
    enlarged. The methed freeblocks_total_size()
    returns the sum of sizes of all free
    blocks.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
   */
  int
  freeblocks_total_size(void) const
  {
    return m_total_free_room;
  }

  /*!\fn int freeblock_count
    A WRATHBufferAllocater maintains a list of
    blocks with the WRATHBufferObject that have
    NOT been allocated. When allocation is
    requested, first that free list is consulted
    and if there is an element on that free list
    of atleast the needed size, then the block
    come from that free element. If there
    is not free element of sufficient size 
    available, then the WRATHBufferObject is
    enlarged. The methed freeblock_count()
    returns the number of free blocks.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
   */
  int
  freeblock_count(void) const;
  
  /*!\fn bool block_is_allocated
    Returns true if the specified range
    in bytes is allocated, and thus
    having well defined values.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
    \param begin first byte
    \param end one past the last byte,
               i.e. checks if the range
               [begin,end) is allocated.
   */
  bool
  block_is_allocated(int begin, int end) const;

  /*!\fn int bytes_allocated
    Returns the total number of bytes
    allocated on this WRATHBufferAllocator.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
   */
  int
  bytes_allocated(void) const;

  /*!\fn void clear
    Deallocates ALL data allocated from this WRATHBufferAllocator.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
   */
  void
  clear(void);

  /*!\fn WRATHMutex& mutex
    Returns the WRATHMutex used by this WRATHBufferAllocator
    and it's underlying WRATHBufferObject 
    (\ref buffer_object(void) ) for locking.
   */
  WRATHMutex&
  mutex(void)
  {
    return m_mutex;
  }

  /*!\fn void print_free_block_info
    Print the free block information to an std::ostream.
    Can be called from a different thread than the GL context.
    Call is thread safe because it locks \ref mutex() 
    during the duration of the call.
    \param ostr std::ostream to print to.
    \param prefix all new lines are prefixed with this string.
   */
  void
  print_free_block_info(std::ostream &ostr, const std::string &prefix="") const;

  /*!\fn DataSink& data_sink
    Returns a \ref DataSink object that manipulates
    the data of this WRATHBufferAllocator
   */
  DataSink&
  data_sink(void)
  {
    return m_data_sink;
  }

protected:

  void
  on_place_on_deletion_list(void);


private:

  typedef std::map<int, range_type<int> >::iterator free_block_iter;
  typedef std::map<int, range_type<int> >::const_iterator free_block_iter_const;


  class compare_block_iters
  {
  public:
    bool
    operator()(std::map<int, range_type<int> >::iterator a,
               std::map<int, range_type<int> >::iterator b) const
    {
      return a->first < b->first;
    }
  };

  typedef std::map<int, std::set<free_block_iter, compare_block_iters> > map_type;

  void
  remove_free_block_from_available_list(free_block_iter);

  void
  insert_free_block_to_available_list(free_block_iter);

  void
  resize_buffer_object_nolock(int new_size);

  enum return_code
  proxy_allocate_nolock(int number_bytes);

  enum return_code
  proxy_fragmented_allocate_nolock(int number_bytes);

  int
  max_fragmented_allocate_possible_nolock(void);

  int
  max_cts_allocate_possible_nolock(void);

  bool
  block_is_allocated_nolock(int begin, int end) const;

  int
  allocate_nolock(int number_bytes);

  enum return_code
  fragmented_allocate_nolock(int number_bytes,
                             std::vector< range_type<int> > &out_allocations);

  void
  deallocate_nolock(int begin_byte, int end_byte);

  void
  clear_nolock(void);

  void
  print_free_block_info_nolock(std::ostream &ostr, 
                               const std::string &prefix) const;

  /*
    conveniance function to find a set of
    freeblocks whose size are at each sz_in_bytes
  */
  map_type::iterator 
  sorted_free_blocks_lower_bound(int sz_in_bytes);

  //m_free_blocks[n]= free block whose _end_ is n
  // i.e. m_free_blocks[n].m_end=n.
  std::map<int, range_type<int> > m_free_blocks;

  // m_sorted_free_blocks[n]=free blocks of size n
  // one unfortunate ugly of our implementation is
  // that the freestore essentially takes O(N) memory
  // where N=#free blocks, such is life.
  std::map<int, std::set<free_block_iter, compare_block_iters> > m_sorted_free_blocks;

  std::pair<bool, int> m_max_buffer_object_size;

  int m_total_free_room;

  mutable WRATHMutex m_mutex;
  WRATHBufferObject *m_buffer_object;
  int m_bytes_allocated;

  DataSink m_data_sink;
};
/*! @} */


#endif
