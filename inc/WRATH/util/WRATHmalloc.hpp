/*! 
 * \file WRATHmalloc.hpp
 * \brief file WRATHmalloc.hpp
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




#ifndef _WRATH_MALLOC_HPP__
#define _WRATH_MALLOC_HPP__

#include "WRATHConfig.hpp"
#include <cstdlib>
#include <cstddef>
#include <iostream>

#ifdef WRATH_MALLOC_DEBUG



/*!\fn WRATHmalloc_implement(size_t, const char*, int)
  Private method used by \ref WRATHmalloc, do not use.
*/
void*
WRATHmalloc_implement(size_t number_bytes, const char *file, int line);

/*!\fn WRATHrealloc_implement(void*, size_t, const char*, int)
  Private method used by \ref WRATHrealloc, do not use.
*/
void*
WRATHrealloc_implement(void *ptr, size_t number_bytes, const char *file, int line);

  
/*!\fn WRATHfree_implement(void*, const char*, int)
  Private method used by \ref WRATHfree, do not use.
   */
void
WRATHfree_implement(void *ptr, const char *file, int line);

/*! \addtogroup Utility
 * @{
 */
namespace WRATHMemory
{
  /*!\fn void print_alive_tracked_allocs(std::ostream&)
    When WRATH_MALLOC_DEBUG, prints all tracked 
    allocations to an std::ostream. The print out 
    will list the file and line number of all 
    unfreed tracked allocations done with
    \ref WRATHmalloc(). When WRATH_MALLOC_DEBUG
    is not defined is an empty inline function.
    See also \ref WRATHMemory::set_alloc_log()
    \param ostr std::ostream to which to print tracked allocations. 
   */
  void 
  print_alive_tracked_allocs(std::ostream &ostr);

  /*!\fn void set_alloc_log(std::ostream*)
    When WRATH_MALLOC_DEBUG, set the stream to which 
    to print all allocations via \ref WRATHmalloc and 
    \ref WRATHrealloc and deallocations via \ref WRATHfree. 
    Passing NULL disables printout. When WRATH_MALLOC_DEBUG
    is not defined is an empty inline function.
    See also \ref WRATHMemory::print_alive_tracked_allocs()
    default value is NULL.
    \param ptr pointer to std::ostream to which to print log.
   */  
  void
  set_alloc_log(std::ostream *ptr);
}

/*!\def WRATHmalloc(X)
  When WRATH_MALLOC_DEBUG is defined, return values of 
  WRATHmalloc are added to a tracking table, 
  when WRATH_MALLOC_DEBUG is not defined maps to std::malloc.
  See also \ref WRATHfree, \ref WRATHrealloc,
  \ref WRATHMemory::print_alive_tracked_allocs()
  and \ref WRATHMemory::set_alloc_log
  \param X number of bytes to allocate
 */
#define WRATHmalloc(X) WRATHmalloc_implement(X, __FILE__, __LINE__)

/*!\def WRATHfree(X)
  When WRATH_MALLOC_DEBUG is defined, argument to WRATHfree 
  is checked if it is in the tracking table and if so removed 
  and then the argument is free'd. When WRATH_MALLOC_DEBUG 
  is not defined to std::free.
  See also \ref WRATHmalloc, \ref WRATHrealloc,
  \ref WRATHMemory::print_alive_tracked_allocs()
  and \ref WRATHMemory::set_alloc_log
  \param X pointer to memory allocated with WRATHmalloc/WRATHrealloc to free 
 */
#define WRATHfree(X) WRATHfree_implement(X,  __FILE__, __LINE__)

/*!\def WRATHrealloc(p,n)
  When WRATH_MALLOC_DEBUG is defined, table entries are updated 
  (if necessary) and the memory pointer to by p is reallocated, 
  When WRATH_MALLOC_DEBUG is not defined maps to std::realloc.
  See also \ref WRATHmalloc, \ref WRATHfree,
  \ref WRATHMemory::print_alive_tracked_allocs()
  and \ref WRATHMemory::set_alloc_log
  \param p pointer to buffer as returned by WRATHmalloc/WRATHrealloc
           to reallocate to larger or smaller area
  \param n new size in bytes to make buffer
 */
#define WRATHrealloc(p,n) WRATHrealloc_implement(p, n,  __FILE__, __LINE__)

/*! @} */

#else


#define WRATHmalloc(X) std::malloc(X)
#define WRATHfree(X) std::free(X)
#define WRATHrealloc(p,n) std::realloc(p,n)

namespace WRATHMemory
{
inline
void 
print_alive_tracked_allocs(std::ostream&)
{}

inline
void
set_alloc_log(std::ostream*){}
}

#endif




#endif
