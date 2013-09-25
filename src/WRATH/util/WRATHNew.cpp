/*! 
 * \file WRATHNew.cpp
 * \brief file WRATHNew.cpp
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
#include "WRATHNew.hpp"
#include "WRATHMutex.hpp"
#include <map>
#include <iostream>
#include <iomanip>
#include <iosfwd>
#include <cstdlib>
#include <sstream>


#define BAD_MALLOC(X) do { std::cerr << "Allocation of " << X << " bytes failed\n"; /*throw std::bad_alloc();*/ } while(0)


#ifdef WRATH_NEW_DEBUG


typedef std::pair<const char*,int> file_list_str;



namespace
{
  int number_allocation_calls=0;
  int number_deallocation_calls=0;
  int external_number_allocation_calls=0;
  int external_number_deallocation_calls=0;
  std::ostream *alloc_log=NULL;

  class address_set_type:public std::map<volatile void*, file_list_str>
  {
  public:

    virtual
    ~address_set_type()
    {
      if(!empty())
        {
          std::cerr << "\n\nTracked allocated objects remaining:\n"; 
          for(std::map<volatile void*, file_list_str>::const_iterator
                i=begin(), e=end(); i!=e; ++i)
            {
               std::cerr << const_cast<void*>(i->first) << "[" << i->second.first
                         << "," << i->second.second << "]\n";
            }
        }
    }

    WRATHMutex&
    address_mutex(void)
    {
      return m_address_mutex;
    }

    WRATHMutex&
    log_mutex(void)
    {
      return m_log_mutex;
    }

  private:
    WRATHMutex m_address_mutex;
    WRATHMutex m_log_mutex;
  };

    
  address_set_type&
  address_set(void)
  {
    static address_set_type retval;
    return retval;
  }

  
  void*
  local_malloc(size_t n)
  {
    void *retval;

    retval=std::malloc(n);
    return retval;
  }

  void
  local_free(void *ptr)
  {
    std::free(ptr);
  }

  void
  print_to_alloc_log(const std::string &message)
  {
    WRATHLockMutex(address_set().log_mutex());
    if(alloc_log!=NULL)
      {
        *alloc_log << message << std::flush;
      }
    WRATHUnlockMutex(address_set().log_mutex());
  }

}


void
WRATHInternalNewInit(void)
{
  /*
    make the object behind the tracker for
    WRATHNew/WRATHDelete be alive.
   */
  address_set();
}

void
WRATHMemory::
set_new_log(std::ostream *ptr)
{
  WRATHLockMutex(address_set().log_mutex());
  alloc_log=ptr;
  WRATHUnlockMutex(address_set().log_mutex());
}



#define AllocLogPrint(X, file, line) do {\
  std::ostringstream ostr;\
  ostr << "\nAllocLog[" << file << "," << line << "] " << X; \
  print_to_alloc_log(ostr.str());                               \
  } while(0)


void 
WRATHMemory::
print_alive_tracked_object(std::ostream &ostr)
{
  std::map<volatile void*, file_list_str>::const_iterator iter;
  

  WRATHLockMutex(address_set().address_mutex());
  const std::map<volatile void*, file_list_str> &obj(address_set());

  for(iter=obj.begin();iter!=obj.end();++iter)
    {
      if(iter->second.first!=NULL)
        {
          ostr << const_cast<void*>(iter->first) << "[" << iter->second.first
               << "," << iter->second.second << "]\n";
        }
    }
  WRATHUnlockMutex(address_set().address_mutex());
}


bool 
WRATHMemory::
object_deletion_message(volatile void *ptr, const char *file, int line, 
                        bool delete_entry)
{
  std::map<volatile void*, file_list_str>::iterator iter;
  bool return_value;

  
  if(ptr==NULL)
    {
      return true; //ignore NULL pointer.
    }

  WRATHLockMutex(address_set().address_mutex());

  if(delete_entry)
    {
      ++number_deallocation_calls;
    }

  iter=address_set().find(ptr);
  if(iter!=address_set().end())
    {
      AllocLogPrint("Deallocate object at " 
                    << const_cast<void*>(ptr)
                    << "(from " << iter->second.first << "," 
                    << iter->second.second << ")", file, line);
      if(delete_entry)
        {
          address_set().erase(iter);
        }
      return_value=true;
    }
  else
    {
      AllocLogPrint("Deallocate object (not in map) at " 
                    << const_cast<void*>(ptr)
                    << " {" << number_allocation_calls-number_deallocation_calls 
                    << "}", file, line);
      return_value=false;
    }
  

 if(delete_entry)
   {
     --external_number_deallocation_calls; //because delete will increment this.
   }

  WRATHUnlockMutex(address_set().address_mutex());
  
  if(!return_value)
    {
      std::cerr << "Deletion from [" << file << ", " 
                << line << "] of untracked object@" 
                << const_cast<void*>(ptr) << "\n";
    }

  return return_value;
}








void 
WRATHMemory::
array_deletion_message(volatile void *ptr, const char *file, int line)
{
  if(ptr==NULL)
    {
      return; //ignore NULL pointer.
    }

  ++number_deallocation_calls;
  AllocLogPrint("Deallocate array at " << ptr << "("
                << (reinterpret_cast< void * volatile*>(ptr)-1) 
                << " {" << number_allocation_calls-number_deallocation_calls 
                << "}", file, line);

  
  --external_number_deallocation_calls; //because delete will increment this.
}

int 
WRATHMemory::
allocation_call_count(void)    { return number_allocation_calls; }

int 
WRATHMemory::
deallocation_call_count(void)  { return number_deallocation_calls; }

int 
WRATHMemory::
external_allocation_call_count(void)   { return external_number_allocation_calls; }

int 
WRATHMemory::
external_deallocation_call_count(void) { return external_number_deallocation_calls; }

void
WRATHMemory::
untrack_object(volatile void *ptr)
{
  WRATHLockMutex(address_set().address_mutex());
  std::map<volatile void*, file_list_str>::iterator iter;

  iter=address_set().find(ptr);
  if(iter!=address_set().end())
    {
      iter->second.first=NULL;
    }
  WRATHUnlockMutex(address_set().address_mutex());
}

void*
operator new(std::size_t n, const char *file, int line) throw ()
{
  void *retval;

  

  retval=local_malloc(n);
  

  WRATHLockMutex(address_set().address_mutex());
 
  std::pair<volatile void*,file_list_str> v(retval, file_list_str(file,line));
  address_set().insert(v);

  WRATHUnlockMutex(address_set().address_mutex());

  AllocLogPrint("Allocate object at " << retval << " of " << n << " bytes", file, line);

  if(retval==NULL)
    {
      BAD_MALLOC(n);
    }

  ++number_allocation_calls;

  return retval;
}

void*
operator new[](std::size_t n, const char *file, int line) throw ()
{
  void *retval;

 

  retval=local_malloc(n);
  if(retval==NULL)
    {
      BAD_MALLOC(n);
    }

  AllocLogPrint("Allocate array at " << retval << " of " << n << " bytes", file, line);

  ++number_allocation_calls;
  return retval;
}




void*
operator new(std::size_t n) throw (std::bad_alloc)
{
  void *retval;

  retval=local_malloc(n);
  if(retval==NULL)
    {
      BAD_MALLOC(n);
    }

  ++external_number_allocation_calls;
  return retval;
}

void*
operator new[](std::size_t n) throw (std::bad_alloc)
{
  void *retval;

  retval=local_malloc(n);
  if(retval==NULL)
    {
      BAD_MALLOC(n);
    }

  ++external_number_allocation_calls;

  return retval;
}


void*
operator new(std::size_t n, const std::nothrow_t&) throw ()
{
  void *retval;
  retval=local_malloc(n);
  
  if(retval==NULL)
    {
      BAD_MALLOC(n);
    }

  ++external_number_allocation_calls;
  return retval;
}

void*
operator new[](std::size_t n, const std::nothrow_t&) throw ()
{
  void *retval;
  retval=local_malloc(n);

  if(retval==NULL)
    {
      BAD_MALLOC(n);
    }
 
  ++external_number_allocation_calls;
  return retval;
}


void 
operator delete(void *ptr) throw()
{
  ++external_number_deallocation_calls;
  local_free(ptr);
}

void 
operator delete[](void *ptr) throw()
{  
  ++external_number_deallocation_calls;
  local_free(ptr);
}


void 
operator delete(void *ptr, const char *file, int line) throw()
{
  AllocLogPrint("Deallocate object at " << ptr
                << "[" << file << "," << line << "]", file, line);
  
  local_free(ptr);
  ++number_deallocation_calls;
}

void 
operator delete[](void *ptr, const char *file, int line) throw()
{
  AllocLogPrint("Deallocate array at " << ptr
                << "[" << file << "," << line << "]", file, line);
  local_free(ptr);
  ++number_deallocation_calls;
}

void 
operator delete(void *ptr, const std::nothrow_t&) throw()
{
  ++external_number_deallocation_calls;
  local_free(ptr);
}

void 
operator delete[](void *ptr, const std::nothrow_t&) throw()
{
  ++external_number_deallocation_calls;
  local_free(ptr);
}


#else


void
WRATHInternalNewInit(void)
{

}


#endif



