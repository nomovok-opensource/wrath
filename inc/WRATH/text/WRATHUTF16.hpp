/*! 
 * \file WRATHUTF16.hpp
 * \brief file WRATHUTF16.hpp
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




#ifndef WRATH_HEADER_UTF16_HPP_
#define WRATH_HEADER_UTF16_HPP_

#include "WRATHConfig.hpp"
#include "WRATHassert.hpp" 
#include <iterator>
#include <stdint.h>
#include "type_tag.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\class WRATHUTF16
  A WRATHUTF16 is a _wrapper_ over
  an iterator range, i.e. it is almost
  completely data free. The iterator API
  that it exposes decodes the iterator
  range into uint32_t's. A WRATHUTF16
  decodes a range of UTF16 characters through
  an STL-iterator interface, below is an example
  where the utf16 stream comes from an STL container:
  \code
  std::list<uint16_t> utf_data;
  
  std::vector<uint32_t> decoded;
  typedef WRATHUTF16<std::list<uint16_t>::const_iterator> UTF16;
  UTF16 C(utf_data.begin(), utf_data.end());

  decoded.reserve(std::distance(C.begin(), C.end()));
  for(UTF16::iterator iter=C.begin(), end=C.end(); iter!=end; ++iter)
    {
      decoded.push_back(*iter);
    }

  \endcode

  Note that WRATHUTF16 does NOT expect BOM markers, i.e.
  if a BOM marker starts the data, it is an error.
  The values returned by T::operator* are expected to
  correctly initialize an uint16_t. Checking for a
  BOM and performing the necessary byte swapping is 
  NOT performed by WRATHUTF16. 

  \param T T should be an input iterator type,
           so that T::value_type can be used
           to initialize a uint16_t. The iterator
           type need only be a read only forward 
           direction iterator. 
 */
template<typename T>
class WRATHUTF16
{
public:
  /*!\class iterator
    Iterator class for WRATHUTF16, support read and increment
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
      position of the iterator, if the utf16 
      stream is invalid at the current iterator
      position, will NOT WRATHassert or throw an 
      exception, but will return values as follows,
      let w1 be the uint16_t at the current position:
      - if w1 is in the range [0xDBFF,0xDFFF], return 0xDC80.
      - if w1 is 0xDC80 return 0xDC80.
      - if w1 indicates a 2 character code by
        being in the range (0xD800,0xDBFF) but w1 
        is the last value from the original stream,
        then return 0xDC80
      - Let w2 be the next value in the utf stream,
        if w2 in not in the range (0xDC00,0xDFFF)
        return 0xDC80.

      \n\n i.e. return 0xDC80 if the UTF16 is incorrectly
           encoded, otherwise return the UTF character
           whoses encoding is at this iterator's location.

      \n\n For now, operator* does NOT check if the
           return value is a valid Unicode character.
     */
    uint32_t
    operator*(void) const
    {
      //UTF16 decoding, see http://www.ietf.org/rfc/rfc2781.txt
      uint16_t w1;

      WRATHassert(m_location!=m_end);
      w1=*m_location;
          
      if(w1<0xD800 or w1>0xDFFF)
        {
          return uint32_t(w1);
        }

      //w1 needs to be between 0xD800 and 0xDBFF
      if(w1>=0xDBFF or w1<=0xD800)
        {
          //bad utf16 stream.
          return 0xDC80;
        }

      T current(m_location);
      ++current;

      if(current==m_end)
        {
          //end of stream, but there
          //should be a continuation character
          return 0xDC80;
        }
      uint16_t w2(*current);

      //w2 needs to be between 0xDC00 and 0xDFFF
      if(w2<=0xDC00 or w2>=0xDFFF)
        {
          return 0xDC80;
        }

      //TODO:
      // check that return_value is a valid Unicode character.

      uint32_t return_value;
      return_value = ( (lower10bits&w1)<<10 ) | (lower10bits&w2);
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

    /*!\fn bool operator!=(const iterator&) const
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
    friend class WRATHUTF16;

    iterator(range_type<T> R):
      m_end(R.m_end),
      m_location(R.m_begin)
    {}

    iterator(T pend):
      m_end(pend),
      m_location(pend)
    {}

    void
    increment(void)
    {
      WRATHassert(m_location!=m_end);
      
      uint16_t w1;
      w1=*m_location;
      
      ++m_location;
      if(m_location!=m_end and
         !(w1<0xD800 or w1>0xDFFF))
        {
          ++m_location;
        }
    }

    T m_end;
    T m_location;
  };

  /*!\fn WRATHUTF16(T, T)
    Ctor. Create a WRATHUTF16 stream
    from a pair of iterator's of type
    T. A WRATHUTF16 does NOT copy the
    data from the range, it only saves
    the iterators, thus they must
    remain valid for the lifetime
    of the WRATHUTF16.
    \param pbegin iterator to first "uint16_t" 
                  to be intepreted as UTF16 encoded data
    \param pend iterator to one element past 
                the last "uint16_t" UTF16 encoded data
   */
  WRATHUTF16(T pbegin, T pend):
    m_range(pbegin, pend),
    m_begin(m_range),
    m_end(m_range.m_end)
  {}

  /*!\fn WRATHUTF16(range_type<T>)
    Ctor. Create a WRATHUTF16 stream
    from a pair of iterator's of type
    T. A WRATHUTF16 does NOT copy the
    data from the range, it only saves
    the iterators, thus they must
    remain valid for the lifetime
    of the WRATHUTF16.
    \param R range of iterators with
             R.m_begin the iterator
             to the first "uint16_t" 
             to be intepreted as UTF16
             encoded data and R.m_end
             an iterator to one element 
             past the last "uint16_t" of 
             the UTF16 encoded data. 
  */
  WRATHUTF16(range_type<T> R):
    m_range(R),
    m_begin(m_range),
    m_end(m_range.m_end)
  {}

  /*!\fn const iterator& begin(void) const
    Returns the iterator to the first
    element of the decoded UTF stream.
   */
  const iterator&
  begin(void) const
  {
    return m_begin;
  }

  /*!\fn const iterator& end
    Returns the iterator to the one
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
    UTF stream is valid. Executes 
    the test by iterating through
    through the range [begin(),end()) and 
    checking if an iterator ever 
    returns 0XDC80 from iterator::operator*().
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
  static const uint16_t lower10bits=(1|2|4|8|16|32|64|128|256|512);
};
/*! @} */

#endif 
