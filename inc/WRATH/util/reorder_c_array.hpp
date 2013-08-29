/*! 
 * \file reorder_c_array.hpp
 * \brief file reorder_c_array.hpp
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



#ifndef __REORDER_C_ARRAY_HPP__
#define __REORDER_C_ARRAY_HPP__

#include "WRATHConfig.hpp"
#include "c_array.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class reorder_c_array
  A reorder_c_array represents accessing
  an array with the indices permuted. A
  reorder_c_array only has a reference to
  both the elements manipulated and the
  permutation array.
  \tparam T underlying type of each element of the array
  \tparam I integer type for permutation array
 */
template<typename T, typename I=int>
class reorder_c_array
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

  /*!\fn reorder_c_array(c_array<T>, const_c_array<I>)
    Ctor. 
    \param pdata c_array of data manipulated. The elements
                 are NOT copied, only the "pointer" value
                 from the c_array is.
    \param permute const_c_array of permuation. The elements
                   are NOT copied, only the "pointer" value
                   from the const_c_array is. Permuation array
                   at index I gives the index into pdata
                   to be accessed by element I of the cosntructed
                   reorder_c_array
   */
  reorder_c_array(c_array<T> pdata,
                  const_c_array<I> permute):
    m_data(pdata),
    m_permute(permute)
  {}

  /*!\fn T&  operator[](int) const
    Access element
    \param j index
   */
  T& 
  operator[](int j) const
  {
    return m_data[ m_permute[j] ];
  }

  /*!\fn bool empty(void) const
    STL compliant function.
   */
  bool
  empty(void) const
  {
    return m_permute.empty();
  }

  /*!\fn size_t size
    STL compliant function.
   */
  size_t 
  size(void) const 
  { 
    return m_permute.size(); 
  }

  /*!\fn reorder_c_array sub_array(int, int) const
    Returns a logical sub-array of this reorder_c_array
    \param pos offset into this of 1st element, must have that pos>=0
    \param length length of array to return. Must have
                  that pos+length<=size() and length>=0
   */
  reorder_c_array
  sub_array(int pos, int length) const
  {
    return reorder_c_array(m_data, m_permute.sub_array(pos, length));
  }

  /*!\fn reorder_c_array sub_array(int) const
    Provided as a conveniance, equivalent to
    \code
    sub_array(pos, size()-pos);
    \endcode
    \param pos offset into this of 1st element, must have that pos>=0
   */
  reorder_c_array
  sub_array(int pos) const
  {
    return reorder_c_array(m_data, m_permute.sub_array(pos));
  }

  /*!\fn sub_array(range_type<int>) const
    Provided as a conveniance, equivalent to
    \code
    sub_array(R.begin, R.m_end-R.m_begin)
    \endcode
    \param R range of indices into this reorder_c_array to be
             accessed by the returned sub_array 
   */
  reorder_c_array
  sub_array(range_type<int> R) const
  {
    return reorder_c_array(m_data, m_permute.sub_array(R));
  }

  /*!\fn const_c_array<I> permutation
    Returns the const_c_array specifying
    the permutation
   */
  const_c_array<I>
  permutation(void) const
  {
    return m_permute;
  }

  /*!\fn c_array<T> unpermuted_data 
    Returns the c_array of the data
    unpermuted.
   */
  c_array<T>
  unpermuted_data(void) const
  {
    return m_data;
  }

private:
  c_array<T> m_data;
  const_c_array<I> m_permute;
};



/*!\class reorder_const_c_array
  A reorder_const_c_array represents accessing
  an array with the indices permuted. A
  reorder_c_array only has a reference to
  both the elements manipulated and the
  permutation array.
  \tparam T underlying type of each element of the array
  \tparam I integer type for permutation array
 */
template<typename T, typename I=int>
class reorder_const_c_array
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


  /*!\fn reorder_const_c_array(const_c_array<T>, const_c_array<I>)
    Ctor. 
    \param pdata const_c_array of data manipulated. The elements
                 are NOT copied, only the "pointer" value
                 from the const_c_array is.
    \param permute const_c_array of permuation. The elements
                   are NOT copied, only the "pointer" value
                   from the const_c_array is. Permuation array
                   at index I gives the index into pdata
                   to be accessed by element I of the cosntructed
                   reorder_c_array
   */
  reorder_const_c_array(const_c_array<T> pdata,
                        const_c_array<I> permute):
    m_data(pdata),
    m_permute(permute)
  {}

  /*!\fn reorder_const_c_array(reorder_c_array<T, I>)
    Ctor. Create a reorder_const_c_array from a \ref reorder_c_array
    \param v reorder_c_array from which to construct this reorder_const_c_array
   */
  reorder_const_c_array(reorder_c_array<T, I> v):
    m_data(v.data()),
    m_permute(v.permutation())
  {}

  /*!\fn const T& operator[](int) const
    Access element
    \param j index
   */
  const T& 
  operator[](int j) const
  {
    return m_data[ m_permute[j] ];
  }

  /*!\fn bool empty(void) const
    STL compliant function.
   */
  bool
  empty(void) const
  {
    return m_permute.empty();
  }

  /*!\fn size_t size
    STL compliant function.
   */
  size_t 
  size(void) const 
  { 
    return m_permute.size(); 
  }

  /*!\fn reorder_const_c_array sub_array(int, int) const
    Returns a logical sub-array of this reorder_const_c_array
    \param pos offset into this of 1st element, must have that pos>=0
    \param length length of array to return. Must have
                  that pos+length<=size() and length>=0
   */
  reorder_const_c_array
  sub_array(int pos, int length) const
  {
    return reorder_const_c_array(m_data, m_permute.sub_array(pos, length));
  }

  /*!\fn reorder_const_c_array sub_array(int) const
    Provided as a conveniance, equivalent to
    \code
    sub_array(pos, size()-pos);
    \endcode
    \param pos offset into this of 1st element, must have that pos>=0
   */
  reorder_const_c_array
  sub_array(int pos) const
  {
    return reorder_const_c_array(m_data, m_permute.sub_array(pos));
  }

  /*!\fn reorder_const_c_array sub_array(range_type<int>) const
    Provided as a conveniance, equivalent to
    \code
    sub_array(R.begin, R.m_end-R.m_begin)
    \endcode
    \param R range of indices into this reorder_c_array to be
             accessed by the returned sub_array 
   */
  reorder_const_c_array
  sub_array(range_type<int> R) const
  {
    return reorder_const_c_array(m_data, m_permute.sub_array(R));
  }

  /*!\fn const_c_array<I> permutation
    Returns the const_c_array specifying
    the permutation
   */
  const_c_array<I>
  permutation(void) const
  {
    return m_permute;
  }

  /*!\fn const_c_array<T> unpermuted_data
    Returns the const_c_array of the data unpermuted.
   */
  const_c_array<T>
  unpermuted_data(void) const
  {
    return m_data;
  }

private:
  const_c_array<T> m_data;
  const_c_array<I> m_permute;
};


/*! @} */

#endif
