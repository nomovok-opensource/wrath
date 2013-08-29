/*! 
 * \file WRATHAbstractDataSink.hpp
 * \brief file WRATHAbstractDataSink.hpp
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




#ifndef __WRATH_ABSTRACT_DATA_SINK_HPP__
#define __WRATH_ABSTRACT_DATA_SINK_HPP__

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include "c_array.hpp"
#include "WRATHMutex.hpp"


/*! \addtogroup Kernel
 * @{
 */

/*!\class WRATHAbstractDataSink
  WRATHAbstractDataSink provides an interface
  to locking and accessing index and/or
  attribute data. It's main purpose is to
  allow the reuse of various \ref WRATHAttributePacker 
  derived classes beyond using \ref WRATHAttributeStore
  to store attributes.
*/
class WRATHAbstractDataSink
{
public:
  virtual
  ~WRATHAbstractDataSink()
  {}
  
  /*!\fn WRATHMutex* mutex(void)
    To be implemented by a derived class
    to return the address of the WRATHMutex
    required to be locked when accessing
    the underlying index data. May return
    NULL to indicate that no such locking
    is rquired.
  */
  virtual
  WRATHMutex*
  mutex(void)=0;
  
  /*!\fn c_array<uint8_t> byte_ptr(int, int)
    To be implemented by a dervied class
    to return a pointer to the named bytes
    for reading and writing. The function 
    will only be called _after_ locking the 
    WRATHMutex returned by \ref mutex().
    \param byte_location offset within data store
    \param number_bytes number bytes starting at byte_location to access
  */
  virtual
  c_array<uint8_t>
  byte_ptr(int byte_location, int number_bytes)=0;
  
  /*!\fn const_c_array<uint8_t> c_byte_ptr(int, int)
    To be implemented by a dervied class
    to return a pointer to the named bytes
    for reading only. The function will only 
    be called _after_ locking the WRATHMutex
    returned by \ref mutex().
    \param byte_location offset within data store
    \param number_bytes number bytes starting at byte_location to access
  */
  virtual
  const_c_array<uint8_t>
  c_byte_ptr(int byte_location, int number_bytes) const=0;
  
  /*!\fn c_array<T> pointer(int, int)
    Provided as a conveniance, equivalent to
    \code
    byte_ptr(byte_location, sizeof(T)*number_elements).reinterpret_pointer<T>();
    \endcode
    \param byte_location offset within data store
    \param number_elements number of _elements_ to access
  */
  template<typename T>
  c_array<T>
  pointer(int byte_location, int number_elements)
  {
    c_array<uint8_t> R;
    R=this->byte_ptr(byte_location, number_elements*sizeof(T));
    return R.reinterpret_pointer<T>();
  }
  
  /*!\fn const_c_array<T> c_pointer(int, int) const
    Provided as a conveniance, equivalent to
    \code
    c_byte_ptr(byte_location, sizeof(T)*number_elements).reinterpret_pointer<T>();
    \endcode
    \param byte_location offset within data store
    \param number_elements number of _elements_ to access
  */
  template<typename T>
  const_c_array<T>
  c_pointer(int byte_location, int number_elements) const
  {
    const_c_array<uint8_t> R;
    R=this->c_byte_ptr(byte_location, number_elements*sizeof(T));
    return R.reinterpret_pointer<T>();
  }
  
  /*!\fn c_array<T> pointer(range_type<int>)
    Provided as a conveniance, equivalent to
    \code
    pointer<T>(R.m_begin*sizeof(T), R.m_end-R.m_begin);
    \endcode
  */
  template<typename T>
  c_array<T>
  pointer(range_type<int> R)
  {
    return pointer<T>(R.m_begin*sizeof(T), R.m_end-R.m_begin);
  }
  
  /*!\fn const_c_array<T> c_pointer(range_type<int>) const
    Provided as a conveniance, equivalent to
    \code
    c_pointer<T>(R.m_begin*sizeof(T), R.m_end-R.m_begin);
    \endcode
  */
  template<typename T>
  const_c_array<T>
  c_pointer(range_type<int> R) const
  {
    return c_pointer<T>(R.m_begin*sizeof(T), R.m_end-R.m_begin);
  }
};

/*! @} */

#endif
