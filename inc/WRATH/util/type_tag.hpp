/*! 
 * \file type_tag.hpp
 * \brief file type_tag.hpp
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


#ifndef __TYPE_TAG_HPP__
#define __TYPE_TAG_HPP__


#include "WRATHConfig.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\enum copy_range_tag_type
  Enumeration type to label to copy a range
  of values.
 */
enum copy_range_tag_type
{
  /*!
    Enumeration type to label to copy a range
    of values.
   */
  copy_range_tag,
};


/*!\enum file_type
  Enumeration to define if a file is viewed
  as a binary or text file.
 */
enum file_type
  {
    /*!
      File is viewed as a binary file.
     */
    binary_file,

    /*!
      File is viewed as a text file.
     */
    text_file
  };

/*!\enum return_code
  Enumeration for simple return codes for functions
  for success or failure.
 */
enum return_code
  {    
    /*!
      Routine failed
     */
    routine_fail,

    /*!
      Routine suceeded
     */
    routine_success
  };

/*!\class range_type
  A class reprenting the STL range
  [m_begin, m_end).
 */
template<typename T>
class range_type
{
public:
  /*!\fn range_type(T,T)
    Ctor.
    \param b value with which to initialize \ref m_begin
    \param e value with which to initialize \ref m_end
   */
  range_type(T b, T e):
    m_begin(b),
    m_end(e)
  {}

  /*!\fn range_type(void)
    Empty ctor, \ref m_begin and \ref m_end are uninitialized.
   */
  range_type(void)
  {}

  /*!\var m_begin
    Iterator to first element
   */
  T m_begin;

  /*!\var m_end
    iterator to one past the last element
   */
  T m_end;
};

/*!\class type_tag
  Template meta-programming helper
  to specify a type via a tag.
 */
template<typename T>
struct type_tag 
{
  /*!\typedef type
    Get the original type from this typedef.
   */
  typedef T type;
};

/*!\fn get_type_tag(const T&)
  Template meta-programming helper
  to get a type tag from a type.
 */
template<typename T>
struct type_tag<T>
get_type_tag(const T&)
{
  return type_tag<T>();
}


/*! @} */



#endif
