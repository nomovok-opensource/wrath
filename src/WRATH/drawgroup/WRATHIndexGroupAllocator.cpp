/*! 
 * \file WRATHIndexGroupAllocator.cpp
 * \brief file WRATHIndexGroupAllocator.cpp
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
#include <cstring>
#include <limits>
#include "WRATHIndexGroupAllocator.hpp"
#include "WRATHStaticInit.hpp"
#include "WRATHDrawCommandIndexBufferAllocator.hpp"


/*
  TODO:
   1) if primitive restart is supported, for those primitive
      types that are not isolated (for example TRIANGLE_STRIP)
      we can set primitive restart index to 0 and either
      prepend or append a 0 to the returned index draw groups.
 */


/////////////////////////////////////////////
// WRATHIndexGroupAllocator::DrawCommand methods
WRATHIndexGroupAllocator::DrawCommand::
DrawCommand(WRATHIndexGroupAllocator *src, GLenum primitive_type):
  WRATHDrawCommand(src->m_index_buffer->triple_buffer_enabler(),
                   src->m_index_buffer->buffer_object()),
  m_src(src),
  m_primitive_type(primitive_type)
{
}

GLenum
WRATHIndexGroupAllocator::DrawCommand::
index_type(void)
{
  return m_src->attribute_store()->index_type();
}

GLenum
WRATHIndexGroupAllocator::DrawCommand::
primitive_type(void)
{
  return m_primitive_type;
}

bool
WRATHIndexGroupAllocator::DrawCommand::
draw_elements_empty(void)
{
  return m_src->empty();
}


void
WRATHIndexGroupAllocator::DrawCommand::
append_draw_elements(std::vector<index_range> &output)
{
  /*
    we skip the lock because this is only called
    from the rendering thread, as such there is no
    need for a lock.
   */
  m_src->update_draw_ranges();  
  for(std::vector<index_range>::const_iterator 
        iter=m_src->m_draw_ranges.begin(),
        end=m_src->m_draw_ranges.end();
      iter!=end;
      ++iter)
    {
      output.push_back(*iter);
    }
}

/////////////////////////////////////////////////
//WRATHIndexGroupAllocator methods
WRATHIndexGroupAllocator::
WRATHIndexGroupAllocator(GLenum primitive_type,
                         WRATHBufferAllocator *pindex_buffer,
                         const WRATHAttributeStore::handle &pstore):
  m_index_buffer(pindex_buffer),
  m_own_index_buffer(false),
  m_attribute_store(pstore),
  m_draw_ranges_dirty(false)
{
  m_draw_command=WRATHNew DrawCommand(this, primitive_type);
}

WRATHIndexGroupAllocator::
WRATHIndexGroupAllocator(GLenum primitive_type,
                         GLenum buffer_object_hint,
                         const WRATHAttributeStore::handle &pstore):
  m_own_index_buffer(true),
  m_attribute_store(pstore),
  m_draw_ranges_dirty(false)
{
  const WRATHTripleBufferEnabler::handle &tr(pstore->buffer_allocator()->triple_buffer_enabler());

  m_index_buffer=WRATHNew WRATHBufferAllocator(tr, buffer_object_hint);
    
  WRATHDrawCommandIndexBufferAllocator::params params(m_index_buffer,
                                                      primitive_type,
                                                      m_attribute_store->index_type(),
                                                      m_attribute_store->index_type_size());
  
  m_draw_command=WRATHNew WRATHDrawCommandIndexBufferAllocator(tr, params);
  
}

WRATHIndexGroupAllocator::
~WRATHIndexGroupAllocator()
{
  #ifdef WRATHDEBUG  
  {  
    if(!m_index_chunks.empty())
      {
        WRATHwarning("[" << this << "]"
                     << m_index_buffer 
                     << ": Warning: not all index data de-allocated! "
                     << m_index_chunks.size() << " index groups remain");
      }
  }
  #endif
  WRATHPhasedDelete(m_draw_command); 
  if(m_own_index_buffer)
    {
      WRATHPhasedDelete(m_index_buffer);
    }
}


/*
  TODO:
    if m_own_index_buffer is true, there is
    NO need to track created index_chunk
    objects. the only catch is that
    empty() implementation need to check
    the index_buffer directly instead.
 */

void
WRATHIndexGroupAllocator::
deallocate_group_implement(index_chunk *p)
{
  WRATHLockMutex(m_mutex);

  WRATHassert(p!=NULL);
  
  int begin(p->m_range.m_begin*index_type_size());
  int end(p->m_range.m_end*index_type_size());

  m_index_buffer->deallocate(begin, end);
  m_index_chunks.erase(p->m_range.m_begin);
  m_draw_ranges_dirty=true;
  WRATHPhasedDelete(p);

  WRATHUnlockMutex(m_mutex);
}

WRATHIndexGroupAllocator::index_chunk*
WRATHIndexGroupAllocator::
allocate_index_group_implement(int number_elements)
{
  if(number_elements<=0)
    {
      return NULL;
    }

  int raw_value;
  raw_value=m_index_buffer->allocate(number_elements*index_type_size());
  WRATHAutoLockMutex(m_mutex);

  WRATHassert(raw_value%index_type_size()==0);
  raw_value/=index_type_size();

  index_chunk *G;
  G=WRATHNew index_chunk(this, raw_value, number_elements);
  m_index_chunks[G->m_range.m_begin]=G;
  m_draw_ranges_dirty=true;

  return G;
}

bool
WRATHIndexGroupAllocator::
empty(void) const
{
  WRATHAutoLockMutex(m_mutex);
  return m_index_chunks.empty();
}

void
WRATHIndexGroupAllocator::
update_draw_ranges(void)
{
  WRATHAutoLockMutex(m_mutex);
  if(m_draw_ranges_dirty)
    {
      int last_end(-1);

      m_draw_ranges.clear();
      for(std::map<int, index_chunk*>::iterator 
            iter=m_index_chunks.begin(),
            end=m_index_chunks.end();
          iter!=end; ++iter)
        {
          const index_chunk *chunk(iter->second);
          range_type<int> R(chunk->m_range);
          int count(R.m_end - R.m_begin);

          if(R.m_begin==last_end)
            {
              m_draw_ranges.back().m_count += count;
            }
          else
            {
              WRATHDrawCommand::index_range V;

              last_end=R.m_end;
              V.m_location=index_type_size()*R.m_begin;
              V.m_count=count;
              m_draw_ranges.push_back(V);
            }
        }
      m_draw_ranges_dirty=false;
    }
}

