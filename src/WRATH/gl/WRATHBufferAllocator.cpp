/*! 
 * \file WRATHBufferAllocator.cpp
 * \brief file WRATHBufferAllocator.cpp
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
#include <limits>
#include <sstream>
#include "WRATHassert.hpp" 
#include "WRATHUtil.hpp"
#include "WRATHBufferAllocator.hpp"

//#define WRATHBUFFERALLOCATORDEBUG

WRATHBufferAllocator::
WRATHBufferAllocator(const WRATHTripleBufferEnabler::handle &h,
                     GLenum buffer_object_hint):
  WRATHTripleBufferEnabler::PhasedDeletedObject(h),
  m_max_buffer_object_size(false, std::numeric_limits<int>::max()),
  m_total_free_room(0),
  m_bytes_allocated(0),
  m_data_sink(this)
{
  m_buffer_object=WRATHNew WRATHBufferObject(h, buffer_object_hint, &m_mutex);
}

WRATHBufferAllocator::
WRATHBufferAllocator(const WRATHTripleBufferEnabler::handle &h,
                     GLenum buffer_object_hint, int max_size_in_bytes):
  WRATHTripleBufferEnabler::PhasedDeletedObject(h),
  m_max_buffer_object_size(true, max_size_in_bytes),
  m_total_free_room(0),
  m_bytes_allocated(0),
  m_data_sink(this)
{
  m_buffer_object=WRATHNew WRATHBufferObject(h, buffer_object_hint, &m_mutex);
}

WRATHBufferAllocator::
~WRATHBufferAllocator(void)
{
  WRATHassert(m_buffer_object==NULL);
}

void
WRATHBufferAllocator::
on_place_on_deletion_list(void)
{
#ifdef WRATHDEBUG 
  if(m_bytes_allocated!=0)
    {
      std::ostringstream blocks_data;

      print_free_block_info(blocks_data, "\tStats:");

      WRATHwarning("[" << this << "]"
                   << ": Warning: not all data de-allocated! "
                   <<  m_bytes_allocated << " bytes remain\n"
                   << blocks_data.str());
    }
          
#endif

  WRATHPhasedDelete(m_buffer_object);
  m_buffer_object=NULL;
}

void
WRATHBufferAllocator::
clear(void)
{
  WRATHLockMutex(m_mutex);
  clear_nolock();
  WRATHUnlockMutex(m_mutex);
}

int
WRATHBufferAllocator::
allocate(int number_bytes)
{
  int R;

  WRATHLockMutex(m_mutex);
  R=allocate_nolock(number_bytes);
  WRATHUnlockMutex(m_mutex);

  return R;
}

enum return_code
WRATHBufferAllocator::
fragmented_allocate(int number_bytes,
                    std::vector< range_type<int> > &out_allocations)
{
  enum return_code R;

  WRATHLockMutex(m_mutex);
  R=fragmented_allocate_nolock(number_bytes, out_allocations);
  WRATHUnlockMutex(m_mutex);

  return R;
}

enum return_code
WRATHBufferAllocator::
proxy_allocate(int number_bytes)
{
  enum return_code R;

  WRATHLockMutex(m_mutex);
  R=proxy_allocate_nolock(number_bytes);
  WRATHUnlockMutex(m_mutex);

  return R;
}

enum return_code
WRATHBufferAllocator::
proxy_fragmented_allocate(int number_bytes)
{
  enum return_code R;

  WRATHLockMutex(m_mutex);
  R=proxy_fragmented_allocate_nolock(number_bytes);
  WRATHUnlockMutex(m_mutex);

  return R;
}

int
WRATHBufferAllocator::
max_fragmented_allocate_possible(void)
{
  int R;

  WRATHLockMutex(m_mutex);
  R=max_fragmented_allocate_possible_nolock();
  WRATHUnlockMutex(m_mutex);

  return R;
}

int
WRATHBufferAllocator::
max_cts_allocate_possible(void)
{
  int R;

  WRATHLockMutex(m_mutex);
  R=max_cts_allocate_possible_nolock();
  WRATHUnlockMutex(m_mutex);

  return R;
}


void
WRATHBufferAllocator::
deallocate(int begin_byte, int end_byte)
{
  WRATHLockMutex(m_mutex);
  deallocate_nolock(begin_byte, end_byte);
  WRATHUnlockMutex(m_mutex);
}


bool
WRATHBufferAllocator::
block_is_allocated(int begin, int end) const
{
  bool r;

  WRATHLockMutex(m_mutex);
  r=block_is_allocated_nolock(begin, end);
  WRATHUnlockMutex(m_mutex);

  return r;
}

int
WRATHBufferAllocator::
bytes_allocated(void) const
{
  int r;

  WRATHLockMutex(m_mutex);
  r=m_bytes_allocated;
  WRATHUnlockMutex(m_mutex);

  return r;
}

int
WRATHBufferAllocator::
freeblock_count(void) const
{
  int r;

  WRATHLockMutex(m_mutex);
  r=m_free_blocks.size();
  WRATHUnlockMutex(m_mutex);

  return r;
}

range_type<int>
WRATHBufferAllocator::
allocated_range(void) const
{
  range_type<int> r;

  WRATHLockMutex(m_mutex);
  r=allocated_range_nolock();
  WRATHUnlockMutex(m_mutex);

  return r;
}


void
WRATHBufferAllocator::
print_free_block_info(std::ostream &ostr, const std::string &prefix) const
{
  WRATHLockMutex(m_mutex);
  print_free_block_info_nolock(ostr, prefix);
  WRATHUnlockMutex(m_mutex);
}


//////////////////////////////////////////////////////////////
// all routines below this mark do not mark m_mutex
// and must NOT call routines that do lock it either.
range_type<int>
WRATHBufferAllocator::
allocated_range_nolock(void) const
{
  int begin(0), end;

  
  if(!m_free_blocks.empty() and m_free_blocks.begin()->second.m_begin==0)
    {
      begin=m_free_blocks.begin()->second.m_end;;
    }

  end=m_buffer_object->size_no_lock();

  return range_type<int>(begin, end);
}


void
WRATHBufferAllocator::
clear_nolock(void)
{
  m_free_blocks.clear();
  m_sorted_free_blocks.clear();
  m_total_free_room=0;
  m_bytes_allocated=0;
  m_buffer_object->resize_no_lock(0);
}

inline
void
WRATHBufferAllocator::
resize_buffer_object_nolock(int new_size)
{
  WRATHassert(!m_max_buffer_object_size.first
              or m_max_buffer_object_size.second>=new_size);
  
  m_buffer_object->resize_no_lock(new_size);
}

int
WRATHBufferAllocator::
max_fragmented_allocate_possible_nolock(void)
{
  int max_value;

  if(m_max_buffer_object_size.first)
    {
      max_value=
        m_max_buffer_object_size.second;
    }
  else
    {
      max_value=std::numeric_limits<int>::max(); 
    }

  return max_value - m_bytes_allocated;
}


int
WRATHBufferAllocator::
max_cts_allocate_possible_nolock(void)
{
  int return_value;

  if(m_max_buffer_object_size.first)
    {      
      return_value=
        m_max_buffer_object_size.second-m_buffer_object->size_no_lock();
    }
  else
    {
      return_value=std::numeric_limits<int>::max() 
        - m_buffer_object->size_no_lock();
    }

  if(!m_sorted_free_blocks.empty())
    {
      return_value=std::max(return_value,
                            m_sorted_free_blocks.rbegin()->first);
    }

  return return_value;
}




void
WRATHBufferAllocator::
deallocate_nolock(int begin_byte, int end_byte)
{
#ifdef WRATHBUFFERALLOCATORDEBUG
  std::cout << this << ": Deallocate: [" << begin_byte
            << ", " << end_byte << "):"
            << end_byte-begin_byte << "\n";
#endif  

  free_block_iter iter;
  std::pair<free_block_iter, bool> r;

  WRATHassert(block_is_allocated_nolock(begin_byte, end_byte));
  m_bytes_allocated-=(end_byte-begin_byte);

  //first see if begin_byte corresponds to
  //end_byte of any existing free chunks:
  iter=m_free_blocks.find(begin_byte);
  if(iter!=m_free_blocks.end())
    {
      //we need to remove the block from
      //our sorted list of available blocks:
      remove_free_block_from_available_list(iter);

      //the blocks ending changes, so 
      //we need to remove it from m_free_blocks
      begin_byte=iter->second.m_begin;
      m_free_blocks.erase(iter);
    }
      
  //now see if the block to free is at the end 
  if(end_byte==m_buffer_object->size_no_lock())
    {
      resize_buffer_object_nolock(begin_byte);
      return;
    }

  //now if end byte corresponds to the beginning of
  //another block, we need to just enlarge that block:
  iter=m_free_blocks.lower_bound(end_byte); //find the first block whose end is atleast end_byte
  if(iter!=m_free_blocks.end() and iter->second.m_begin==end_byte)
    {
      end_byte=iter->second.m_end;
      
      remove_free_block_from_available_list(iter);
      m_free_blocks.erase(iter);
    }

  r=m_free_blocks.insert( std::make_pair( end_byte, range_type<int>(begin_byte, end_byte)) );
  WRATHassert(r.second);
  
  insert_free_block_to_available_list(r.first);    
}

void
WRATHBufferAllocator::
remove_free_block_from_available_list(free_block_iter iter)
{
  std::map<int, std::set<free_block_iter, compare_block_iters> >::iterator miter;
  int sz(iter->second.m_end - iter->second.m_begin);

  miter=m_sorted_free_blocks.find(sz);

  WRATHassert(miter!=m_sorted_free_blocks.end());
  WRATHassert(miter->second.find(iter)!=miter->second.end());

  miter->second.erase(iter);
  
  if(miter->second.empty())
    {
      m_sorted_free_blocks.erase(miter);
    }

  m_total_free_room-= (iter->second.m_end - iter->second.m_begin);
}

void
WRATHBufferAllocator::
insert_free_block_to_available_list(free_block_iter iter)
{
  int sz(iter->second.m_end - iter->second.m_begin);
  m_sorted_free_blocks[sz].insert(iter);

  m_total_free_room+= (iter->second.m_end - iter->second.m_begin);
}

WRATHBufferAllocator::map_type::iterator
WRATHBufferAllocator::
sorted_free_blocks_lower_bound(int sz_in_bytes)
{
  return m_sorted_free_blocks.lower_bound(sz_in_bytes);
}



enum return_code
WRATHBufferAllocator::
proxy_allocate_nolock(int number_bytes)
{
  if(!m_max_buffer_object_size.first
     or (m_buffer_object->size_no_lock() + number_bytes <= m_max_buffer_object_size.second) )
    {
      return routine_success;
    }

  return (sorted_free_blocks_lower_bound(number_bytes)!=m_sorted_free_blocks.end())?
    routine_success:
    routine_fail;
  
}


enum return_code
WRATHBufferAllocator::
proxy_fragmented_allocate_nolock(int number_bytes)
{
  return (max_fragmented_allocate_possible_nolock()>=number_bytes)?
    routine_success:
    routine_fail;
}


int
WRATHBufferAllocator::
allocate_nolock(int number_bytes)
{
  //we need to find an element from m_sorted_free_blocks
  map_type::iterator miter;

  miter=sorted_free_blocks_lower_bound(number_bytes);
  if(miter==m_sorted_free_blocks.end())
    {
      int return_value;

      return_value=m_buffer_object->size_no_lock();
      if(!m_max_buffer_object_size.first
         or (return_value + number_bytes <= m_max_buffer_object_size.second) )
        {
          resize_buffer_object_nolock(return_value+number_bytes);

#ifdef WRATHBUFFERALLOCATORDEBUG
          std::cout << this << ": Allocate: [" << return_value
                    << ", " << return_value+number_bytes << "):"
                    << number_bytes << "\n";
#endif

          m_bytes_allocated+=number_bytes;
          return return_value;
        }
      else
        {
          //allocation fails since outside of allowed range
          return -1;
        }
    }
  else
    {
      free_block_iter free_iter;
      range_type<int> range(-1,-1);
      int return_value;

      //a block was found, use it and remove it
      //from miter->second.
      WRATHassert(!miter->second.empty());
      free_iter=*miter->second.begin();

      //save the free bock data:
      range=free_iter->second;
      return_value=range.m_begin;

      miter->second.erase(miter->second.begin());
      if(miter->second.empty())
        {
          m_sorted_free_blocks.erase(miter);
        }

      //now we need to "put back" the chuck of data
      //of return_value which is not needed for 
      //the allocation.

      //update free_iter, this is fine because
      //m_free_blocks is indexed by the end element.
      free_iter->second.m_begin+=number_bytes;
      
      if(free_iter->second.m_begin==free_iter->second.m_end)
        {
          //took all the room of the free block, remove it.
          m_free_blocks.erase(free_iter);
        }
      else
        {
          insert_free_block_to_available_list(free_iter);
        }
      
#ifdef WRATHBUFFERALLOCATORDEBUG     
      std::cout << this << "Allocate: [" << return_value
                << ", " << return_value+number_bytes << ")\n";
#endif
      
      m_bytes_allocated+=number_bytes;

      return return_value;
    }
}


enum return_code
WRATHBufferAllocator::
fragmented_allocate_nolock(int number_bytes,
                           std::vector< range_type<int> > &out_allocations)
{
  
  enum return_code R(proxy_fragmented_allocate_nolock(number_bytes));

  if(R==routine_success and number_bytes>0)
    {
      std::list<map_type::iterator> empty_keys;

      //we will gobble up the freestore list
      //starting with the _smallest_ elements.
      for(map_type::iterator miter=m_sorted_free_blocks.begin(), 
            mend=m_sorted_free_blocks.end(); 
          miter!=mend and number_bytes>=miter->first; ++miter)
        {
          int current_sz(miter->first);
          std::set<free_block_iter, compare_block_iters>::iterator i, e;

          for(i=miter->second.begin(), e=miter->second.end(); 
              i!=e and number_bytes>=current_sz; ++i)
            {
              number_bytes-=current_sz;
              out_allocations.push_back((*i)->second);
              m_free_blocks.erase(*i);

              m_bytes_allocated+=current_sz;
            }

          miter->second.erase(miter->second.begin(), i);

          if(miter->second.empty())
            {
              empty_keys.push_back(miter);
            }
        }

      for(std::list<map_type::iterator>::iterator iter=empty_keys.begin(),
            end=empty_keys.end(); iter!=end; ++iter)
        {
          m_sorted_free_blocks.erase(*iter);
        }

      if(number_bytes>0)
        {
          int last_loc;

          last_loc=allocate_nolock(number_bytes);
          WRATHassert(last_loc!=-1);
          out_allocations.push_back( range_type<int>(last_loc, last_loc+number_bytes));
        }

      
    }

  return R;
}






bool
WRATHBufferAllocator::
block_is_allocated_nolock(int begin, int end) const
{
  //simple to check since we maintain a list
  //of _FREE_ blocks. if there is a free block
  //between [begin,end) then the block is
  //not allocated.

  free_block_iter_const biter, eiter;

  WRATHassert(begin<end);
  WRATHassert(!m_max_buffer_object_size.first or
         end<=m_max_buffer_object_size.second);

  if(end>m_buffer_object->size_no_lock())
    {
      //block extends past size of buffer object
      return false;
    }

  //find the first block whose end is strictly larger than begin:
  biter=m_free_blocks.upper_bound(begin);
  if(biter==m_free_blocks.end())
    {
      //all free blocks end before or at begin,
      //so block is allocated.
      return true;
    }

  WRATHassert(begin<biter->second.m_end);

  if(end>biter->second.m_begin)
    {
      //portion of [begin.end) intersects
      //[biter->second.m_begin, biter->second.m_end)
      return false;
    }

  if(biter==m_free_blocks.begin())
    {
      //end<=biter->second.m_begin and no blocks
      //before biter, so no intersecion
      return true;
    }

  --biter;

  if(begin<biter->second.m_end)
    {
      //intersecion
      return false;
    }

  return true;
}

void
WRATHBufferAllocator::
print_free_block_info_nolock(std::ostream &ostr, const std::string &prefix) const
{

  ostr << "\n" << prefix << "Size of Buffer Object:" << m_buffer_object->size_no_lock()
       << "\n" << prefix << "Bytes allocated: " << m_bytes_allocated;

  if(!m_free_blocks.empty())
    {
      ostr << "\n" << prefix << "All free blocks: ";
      for(std::map<int, range_type<int> >::const_iterator 
            iter=m_free_blocks.begin(), end=m_free_blocks.end();
          iter!=end; ++iter)
        {
          ostr << "\n" << prefix << "\t[" 
               << iter->second.m_begin << ", "
           << iter->second.m_end << ")";
        }
    }

  if(m_bytes_allocated>0)
    {
      int last_end(0);
      int observed_bytes_allocated(0);

      ostr << "\n" << prefix << "All allocated blocks: ";
      for(std::map<int, range_type<int> >::const_iterator 
            iter=m_free_blocks.begin(), end=m_free_blocks.end();
          iter!=end; ++iter)
        {
          range_type<int> I(iter->second);
          if(last_end<I.m_begin)
            {
              int delta(I.m_begin- last_end);

              ostr << "\n" << prefix << "["
                   << last_end << ", "
                   << I.m_begin << "): "
                   << delta;

              observed_bytes_allocated+=delta;
            }
          last_end=I.m_end;
        }
      if(last_end<m_buffer_object->size_no_lock())
        {
          int delta(m_buffer_object->size_no_lock() - last_end);

          ostr << "\n" << prefix << "["
               << last_end << ", "
               << m_buffer_object->size_no_lock() << "): "
               << delta;
          observed_bytes_allocated+=delta;
        }

      ostr << "\n\tObserved bytes allocated=" << observed_bytes_allocated;
      if(observed_bytes_allocated!=m_bytes_allocated)
        {
          ostr << " NOT the same as recorded bytes allocated!";
        }
    }

  ostr << "\n" << prefix << "Free blocks sorted by sizes";
  for(map_type::const_iterator iter=m_sorted_free_blocks.begin(),
        end=m_sorted_free_blocks.end();
      iter!=end; ++iter)
    {
      ostr << "\n" << prefix << "\t" << iter->first << ":";
      for(std::set<free_block_iter, compare_block_iters>::iterator 
            siter=iter->second.begin(), send=iter->second.end();
          siter!=send; ++siter)
        {
          ostr << "\n" << prefix << "\t\t["
               << (*siter)->second.m_begin << ", "
               << (*siter)->second.m_end << "): "
               << (*siter)->second.m_end - (*siter)->second.m_begin;
        }
    }
}




