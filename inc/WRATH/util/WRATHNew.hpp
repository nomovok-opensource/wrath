/*! 
 * \file WRATHNew.hpp
 * \brief file WRATHNew.hpp
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


#ifndef __WRATH_NEW_HPP__
#define __WRATH_NEW_HPP__

#include "WRATHConfig.hpp"
#include <ostream>

#ifdef WRATH_NEW_DEBUG
#include <new>

/*!\fn operator new(std::size_t, const char*, int)
  Internal routine used by WRATHNew, do not use directly.
 */
void*
operator new(std::size_t n, const char *file, int line) throw ();

/*!\fn operator new[](std::size_t, const char*, int)
  Internal routine used by WRATHNew, do not use directly.
 */
void*
operator new[](std::size_t n, const char *file, int line) throw ();

/*!\fn operator new(std::size_t)
  Internal routine used by WRATHNew, do not use directly.
 */
void*
operator new(std::size_t n) throw (std::bad_alloc);

/*!\fn operator new[](std::size_t)
  Internal routine used by WRATHNew, do not use directly.
 */
void*
operator new[](std::size_t n) throw (std::bad_alloc);

/*!\fn operator new(std::size_t, const std::nothrow_t&)
  Internal routine used by WRATHNew, do not use directly.
 */
void* 
operator new(std::size_t, const std::nothrow_t&) throw();

/*!\fn operator new[](std::size_t, const std::nothrow_t&)
  Internal routine used by WRATHNew, do not use directly.
 */
void* 
operator new[](std::size_t, const std::nothrow_t&) throw();

/*!\fn operator delete(void*)

  Internal routine used by WRATHNew, do not use directly.
 */
void
operator delete(void *ptr) throw();

/*!\fn operator delete[](void*)
  Internal routine used by WRATHNew, do not use directly.
 */
void
operator delete[](void *ptr) throw();

/*!\fn operator delete(void*, const char*, int)
  Internal routine used by WRATHNew, do not use directly.
 */
void
operator delete(void *ptr, const char *file, int line) throw();

/*!\fn operator delete[](void*, const char*, int)
  Internal routine used by WRATHNew, do not use directly.
 */
void
operator delete[](void *ptr, const char *file, int line) throw();

/*!\fn operator delete(void*, const std::nothrow_t&)
  Internal routine used by WRATHNew, do not use directly.
 */
void 
operator delete(void*, const std::nothrow_t&) throw();

/*!\fn operator delete[](void*, const std::nothrow_t&)
  Internal routine used by WRATHNew, do not use directly.
 */
void 
operator delete[](void*, const std::nothrow_t&) throw();


/*! \addtogroup Utility
 * @{
 */

namespace WRATHMemory
{
  /*!\fn int allocation_call_count
    When WRATH_NEW_DEBUG is defined, returns the
    number of calls to \ref WRATHNew
   */
  int  
  allocation_call_count(void);

  /*!\fn int deallocation_call_count
    When WRATH_NEW_DEBUG is defined, returns the
    number of calls to 
    \ref WRATHDelete and \ref WRATHDelete_array.
   */
  int 
  deallocation_call_count(void); 

  /*!\fn int external_allocation_call_count
    When WRATH_NEW_DEBUG is defined, returns the
    number of calls to new either
    directly via new or via the 
    \ref WRATHNew macro.
   */
  int
  external_allocation_call_count(void); 
  
  /*!\fn int external_deallocation_call_count
    When WRATH_NEW_DEBUG is defined, returns the
    number of calls to delete directly 
    via delete or via the 
    \ref WRATHDelete and \ref WRATHDelete_array macros. 
   */
  int  
  external_deallocation_call_count(void); 
  
  /*!\fn void print_alive_tracked_object(std::ostream&)
    When WRATH_NEW_DEBUG is defined, prints to the passed
    std::ostream a listing of what objects
    that are still alive that were allocated
    with \ref WRATHNew, the printing also includes
    the file and line of the \ref WRATHNew call.
    \param ostr std::ostream to which to print
   */
  void 
  print_alive_tracked_object(std::ostream &ostr);
  
  /*!\fn void untrack_object(volatile void*)
    When WRATH_NEW_DEBUG is defined, untracks an object, i.e.
    it is removed from a private debug table.
    In particular, calling print_alive_tracked_object()
    will no longer list the object.
    \param ptr address (i.e. return value of \ref WRATHNew)
               to untrack.
   */
  void 
  untrack_object(volatile void *ptr);
  
  /*!\fn void set_new_log(std::ostream*)
    Set the std::ostream to which to pring allocation
    debug messages. Such messages are writting at 
    every \ref WRATHNew, \ref WRATHDelete and \ref WRATHDelete_array
    call. To stop printing pass NULL. The default value
    is NULL, i.e. no printing.
    \param ptr pointer to std::ostream to which to print
               allocation and deallocation commands.
   */
  void 
  set_new_log(std::ostream *ptr);
  
  /*!\fn bool object_deletion_message(volatile void*, const char*, int, bool)
    Private function used by macro \ref WRATHDelete,
    do NOT call.
   */
  bool
  object_deletion_message(volatile void *ptr, const char *file, int line, 
                          bool delete_object);
  
  /*!\fn void array_deletion_message(volatile void*, const char*, int)
    Private function used by macro \ref WRATHDelete_array,
    do NOT call.
   */
  void 
  array_deletion_message(volatile void *ptr, const char *file, int line);
}

/*!\def WRATHNew
  For WRATH using projects, use WRATHNew in place of
  new. When WRATH_NEW_DEBUG is defined, allocations and deallocations
  of such objects are tracked and at program exit a list
  of those objects not deleted are printed with the file
  and line number of the allocation. When WRATH_NEW_DEBUG,
  is not defined, WRATHNew maps to new.
 */
#define WRATHNew ::new(__FILE__, __LINE__) 

/*!\def WRATHDelete
  Use WRATHDelete for objects allocated with WRATHNew.
  When WRATH_NEW_DEBUG is not defined, maps to delete.
  \param ptr address of object to delete, value must be a return
             value of \ref WRATHNew
 */
#define WRATHDelete(ptr) do { \
    WRATHMemory::object_deletion_message(ptr, __FILE__, __LINE__, true);       \
::delete ptr; } while(0)

/*!\def WRATHDelete_array
  Use WRATHDelete_array for arrays of objects allocated with WRATHNew.
  When WRATH_NEW_DEBUG is not defined, maps to delete[]. 
  \param ptr address of array of objects to delete, value must be a return
             value of \ref WRATHNew
 */
#define WRATHDelete_array(ptr) do {  \
WRATHMemory::array_deletion_message(ptr,__FILE__,__LINE__); \
::delete []ptr; } while(0)


/*! @} */

#else



#define WRATHNew new
#define WRATHDelete(ptr)       do { delete ptr; } while(0)
#define WRATHDelete_array(ptr) do { delete[] ptr; } while(0)

namespace WRATHMemory
{
inline
int
allocation_call_count(void)   { return 0; }

inline
int
deallocation_call_count(void) { return 0; }

inline
int
external_allocation_call_count(void)   { return 0; }

inline
int
external_deallocation_call_count(void) { return 0; }

inline
void
print_alive_tracked_object(std::ostream&) {}

inline
void
untrack_object(volatile void*){}


inline
void
set_new_log(std::ostream*){}
}

#endif


/*!\fn WRATHDeleteEach(iterator, iterator)

  Conveinance function to delete each element
  of an iterator range of iterators to pointers,
  i.e. iterator::operator*() returns a pointer
  type.
 */
template<typename iterator>
void
WRATHDeleteEach(iterator begin, iterator end)
{
  for(;begin!=end; ++begin)
    {
      WRATHDelete(*begin);
    }
}


#endif 
