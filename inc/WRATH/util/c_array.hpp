/*! 
 * \file c_array.hpp
 * \brief file c_array.hpp
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


#ifndef __C_ARRAY_HPP__
#define __C_ARRAY_HPP__

#include "WRATHConfig.hpp"
#include <cmath>
#include <iterator>
#include <iostream>
#include <vector>
#include "type_tag.hpp"
#include "vecN.hpp"
#include "ostream_utility.hpp"

/*! \addtogroup Utility
 * @{
 */

template<typename T>
class c_array;

template<typename T>
class const_c_array;


/*!\class c_array
  A c_array is a wrapper over a 
  C pointer with a size parameter
  to facilitate bounds checking
  and provide an STL-like iterator
  interface.
  If WRATH_VECTOR_BOUND_CHECK is defined, 
  will perform bounds checking.
 */
template<typename T>
class c_array
{
public:
  
  /*!\typedef pointer
    STL compliant typedef
  */
  typedef T* pointer;

  /*!\typedef const_pointer
    STL compliant typedef
  */
  typedef const T* const_pointer;

  /*!\typedef reference
    STL compliant typedef
  */
  typedef T& reference;

  /*!\typedef const_reference
    STL compliant typedef
  */
  typedef const T& const_reference;

  /*!\typedef value_type
    STL compliant typedef
  */
  typedef T value_type;

  /*!\typedef size_type
    STL compliant typedef
  */
  typedef size_t size_type;

  /*!\typedef difference_type
    STL compliant typedef
  */
  typedef ptrdiff_t difference_type;

  /*!\typedef iterator
    iterator typedef using __gnu_cxx:: namespace.
   */
  typedef __gnu_cxx::__normal_iterator<pointer, c_array> iterator;

  /*!\typedef const_iterator
    iterator typedef using __gnu_cxx:: namespace.
   */
  typedef __gnu_cxx::__normal_iterator<const_pointer, c_array> const_iterator;

  /*!\typedef const_reverse_iterator
    iterator typedef using  std::reverse_iterator.
   */
  typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;

  /*!\typedef reverse_iterator
    iterator typedef using  std::reverse_iterator.
   */
  typedef std::reverse_iterator<iterator>        reverse_iterator;

  /*!\fn c_array(void)
    Default ctor, initializing the pointer as NULL
    with size 0.
   */
  c_array(void):
    m_size(0),
    m_ptr(NULL)
  {}

  /*!\fn c_array(T*, size_t)
    Ctor initializing the pointer and size
    \param pptr pointer value
    \param sz size, must be no more than the number of elements that pptr points to.
   */
  c_array(T *pptr, size_t sz):
    m_size(sz),
    m_ptr(pptr)
  {}

  /*!\fn c_array(vecN<T,N>&)
    Ctor from a vecN, size is the size of the fixed size array
    \param pptr fixed size array that c_array references, must be
                in scope as until c_array is changed
   */
  template<unsigned int N>
  c_array(vecN<T,N> &pptr):
    m_size(N),
    m_ptr(pptr.c_ptr())
  {}

  /*!\fn c_array(std::vector<T,_Alloc>&)
    Ctor from an std::vector, size is the size of the std::vector.
    Note that calling resize() on the std::vector will require
    to reassign the c_array as the adress of the element of
    the std::vector may change on resize().
    \param pptr std::vector that c_array references, must be
                in scope AND not be resized until c_array is changed
   */
  template<typename _Alloc>
  c_array(std::vector<T,_Alloc> &pptr):
    m_size(pptr.size()),
    m_ptr( m_size!=0? &pptr[0]:NULL)
  {}

  /*!\fn c_array(range_type<iterator>)
    Ctor from a range of pointers.
    \param R R.m_begin will be the pointer and R.m_end-R.m_begin the size.
   */
  c_array(range_type<iterator> R):
    m_size(R.m_end-R.m_begin),
    m_ptr(m_size>0?&*R.m_begin:NULL)
  {}
    
  /*!\fn c_array<S> reinterpret_pointer
    Reinterpret style cast for c_array. It is required
    that the sizeof(T)*size() evenly divides sizeof(S).
    \tparam S type to which to be reinterpreted casted
   */
  template<typename S>
  c_array<S>
  reinterpret_pointer(void) const
  {
    S *ptr;
    size_t num_bytes(size()*sizeof(T));
    WRATHassert( num_bytes%sizeof(S)==0);
    ptr=reinterpret_cast<S*>(c_ptr());
    return c_array<S>(ptr, num_bytes/sizeof(S));
  }

  /*!\fn T* c_ptr
    Pointer of the c_array.
   */
  T*
  c_ptr(void) const
  {
    return m_ptr;
  }

  /*!\fn T* end_c_ptr
    Pointer to the element one past
    the last element of the c_array.
   */
  T*
  end_c_ptr(void) const
  {
    return m_ptr+m_size;
  }
  
  /*!\fn T& operator[](int) const
    Access named element of c_array, under
    debug build also performs bounds checking.
    \param j index
   */
  T& 
  operator[](int j) const
  { 
    WRATHassert(c_ptr()!=NULL);
    #ifdef WRATH_VECTOR_BOUND_CHECK 
      assert(0<=j);
      assert(static_cast<unsigned int>(j)<m_size); 
    #endif
    return c_ptr()[j]; 
  }

  /*!\fn bool empty(void) const
    STL compliant function.
   */
  bool
  empty(void) const
  {
    return m_size==0;
  }

  /*!\fn size_t size
    STL compliant function.
   */
  size_t 
  size(void) const 
  { 
    return m_size; 
  }
    
  /*!\fn iterator begin
    STL compliant function.
   */
  iterator
  begin(void) const 
  { 
    return iterator(c_ptr()); 
  }

  /*!\fn iterator end
    STL compliant function.
   */
  iterator
  end(void) const 
  { 
    return iterator( c_ptr()+static_cast<difference_type>(size()) ); 
  }
  
  /*!\fn reverse_iterator rbegin
    STL compliant function.
   */
  reverse_iterator
  rbegin(void) const 
  { 
    return reverse_iterator(end()); 
  }
  
  /*!\fn reverse_iterator rend
    STL compliant function.
   */
  reverse_iterator
  rend(void) const 
  { 
    return reverse_iterator(begin()); 
  }
  
  /*!\fn range_type<iterator> range
    Returns the range of the c_array as an
    iterator range.
   */
  range_type<iterator>
  range(void)
  {
    return range_type<iterator>(begin(), end());
  }

  /*!\fn range_type<reverse_iterator> reverse_range
    Returns the range of the c_array as a
    reverse_iterator range
   */
  range_type<reverse_iterator>
  reverse_range(void)
  {
    return range_type<reverse_iterator>(rbegin(), rend());
  }

  /*!\fn T& back(unsigned int) const
    Equivalent to
    \code
    operator[](size()-1-I)
    \endcode
    \param I index from the back to retrieve, I=0
             corrseponds to the back of the array.
   */
  T&
  back(unsigned int I) const
  {
    WRATHassert(I<size());
    return (*this)[size()-1-I];
  }

  /*!\fn T& back(void) const
    STL compliant function.
   */
  T&
  back(void) const
  { 
    return (*this)[size()-1]; 
  }
  
  /*!\fn T& front
    STL compliant function.
   */
  T&
  front(void) const
  { 
    return (*this)[0]; 
  }

  /*!\fn c_array sub_array(int, int) const
    Returns a sub-array
    \param pos position of returned sub-array to start,
               i.e. returned c_array's c_ptr() will return
               this->c_ptr()+pos. It is an error if pos
               is negative.
    \param length length of sub array to return, note
                  that it is an error if length+pos>size()
                  or if length is negative.
   */
  c_array
  sub_array(int pos, int length) const
  {
    WRATHassert(0<=pos);
    WRATHassert(length>=0);
    WRATHassert(static_cast<unsigned int>(pos+length)<=m_size);
    return c_array(m_ptr+pos, length);
  }

  /*!\fn c_array sub_array(int) const
    Returns a sub-array, equivalent to
    \code
    sub_array(pos, size()-pos)
    \endcode
    \param pos position of returned sub-array to start,
               i.e. returned c_array's c_ptr() will return
               this->c_ptr()+pos. It is an error is pos
               is negative.
   */
  c_array
  sub_array(int pos) const
  {
    WRATHassert(static_cast<unsigned int>(pos)<=m_size);
    return c_array(m_ptr+pos, m_size-pos);
  }

  /*!\fn c_array sub_array(range_type<int>) const
    Returns a sub-array, equivalent to
    \code
    sub_array(R.m_begin, R.m_end-R.m_begin)
    \endcode
    \param R range of returned sub-array 
   */
  c_array
  sub_array(range_type<int> R) const
  {
    WRATHassert(0<=R.m_begin);
    WRATHassert(R.m_end>=R.m_begin);
    WRATHassert(static_cast<unsigned int>(R.m_end)<=m_size);

    return c_array(m_ptr+R.m_begin, R.m_end - R.m_begin);
  }

  /*!\fn c_array sub_array(range_type<unsigned int>) const
    Returns a sub-array, equivalent to
    \code
    sub_array(R.m_begin, R.m_end-R.m_begin)
    \endcode
    \param R range of returned sub-array 
   */
  c_array
  sub_array(range_type<unsigned int> R) const
  {
    WRATHassert(R.m_end>=R.m_begin);
    WRATHassert(R.m_end<=m_size);

    return c_array(m_ptr+R.m_begin, R.m_end - R.m_begin);
  }

  /*!\fn bool same_data(const c_array &rhs) const
    Returns true if and only if the passed
    the c_array references exactly the same
    data as this c_array.
    \param rhs c_array to which to compare
   */
  bool
  same_data(const c_array &rhs) const
  {
    return m_size==rhs.m_size
      and m_ptr==rhs.m_ptr;
  }

  /*!\fn bool same_data(const const_c_array<T> &rhs) const
    Returns true if and only if the passed
    the c_array references exactly the same
    data as this c_array.
    \param rhs c_array to which to compare
   */  
  bool
  same_data(const const_c_array<T> &rhs) const;

private:
  size_t m_size;
  T *m_ptr;

};





/*!\class const_c_array
  A const_c_array is a wrapper over a 
  const C pointer with a size parameter
  to facilitate bounds checking
  and provide an STL-like iterator
  interface.
  If WRATH_VECTOR_BOUND_CHECK is defined, 
  will perform bounds checking.
 */
template<typename T>
class const_c_array
{
public:

  /*!\typedef pointer
    STL compliant typedef
  */
  typedef T* pointer;

  /*!\typedef const_pointer  
    STL compliant typedef
  */
  typedef const T* const_pointer;

  /*!\typedef reference
    STL compliant typedef
  */
  typedef T& reference;

  /*!\typedef const_reference  
    STL compliant typedef
  */
  typedef const T& const_reference;

  /*!\typedef value_type  
    STL compliant typedef
  */
  typedef T value_type;

  /*!\typedef size_type
    STL compliant typedef
  */
  typedef size_t size_type;

  /*!\typedef difference_type
    STL compliant typedef
  */
  typedef ptrdiff_t difference_type;

  /*!\typedef iterator
    iterator typedef using __gnu_cxx:: namespace.
   */
  typedef __gnu_cxx::__normal_iterator<const_pointer, const_c_array> iterator;

  /*!\typedef const_iterator
    iterator typedef using __gnu_cxx:: namespace.
   */
  typedef __gnu_cxx::__normal_iterator<const_pointer, const_c_array> const_iterator;

  /*!\typedef const_reverse_iterator
    iterator typedef using  std::reverse_iterator.
   */
  typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;

  /*!\typedef reverse_iterator
    iterator typedef using  std::reverse_iterator.
   */
  typedef std::reverse_iterator<iterator>        reverse_iterator;

  /*!\fn const_c_array(void)
    Default ctor, initializing the pointer as NULL
    with size 0.
   */
  const_c_array(void):
    m_size(0),
    m_ptr(NULL)
  {}

  /*!\fn const_c_array(const T*, size_t)
    Ctor initializing the pointer and size
    \param pptr pointer value
    \param sz size, must be no more than the number of elements that pptr points to.
   */
  const_c_array(const T *pptr, size_t sz):
    m_size(sz),
    m_ptr(pptr)
  {}

  /*!\fn const_c_array(c_array<T>)
    Contruct a const_c_array from a c_array
    \param v c_array from which to copy size and pointer value
   */
  const_c_array(c_array<T> v):
    m_size(v.size()),
    m_ptr(v.c_ptr())
  {}

  /*!\fn const_c_array(const vecN<T,N>&)
    Ctor from a vecN, size is the size of the fixed size array
    \param v fixed size array that const_c_array references, must be
             in scope as until c_array is changed
   */
  template<unsigned int N>
  const_c_array(const vecN<T,N> &v):
    m_size(v.size()),
    m_ptr(v.c_ptr())
  {}

  /*!\fn const_c_array(const std::vector<T,_Alloc>&)
    Ctor from an std::vector, size is the size of the std::vector.
    Note that calling resize() on the std::vector will require
    to reassign the c_array as the adress of the element of
    the std::vector may change on resize().
    \param v std::vector that const_c_array references, must be
                in scope AND not be resized until c_array is changed
   */
  template<typename _Alloc>
  const_c_array(const std::vector<T,_Alloc> &v):
    m_size(v.size()),
    m_ptr( v.size()>0? &v[0]: NULL)
  {}

  /*!\fn const_c_array(range_type<iterator>)
    Ctor from a range of pointers.
    \param R R.m_begin will be the pointer and R.m_end-R.m_begin the size.
   */
  const_c_array(range_type<iterator> R):
    m_size(R.m_end-R.m_begin),
    m_ptr(m_size>0?&*R.m_begin:NULL)
  {}
    

  /*!\fn const_c_array<S> reinterpret_pointer
    Reinterpret style cast for c_array. It is required
    that the sizeof(T)*size() evenly divides sizeof(S).
    \tparam S type to which to be reinterpreted casted
   */
  template<typename S>
  const_c_array<S>
  reinterpret_pointer(void) const
  {
    const S *ptr;
    size_t num_bytes(size()*sizeof(T));   
    WRATHassert( num_bytes%sizeof(S)==0);
    ptr=reinterpret_cast<const S*>(c_ptr());
    return const_c_array<S>(ptr, num_bytes/sizeof(S));
  }

  /*!\fn const T* c_ptr
    Pointer of the const_c_array.
   */
  const T*
  c_ptr(void) const
  {
    return m_ptr;
  }

  /*!\fn const T* end_c_ptr
    Pointer to the element one past
    the last element of the const_c_array.
   */
  const T*
  end_c_ptr(void) const
  {
    return m_ptr+m_size;
  }

  /*!\fn const T&  operator[](int) const
    Access named element of const_c_array, under
    debug build also performs bounds checking.
    \param j index
   */
  const T& 
  operator[](int j) const 
  { 
    WRATHassert(c_ptr()!=NULL);
    #ifdef WRATH_VECTOR_BOUND_CHECK 
      assert(0<=j);
      assert(static_cast<unsigned int>(j)<m_size); 
    #endif
    return c_ptr()[j]; 
  }
 
  /*!\fn bool empty
     STL compliant function.
   */
  bool
  empty(void) const
  {
    return m_size==0;
  }

  /*!\fn size_t size
    STL compliant function.
   */
  size_t 
  size(void) const 
  { 
    return m_size; 
  }
  
  /*!\fn const_iterator begin
    STL compliant function.
   */
  const_iterator
  begin(void) const 
  { 
    return const_iterator(c_ptr()); 
  }
  
  /*!\fn const_reverse_iterator rbegin
    STL compliant function.
   */
  const_reverse_iterator
  rbegin(void) const 
  { 
    return const_reverse_iterator(end()); 
  }
  
  /*!\fn const_iterator end
    STL compliant function.
   */
  const_iterator
  end(void) const 
  { 
    return const_iterator( c_ptr()+static_cast<difference_type>(size()) ); 
  }
  
  /*!\fn rend
    STL compliant function.
   */
  const_reverse_iterator
  rend(void) const 
  { 
    return const_reverse_iterator(begin()); 
  }

  /*!\fn range
    Returns the range of the const_c_array as an
    const_iterator range.
   */
  range_type<const_iterator>
  range(void) const
  {
    return range_type<const_iterator>(begin(), end());
  }

  /*!\fn reverse_range
    Returns the range of the const_c_array as a
    const_reverse_iterator range
   */
  range_type<const_reverse_iterator>
  reverse_range(void) const
  {
    return range_type<const_reverse_iterator>(rbegin(), rend());
  }

  /*!\fn const T& back(unsigned int) const
    Equivalent to
    \code
    operator[](size()-1-I)
    \endcode
    \param I index from the back to retrieve, I=0
             corrseponds to the back of the array.
   */
  const T&
  back(unsigned int I) const
  {
    WRATHassert(I<size());
    return (*this)[size()-1-I];
  }
  
  /*!\fn const T& back(void) const 
    STL compliant function.
   */
  const T&
  back(void) const 
  { 
    return (*this)[size()-1]; 
  }
  
  /*!\fn const T& front
    STL compliant function.
   */
  const T&
  front(void) const 
  { 
    return (*this)[0]; 
  }

  /*!\fn const_c_array sub_array(int, int) const
    Returns a sub-array
    \param pos position of returned sub-array to start,
               i.e. returned c_array's c_ptr() will return
               this->c_ptr()+pos. It is an error is pos
               is negative.
    \param length length of sub array to return, note
                  that it is an error if length+pos>size()
                  or if length is negative.
   */
  const_c_array
  sub_array(int pos, int length) const
  {
    WRATHassert(0<=pos);
    WRATHassert(length>=0);
    WRATHassert(static_cast<unsigned int>(pos+length)<=m_size);

    return const_c_array(m_ptr+pos, length);
  }

  /*!\fn const_c_array sub_array(int) const
    Returns a sub-array, equivalent to
    \code
    sub_array(pos, size()-pos)
    \endcode
    \param pos position of returned sub-array to start,
               i.e. returned c_array's c_ptr() will return
               this->c_ptr()+pos. It is an error is pos
               is negative.
   */
  const_c_array
  sub_array(int pos) const
  {
    WRATHassert(static_cast<unsigned int>(pos)<=m_size);
    return const_c_array(m_ptr+pos, m_size-pos);
  }

  /*!\fn const_c_array sub_array(range_type<int>) const
    Returns a sub-array, equivalent to
    \code
    sub_array(R.m_begin, R.m_end-R.m_begin)
    \endcode
    \param R range of returned sub-array 
   */
  const_c_array
  sub_array(range_type<int> R) const
  {
    WRATHassert(0<=R.m_begin);
    WRATHassert(R.m_end>=R.m_begin);
    WRATHassert(static_cast<unsigned int>(R.m_end)<=m_size);

    return const_c_array(m_ptr+R.m_begin, R.m_end - R.m_begin);
  }

  /*!\fn const_c_array sub_array(range_type<unsigned int>) const
    Returns a sub-array, equivalent to
    \code
    sub_array(R.m_begin, R.m_end-R.m_begin)
    \endcode
    \param R range of returned sub-array 
   */
  const_c_array
  sub_array(range_type<unsigned int> R) const
  {
    WRATHassert(R.m_end>=R.m_begin);
    WRATHassert(R.m_end<=m_size);

    return const_c_array(m_ptr+R.m_begin, R.m_end - R.m_begin);
  }

  /*!\fn bool same_data(const const_c_array &rhs) const
    Returns true if and only if the passed
    the const_c_array references exactly the same
    data as this const_c_array.
    \param rhs const_c_array to which to compare
   */
  bool
  same_data(const const_c_array &rhs) const
  {
    return m_size==rhs.m_size
      and m_ptr==rhs.m_ptr;
  }

private:
  size_t m_size;
  const T *m_ptr;
};

/*!\fn std::ostream& operator<<(std::ostream&, const_c_array<T>)
  Conveniance overload of operator<< to print the contents
  pointed to by a c_array to an std::ostream
  \param str std::ostream to which to print
  \param obj c_array to print contents
 */
template<typename T>
inline
std::ostream&
operator<<(std::ostream &str, const_c_array<T> obj)
{
  str <<  "( " << WRATHUtil::print_range(obj.begin(), obj.end(), ", ") << " )";
  return str;
}

/*!\fn std::ostream& operator<<(std::ostream&, c_array<T>)
  Conveniance overload of operator<< to print the contents
  pointed to by a const_c_array to an std::ostream
  \param str std::ostream to which to print
  \param obj const_c_array to print contents
 */
template<typename T>
inline
std::ostream&
operator<<(std::ostream &str, c_array<T> obj)
{
  str <<  "( " << WRATHUtil::print_range(obj.begin(), obj.end(), ", ") << " )";
  return str;
}


/*! @} */



template<typename T>
bool
c_array<T>::
same_data(const const_c_array<T> &rhs) const
{
  return rhs.same_data(*this);
}


#endif
