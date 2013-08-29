/*! 
 * \file WRATHBufferObject.cpp
 * \brief file WRATHBufferObject.cpp
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


#include "WRATHConfig.hpp"
#include <iostream>
#include "WRATHBufferObject.hpp"
#include "WRATHStaticInit.hpp"

namespace
{
  unsigned int& 
  sm_total_bytes_uploaded(void)
  {
    static unsigned int r(0);
    return r;
  }

  WRATHMutex&
  sm_total_bytes_uploaded_mutex(void)
  {
    WRATHStaticInit();
    static WRATHMutex R;
    return R;
  }
}

WRATHBufferObject::
WRATHBufferObject(const WRATHTripleBufferEnabler::handle &h,
                  GLenum usage,
                  WRATHMutex *pmutex):
  WRATHTripleBufferEnabler::PhasedDeletedObject(h),
  m_dirty(true),
  m_name(0),
  m_usage(usage),
  m_buffer_object_size_in_bytes(0),
  m_virtual_size(0),
  m_cache_size(0),
  m_mutex(pmutex)
{
}

WRATHBufferObject::
~WRATHBufferObject(void)
{
  WRATHassert(m_name==0);
}


void
WRATHBufferObject::
phase_render_deletion(void)
{
  if(m_name!=0)
    {
      glDeleteBuffers(1, &m_name);
      m_name=0;
    }
}

bool
WRATHBufferObject::
is_dirty(void) const
{
  bool R;

  WRATHLockMutexIfNonNULL(m_mutex);
  R=is_dirty_no_lock();
  WRATHUnlockMutexIfNonNULL(m_mutex);

  return R;
}


int
WRATHBufferObject::
size(void) const
{
  int R;

  WRATHLockMutexIfNonNULL(m_mutex);
  R=m_virtual_size;
  WRATHUnlockMutexIfNonNULL(m_mutex);

  return R;
}

void
WRATHBufferObject::
resize(int new_size_in_bytes) 
{
  WRATHLockMutexIfNonNULL(m_mutex);
  resize_no_lock(new_size_in_bytes);
  WRATHUnlockMutexIfNonNULL(m_mutex);
}

void
WRATHBufferObject::
bind(GLenum bind_target)
{
  flush(bind_target);    
  glBindBuffer(bind_target, name());
    
}

void
WRATHBufferObject::
mark_bytes_dirty(int begin_byte_location, int end_byte_location)
{
  WRATHLockMutexIfNonNULL(m_mutex);
  mark_bytes_dirty_no_lock(begin_byte_location, end_byte_location);
  WRATHUnlockMutexIfNonNULL(m_mutex);
}

const void*
WRATHBufferObject::
offset_pointer(int byte_offset)
{  
  const uint8_t *R;

  R=(m_usage!=GL_INVALID_ENUM)?
    NULL:
    raw_data_pointer();

  return R+byte_offset;
}

unsigned int
WRATHBufferObject::
total_bytes_uploaded(void)
{
  unsigned int R;

  WRATHLockMutex(sm_total_bytes_uploaded_mutex());
  R=sm_total_bytes_uploaded();
  WRATHUnlockMutex(sm_total_bytes_uploaded_mutex());

  return R;
}

bool
WRATHBufferObject::
flush(GLenum bind_target)
{
  bool R;

  WRATHLockMutexIfNonNULL(m_mutex);
  R=flush_no_lock(bind_target);
  WRATHUnlockMutexIfNonNULL(m_mutex);

  return R;
}




///////////////////////////////////////////////////////////
//routines without locking:
void
WRATHBufferObject::
resize_no_lock(int new_size_in_bytes)
{
  int size_in_4bytes(new_size_in_bytes>>2);

  if(new_size_in_bytes&3)
    {
      ++size_in_4bytes;
    }

  m_cached_data.resize(size_in_4bytes, 0);
  m_cache_size=4*size_in_4bytes;
  m_virtual_size=new_size_in_bytes;
}


bool
WRATHBufferObject::
flush_no_lock(GLenum bind_target)
{
  if(m_usage==GL_INVALID_ENUM)
    {
      return false;
    }

  if(m_name==0)
    {
      glGenBuffers(1, &m_name);
      WRATHassert(m_name!=0);
    }

  bool bounded(false);

  
  if(m_cache_size>m_buffer_object_size_in_bytes)
    {
      bounded=true;

      glBindBuffer(bind_target, m_name);
      m_dirty_blocks.clear();
      m_dirty=false;
      
      m_buffer_object_size_in_bytes=m_cache_size;
      glBufferData(bind_target, m_buffer_object_size_in_bytes, raw_data_pointer(), m_usage);
    }
  else if(m_dirty)
    {
      for(std::map<int, range_type<int> >::iterator 
            iter=m_dirty_blocks.begin(), end=m_dirty_blocks.end(); iter!=end; ++iter)
        {
          WRATHLockMutex(sm_total_bytes_uploaded_mutex());
          sm_total_bytes_uploaded()+=(iter->second.m_end - iter->second.m_begin);
          WRATHUnlockMutex(sm_total_bytes_uploaded_mutex());

          if(!bounded)
            {
              glBindBuffer(bind_target, m_name);
              bounded=true;
            }
          WRATHassert(iter->first==iter->second.m_end);

          
          range_type<int> R(iter->second);

          glBufferSubData(bind_target, 
                          R.m_begin, 
                          R.m_end - R.m_begin,
                          raw_data_pointer()+ R.m_begin);
        }

      m_dirty=false;
      m_dirty_blocks.clear();
    }

  return bounded;
}


void
WRATHBufferObject::
mark_bytes_dirty_no_lock(int begin_byte_location, int end_byte_location)
{   
  if(begin_byte_location<end_byte_location 
     and m_name!=0 /* no point of tracking if there is no GL buffer object*/
     and m_cache_size<=m_buffer_object_size_in_bytes) /*a resize to a larger resize triggers a full update*/
    {
#ifdef WRATHDEBUG_BUFFEROBJECT
      debug_madness BB(this,
                       begin_byte_location,
                       end_byte_location,
                       m_dirty_blocks);
#endif

      m_dirty=true;

      
      /*
        TODO:
          This is not necessarily the most optimal method,
          likely we should merge dirty blocks that are large
          even if there are a few bytes between them that
          are not dirty.. or possibly we use a more static
          model where we partition the buffer object into pieces
          and only work at the resolution of the pieces.

          Lastly, this icky code would be avoided if we
          use boost's interval container library, 
          see http://www.boost.org/doc/libs/1_47_0/libs/icl/doc/html/index.html,
          but that is only available in boost versions atleast 1.46
          which is quite recent.
       */

      /*
        find where the block would be placed
       */
      std::map<int, range_type<int> >::iterator blockA, blockB;

      /*
        get the last block whose ending is greater or equal
        to the beginning of the new dirty block:
       */
      blockA=m_dirty_blocks.lower_bound(begin_byte_location);

      if(blockA==m_dirty_blocks.end())
        {
          /*
            all dirty block end strictly before
            begin_byte_location, so just add the 
            dirty block:
           */
          m_dirty_blocks[end_byte_location]=range_type<int>(begin_byte_location,
                                                            end_byte_location);
          return;
        }

      if(end_byte_location<blockA->second.m_begin)
        {
          /*
            the new dirty block begins AND ends before blockA,
            we also know that blockA is the _first_ block
            whose end is greater than the beginning of the
            new dirty block, thus we can just insert the
            new dirty block.
           */
          m_dirty_blocks[end_byte_location]=range_type<int>(begin_byte_location,
                                                            end_byte_location);
          return;
        }

      if(begin_byte_location==blockA->second.m_end)
        {
          begin_byte_location=blockA->second.m_begin;
          m_dirty_blocks.erase(blockA);
          mark_bytes_dirty_no_lock(begin_byte_location, end_byte_location);
          return;
        }

      /*
        at this point we know that
        end_byte_location comes after or at blockA->second.m_begin,
        hence we enlarge block_before to include the byte range
        [begin_byte_location, blockA->m_second.m_begin)
      */                                    
      blockA->second.m_begin=std::min(blockA->second.m_begin,
                                      begin_byte_location);
      

      
      if(end_byte_location<=blockA->second.m_end)
        {
          /*
            blockA contains the [begin_byte_location, end_byte_location)
            so just return now.
           */
          return;
        }
      
      /*
        get the first block whose ending is strictly larger
        than the end of the new dirty block.
       */
      blockB=m_dirty_blocks.upper_bound(end_byte_location);
      WRATHassert(blockA!=blockB);

      
     
      if(blockB!=m_dirty_blocks.end() and blockB!=blockA)
        {
          /*
            we will decrement blockB until 
            we get to a block that begins at or before
            end_byte_location
           */
          while(blockB->second.m_begin>end_byte_location
                and blockB!=blockA)
            {
              --blockB;
            }
         
          range_type<int> R(blockA->second.m_begin, end_byte_location);
          /*
            now the blocks in the STL range [blockA, blockB)
            are contained within the range R, and thus can 
            be deleted once we enlarge blockB to include
            the contents of blockA:
          */
          blockB->second.m_begin=blockA->second.m_begin;
          m_dirty_blocks.erase(blockA, blockB);
        }
      else
        {
          WRATHassert(end_byte_location>=blockA->second.m_end);
          range_type<int> R(blockA->second.m_begin, end_byte_location);
          
          m_dirty_blocks.erase(blockA, m_dirty_blocks.end());
          m_dirty_blocks[R.m_end]=R;
        }
      
      
      
      

    }
}

int
WRATHBufferObject::
size_no_lock(void) const
{
  return m_virtual_size;
}

bool
WRATHBufferObject::
is_dirty_no_lock(void) const
{
  return m_dirty
      or m_cache_size>m_buffer_object_size_in_bytes;
}
