/*! 
 * \file WRATHIndexGroupAllocator.hpp
 * \brief file WRATHIndexGroupAllocator.hpp
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




#ifndef __WRATH_INDEX_GROUP_ALLOCATOR_HPP__
#define __WRATH_INDEX_GROUP_ALLOCATOR_HPP__

#include "WRATHConfig.hpp"
#include "WRATHBufferAllocator.hpp"
#include "WRATHAttributeStore.hpp"
#include "WRATHDrawCommand.hpp"
#include "WRATHAbstractDataSink.hpp"
#include "type_tag.hpp"

/*! \addtogroup Group
 * @{
 */


/*!\class WRATHIndexGroupAllocator
  A WRATHIndexGroupAllocator allocates indices
  in continuous chunks. Each such chunk
  is represented by a \ref index_group
  (which itself is really a POD type). 

  It is expected that this class is only
  used directly by implementation of 
  \ref WRATHCanvas
 */
class WRATHIndexGroupAllocator:
  public WRATHReferenceCountedObjectT<WRATHIndexGroupAllocator>
{
private:
  class index_chunk;

public:

  /*!\class DataSink
    \ref WRATHAbstractDataSink derived class for 
    manipulating index data of an \ref index_group 
    with the \ref WRATHAbstractDataSink interface
   */
  class DataSink:public WRATHAbstractDataSink
  {
  public:
    /*!\fn DataSink(void)
      Default ctor initializes the DataSink as invalid.
      Attempting to use the DataSink will assert in 
      debug builds.
     */
    DataSink(void):
      m_data(NULL)
    {}

    virtual
    WRATHMutex*
    mutex(void)
    {
      WRATHassert(m_data!=NULL);
      return &m_data->m_source->mutex();
    }
    
    virtual
    c_array<uint8_t>
    byte_ptr(int byte_location, int number_bytes)
    {
      WRATHassert(m_data!=NULL);
      const WRATHIndexGroupAllocator::handle &s(m_data->m_source); 
      WRATHassert(number_bytes <= s->index_type_size()*(m_data->m_range.m_end - m_data->m_range.m_begin));
      WRATHassert(byte_location>=0);
      WRATHassert(number_bytes>=0);
     
      byte_location+=m_data->m_range.m_begin*s->index_type_size();
      return s->m_index_buffer->pointer<uint8_t>(byte_location, number_bytes);
    }
    
    virtual
    const_c_array<uint8_t>
    c_byte_ptr(int byte_location, int number_bytes) const
    {
      WRATHassert(m_data!=NULL);
      const WRATHIndexGroupAllocator::handle &s(m_data->m_source);   
      WRATHassert(number_bytes <= s->index_type_size()*(m_data->m_range.m_end - m_data->m_range.m_begin));
      WRATHassert(byte_location>=0);
      WRATHassert(number_bytes>=0);
   
      byte_location+=m_data->m_range.m_begin*s->index_type_size();
      return s->m_index_buffer->read_pointer<uint8_t>(byte_location, number_bytes);
    }
      
  private:    
    friend class WRATHIndexGroupAllocator;
    explicit
    DataSink(index_chunk *q):
      m_data(q)
    {}

    index_chunk *m_data;
  };

  

  /*!\class index_group
    An index_group is an interface for
    setting and getting a set of indices.
    The indices are within a WRATHBufferObject,
    but their location within the 
    WRATHBufferObject is not static. To that
    end, the WRATHMutex used by the
    WRATHBufferObject is made available
    and the location returned by pointer()
    and read_pointer() can change and
    be invalid unless the mutex locked.
    An index_group itself is a handle
    to index data. In particular if two
    index_group values refer to the same
    index block and if one of them deletes
    it, then the other index_group will still
    point to the incorrect data.
    \tparam I (template parameter) index type, should be
              GLushort or GLubyte (GLuint is allowed in GL
              for desktop and those GLES implementation 
              supporting GL_OES_element_index_uint)
   */
  template<typename I>
  class index_group
  {
  public:
    /*!\fn index_group(void)
      Default ctor, returns an \ref  index_group
      that does not refer to an index block (yet).
     */
    index_group(void):
      m_data(NULL)
    {}

    /*!\fn bool valid
      Returns true if and only if the
      index_group does refer to
      an index block, however if that
      block was deleted elsewhere by a
      different \ref index_group,
      then this \ref index_group will
      point to a deleted index block,
      as such it will think it is "valid",
      but it will refer to an invalid
      memory location.
     */
    bool
    valid(void) const
    {
      return m_data!=NULL;
    }
    
    /*!\fn int size
      Returns the number of indices in
      the index group, will WRATHassert if
      valid() is false.
    */
    int
    size(void) const
    {
      WRATHassert(valid());
      return m_data->m_range.m_end - m_data->m_range.m_begin;
    }

    /*!\fn WRATHMutex& mutex
      Returns the mutex used for locking the
      index data referred to by this \ref index_group,
      this is the same mutex as returned by
      WRATHIndexGroupAllocator::mutex() of the 
      \ref WRATHIndexGroupAllocator
      that created the \ref index_group.
     */
    WRATHMutex&
    mutex(void) const
    {
      WRATHassert(valid());
      return m_data->m_source->mutex();
    }

    /*!\fn bool same_mutex
      Since multiple index groups can come from
      the same source, when writing to multiple
      index groups, one needs to lock but
      avoid locking the same WRATHMutex multiple
      times. This routine returns true if another
      index group shares the same mutex.
      \param h index_group to which to compare
     */
    bool
    same_mutex(index_group h) const
    {
      return &mutex()==&h.mutex();
    }

    /*!\fn void copy(index_group, range_type<int>, int)
      Copy indices from another \ref index_group into this \ref index_group.
      Function performs mutex locking on this and the passed index_group
      for its duration.
      \param src_group index_group from which to copy
      \param src_range ranges within src_group from which to copy
      \param dest location within this to which to copy
     */
    void
    copy(index_group src_group, range_type<int> src_range, int dest=0)
    {
      WRATHassert(valid());
      WRATHassert(src_group.valid());

      WRATHLockMutex(mutex());
      if(!same_mutex(src_group))
        {
          WRATHLockMutex(src_group.mutex());
        }

      c_array<I> dest_ptr;
      const_c_array<I> src_ptr;
      src_ptr=src_group.pointer().sub_array(src_range);
      dest_ptr=pointer().sub_array(dest);
      std::copy(src_ptr.begin(), src_ptr.end(), dest_ptr.begin());

      if(!same_mutex(src_group))
        {
          WRATHUnlockMutex(src_group.mutex());
        }
      WRATHUnlockMutex(mutex());
    }

    /*!\fn void copy(index_group, int)
      Copy indices from another \ref index_group into this index_group.
      Function performs mutex locking on this and the passed index_group
      for its duration. Provided as a conveniance, equivalent to
      \code
      copy(src_group, range_type<int>(0, src_group.size()), dest);
      \endcode
      \param src_group index_group from which to copy indices
      \param dest location within this to which to copy
     */
    void
    copy(index_group src_group, int dest=0)
    {
      copy(src_group, 
           range_type<int>(0, src_group.size()),
           dest);
    }

    /*!\fn const_c_array<I> read_pointer
      Returns a read only pointer to the data of the index
      group, the pointer is gauranteed to be valid until
      either index groups are added or removed from the
      underlying WRATHIndexGroupAllocator. In a multi-threaded
      environment, to guarantee that the pointer remains
      valid, preceed the call to read_pointer<I>(void) const
      with locking the WRATHMutex \ref mutex() and once
      reads are completed, unlock that WRATHMutex.
      This method is essentially a wrapper over 
      \ref WRATHBufferAllocator::read_pointer().
     */
    const_c_array<I>
    read_pointer(void) const
    {
      WRATHassert(valid());
      const WRATHIndexGroupAllocator::handle &s(m_data->m_source);
      return s->m_index_buffer->read_pointer<I>(m_data->m_range.m_begin*s->index_type_size(),
                                                m_data->m_range.m_end - m_data->m_range.m_begin);
    }

    /*!\fn c_array<I> pointer
      Returns a read/write pointer to the data of the index
      group, the pointer is gauranteed to be valid until
      either index groups are added or removed from the
      underlying WRATHIndexGroupAllocator. In a multi-threaded
      environment, to guarantee that the pointer remains
      valid, preceed the call to pointer<I>(void) const
      with locking the WRATHMutex \ref mutex() and once
      reads and writes are completed, unlock that WRATHMutex.
      This method is essentially a wrapper over 
      \ref WRATHBufferAllocator::pointer(int, int).
     */
    c_array<I>
    pointer(void) const
    {
      WRATHassert(valid());
      const WRATHIndexGroupAllocator::handle &s(m_data->m_source);
      return s->m_index_buffer->pointer<I>(m_data->m_range.m_begin*s->index_type_size(),
                                           m_data->m_range.m_end - m_data->m_range.m_begin);
    }

    /*!\fn void delete_group
      Deletes the index block that this
      \ref index_group refers to. 
      Afterwards, this index_group
      will not point to an index block,
      but if another index_grup
      refers to the same index block, those
      other index_group will be 
      analouges to wild pointers, i.e.
      same spirit as C++ operator delete.
     */
    void
    delete_group(void) 
    {
      WRATHassert(valid());
      {
        /*
          TODO:
          we can skip the write 0's to range if
          the underlying WRATHIndexGroupAllocator
          tracks ranges to send to GL 
        */
        WRATHAutoLockMutex(mutex());
        c_array<I> ptr;
        ptr=pointer();
        std::fill(ptr.begin(), ptr.end(), I(0));
      }

      /*
        make sure the handle m_source does not
        go out of scope until after deallocate_group_implement()
        returns:
       */
      WRATHIndexGroupAllocator::handle s(m_data->m_source);
      s->deallocate_group_implement(m_data);
      m_data=NULL;
    }

    /*!\fn DataSink data_sink
      Returns a \ref DataSink object that manipulates
      the index data of this \ref index_group.
      It is an error with undefined consequences to
      use a DataSink once the \ref index_group that
      created is destroyed with delete_group().
    */
    DataSink
    data_sink(void)
    {
      return DataSink(m_data);
    }

  private:
    friend class WRATHIndexGroupAllocator;

    explicit
    index_group(index_chunk *v):
      m_data(v)
    {}

    index_chunk *m_data;
  };


  /*!\fn WRATHIndexGroupAllocator(GLenum, WRATHBufferAllocator*, 
                                  const WRATHAttributeStore::handle &)
    Ctor. Creates an WRATHIndexGroupAllocator that uses a passed
    WRATHBufferAllocator for storing the indices of the \ref index_group
    objects that the WRATHIndexGroupAllocator creates
    \param primitive_type GL enumeration indicating the primitive type
                          to be fed to a GL draw call (for example
                          GL_TRIANGLES).
    \param pindex_buffer pointer to WRATHBufferAllocator where the indices
                         of the created \ref index_group will live
    \param pstore WRATHAttributeStore from which to get the index type
   */
  WRATHIndexGroupAllocator(GLenum primitive_type,
                           WRATHBufferAllocator *pindex_buffer,
                           const WRATHAttributeStore::handle &pstore);

  

  /*!\fn WRATHIndexGroupAllocator(GLenum, GLenum, 
                                  const WRATHAttributeStore::handle &)
    Ctor. Creates an WRATHIndexGroupAllocator that uses a private
    WRATHBufferAllocator for storing the indices of the \ref index_group
    objects that the WRATHIndexGroupAllocator creates
    \param primitive_type GL enumeration indicating the primitive type
                          to be fed to a GL draw call (for example
                          GL_TRIANGLES)
    \param pbuffer_object_hint buffer object hint for the WRATHBufferObject that
                               stores the indices
    \param pstore WRATHAttributeStore from which to get the index type
   */
  WRATHIndexGroupAllocator(GLenum primitive_type,
                           GLenum pbuffer_object_hint,
                           const WRATHAttributeStore::handle &pstore);


  ~WRATHIndexGroupAllocator();

  /*!\fn WRATHMutex& mutex
    Returns the WRATHMutex of the WRATHBufferObject
    holding the index data.
   */
  WRATHMutex&
  mutex(void) const
  {
    return m_index_buffer->mutex();
  }

  /*!\fn bool empty
    Returns true if each created \ref index_group
    created by this WRATHIndexGroupAllocator has
    been deleted.
   */
  bool
  empty(void) const;

  /*!\fn WRATHDrawCommand* draw_command
    Returns the \ref WRATHDrawCommand associated
    to the index data of this WRATHIndexGroupAllocator.
   */
  WRATHDrawCommand*
  draw_command(void) const
  {
    return m_draw_command;
  }

  /*!\fn const WRATHAttributeStore::handle& attribute_store
    At construction, a WRATHIndexGroupAllocator is 
    linked to a WRATHAttributeStore. That attribute
    store determines the expected index type
    of the WRATHIndexGroupAllocator. Returns
    the attribute strore to which this
    WRATHIndexGroupAllocator is linked.
   */
  const WRATHAttributeStore::handle&
  attribute_store(void) const
  {
    return m_attribute_store;
  }
  
  /*!\fn index_group<I> allocate_index_group 
    Allocate index date, the returned handle is used to
    set the values, all values are initialized as 0.
    Returns an invalid handle if cannot allocate
    so many continuous elements. Method WRATHasserts
    if sizeof(T)!=key().index_type_size().

    Can be called from threads outside of the GL 
    context from multiple threads simultaneously
    because it locks mutex(). 
    
    \param number_elements number of indices to allocate, a value
                           of zero or a negative value will
                           return an index group handle whose
                           valid() method returns false.
   */
  template<typename I>
  index_group<I>
  allocate_index_group(int number_elements)
  {
    index_chunk *g;
    index_group<I> R;

    WRATHassert(sizeof(I)==m_attribute_store->index_type_size());
    g=allocate_index_group_implement(number_elements);

    R=index_group<I>(g);
    if(g!=NULL)
      {
        WRATHAutoLockMutex(R.mutex());
        c_array<I> ptr(R.pointer());
        std::fill(ptr.begin(), ptr.end(), I(0));
      }
    return R;
  }

  /*!\fn index_group<I> allocate_copy_index_group
    Creates a new index group whose parameters and index
    data are copied from a source index group.
    \param h handle to an index group from which to copy
   */
  template<typename I>
  index_group<I>
  allocate_copy_index_group(index_group<I> h)
  {
    WRATHassert(h.valid());
    int number_elements;
    index_group<I> R;
    WRATHassert(sizeof(I)==m_attribute_store->index_type_size());
    number_elements=h.size();
    R=allocate_index_group<I>(number_elements);
    if(R.valid())
      {
        R.copy(h);
      }
    return R;
  } 

  /*!\fn const WRATHTripleBufferEnabler::handle& triple_buffer_enabler
    Returns a handle to the WRATHTripleBufferEnabler
    used by the buffers associated to this
    WRATHIndexGroupAllocator.
   */
  const WRATHTripleBufferEnabler::handle&
  triple_buffer_enabler(void)
  {
    return m_index_buffer->triple_buffer_enabler();
  }

private:

  class index_chunk
  {
  public:
    WRATHIndexGroupAllocator::handle m_source;
    range_type<int> m_range;

    index_chunk(const WRATHIndexGroupAllocator::handle &s,
                int begin, int size):
      m_source(s),
      m_range(begin, size+begin)
    {}
  };

  class DrawCommand:public WRATHDrawCommand
  {
  public:
    explicit
    DrawCommand(WRATHIndexGroupAllocator *src, GLenum primitive_type);

    virtual
    GLenum
    index_type(void);
    
    virtual
    GLenum
    primitive_type(void);

    virtual
    bool
    draw_elements_empty(void);

    virtual
    void
    append_draw_elements(std::vector<index_range> &output);

  private:
    WRATHIndexGroupAllocator *m_src;
    GLenum m_primitive_type;
  };  

  index_chunk*
  allocate_index_group_implement(int number_elements);

  void
  deallocate_group_implement(index_chunk *p);

  void
  update_draw_ranges(void);

  int
  index_type_size(void) const
  {
    return m_attribute_store->index_type_size();
  }

  WRATHBufferAllocator *m_index_buffer;
  bool m_own_index_buffer;
  WRATHDrawCommand *m_draw_command;
  std::map<int, index_chunk*> m_index_chunks;
  mutable WRATHMutex m_mutex;
  WRATHAttributeStore::handle m_attribute_store;

  std::vector<WRATHDrawCommand::index_range> m_draw_ranges;
  bool m_draw_ranges_dirty;
};

/*! @} */

#endif
