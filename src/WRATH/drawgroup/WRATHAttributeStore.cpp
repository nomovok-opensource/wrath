/*! 
 * \file WRATHAttributeStore.cpp
 * \brief file WRATHAttributeStore.cpp
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
#include "WRATHAttributeStore.hpp"
#include "WRATHStaticInit.hpp"

/////////////////////////////////////
// WRATHAttributeStoreKey methods
bool
WRATHAttributeStoreKey::
operator<(const WRATHAttributeStoreKey &rhs) const
{
  //WRATHassert(m_type!=NULL);
  //WRATHassert(rhs.m_type!=NULL);

  if(m_buffer_object_hint!=rhs.m_buffer_object_hint)
    {
      return m_buffer_object_hint<rhs.m_buffer_object_hint;
    }

  if(m_index_bit_count!=rhs.m_index_bit_count)
    {
      return m_index_bit_count<rhs.m_index_bit_count;
    }

  //  if(type()!=rhs.type())
  //{
  //  return type().before(rhs.type());
  //}

  if(m_type_size!=rhs.m_type_size)
    {
      return m_type_size < rhs.m_type_size;
    }

  if(m_attribute_format_location!=rhs.m_attribute_format_location)
    {
      return m_attribute_format_location<rhs.m_attribute_format_location;
    }

  return false;
}

bool
WRATHAttributeStoreKey::
operator==(const WRATHAttributeStoreKey &rhs) const
{
  //WRATHassert(m_type!=NULL);
  //WRATHassert(rhs.m_type!=NULL);

  return m_buffer_object_hint==rhs.m_buffer_object_hint
    and m_index_bit_count==rhs.m_index_bit_count
    and m_type_size==rhs.m_type_size
    and m_attribute_format_location==rhs.m_attribute_format_location;
}

bool
WRATHAttributeStoreKey::
valid(void) const
{
  bool found_unused_attribute(false);
  
  if(!m_attribute_format_location[0].valid())
    {
      return false;
    }

  for(int i=1; i<WRATHDrawCallSpec::attribute_count; ++i)
    {
      bool b;

      b=m_attribute_format_location[i].valid();
      if(b and found_unused_attribute)
        {
          return false;
        }
      found_unused_attribute=found_unused_attribute or !b;
    }

  return true;
}

//////////////////////////////////////
// WRATHAttributeStoreAllocator methods

WRATHAttributeStoreAllocator::
~WRATHAttributeStoreAllocator()
{
  map_type tmp;

  /*
    We need to make sure WRATHAttributeStore's made
    by this WRATHAttributeStoreAllocator do not try
    to unregister themselves.

    We do a swap with a local std::map
    to prevent a potential deadlock, since
    WRATHAttributeStore essentially performs 
    the following at dtor:

    WRATHLockMutex(m_allocator_ptr_mutex);
    WRATHLockMutex(m_allocator->m_mutex);
    m_attribute_stores[].erase();
    WRATHUnlockMutex(m_allocator->m_mutex);
    WRATHUnlockMutex(m_allocator_ptr_mutex);

    Having an inversion of the locking order
    creates a potential dead lock, thus
    we avoid it by first getting the elements
    into a local std::map and walking that local
    map. We are not even deleting the objects
    anyways, we sre simply marking that
    their m_allocator field is NULL.
   */
  WRATHLockMutex(m_mutex);
  m_phase_deleted=true;
  std::swap(tmp, m_attribute_stores);
  WRATHUnlockMutex(m_mutex);


  for(map_type::iterator iter=tmp.begin(),
        end=tmp.end(); iter!=end; ++iter)
    {
      for(std::set<WRATHAttributeStore*>::iterator s=iter->second.begin(),
            e=iter->second.end(); s!=e; ++s)
        {
          WRATHLockMutex((*s)->m_allocator_ptr_mutex);
          (*s)->m_allocator=NULL;
          WRATHUnlockMutex((*s)->m_allocator_ptr_mutex);
        }
    }
}

void
WRATHAttributeStoreAllocator::
unregister(WRATHAttributeStore *q)
{
  map_type::iterator iter;

  WRATHAutoLockMutex(m_mutex);

  iter=m_attribute_stores.find( map_key(q->m_key) );
  if(iter!=m_attribute_stores.end())
    {
      iter->second.erase(q);
      if(iter->second.empty())
        {
          m_attribute_stores.erase(iter);
        }
    }

}


/*
  Sighs: each of the WRATHAttributeStoreAllocator::attribute_store() 
  methods is ALMOST identical... should likely make a little
  jazz so that all are implemented by a common routine..

  Additionally, the search is far from optimal, what would
  be best is if the WRATHAttributeStore are stored sorted
  by how much room they have left...
*/

WRATHAttributeStore::handle
WRATHAttributeStoreAllocator::
attribute_store(const WRATHAttributeStoreKey &pk,
                int req_number_elements, range_type<int> &R,
                enum implicit_attribute_req_t req)
{
  
  map_type::iterator iter;
  map_key k(pk);

  WRATHAutoLockMutex(m_mutex);

  if(m_phase_deleted)
    {
      return WRATHAttributeStore::handle();
    }


  iter=m_attribute_stores.find(k);
  if(iter!=m_attribute_stores.end())
    {
      for(std::set<WRATHAttributeStore*>::iterator s=iter->second.begin(),
            e=iter->second.end(); s!=e; ++s)
        {
          WRATHAttributeStore *ptr(*s);
          
          if(routine_success==ptr->allocate_attribute_data(req_number_elements, R))
            {
              return ptr;
            }
        }
    }

  WRATHAttributeStore *pnew_store;
  enum return_code e;

  pnew_store=WRATHNew WRATHAttributeStore(k, this, req);
  m_attribute_stores[k].insert(pnew_store);

  e=pnew_store->allocate_attribute_data(req_number_elements, R);
  WRATHassert(e==routine_success);
  WRATHunused(e);


  return pnew_store;
}




WRATHAttributeStore::handle
WRATHAttributeStoreAllocator::
attribute_store(const WRATHAttributeStoreKey &pk,
                int req_number_elements, 
                std::vector<range_type<int> > &R,
                enum implicit_attribute_req_t req)
{
  
  map_type::iterator iter;
  map_key k(pk);

  R.clear();

  WRATHAutoLockMutex(m_mutex);

  if(m_phase_deleted)
    {
      return WRATHAttributeStore::handle();
    }

  iter=m_attribute_stores.find(k);
  if(iter!=m_attribute_stores.end())
    {
      for(std::set<WRATHAttributeStore*>::iterator s=iter->second.begin(),
            e=iter->second.end(); s!=e; ++s)
        {
          WRATHAttributeStore *ptr(*s);
          
          if(routine_success==ptr->fragmented_allocate_attribute_data(req_number_elements, R))
            {
              return ptr;
            }
        }
    }

  WRATHAttributeStore *pnew_store;
  enum return_code e;

  pnew_store=WRATHNew WRATHAttributeStore(k, this, req);
  m_attribute_stores[k].insert(pnew_store);

  e=pnew_store->fragmented_allocate_attribute_data(req_number_elements, R);
  WRATHassert(e==routine_success);
  WRATHunused(e);


  return pnew_store;
}

WRATHAttributeStore::handle
WRATHAttributeStoreAllocator::
attribute_store(const WRATHAttributeStoreKey &pk,
                int req_number_elements,
                int req_number_elements_continuous,
                enum implicit_attribute_req_t req)
{
 
  map_type::iterator iter;
  map_key k(pk);

  WRATHAutoLockMutex(m_mutex);

  if(m_phase_deleted)
    {
      return WRATHAttributeStore::handle();
    }

  iter=m_attribute_stores.find(k);
  if(iter!=m_attribute_stores.end())
    {
      for(std::set<WRATHAttributeStore*>::iterator s=iter->second.begin(),
            e=iter->second.end(); s!=e; ++s)
        {
          WRATHAttributeStore *ptr(*s);
          
          if(routine_success==ptr->proxy_attribute_allocate(req_number_elements)
             and 
             routine_success==ptr->proxy_fragmented_allocate_attribute(req_number_elements_continuous) )
            {
              return ptr;
            }
        }
    }

  WRATHAttributeStore *pnew_store;

  pnew_store=WRATHNew WRATHAttributeStore(k, this, req);
  m_attribute_stores[k].insert(pnew_store);

  return pnew_store;
}


bool
WRATHAttributeStoreAllocator::
same_implicit_attribute_type(const WRATHAttributeStoreAllocator *ptr) const
{
  return m_value_at_index0.size()==ptr->m_value_at_index0.size()
    and m_implicit_attribute_format==ptr->m_implicit_attribute_format;
}

//////////////////////////////////////
// WRATHAttributeStore methods
WRATHAttributeStore::
WRATHAttributeStore(const WRATHAttributeStoreKey &pkey,
                    WRATHAttributeStoreAllocator *allocator,
                    bool allocate_implicit_attribute_data):
  m_key(pkey),
  m_value_at_index0(allocator->m_value_at_index0),
  m_implicit_attribute_format(allocator->m_implicit_attribute_format),
  m_attribute_format_location(m_key.m_attribute_format_location),
  m_index_bits(m_key.m_index_bit_count),
  m_buffer_object_hint(m_key.m_buffer_object_hint),
  m_implicit_attribute_size(m_value_at_index0.size()),
  m_allocator(allocator),
  m_req_implicit_attribute_size(m_implicit_attribute_size)
{
  WRATHassert(m_key.valid());

  WRATHAutoLockMutex(m_allocator_ptr_mutex);

  int bo_end_byte;

  switch(m_index_bits)
    {
    case WRATHAttributeStoreKey::index_8bits:
      m_index_type=GL_UNSIGNED_BYTE;
      m_index_type_size=1;
      bo_end_byte=256*attribute_size();
      break;

    default:
      WRATHassert(0);
      //fall through on release builds..
    case WRATHAttributeStoreKey::index_16bits:
      m_index_type=GL_UNSIGNED_SHORT;
      m_index_type_size=2;
      bo_end_byte=65536*attribute_size();
      break;

    case WRATHAttributeStoreKey::index_32bits:
      m_index_type=GL_UNSIGNED_INT;
      m_index_type_size=4;
      bo_end_byte=std::numeric_limits<int>::max();
      break;
    }

  m_vertex_buffer=WRATHNew WRATHBufferAllocator(m_allocator->triple_buffer_enabler(),
                                                m_buffer_object_hint, 
                                                bo_end_byte);

  

  for(m_number_non_implicit_attributes=0; 
      m_number_non_implicit_attributes < WRATHDrawCallSpec::attribute_count
        and m_attribute_format_location[m_number_non_implicit_attributes].valid(); 
      ++m_number_non_implicit_attributes)
    {}

  for(int K=0, endK=m_implicit_attribute_format.size(), I=m_number_non_implicit_attributes; 
      K<endK and I<WRATHDrawCallSpec::attribute_count; ++K, ++I)
    {
      m_attribute_format_location[I]=m_implicit_attribute_format[K];
    }

  WRATHassert(proxy_attribute_allocate(1)==routine_success);

  if(allocate_implicit_attribute_data)
    {
      add_implicit_store(0);
    }

  unsigned int R;
      
  R=allocate_attribute_data(1);
  WRATHassert(R==0);
  WRATHunused(R);

  
}


WRATHAttributeStore::
~WRATHAttributeStore()
{
  deallocate_attribute_data(0,1);

  if(attributes_allocated()!=0)
    {
      WRATHwarning("[" << this << "]"
                   << ":" << m_vertex_buffer
                   << ": Warning: not all attributes de-allocated! "
                   << attributes_allocated()  << " attributes remain"
                   << "{ attribute size=" << attribute_size()
                   << " }");
    }

  WRATHPhasedDelete(m_vertex_buffer);

  WRATHLockMutex(m_allocator_ptr_mutex);
  if(m_allocator!=NULL)
    {
      m_allocator->unregister(this);
    }

  for(std::map<unsigned int, per_implicit_store*>::iterator 
        iter=m_implicit_attribute_data.begin(),
        end=m_implicit_attribute_data.end();
      iter!=end; ++iter)
    {
      WRATHPhasedDelete(iter->second);
    }

  WRATHUnlockMutex(m_allocator_ptr_mutex);
}

WRATHAttributeStore::per_implicit_store*
WRATHAttributeStore::
fetch_implicit_store(unsigned int idx) const
{
  WRATHAutoLockMutex(m_implicit_store_mutex);
  std::map<unsigned int, per_implicit_store*>::const_iterator iter;

  iter=m_implicit_attribute_data.find(idx);
  return (iter!=m_implicit_attribute_data.end())?
    iter->second:
    NULL;
}

WRATHBufferObject*
WRATHAttributeStore::
implicit_attribute_data(unsigned int idx) const
{
  per_implicit_store *p;
  p=fetch_implicit_store(idx);
  WRATHassert(p!=NULL);
  return p;
}

const vecN<WRATHBufferObject*, WRATHDrawCallSpec::attribute_count>&
WRATHAttributeStore::
buffer_object_vector(unsigned int idx) const
{
  per_implicit_store *p;
  p=fetch_implicit_store(idx);
  WRATHassert(p!=NULL);
  return p->m_buffer_object_vector;
}

void
WRATHAttributeStore::
add_implicit_store(unsigned int idx)
{
  WRATHAutoLockMutex(m_implicit_store_mutex);

  if(m_implicit_attribute_data.find(idx)!=m_implicit_attribute_data.end())
    {
      return;
    }

  if(!m_implicit_attribute_format.empty())
    {
      per_implicit_store *ptr;

      /*
        allocate the structure holding the mutex, buffer object
        and buffer object pointer vector
       */
      ptr=WRATHNew per_implicit_store(m_vertex_buffer->triple_buffer_enabler(),
                                      m_buffer_object_hint);

      /*
        resize the implicit attribute data as needed,
        we resize as according to if there is already
        an implicit attribute store, and make it the
        size as it.        
       */
      ptr->resize(m_req_implicit_attribute_size);

      /*
        fill the buffer object pointer vector
       */
      for(int I=0; I<m_number_non_implicit_attributes; ++I)
        {
          ptr->m_buffer_object_vector[I]=m_vertex_buffer->buffer_object();
        }

      for(int K=0, I=m_number_non_implicit_attributes, endK=m_implicit_attribute_format.size(); 
          K<endK and I<WRATHDrawCallSpec::attribute_count; ++K, ++I)
        {
          ptr->m_buffer_object_vector[I]=ptr;
        }
      
      /*
        set the value at index 0 as the non-visible value.
       */
      if(!m_value_at_index0.empty())
        {
          ptr->mark_bytes_dirty_no_lock(0, m_value_at_index0.size());
          std::copy(m_value_at_index0.begin(), m_value_at_index0.end(), ptr->c_ptr(0));
        }

      /*
        save it the the map
       */
      m_implicit_attribute_data[idx]=ptr;
    }

}

void
WRATHAttributeStore::
resize_implicit_stores(int req_size)
{
  if(m_req_implicit_attribute_size < req_size)
    {
      /*
        We make m_req_implicit_attribute_size just grow in size.
        This is mostly okay, because the underlying object used,
        WRATHBufferObject, when shrunk does NOT free memory.
       */
      m_req_implicit_attribute_size=std::max(m_req_implicit_attribute_size, req_size);

      for(std::map<unsigned int, per_implicit_store*>::const_iterator 
            iter=m_implicit_attribute_data.begin(),
            end=m_implicit_attribute_data.end();
          iter!=end; ++iter)
        {
          iter->second->resize(m_req_implicit_attribute_size);
        }
    }
}

int
WRATHAttributeStore::
allocate_attribute_data(int number_elements)
{
  WRATHAutoLockMutex(m_implicit_store_mutex);

  int raw_value;
  raw_value=m_vertex_buffer->allocate(number_elements*attribute_size());

  if(raw_value==-1)
    {
      return raw_value;
    }

  WRATHassert(raw_value%attribute_size()==0);
  raw_value/=attribute_size();

  int required_implicit_attr_size(m_implicit_attribute_size*(number_elements+raw_value));
  resize_implicit_stores(required_implicit_attr_size);

  return raw_value;
}

enum return_code
WRATHAttributeStore::
proxy_attribute_allocate(int number_elements) const
{
  return (number_elements<=0)?
    routine_success:
    m_vertex_buffer->proxy_allocate(number_elements*attribute_size());
}


enum return_code
WRATHAttributeStore::
proxy_fragmented_allocate_attribute(int number_elements) const
{
  return (number_elements<=0)?
    routine_success:
    m_vertex_buffer->proxy_fragmented_allocate(number_elements*attribute_size());
}

int
WRATHAttributeStore::
max_cts_allocate_possible(void) const
{
  return m_vertex_buffer->max_cts_allocate_possible()/attribute_size();
}

int
WRATHAttributeStore::
max_fragmented_allocate_possible(void) const
{
  return m_vertex_buffer->max_fragmented_allocate_possible()/attribute_size();
}

int
WRATHAttributeStore::
attributes_allocated(void) const
{
  return m_vertex_buffer->bytes_allocated()/attribute_size();
}


void
WRATHAttributeStore::
deallocate_attribute_data(int begin_element, int end_element)
{
  begin_element*=attribute_size();
  end_element*=attribute_size();

  m_vertex_buffer->deallocate(begin_element, end_element);
}



enum return_code
WRATHAttributeStore::
fragmented_allocate_attribute_data(int number_elements,
                                   std::vector< range_type<int> > &out_allocations)
{
  
  WRATHAutoLockMutex(m_implicit_store_mutex);

  enum return_code R;
  int biggest_end(0), start_div_at(out_allocations.size());

  R=m_vertex_buffer->fragmented_allocate(attribute_size()*number_elements, out_allocations);

  if(R==routine_success)
    {
      for(int i=start_div_at, end_i=out_allocations.size(); i<end_i; ++i)
        {
          WRATHassert(out_allocations[i].m_begin%attribute_size()==0);
          WRATHassert(out_allocations[i].m_end%attribute_size()==0);
          
          out_allocations[i].m_begin/=attribute_size();
          out_allocations[i].m_end/=attribute_size();
          biggest_end=std::max(out_allocations[i].m_end, biggest_end);
        }
      
      int required_implicit_attr_size(m_implicit_attribute_size*biggest_end);
      resize_implicit_stores(required_implicit_attr_size);
    }

  return R;
}

unsigned int
WRATHAttributeStore::
total_size(const std::vector<range_type<int> > &attr_locations)
{
  unsigned int L(0);

  for(std::vector<range_type<int> >::const_iterator iter=attr_locations.begin(),
        end=attr_locations.end(); iter!=end; ++iter)
    {
      L+= (iter->m_end - iter->m_begin);
    }
  return L;
}
