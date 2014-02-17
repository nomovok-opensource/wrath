/*! 
 * \file WRATHUTF8.hpp
 * \brief file WRATHUTF8.hpp
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




#ifndef WRATH_HEADER_UTF8_HPP_
#define WRATH_HEADER_UTF8_HPP_

#include "WRATHConfig.hpp"
#include "WRATHassert.hpp" 
#include <iterator>
#include <stdint.h>
#include "type_tag.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\class WRATHUTF8
  A WRATHUTF8 is a _wrapper_ over
  an iterator range, i.e. it is almost
  completely data free. The iterator API
  that it exposes decodes the iterator
  range into uint32_t's. A WRATHUTF8
  decodes a range of UTF8 characters through
  an STL-iterator interface, below is an example
  where the utf8 stream comes from an STL container:
  \code
  std::list<uint8_t> utf_data;
  
  std::vector<uint32_t> decoded;
  typedef WRATHUTF8<std::list<uint8_t>::const_iterator> UTF8;
  UTF8 C(utf_data.begin(), utf_data.end());

  decoded.reserve(std::distance(C.begin(), C.end()));
  for(UTF8::iterator iter=C.begin(), end=C.end(); iter!=end; ++iter)
    {
      decoded.push_back(*iter);
    }

  \endcode

  Note that WRATHUTF8 does NOT expect BOM markers, i.e.
  if a BOM marker starts the data, it is an error.

  \param T T should be an input iterator type,
           so that T::value_type can be used
           to initialize a uint8_t. The iterator
           type need only be a read only forward 
           direction iterator. 

 */
template<typename T>
class WRATHUTF8
{
public:  
  /*!\class iterator
    Iterator class for WRATHUTF8, support read and increment
    operations only. Internally is represented by 2 T's.
   */
  class iterator:public std::iterator<std::input_iterator_tag, uint32_t>
  {
  public:
    /*!\fn iterator(void)
      Ctor. Empty initialization.
     */
    iterator(void):
      m_end(),
      m_location()
    {}

    /*!\fn uint32_t operator*(void) const
      Returns the Unicode value at the current
      position of the iterator, if the utf8
      stream is invalid at the current iterator
      return 0xDC80. Invalid in this context
      means any of the following is encountered:
      - missing continuation bytes
      - starting byte does not have a 0 in
        a higher bit that bit 0 (i.e. the
        starting byte indicates greater
        than 6 bytes total to describe unicode).
      - start character leading bits is "10"
        (such leading bits are for continuation
        bytes only)
      - unicode value encoded in leading byte
        plus it's continuation bytes can be
        encoded with fewer bytes.

      \n\n i.e. return 0xDC80 if the UTF8 is incorrectly
           encoded, otherwise return the UTF character
           whoses encoding is at this iterator's location.

      \n\n For now, operator* does NOT check if the
           return value is a valid Unicode character.
     */
    uint32_t
    operator*(void) const
    {
      //get the character starting at the current 
      //position, take care to not go past end.
      //if we are already at end, WRATHassert.
      WRATHassert(m_location!=m_end);
      T current(m_location);

      uint8_t start_value(*current), header_length(0);
      WRATHassert( is_start_character(start_value));
      
      //count the number of leading 1 bits
      //and set them as zero:
      uint8_t current_mask(128);
      while(current_mask&start_value)
        {
          ++header_length;
          start_value&=~current_mask;
          current_mask=(current_mask>>1);
        }

      WRATHassert(header_length!=1);
      if(header_length==0)
        {
          //just an ASCII code:
          return start_value;
        }

      if(header_length==1 or header_length>6)
        {
          return 0xDC80;
        }
      
      int number_continuation_bytes(header_length-1);
      uint32_t return_value(start_value);

      for(++current; number_continuation_bytes>0 and current!=m_end; ++current, --number_continuation_bytes)
        {
          uint8_t c;
          
          c=uint8_t(*current);
          if((c&(128|64))!=128)
            {
              //not good, we found a start character
              //too soon, we will return 0xDC80, a common
              //place holder value for bad UTF8 characters:
              return 0xDC80;
            }
          c&=~128;

          return_value=return_value<<6;
          return_value=return_value | c;
        }

      //check if the return value really needed
      //to take up so many bytes, this is done as follows:
      // if 2 bytes in size --> return_value must take atleast  8 bits to hold, i.e. atleast  128
      // if 3 bytes in size --> return_value must take atleast 12 bits to hold, i.e. atleast 2048
      // if 4 bytes in size --> return_value must take atleast 17 bits to hold, i.e. atleast 
      // if 5 bytes in size --> return_value must take atleast 22 bits to hold, i.e. atleast
      // if 6 bytes in size --> return_value must take atleast 27 bits to hold, i.e. atleast
      const uint32_t minimum_size[]=
        {
          1<<7 , //header_length=2
          1<<11, //header_length=3
          1<<16, //header_length=4
          1<<21, //header_length=5
          1<<26, //header_length=6
        };

      if(return_value<minimum_size[header_length-2])
        {
          return 0xDC80;
        }

      //TODO:
      // check that return_value is a valid Unicode character.

      return return_value;
    }

    /*!\fn const iterator& operator++(void)
      Pre-increment, i.e. increment
      the iterator then return it's 
      value.
     */
    const iterator&
    operator++(void)
    {
      increment();
      return *this;
    }

    /*!\fn iterator operator++(int)
      Post-increment, i.e save the current
      value, increment the iterator and then
      return the previosuly saved value.
     */
    iterator
    operator++(int)
    {
      iterator return_value(*this);
      increment();
      return return_value;
    }

    /*!\fn bool operator==(const iterator&) const
      Test if 2 iterators are equal, the 
      test only checks if the current
      location of the iterators are the
      same, it does not check if the
      iterator considered to be the end of the
      streams to be different. Under debug
      build will WRATHassert if the iterators
      do not agree on where the stream
      ends.
      \param rhs iterator to test against.
     */
    bool
    operator==(const iterator &rhs) const
    {
      WRATHassert(rhs.m_end==m_end);
      return rhs.m_location==m_location;
    }

    /*!\fn bool operator!=(const iterator &) const
      Test if two iterators are different,
      equivalent to !operator==.
      \param rhs iterator to test against.
     */
    bool
    operator!=(const iterator &rhs) const
    {
      return !operator==(rhs);
    }

  private:
    friend class WRATHUTF8;

    iterator(range_type<T> R):
      m_end(R.m_end),
      m_location(R.m_begin)
    {
      increment_to_start_character();
    }

    iterator(T pend):
      m_end(pend),
      m_location(pend)
    {}

    void
    increment(void)
    {
      WRATHassert(m_location!=m_end);
      ++m_location;
      increment_to_start_character();
    }

    static
    bool
    is_start_character(uint8_t v)
    {
      //a start character in UTF8 is just
      //a character that does NOT start with 10
      //in binary.
      return (v&(128|64))!=128;
    }

    void
    increment_to_start_character(void)
    {
      while(m_location!=m_end and !is_start_character( uint8_t(*m_location)))
        {
          ++m_location;
        }
    }

    T m_end;
    T m_location;
  };

  /*!\fn WRATHUTF8(T, T)
    Ctor. Create a WRATHUTF8 stream
    from a pair of iterator's of type
    T. A WRATHUTF8 does NOT copy the
    data from the range, it only saves
    the iterators, thus they must
    remain valid for the lifetime
    of the WRATHUTF8.
    \param pbegin iterator to first "uint8_t" 
                  to be intepreted as UTF8 encoded data 
    \param pend iterator to one element past 
                the last "uint8_t" UTF8 encoded data. 
   */
  WRATHUTF8(T pbegin, T pend):
    m_range(pbegin, pend),
    m_begin(m_range),
    m_end(m_range.m_end)
  {}

  /*!\fn WRATHUTF8(range_type<T>)
    Ctor. Create a WRATHUTF8 stream
    from a pair of iterator's of type
    T. A WRATHUTF8 does NOT copy the
    data from the range, it only saves
    the iterators, thus they must
    remain valid for the lifetime
    of the WRATHUTF8.
    \param R range of iterators with
             R.m_begin the iterator
             to the first "uint8_t" 
             to be intepreted as UTF8 
             encoded data and R.m_end 
             to one element past 
             the last "uint8_t" UTF8 
             encoded data. 
    
   */
  WRATHUTF8(range_type<T> R):
    m_range(R),
    m_begin(m_range),
    m_end(m_range.m_end)
  {}

  /*!\fn const iterator& begin
    Returns the iterator to the first
    element of the decoded UTF stream.
   */
  const iterator&
  begin(void) const
  {
    return m_begin;
  }

  /*!\fn const iterator& end
    Returns the "iterator" to the one
    past the last element of the decoded
    UTF stream.
   */
  const iterator&
  end(void) const
  {
    return m_end;
  }

  /*!\fn bool empty
    Returns true if the UTF stream
    is empty, equivalent to 
    begin()==end().
   */
  bool
  empty(void) const
  {
    return m_begin==m_end;
  }

  /*!\fn bool valid_utf
    Returns true if and only if the
    UTF8 encoding is valid. Executes 
    the test by iterating through
    the range [begin(),end()) and 
    checking if an iterator ever 
    returns 0XDC80 from 
    iterator::operator*().
   */
  bool
  valid_utf(void) const
  {
    iterator b(begin()), e(end());
    for(;b!=e; ++b)
      {
        if(*b==0xDC80)
          {
            return false;
          }
      }
    return true;
  }

private:
  range_type<T> m_range;
  iterator m_begin, m_end;
};

/*! @} */

#endif 
