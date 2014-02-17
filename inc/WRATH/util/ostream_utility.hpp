/*! 
 * \file ostream_utility.hpp
 * \brief file ostream_utility.hpp
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



#ifndef WRATH_OSTREAM_UTILITY_HPP_
#define WRATH_OSTREAM_UTILITY_HPP_

#include "WRATHConfig.hpp"
#include <iostream>
#include <string>
#include <list>
#include <set>
#include <utility>

/*! \addtogroup Utility
 * @{
 */

namespace WRATHUtil
{

  /*!\class format_tabbing
    Simple class with overloaded operator<<
    to print a number of indenting characters
    to an std::ostream.
  */
  class format_tabbing
  {
  public:
    /*!\fn format_tabbing
      construct a format_tabbing
      \param ct number of times to print the indent character
      \param c indent character, default is a tab (i.e. \\t)
    */
    explicit
    format_tabbing(unsigned int ct, char c='\t'):m_count(ct), m_char(c) {}

    /*!\var m_count
      Number of times to print \ref m_char.
     */
    unsigned int m_count;

    /*!\var m_char
      Indent character to print.
     */
    char m_char;
  };
  
  /*!\class print_range_type
    Simple type to print an STL range of elements
    via overloading operator<<.
  */
  template<typename iterator>
  class print_range_type
  {    
  public:
    /*!\fn print_range_type
      Ctor.
      \param begin iterator to 1st element to print
      \param end iterator to one past the last element to print
      \param spacingCharacter string to print between consecutive elements
    */
    print_range_type(iterator begin, iterator end, 
                     const std::string &spacingCharacter=", "):
      m_begin(begin), m_end(end), m_spacingCharacter(spacingCharacter) {}

    /*!\var m_begin
      Iterator to first element to print
     */
    iterator m_begin;

    /*!\var m_end
      Iterator to one element past the last element to print
     */
    iterator m_end;

    /*!\var m_spacingCharacter
      string to print between consecutive elements
     */
    std::string m_spacingCharacter;
  };

  /*!\fn print_range_type<iterator> print_range(iterator, iterator, const std::string&)
    Returns a print_range_type to print a 
    range of elements to an std::ostream.
    \tparam iterator type
    \param begin iterator to 1st element to print
    \param end iterator to one past the last element to print
    \param str string to print between consecutive elements  
  */
  template<typename iterator> 
  print_range_type<iterator> 
  print_range(iterator begin, iterator end, const std::string &str=", ")
  {
    return print_range_type<iterator>(begin, end, str);
  }

}


/*!\fn std::ostream& operator<<(std::ostream&, const WRATHUtil::print_range_type<iterator>&)
  overload of operator<< to print the contents of
  an iterator range to an std::ostream.
  \param ostr std::ostream to which to print
  \param obj print_range_type object to print to ostr
*/
template<typename iterator>
std::ostream&
operator<<(std::ostream &ostr, 
           const WRATHUtil::print_range_type<iterator> &obj)
{
  iterator iter;
  
  for(iter=obj.m_begin; iter!=obj.m_end; ++iter)
    {
      if(iter!=obj.m_begin)
        {
          ostr << obj.m_spacingCharacter;
        }
      
      ostr << (*iter);
      
    }
  return ostr;
}

/*!\fn std::ostream& operator<<(std::ostream&, const WRATHUtil::format_tabbing&)
  overload of operator<< to print a format_tabbing object
  to an std::ostream.
  \param str std::ostream to which to print
  \param tabber format_tabbing object to print to str
*/ 
inline
std::ostream&
operator<<(std::ostream &str, const WRATHUtil::format_tabbing &tabber)
{
  for(unsigned int i=0;i<tabber.m_count;++i)
    {
      str << tabber.m_char;
    }
  return str;
}

/*!\fn std::ostream& operator<<(std::ostream&, const std::pair<T,S>&)
  conveniance operator<< to print an std::pair to an std::ostream.
  \param str std::ostream to which to print 
  \param obj std::pair to print to str
 */
template<typename T, typename S>
inline 
std::ostream&
operator<<(std::ostream &str, const std::pair<T,S> &obj)
{
  str << "(" << obj.first << "," << obj.second << ")";
  return str;
}

/*!\fn std::ostream& operator<<(std::ostream&, const std::set<T,_Compare,_Alloc>&)
  conveniance operator<< to print the contents
  of an std::set to an std::ostream.
  \param str std::ostream to which to print 
  \param obj std::set to print to str
 */
template<typename T, typename _Compare, typename _Alloc>
inline
std::ostream&
operator<<(std::ostream &str, const std::set<T,_Compare,_Alloc> &obj)
{
  str <<  "{ " << WRATHUtil::print_range(obj.begin(), obj.end(), ", ") << " }";
  return str;
}

/*!\fn std::ostream&  operator<<(std::ostream&, const std::list<T,_Alloc>&)
  conveniance operator<< to print the contents
  of an std::list to an std::ostream.
  \param str std::ostream to which to print 
  \param obj std::list to print to str
 */
template<typename T, typename _Alloc>
inline 
std::ostream&
operator<<(std::ostream &str, const std::list<T,_Alloc> &obj)
{
  str <<   "( " << WRATHUtil::print_range(obj.begin(), obj.end(), ", ") << " )";
  return str;
}


/*!\fn std::ostream& operator<<(std::ostream&, const range_type<T>&)
  conveniance operator<< to print the values
  of a range_type.
  \param ostr std::ostream to which to print 
  \param obj range_type to print to str
 */
template<typename T>
std::ostream&
operator<<(std::ostream &ostr, const range_type<T> &obj)
{
  ostr << "[" << obj.m_begin << "," << obj.m_end << ")";
  return ostr;
}


/*! @} */

#endif
