/*! 
 * \file WRATHmalloc.cpp
 * \brief file WRATHmalloc.cpp
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
#include "WRATHmalloc.hpp"

#ifdef WRATH_MALLOC_DEBUG

#include "WRATHMutex.hpp"
#include <map>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <iosfwd>


namespace
{
  typedef std::pair<const char*,int> file_list_str;
  std::ostream *alloc_log=NULL;

  class address_set_type:public std::map<volatile void*, file_list_str>
  {
  public:

    virtual
    ~address_set_type()
    {
      if(!empty())
        {
          std::cerr << "\n\nTracked mallocs remaining:\n"; 
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
WRATHInternalMallocInit(void)
{
  /*
    make the object behind the tracker for
    WRATHNew/WRATHDelete be alive.
   */
  address_set();
}


#define AllocLogPrint(X, file, line) do {\
  std::ostringstream ostr;\
  ostr << "AllocLog[" << std::setw(40) << file << "," << std::setw(6) << line << "] " << X << "\n"; \
  print_to_alloc_log(ostr.str());                               \
  } while(0)

void
WRATHMemory::
set_alloc_log(std::ostream *ptr)
{
  WRATHLockMutex(address_set().log_mutex());
  alloc_log=ptr;
  WRATHUnlockMutex(address_set().log_mutex());
}

void 
WRATHMemory::
print_alive_tracked_allocs(std::ostream &ostr)
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


void*
WRATHrealloc_implement(void *ptr, size_t number_bytes, const char *file, int line)
{
  if(ptr==NULL)
    {
      return WRATHmalloc_implement(number_bytes, file, line);
    }

  WRATHLockMutex(address_set().address_mutex());

  std::map<volatile void*, file_list_str>::iterator iter;

  iter=address_set().find(ptr);
  if(iter!=address_set().end())
    {
      AllocLogPrint("Realloc memory for " << number_bytes << " at " 
                    << std::setw(30) << ptr
                    << " (from " << iter->second.first << "," 
                    << iter->second.second << ")", file, line);   
    }
  else
    {
      AllocLogPrint("Realloc memory (not in map) at " 
                    << const_cast<void*>(ptr), file, line);
      
      std::cerr <<"Realloc from [" << file << ", " 
                << line << "] of untracked memory@" << ptr << "\n";
    }

  void *return_value;
  return_value=std::realloc(ptr, number_bytes);

  if(return_value==NULL and number_bytes!=0)
    {
      std::cerr << "Reallocation of " <<  number_bytes
                << " bytes failed [" << file << ", "
                << line << "]\n";
    }

  if(return_value!=ptr and iter!=address_set().end())
    {
      if(return_value!=NULL)
        {
          address_set()[return_value]=iter->second;
        }
      address_set().erase(iter);
    }


  WRATHUnlockMutex(address_set().address_mutex());

  return return_value;
}

void
WRATHfree_implement(void *ptr, const char *file, int line)
{
  std::map<volatile void*, file_list_str>::iterator iter;
  bool return_value;
  
  if(ptr==NULL)
    {
      return; //ignore NULL pointer.
    }

  WRATHLockMutex(address_set().address_mutex());
 
  iter=address_set().find(ptr);
  if(iter!=address_set().end())
    {
      AllocLogPrint("Deallocate memory at " 
                    << std::setw(30) << const_cast<void*>(ptr)
                    << " (from " << iter->second.first << "," 
                    << iter->second.second << ")", file, line);
      address_set().erase(iter);
      return_value=true;
    }
  else
    {
      AllocLogPrint("Deallocate memory (not in map) at " 
                    << const_cast<void*>(ptr), file, line);
      return_value=false;
    }
  
  std::free(ptr);

  WRATHUnlockMutex(address_set().address_mutex());
  
  if(!return_value)
    {
      std::cerr << "Free from [" << file << ", " 
                << line << "] of untracked memory@" << ptr;
    }

}

void*
WRATHmalloc_implement(size_t n, const char *file, int line)
{
  void *retval;

  WRATHLockMutex(address_set().address_mutex());

  retval=std::malloc(n);
  AllocLogPrint("Allocate memory at " << retval << " of " << n << " bytes", file, line);

  if(retval!=NULL)
    {
      

      std::pair<void*,file_list_str> v(retval, file_list_str(file,line));
      address_set().insert(v);
    }

  if(retval==NULL and n!=0)
    {
      std::cerr << "Allocation of " << n 
                << " bytes failed [" << file << ", "
                << line << "]";
    }

  WRATHUnlockMutex(address_set().address_mutex());

 
  return retval;
}


#else


void
WRATHInternalMallocInit(void)
{

}

#endif
