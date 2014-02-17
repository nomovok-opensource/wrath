/*! 
 * \file WRATHBufferObject.hpp
 * \brief file WRATHBufferObject.hpp
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




#ifndef WRATH_HEADER_BUFFER_OBJECT_HPP_
#define WRATH_HEADER_BUFFER_OBJECT_HPP_

#include "WRATHConfig.hpp"
#include <vector>
#include <boost/utility.hpp>
#include <stdlib.h>
#include <cstring>
#include "WRATHTripleBufferEnabler.hpp"
#include "WRATHassert.hpp" 
#include "WRATHMutex.hpp"
#include "WRATHgl.hpp"

/*! \addtogroup GLUtility
 * @{
 */


/*!\class WRATHBufferObject
  A WRATHBufferObject is a buffer object interface
  that tracks what is stored within the buffer object
  as well, this is necessary as the GLES2 API has no
  entry points to read back from a buffer object.
 */
class WRATHBufferObject:public WRATHTripleBufferEnabler::PhasedDeletedObject
{
public:

  /*!\fn WRATHBufferObject
    Ctor. Creates a WRATHBufferObject optionally backed
    by a GL buffer object. The creation of the underlying
    GL buffer object (if there will be one) is done the
    first time the bind() method is called, hence it is 
    safe to create WRATHBufferObject in a separate thread
    than the GL context.
    \param usage determines if there is a buffer object, and if so
                 its usage hint provided to GL. If the value of
                 usage is GL_INVALID_ENUM, then the WRATHBufferObject
                 will NOT be backed by a GL buffer object, otherwise
                 the enumeration is passed directly to glBufferData
                 in creation of the buffer object.
    \param h handle to a WRATHTripleBufferEnabler that the created
             WRATHBufferObject will use for coordinating deleting
             the underlying GL buffer object.
    \param pmutex if pmutex is non-NULL, then calls that read
                  or use the client side clone of the data are
                  locked by pmutex. These calls are:
                  - \ref resize
                  - \ref size
                  - \ref is_dirty
                  - \ref mark_bytes_dirty
                  - \ref flush
                  - \ref bind
                       
   */
  explicit
  WRATHBufferObject(const WRATHTripleBufferEnabler::handle &h,
                    GLenum usage=GL_STATIC_DRAW,
                    WRATHMutex *pmutex=NULL);
  
  ~WRATHBufferObject(void);

  /*!\fn GLuint name
    Returns the GL name (i.e. the 32 bit integer
    used by GL to identifiy the buffer object).
    If there is no GL buffer object backing the
    WRATHBufferObject, returns 0, which under GLES2
    and GL compatibility profile, is used to indicate
    to source from client side memory and not a
    buffer object. Additionally, the underlying
    GL buffer object of a WRATHBufferObject is
    NOT created until the first time the buffer
    object is bound, thus name() will always return
    0 until the first time the buffer object is
    bound. 
   */
  GLuint
  name(void) const
  {
    return m_name;
  }

  /*!\fn bool has_buffer_object_on_bind
    Returns true if this WRATHBufferObject is
    to be backed by a GL buffer object.
    Note that checking that name() is 
    non-zero is not sufficient beause the
    buffer object is not created until
    the first time the WRATHBufferObject
    is bound.
   */
  bool
  has_buffer_object_on_bind(void) const
  {
    return m_usage!=GL_INVALID_ENUM;
  }

  /*!\fn int size
    Returns the size of the buffer object,
    the size is guaranteed to be a multiple
    of 4. May be called from a thread outside
    of the GL context. If the WRATHBufferObject
    has a WRATHMutex (see \ref mutex), that mutex
    is locked for the duration of the call.
   */
  int
  size(void) const;

  /*!\fn int size_no_lock
    Same as \ref size() except that it does not
    perform locking on \ref mutex() for the
    duration of the call.
   */
  int
  size_no_lock(void) const;

  /*!\fn void resize
    Resizes this WRATHBufferObject, note that 
    resizing to a smaller size, like std::vector 
    does not actually free memory. Additionally if 
    this  WRATHBufferObject is backed by a GL buffer 
    object then the buffer object's resize will 
    only be deferred until flush() is called AND 
    it will only be resized to larger sizes.
    May be called from a thread outside
    of the GL context. If the WRATHBufferObject
    has a WRATHMutex (see \ref mutex), that mutex
    is locked for the duration of the call.
    \param new_size_in_bytes 
   */
  void
  resize(int new_size_in_bytes);

  /*!\fn void resize_no_lock
    Same as \ref resize(int) except that it does not
    perform locking on \ref mutex() for the
    duration of the call.
    \param new_size_in_bytes 
   */
  void
  resize_no_lock(int new_size_in_bytes);

  /*!\fn bool is_dirty
    Returns true if the GL buffer object does not have the 
    same contents as the internal buffer.
    May be called from a thread outside
    of the GL context. If the WRATHBufferObject
    has a WRATHMutex (see \ref mutex), that mutex
    is locked for the duration of the call. 
   */
  bool
  is_dirty(void) const;

  /*!\fn bool is_dirty_no_lock
    Same as \ref is_dirty() except that it does not
    perform locking on \ref mutex() for the
    duration of the call.
   */
  bool
  is_dirty_no_lock() const;

  /*!\fn const uint8_t* c_ptr(int) const
    Return a const pointer to the byte location
    specified, the pointer is only guaranteed
    to be valid until resize() is called.    
    May be called from a thread outside
    of the GL context. Note! It is a user
    of WRATHBufferObject's responsibility
    to not read and write to that buffer
    (or resize it) from multiple threads
    at the same time. If this WRATHBufferObject
    has a WRATHMutex (see \ref mutex), then it is
    strongly advised to use that WRATHMutex for
    locking.
    \param byte_location offset in bytes to where pointer will point to.
   */
  const uint8_t*
  c_ptr(int byte_location) const
  {
    return raw_data_pointer()+byte_location;
  }

  /*!\fn uint8_t* c_ptr(int)
    Return a const pointer to the byte location
    specified, the pointer is only guaranteed
    to be valid until resize() is called.   
    May be called from a thread outside
    of the GL context. Note! It is a user
    of WRATHBufferObject's responsibility
    to not read and write to that buffer
    (or resize it) from multiple threads
    at the same time. If this WRATHBufferObject
    has a WRATHMutex (see \ref mutex), then it is
    strongly advised to use that WRATHMutex for
    locking.
    \param byte_location offset in bytes to where pointer will point to.
   */
  uint8_t*
  c_ptr(int byte_location) 
  {
    return raw_data_pointer()+byte_location;
  }

  /*!\fn void mark_bytes_dirty
    Marks a range of bytes of the buffer object as dirty
    and to be re-uploaded to GL (such as when changing values
    via the return value of c_ptr()). Note the each of the
    write_raw_values() and write_raw_memset() will interally
    call mark_bytes_dirty() on the range of memory the affect.
    May be called from a thread outside
    of the GL context. If the WRATHBufferObject
    has a WRATHMutex (see \ref mutex), that mutex
    is locked for the duration of the call.
    \param begin_byte_location 1st byte to mark dirty
    \param end_byte_location one past the last byte to mark dirty,
                             i.e. mark bytes [begin_byte_location, end_byte_location)
                             as dirty
   */
  void
  mark_bytes_dirty(int begin_byte_location, int end_byte_location);

  /*!\fn void mark_bytes_dirty_no_lock
    Same as \ref mark_bytes_dirty(int, int) except that it does not
    perform locking on \ref mutex() for the
    duration of the call.
   */
  void
  mark_bytes_dirty_no_lock(int begin_byte_location, int end_byte_location);

  /*!\fn bool flush
    Use the named binding point to flush changes
    to buffer object, will return true if the
    buffer object is left bound. Returns
    true if after the flush, the buffer object
    is bound. Must be called from the thread of the 
    GL context. If the WRATHBufferObject
    has a WRATHMutex (see \ref mutex), that mutex
    is locked for the duration of the call.

    \param bind_target binding location to use to bind the
                       buffer object inorder to flush
                       client side changes.
   */
  bool
  flush(GLenum bind_target);

  /*!\fn bool flush_no_lock
    Same as \ref flush(GLenum) except that it does not
    perform locking on \ref mutex() for the
    duration of the call.
   */
  bool
  flush_no_lock(GLenum bind_target);

  /*!\fn void bind
    Flushed and binds the buffer object
    to the named binding point.
    Must be called from the thread of the 
    GL context. If the WRATHBufferObject
    has a WRATHMutex (see \ref mutex), the call 
    is also threadsafe behind that WRATHMutex.
    \param bind_target buffer binding point to
                       bind the buffer object to
   */
  void
  bind(GLenum bind_target);

  /*!\fn mutex
    Returns the underlying WRATHMutex that
    this WRATHBufferObject uses for locking.
    It is exposed so that one can use the
    return value of \ref c_ptr(int) and 
    \ref c_ptr(int) const safely behind
    a lock. The return value is NULL if
    the WRATHBufferObject was not given
    a WRATHMutex at it's construction.
   */
  WRATHMutex*
  mutex(void) 
  {
    return m_mutex;
  }

  /*!\fn const void* offset_pointer
    Returns a const void* value as follows:
    - if there is no backing GL buffer object,
      returns same value as c_ptr(int byte_offset)
    - if there is a backing GL buffer object,
      returns essentially (char*)(NULL) +   byte_offset

    For example, if the buffer object is used
    to store attribute data, after issuing bind(),
    the return value is suitable for the last
    argument to glVertexAttribPointer().

    Note that return value can change if
    _both_ the WRATHBufferObject is 
    not backed by a GL buffer object
    and it's size changes. In particular,
    in a multi-threaded environment, an 
    application should lock with \ref
    mutex() when the WRATHBufferObject
    is not backed by a GL buffer object.
    
    Should only be called from the thread 
    of the GL context. Call is NOT locked
    by \ref mutex(). 
    \param byte_offset number of bytes from beginning
                       of buffer object.
   */
  const void*
  offset_pointer(int byte_offset);

  /*!\fn unsigned int total_bytes_uploaded
    Returns the total number of bytes uploaded
    to GL via all WRATHBufferObjects.
   */
  static
  unsigned int
  total_bytes_uploaded(void);

protected:

  virtual
  void
  phase_render_deletion(void);

private:

  /*
    tracking of dirty regions is via a list
    of of regions that are dirty. Neighboring
    regions are always merged.

    m_dirty_blocks[n]=dirty block whose _end_
                      is n, i.e. m_dirty_blocks[n].m_end=n always
   */
  bool m_dirty;
  std::map<int, range_type<int> > m_dirty_blocks;

  GLuint m_name;
  GLenum m_usage;
  int m_buffer_object_size_in_bytes;
  int m_virtual_size, m_cache_size;

  //epic GLES2 failure, one cannot fetch the
  //buffer object contents.
  //The irony here is even deeper, usually in GLES2
  //implementations, the memory arch is unified.
  //data is stored as uint32_t to get 4 byte alignment.
  std::vector<uint32_t> m_cached_data;


  WRATHMutex *m_mutex;

  const uint8_t*
  raw_data_pointer(void) const
  {
    return (!m_cached_data.empty())?
      reinterpret_cast<const uint8_t*>(&m_cached_data[0]):
      NULL;
  }

  uint8_t*
  raw_data_pointer(void) 
  {
    return (!m_cached_data.empty())?
      reinterpret_cast<uint8_t*>(&m_cached_data[0]):
      NULL;
  }
};

/*! @} */


#endif
