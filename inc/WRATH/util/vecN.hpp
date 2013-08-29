/*! 
 * \file vecN.hpp
 * \brief file vecN.hpp
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


#ifndef __VECN_HPP__
#define __VECN_HPP__


#include "WRATHConfig.hpp"
#include <cmath>
#include <cstddef>
#include <iterator>
#include <iostream>
#include "WRATHassert.hpp"
#include <boost/static_assert.hpp>


#include "type_tag.hpp"
#include "ostream_utility.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class vecN
  vecN is a simple static array class with no virtual
  functions and no memory overhead. Supports runtim array 
  index checking and STL style iterators via pointer iterators.
  If WRATH_VECTOR_BOUND_CHECK is defined, will perform bounds
  checking.
  \param T typename with a constructor that takes no arguments.
  \param N unsigned integer size of array
 */
template<typename T, unsigned int N>
class vecN
{
public:

  enum 
    { 
      /*!
        Enumeration value for length of array.
       */
      array_size=N 
    };

  /*!\typedef pod_type
    underlying data object
  */
  typedef T pod_type[N];

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

  /*!\typedef value_type_tag
    Conveniance typedef.
   */
  typedef type_tag<T> value_type_tag;

  /*!\typedef iterator
    iterator typedef using __gnu_cxx:: namespace.
   */
  typedef __gnu_cxx::__normal_iterator<pointer, vecN> iterator;

  /*!\typedef const_iterator
    iterator typedef using __gnu_cxx:: namespace.
   */
  typedef __gnu_cxx::__normal_iterator<const_pointer, vecN> const_iterator;

  /*!\typedef const_reverse_iterator
    iterator typedef using  std::reverse_iterator.
   */
  typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;

  /*!\typedef reverse_iterator
    iterator typedef using std::reverse_iterator
   */
  typedef std::reverse_iterator<iterator>        reverse_iterator;

  
  /*!\fn vecN(void)
    Ctor, no intiliaztion on POD types.
   */
  vecN(void)
  {}

  /*!\fn vecN(const T&)
    Ctor.
    Calls T::operator= on each element of the array.
    \param value constant reference to a T value.
  */
  explicit 
  vecN(const T &value)
  {
    for(unsigned int i=0;i<N;++i)
      {
        *(c_ptr()+i)=T(value);
      }
  }

  /*!\fn vecN(const vecN<T,M>&, const T&)
    Copy constructor from array of different size.
    Calls T::operator= on each array element, if M<N then
    for each element beyond M, T::operator=
    is called with the parameter value.
    \tparam M size of other array.
    \param obj constant reference to copy elements from.
    \param value constant reference for value to use beyond index M
  */
  template<unsigned int M>
  explicit
  vecN(const vecN<T,M> &obj, const T &value=T())
  {
    unsigned int i;

    for(i=0;i<std::min(N,M);++i)
      {
        *(c_ptr()+i)=obj[i];
      }

    for(;i<N;++i)
      {
        *(c_ptr()+i)=value;
      }
  }

  /*!\fn vecN(const vecN<S,M>&, const T&)
    Ctor
    Calls T::operator= on each array element, if M<N then
    for each element beyond M, T::operator=
    is called with the parameter value.
    \tparam M size of other array.
    \param obj constant reference to copy elements from.
    \param value constant reference for value to use beyond index M
   */
  template<typename S, unsigned int M>
  explicit
  vecN(const vecN<S,M> &obj, const T &value=T())
  {
    unsigned int i;

    for(i=0;i<std::min(N,M);++i)
      {
        *(c_ptr()+i)=T(obj[i]);
      }

    for(;i<N;++i)
      {
        *(c_ptr()+i)=value;
      }
  }

  /*!\fn vecN(p_iterator, p_iterator, enum copy_range_tag_type, const T&)
    Copy constructor from an iterator range
    \param pbegin iterator to first element
    \param pend iterator to one past the last element
    \param cp must have value \ref copy_range_tag
    \param default_value value to use if iterator range is smaller than N
  */
  template<typename p_iterator>
  vecN(p_iterator pbegin, p_iterator pend, enum copy_range_tag_type cp, 
       const T &default_value=T())
  {
    WRATHunused(cp);

    unsigned int i;
    
    for(i=0; pbegin!=pend and i<N; ++pbegin, ++i)
      {
        *(c_ptr()+i)=T(*pbegin);
      }

    for(;i<N;++i)
      {
        *(c_ptr()+i)=default_value;
      }
  }

  /*!\fn vecN(const vecN<T,M>&, unsigned int, unsigned int, const T&)
    Copy constructor from array of different size specifying the range
    Copies every stride'th value stored at obj starting at index start
    to this. For elements of this which are not assigned in this
    fashion, they are assigned as default_value.
    \tparam M size of other array.
    \param obj constant reference to copy elements from.
    \param start first index of M to use
    \param stride stride of copying
    \param default_value
  */
  template<unsigned int M>
  vecN(const vecN<T,M> &obj, 
       unsigned int start, unsigned int stride=1,
       const T &default_value=T())
  {
    unsigned int i,j;

    for(i=0,j=start; i<N and j<M; ++i, j+=stride)
      {
        *(c_ptr()+i)=obj[i];
      }

    for(;i<N;++i)
      {
        *(c_ptr()+i)=default_value;
      }
  }

  /*!\fn vecN(const T&, const T&)
    Conveniance ctor, will fail to compile unless N=2.
    \param px value to which to assing the return value of x().
    \param py value to which to assing the return value of y().
   */
  vecN(const T &px, const T &py)
  {
    BOOST_STATIC_ASSERT(N==2);
    *(c_ptr()+0)=px;
    *(c_ptr()+1)=py;
  }

  /*!\fn vecN(const T&, const T&, const T&)
    Conveniance ctor, will fail to compile unless N=3.
    \param px value to which to assing the return value of x().
    \param py value to which to assing the return value of y().
    \param pz value to which to assing the return value of z().
   */
  vecN(const T &px, const T &py, const T &pz)
  {
    BOOST_STATIC_ASSERT(N==3);
    *(c_ptr()+0)=px;
    *(c_ptr()+1)=py;
    *(c_ptr()+2)=pz;
  }

  /*!\fn vecN(const T&, const T&, const T&, const T&)
    Conveniance ctor, will fail to compile unless N=4.
    \param px value to which to assing the return value of x().
    \param py value to which to assing the return value of y().
    \param pz value to which to assing the return value of z().
    \param pw value to which to assing the return value of w().
   */
  vecN(const T &px, const T &py, const T &pz, const T &pw)
  {
    BOOST_STATIC_ASSERT(N==4);
    *(c_ptr()+0)=px;
    *(c_ptr()+1)=py;
    *(c_ptr()+2)=pz;
    *(c_ptr()+3)=pw;
  }

  /*!\fn vecN(const T&, const T&, const T&, const T&, const T&)
    Conveniance ctor, will fail to compile unless N=5
    \param p0 value to which to assing the return value of operator[](0)
    \param p1 value to which to assing the return value of operator[](1)
    \param p2 value to which to assing the return value of operator[](2)
    \param p3 value to which to assing the return value of operator[](3)
    \param p4 value to which to assing the return value of operator[](4)
  */
  vecN(const T &p0, const T &p1, const T &p2, 
       const T &p3, const T &p4)
  {
    BOOST_STATIC_ASSERT(N==5);
    *(c_ptr()+0)=p0;
    *(c_ptr()+1)=p1;
    *(c_ptr()+2)=p2;
    *(c_ptr()+3)=p3;
    *(c_ptr()+4)=p4;
  }

  /*!\fn vecN(const T&, const T&, const T&, const T&, const T&, const T&)
    Conveniance ctor, will fail to compile unless N=6
    \param p0 value to which to assing the return value of operator[](0)
    \param p1 value to which to assing the return value of operator[](1)
    \param p2 value to which to assing the return value of operator[](2)
    \param p3 value to which to assing the return value of operator[](3)
    \param p4 value to which to assing the return value of operator[](4)
    \param p5 value to which to assing the return value of operator[](5)
  */
  vecN(const T &p0, const T &p1, const T &p2, 
       const T &p3, const T &p4, const T &p5)
  {
    BOOST_STATIC_ASSERT(N==6);
    *(c_ptr()+0)=p0;
    *(c_ptr()+1)=p1;
    *(c_ptr()+2)=p2;
    *(c_ptr()+3)=p3;
    *(c_ptr()+4)=p4;
    *(c_ptr()+5)=p5;
  }

  /*!\fn vecN(const T&, const T&, const T&, const T&, const T&, const T&, const T&)
    Conveniance ctor, will fail to compile unless N=7
    \param p0 value to which to assing the return value of operator[](0)
    \param p1 value to which to assing the return value of operator[](1)
    \param p2 value to which to assing the return value of operator[](2)
    \param p3 value to which to assing the return value of operator[](3)
    \param p4 value to which to assing the return value of operator[](4)
    \param p5 value to which to assing the return value of operator[](5)
    \param p6 value to which to assing the return value of operator[](6)
  */
  vecN(const T &p0, const T &p1, const T &p2, 
       const T &p3, const T &p4, const T &p5,
       const T &p6)
  {
    BOOST_STATIC_ASSERT(N==7);
    *(c_ptr()+0)=p0;
    *(c_ptr()+1)=p1;
    *(c_ptr()+2)=p2;
    *(c_ptr()+3)=p3;
    *(c_ptr()+4)=p4;
    *(c_ptr()+5)=p5;
    *(c_ptr()+6)=p6;
  }

  /*!\fn vecN(const T&, const T&, const T&, const T&,
              const T&, const T&, const T&, const T&)
    Conveniance ctor, will fail to compile unless N=8
    \param p0 value to which to assing the return value of operator[](0)
    \param p1 value to which to assing the return value of operator[](1)
    \param p2 value to which to assing the return value of operator[](2)
    \param p3 value to which to assing the return value of operator[](3)
    \param p4 value to which to assing the return value of operator[](4)
    \param p5 value to which to assing the return value of operator[](5)
    \param p6 value to which to assing the return value of operator[](6)
    \param p7 value to which to assing the return value of operator[](7)
  */
  vecN(const T &p0, const T &p1, const T &p2, 
       const T &p3, const T &p4, const T &p5,
       const T &p6, const T &p7)
  {
    BOOST_STATIC_ASSERT(N==8);
    *(c_ptr()+0)=p0;
    *(c_ptr()+1)=p1;
    *(c_ptr()+2)=p2;
    *(c_ptr()+3)=p3;
    *(c_ptr()+4)=p4;
    *(c_ptr()+5)=p5;
    *(c_ptr()+6)=p6;
    *(c_ptr()+7)=p7;
  }

  /*!\fn vecN(const T&, const T&, const T&,
              const T&, const T&, const T&,
              const T&, const T&, const T&)
    Conveniance ctor, will fail to compile unless N=9
    \param p0 value to which to assing the return value of operator[](0)
    \param p1 value to which to assing the return value of operator[](1)
    \param p2 value to which to assing the return value of operator[](2)
    \param p3 value to which to assing the return value of operator[](3)
    \param p4 value to which to assing the return value of operator[](4)
    \param p5 value to which to assing the return value of operator[](5)
    \param p6 value to which to assing the return value of operator[](6)
    \param p7 value to which to assing the return value of operator[](7)
    \param p8 value to which to assing the return value of operator[](8)
  */
  vecN(const T &p0, const T &p1, const T &p2, 
       const T &p3, const T &p4, const T &p5,
       const T &p6, const T &p7, const T &p8)
  {
    BOOST_STATIC_ASSERT(N==9);
    *(c_ptr()+0)=p0;
    *(c_ptr()+1)=p1;
    *(c_ptr()+2)=p2;
    *(c_ptr()+3)=p3;
    *(c_ptr()+4)=p4;
    *(c_ptr()+5)=p5;
    *(c_ptr()+6)=p6;
    *(c_ptr()+7)=p7;
    *(c_ptr()+8)=p8;
  }

  /*!\fn vecN(const vecN<T, N-1>&, const T&)
    Conveniance function.
    \param p gives valeus for array indices 0 to N-2 inclusive
    \param d gives value for array index N-1
   */
  vecN(const vecN<T, N-1> &p, const T &d)
  {
    BOOST_STATIC_ASSERT(N>1);
    for(unsigned int i=0;i<N-1;++i)
      {
        *(c_ptr()+i)=p[i];
      }
    *(c_ptr()+N-1)=d;
  }
  
  /*!\fn T* c_ptr(void)
    Returns a C-style pointer to the array.
   */
  T*
  c_ptr(void) { return m_data; }

  /*!\fn const T* c_ptr(void) const
    Returns a constant C-style pointer to the array.
   */
  const T*
  c_ptr(void) const { return m_data; }

  /*!\fn const T&  operator[](int) const
    Return a constant refernce to the j'th element.
    If WRATH_VECTOR_BOUND_CHECK is defined checks that
    j is in range, i.e. 0<=j and j<N

    \param j index of element to return.
   */
  const T& 
  operator[](int j) const 
  { 
    #ifdef WRATH_VECTOR_BOUND_CHECK 
      assert(0<=j);
      assert(static_cast<unsigned int>(j)<N); 
    #endif
    return c_ptr()[j]; 
  }

  /*!\fn T& operator[](int)
    Return a refernce to the j'th element.
    If WRATH_VECTOR_BOUND_CHECK is defined checks that
    j is in range, i.e. 0<=j and j<N
    \param j index of element to return.
   */
  T& 
  operator[](int j) 
  { 
    #ifdef WRATH_VECTOR_BOUND_CHECK 
      assert(0<=j);
      assert(static_cast<unsigned int>(j)<N); 
    #endif
    return c_ptr()[j]; 
  }

  /*!\fn T& x(void)
    Conveniance readability member function,
    equivalent to operator[](0).
   */
  T&
  x(void) { BOOST_STATIC_ASSERT(N>=1); return c_ptr()[0]; }
  
  /*!\fn T& y(void)
    Conveniance readability member function,
    equivalent to operator[](1). Fails to compile
    if N is not atleast 2.
   */
  T&
  y(void) { BOOST_STATIC_ASSERT(N>=2); return c_ptr()[1]; }

  /*!\fn T& z(void)
    Conveniance readability member function,
    equivalent to operator[](2). Fails to compile
    if N is not atleast 3.
   */
  T&
  z(void) { BOOST_STATIC_ASSERT(N>=3); return c_ptr()[2]; }
  
  /*!\fn T& w(void)
    Conveniance readability member function,
    equivalent to operator[](3). Fails to compile
    if N is not atleast 4.
   */
  T&
  w(void) { BOOST_STATIC_ASSERT(N>=4); return c_ptr()[3]; }

  /*!\fn const T& x(void) const
    Conveniance readability member function,
    equivalent to operator[](0).
   */
  const T&
  x(void) const { BOOST_STATIC_ASSERT(N>=1); return c_ptr()[0]; }
  
  /*!\fn const T& y(void) const  
    Conveniance readability member function,
    equivalent to operator[](1). Fails to compile
    if N is not atleast 2.
   */
  const T&
  y(void) const { BOOST_STATIC_ASSERT(N>=2); return c_ptr()[1]; }

  /*!\fn const T& z(void) const  
    Conveniance readability member function,
    equivalent to operator[](2). Fails to compile
    if N is not atleast 3.
   */
  const T&
  z(void) const { BOOST_STATIC_ASSERT(N>=3); return c_ptr()[2]; }
  
  /*!\fn const T& w(void) const  
    Conveniance readability member function,
    equivalent to operator[](3). Fails to compile
    if N is not atleast 4.
   */
  const T&
  w(void) const { BOOST_STATIC_ASSERT(N>=4); return c_ptr()[3]; }

  /*!\fn void swap  
    STL compatible swap, performs swap on 
    each array element individually.
    \param obj vecN with which to swap
   */
  void
  swap(vecN &obj)
  {
    if(this!=&obj)
      {
        for(unsigned int i=0;i<N;++i)
          {
            std::swap(operator[](i), obj[i]);
          }
      }
  }

  /*!\fn const vecN& operator=(const vecN&)
    Assignment operator, performs T::operator= on each element.
    \param obj: constant reference to a same sized array.    
   */
  const vecN&
  operator=(const vecN &obj)
  {
    if(&obj!=this)
      {
        for(unsigned int i=0; i<N; ++i)
          {
            operator[](i)=obj[i];
          }
      }
    return *this;
  }

  /*!\fn Set(const T&)
    Set all values of array, performs T::operator=
    on each element of the array agains obj.
    \param obj Value to set all objects as.
   */
  const vecN&
  Set(const T &obj)
  {
    for(unsigned int i=0;i<N;++i)
      {
        *(c_ptr()+i)=obj;
      }

    return *this;
  }
  
  /*!\fn vecN operator-(void) const
    Component-wise negation operator.
    returns the componenet-wise negation
   */
  vecN
  operator-(void) const
  {
    vecN retval;
    for(unsigned int i=0;i<N;++i)
      {
        retval[i]=-operator[](i);
      }
    return retval;
  }

  /*!\fn vecN operator+(const vecN&) const
    Compoenent wise addition operator
    returns the componenet wise addition of two
    arrays.
    \param obj right hand side of + operator
   */
  vecN
  operator+(const vecN &obj) const
  {
    vecN retval(*this);
    retval+=obj;
    return retval;
  }
  
  /*!\fn vecN operator-(const vecN&) const
    Component-wise subtraction operator
    returns the componenet-wise subtraction of two
    arrays.
    \param obj right hand side of - operator
   */
  vecN
  operator-(const vecN &obj) const
  {
    vecN retval(*this);
    retval-=obj;
    return retval;
  }

  /*!\fn vecN operator*(const vecN&) const
    Component-wise multiplication operator,
    returns the componenet-wise multiplication of 
    two arrays.
    \param obj right hand side of * operator
   */
  vecN
  operator*(const vecN &obj) const
  {
    vecN retval(*this);
    retval*=obj;
    return retval;
  }

  /*!\fn vecN operator/(const vecN&) const
    Component-wise division operator,
    returns the componenet-wise division of 
    two arrays.
    \param obj right hand side of / operator
   */
  vecN
  operator/(const vecN &obj) const
  {
    vecN retval(*this);
    retval/=obj;
    return retval;
  }

  /*!\fn vecN operator%(const vecN&) const
    Component-wise modulas operator
    returns the componenet-wise modulas of 
    two arrays.
    \param obj right hand side of % operator
   */
  vecN
  operator%(const vecN &obj) const
  {
    vecN retval(*this);
    retval%=obj;

    return retval;
  }
  
  /*!\fn vecN operator+(const T&) const
    Component-wise addition operator against a singleton
    \param obj right hand side of + operator
   */
  vecN
  operator+(const T &obj) const
  {
    vecN retval(*this);
    retval+=obj;
    return retval;
  }

  /*!\fn vecN operator-(const T&) const
    Component-wise subtraction operator against a singleton
    \param obj right hand side of + operator
   */
  vecN
  operator-(const T &obj) const
  {
    vecN retval(*this);
    retval-=obj;

    return retval;
  }

  /*!\fn vecN operator*(const T&) const
    Component-wise multiplication operator against a singleton
    \param obj right hand side of * operator
   */
  vecN
  operator*(const T &obj) const
  {
    vecN retval(*this);
    retval*=obj;
    return retval;
  }

  /*!\fn vecN operator/(const T&) const
    Component-wise division against a singleton
    \param obj right hand side of / operator
   */
  vecN
  operator/(const T &obj) const
  {
    vecN retval(*this);
    retval/=obj;
    return retval;
  }

  /*!\fn vecN operator%(const T&) const
    Component-wise modulas against a singleton
    \param obj right hand side of % operator
   */
  vecN
  operator%(const T &obj) const
  {
    vecN retval(*this);
    retval%=obj;
    return retval;
  }

  /*!\fn vecN operator+=(const vecN<T,M>&)
    Component-wise addition increment operator against an 
    array of possibly different size, if M is smaller
    then only those indexes less than M are affected.
    \param obj right hand side of += operator
  */
  template<unsigned int M>
  void
  operator+=(const vecN<T,M> &obj) 
  {
    for(unsigned int i=0;i<std::min(M,N);++i)
      {
        operator[](i)+=obj[i];
      }
  }

  /*!\fn vecN operator-=(const vecN<T,M>&)
    Component-wise subtraction increment operator against an 
    array of possibly different size, if M is smaller
    then only those indexes less than M are affected.
    \param obj right hand side of -= operator
  */
  template<unsigned int M>
  void
  operator-=(const vecN<T,M> &obj) 
  {
    for(unsigned int i=0;i<std::min(M,N);++i)
      {
        operator[](i)-=obj[i];
      }
  }

  /*!\fn vecN operator*=(const vecN<T,M>&)
    Component-wise multiplication increment operator against an 
    array of possibly different size, if M is smaller
    then only those indexes less than M are affected.
    \param obj right hand side of *= operator
  */
  template<unsigned int M>
  void
  operator*=(const vecN<T,M> &obj) 
  {
    for(unsigned int i=0;i<std::min(N,M);++i)
      {
        operator[](i)*=obj[i];
      }
  }
 
  /*!\fn vecN operator/=(const vecN<T,M>&)
    Component-wise division increment operator against an 
    array of possibly different size, if M is smaller
    then only those indexes less than M are affected.
    \param obj right hand side of /= operator
  */
  template<unsigned int M>
  void
  operator/=(const vecN<T,M> &obj) 
  {
    for(unsigned int i=0;i<std::min(M,N);++i)
      {
        operator[](i)/=obj[i];
      }
  }
  
  /*!\fn void operator+=(const T&)
    Increment add operator against a singleton,
    i.e. increment each element of this against
    the passed T value.
    \param obj right hind side of operator+=
   */
  void
  operator+=(const T &obj) 
  {
    for(unsigned int i=0;i<N;++i)
      {
        operator[](i)+=obj;
      }
  }
  
  /*!\fn void operator-=(const T&)
    Increment subtract operator against a singleton,
    i.e. decrement each element of this against
    the passed T value.
    \param obj right hind side of operator-=
   */
  void 
  operator-=(const T &obj) 
  {
    for(unsigned int i=0;i<N;++i)
      {
        operator[](i)-=obj;
      }
  }

  /*!\fn void operator*=(const T&)
    Increment multiply operator against a singleton,
    i.e. increment multiple each element of this against
    the passed T value.
    \param obj right hind side of operator*=
   */
  void
  operator*=(const T &obj) 
  {
    for(unsigned int i=0;i<N;++i)
      {
        operator[](i)*=obj;
      }
  }

  /*!\fn void operator/=(const T&)
    Increment divide operator against a singleton,
    i.e. increment divide each element of this against
    the passed T value.
    \param obj right hind side of operator/=
   */
  void
  operator/=(const T &obj) 
  {
    for(unsigned int i=0;i<N;++i)
      {
        operator[](i)/=obj;
      }
  }
  
  /*!\fn vecN operator+(const T&, const vecN&)
    Component-wise addition against a singleton
    \param obj left hand side of + operator
    \param vec right hand side of + operator
   */
  friend
  vecN operator+(const T &obj, const vecN &vec) 
  {
    vecN retval(obj);
    retval+=vec;
    return retval;
  }

  /*!\fn vecN operator-(const T&, const vecN&)
    Component-wise subtraction against a singleton
    \param obj left hand side of - operator
    \param vec right hand side of - operator
   */
  friend
  vecN operator-(const T &obj, const vecN &vec) 
  {
    vecN retval(obj);
    retval-=vec;
    return retval;
  }

  /*!\fn vecN operator*(const T&, const vecN&)
    Component-wise multiplication against a singleton
    \param obj left hand side of * operator
    \param vec right hand side of * operator
   */
  friend
  vecN operator*(const T &obj, const vecN &vec) 
  {
    vecN retval(obj);
    retval*=vec;
    return retval;
  }

  /*!\fn vecN operator/(const T&, const vecN&)  
    Component-wise divition against a singleton
    \param obj left hand side of / operator
    \param vec right hand side of / operator
   */
  friend
  vecN operator/(const T &obj, const vecN &vec) 
  {
    vecN retval(obj);
    retval/=vec;
    return retval;
  }
  
  /*!\fn T dot(const vecN&) const
    Performs inner product against another vecN.
    uses vecN::operator+=(const T&) and operator*(T,T)
    \param obj vecN to perform inner product against
   */
  T
  dot(const vecN &obj) const
  {
    T retval( operator[](0)*obj[0]);

    for(unsigned int i=1;i<N;++i)
      {
        retval+=operator[](i) * obj[i];
      }
    return retval;
  }

  /*!\fn T magnitudeSq
    Conveninace function, equivalent to
    \code dot(*this) \endcode
   */
  T
  magnitudeSq(void) const
  {
    return dot(*this);
  }

  /*!\fn T magnitude
    Conveninace function, equivalent to
    \code std::sqrt(dot(*this)) \endcode
   */
  T
  magnitude(void) const
  {
    return std::sqrt(magnitudeSq());
  }

  /*!\fn T L1norm
    Computes the sum of std::abs() of each
    of the elements of the vecN.
   */
  T 
  L1norm(void) const
  {
    T retval(std::abs(operator[](0)) );
    
    for(unsigned int i=1;i<N;++i)
      {
        retval+=std::abs(operator[](i));
      }
    return retval;
  }

  /*!\fn void AddMult
    Conveniance function, increments
    this vecN by dood*mult by doing so
    on each component individually,
    slighly more efficient than
    operator+=(dood*mult).
   */
  void
  AddMult(const vecN &dood, const T &mult)
  {
    for(unsigned int i=0;i<N;++i)
      {
        operator[](i)+=mult*dood[i];
      }
  }
  
  /*!\fn void face_forward
    Conveniance geometryic function; if dot(*this, referencePt)
    is negative, negates the elements of this
    \param referencePt refrence point to face forward to.
   */
  void
  face_forward(const vecN &referencePt)
  {
    T val;
    
    val=dot(*this,referencePt);
    if(val<T(0))
      {
        for(unsigned int i=0;i<N;++i)
          {
            operator[](i)=-operator[](i);
          }
      }
  }

  /*!\fn bool operator!=(const vecN&) const
    Inequality operator by checking each array index individually.
    \param obj vecN to which to compare.
   */
  bool
  operator!=(const vecN &obj) const
  {
    if(&obj==this)
      {
        return false;
      }
    for(unsigned int i=0;i<N;++i)
      {
        if(obj[i]!=(*this)[i])
          {
            return true;
          }
      }
    return false;
  } 

  /*!\fn bool operator==(const vecN&) const
    Equality operator by checking each array index individually.
    \param obj vecN to which to compare.
   */
  bool
  operator==(const vecN &obj) const
  {
    if(this==&obj)
      {
        return true;
      }
    for(unsigned int i=0;i<N;++i)
      {
        if(obj[i]!=(*this)[i])
          {
            return false;
          }
      }
    return true;
  } 

  /*!\fn bool operator<=(const vecN&) const
    Returns lexographical operator<= by performing on individual elements.
    \param obj vecN to which to compare.
   */
  bool
  operator<=(const vecN &obj) const
  {
    if(this==&obj)
      {
        return true;
      }
    for(unsigned int i=0;i<N;++i)
      {
        if(!( (*this)[i]<=obj[i] ))
          {
            return false;
          }

        if((*this)[i]!=obj[i])
          {
            return true;
          }
      }
    return true;
  } 

  /*!\fn bool operator<(const vecN&) const
    Returns lexographical operator< by performing
    on individual elements.
    \param obj vecN to which to compare.
   */
  bool
  operator<(const vecN &obj) const
  {
    if(this==&obj)
      {
        return false;
      }
    for(unsigned int i=0;i<N;++i)
      {
        if((*this)[i]<obj[i])
          {
            return true;
          }
        if((*this)[i]!=obj[i])
          {
            return false;
          }
      }
    return false;
  } 

  /*!\fn bool operator>=(const vecN&) const
    Returns lexographical operator>= by performing
    on individual elements.
    \param obj vecN to which to compare.
   */
  bool
  operator>=(const vecN &obj) const
  {
    if(this==&obj)
      {
        return true;
      }
    for(unsigned int i=0;i<N;++i)
      {
        if(!( (*this)[i]>=obj[i] ))
          {
            return false;
          }

        if((*this)[i]!=obj[i])
          {
            return true;
          }
      }
    return true;
  } 

  /*!\fn bool operator>(const vecN&) const
    Returns lexographical operator> by performing
    on individual elements.
    \param obj vecN to which to compare.
   */
  bool
  operator>(const vecN &obj) const
  {
    if(this==&obj)
      {
        return false;
      }
    for(unsigned int i=0;i<N;++i)
      {
        if((*this)[i]>obj[i])
          {
            return true;
          }

        if((*this)[i]!=obj[i])
          {
            return false;
          }
      }
    return false;
  } 

  /*!\fn void normalize(T)
    Normalize this vecN up to a tolerance,
    equivalent to
    \code
    operator/=std::sqrt(tol, std::max(magnitudeSq()))
    \endcode
    \param tol tolerance to avoid dividing by too small values
   */
  void
  normalize(T tol)
  {
    T denom;
    denom=magnitudeSq();
    denom=std::sqrt(std::max(denom, tol));    
    (*this)/=denom;
  }

  
  /*!\fn void normalize(void)
    Normalize this vecN to a default tolerancee,
    equivalent to
    \code
    normalize( T(0.00001*0.00001)
    \endcode
  */
  void
  normalize(void)
  {
    normalize(T(0.00001*0.00001) );
  }

  /*!\fn vecN normal_vector(void) const
    Returns the vector that would be made
    by calling normalize(void).
   */
  vecN
  normal_vector(void) const
  {
    vecN retval(*this);
    retval.normalize();
    return retval;
  }
  
  /*!\fn size_t size  
    STL compliand size function, note that it is static
    since the size of the array is determined by the template
    parameter N.
   */
  static
  size_t 
  size(void) { return static_cast<size_type>(N); }

  /*!\fn iterator begin(void)
    STL compliant iterator function.
   */
  iterator
  begin(void) { return iterator(c_ptr()); }
  
  /*!\fn const_iterator begin(void) const
    STL compliant iterator function.
   */
  const_iterator
  begin(void) const { return const_iterator(c_ptr()); }

  /*!\fn iterator end(void)
    STL compliant iterator function.
   */
  iterator
  end(void) { return iterator( c_ptr()+static_cast<difference_type>(size()) ); }
  
  /*!\fn const_iterator end(void) const
    STL compliant iterator function.
   */
  const_iterator
  end(void) const { return const_iterator( c_ptr()+static_cast<difference_type>(size()) ); }

  /*!\fn reverse_iterator rbegin(void)
    STL compliant iterator function.
   */
  reverse_iterator
  rbegin(void) { return reverse_iterator(end()); }
  
  /*!\fn const_reverse_iterator rbegin(void) const
    STL compliant iterator function.
   */
  const_reverse_iterator
  rbegin(void) const { return const_reverse_iterator(end()); }

  /*!\fn reverse_iterator rend(void)
    STL compliant iterator function.
   */
  reverse_iterator
  rend(void) { return reverse_iterator(begin()); }
  
  /*!\fn const_reverse_iterator rend(void) const
    STL compliant iterator function.
   */
  const_reverse_iterator
  rend(void) const { return const_reverse_iterator(begin()); }

  /*!\fn T& back(void)
    STL compliant back() function.
   */
  T&
  back(void) { return (*this)[size()-1]; }
  
  /*!\fn const T& back(void) const
    STL compliant back() function.
   */
  const T&
  back(void) const { return (*this)[size()-1]; }

  /*!\fn T& front(void)
    STL compliant front() function.
   */
  T&
  front(void) { return (*this)[0]; }
  
  /*!\fn const T& front(void) const
    STL compliant front() function.
   */
  const T&
  front(void) const { return (*this)[0]; }

private:
  T m_data[N];
  
};


/*!\fn T dot(const vecN<T,N>&, const vecN<T,N>&)
  conveniance function, equivalent to
  \code
  a.dot(b)
  \endcode
  \param a object to perform dot
  \param b object passed as parameter to dot.
 */
template<typename T, unsigned int N>
T 
dot(const vecN<T,N> &a, const vecN<T,N> &b)
{
  return a.dot(b);
}

/*!\fn T magnitudeSq(const vecN<T,N>&)
  conveniance function, equivalent to
  \code
  in.magnitudeSq()
  \endcode
  \param in object to perform magnitudeSq
*/
template<typename T, unsigned int N>
inline 
T 
magnitudeSq(const vecN<T,N> &in)
{
  return in.magnitudeSq();
}
 
/*!\fn T magnitude(const vecN<T,N>&)
  conveniance function, equivalent to
  \code
  in.magnitude()
  \endcode
  \param in object to perform magnitude
*/
template<typename T, unsigned int N>
inline 
T 
magnitude(const vecN<T,N> &in)
{
  return in.magnitude();
} 

/*!\fn bool magnitude_compare(const vecN<T,N>&, const vecN<T,N>&)
  conveniance function to compare magnitude squares, equivalent to
  \code
  a.magnitudeSq()<b.magnitudeSq();
  \endcode
  \param a left hand side of comparison
  \param b right hand side of comparison
*/
template<typename T, unsigned int N>
bool
magnitude_compare(const vecN<T,N> &a,
                  const vecN<T,N> &b)
{
  return a.magnitudeSq()<b.magnitudeSq();
}

/*!\fn std::ostream& operator<<(std::ostream&, const vecN<T,N>&)
  Overloaded operator<< to print to an std::stream
  the contents of a vecN
  \param str std::stream to which to print
  \param obj contents to print to obj
 */
template<typename T, unsigned int N>
inline
std::ostream&
operator<<(std::ostream &str, const vecN<T,N> &obj)
{
  str <<  "( " << WRATHUtil::print_range(obj.begin(), obj.end(), ", ") << " )";
  return str;
}


/*! @} */

namespace std 
{
  template<typename T, unsigned int N>
  inline
  void
  swap(vecN<T,N> &obj0, vecN<T,N> &obj1)
  {
    obj0.swap(obj1);
  }
}




#endif
