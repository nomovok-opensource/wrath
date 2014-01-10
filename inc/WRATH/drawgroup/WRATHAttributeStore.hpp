/*! 
 * \file WRATHAttributeStore.hpp
 * \brief file WRATHAttributeStore.hpp
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




#ifndef __WRATH_ATTRIBUTE_STORE_HPP__
#define __WRATH_ATTRIBUTE_STORE_HPP__

#include "WRATHConfig.hpp"
#include <typeinfo>
#include <limits>
#include <boost/utility.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include "WRATHBufferAllocator.hpp"
#include "WRATHRawDrawData.hpp"
#include "WRATHInterleavedAttributes.hpp"
#include "WRATHTripleBufferEnabler.hpp"

/*! \addtogroup Group
 * @{
 */

class WRATHAttributeStore;
class WRATHAttributeStoreAllocator;

/*!\class WRATHAttributeStoreKey 
  Class to specify the parameters of a WRATHAttributeStore.
  Specifies indexing type and attribute format and type.
*/
class WRATHAttributeStoreKey
{
public:

  /*!\enum index_bit_count_type
    Enumeration that specifies
    how many bits an index occupies.
   */
  enum index_bit_count_type
    {
      /*!
        indices will be 8 bits wide, thus
        a WRATHAttributeStore created with
        such a key may only hold up to
        255 distinct different elements.
       */
      index_8bits,

      /*!
        indices will be 16 bits wide, thus
        a WRATHAttributeStore created with
        such a key may only hold up to
        65,535 distinct different elements.
       */
      index_16bits,

      /*!
        indices will be 32 bits wide, however
        a WRATHAttributeStore created with
        such a key may only hold up to
        the number of elements that would
        occupy 2GB of memory.
       */
      index_32bits
    };

  /*!\fn WRATHAttributeStoreKey
    Default ctor indicating 16-bit unsigned indices
    and \ref m_buffer_object_hint to GL_STATIC_DRAW.
    The type (and hence also) the default value are 
    not set.
   */
  WRATHAttributeStoreKey(void):
    m_buffer_object_hint(GL_STATIC_DRAW),
    m_index_bit_count(index_16bits),
    m_type_size(0)
  {}

  /*!\fn WRATHAttributeStoreKey(type_tag<T>, GLenum, enum index_bit_count_type)
    Ctor to set index type, \ref m_buffer_object_hint 
    the type, attribute format, and default value. 
    \tparam T type to use for the attribute.
              The type T must implement the static method 
              attribute_key(WRATHDrawCallSpec::attribute_array_param &)
              which sets the passed reference in the same
              fashion as WRATHInterleavedAttributes::attribute_key()
              does.
    \param pbuffer_object_hint value to which to set \ref m_buffer_object_hint
    \param pindex_bit_count value to which to set \ref m_index_bit_count
   */
  template<typename T>
  explicit
  WRATHAttributeStoreKey(type_tag<T>, 
                         GLenum pbuffer_object_hint=GL_STATIC_DRAW,
                         enum index_bit_count_type pindex_bit_count=index_16bits):
    m_buffer_object_hint(pbuffer_object_hint),
    m_index_bit_count(pindex_bit_count),      
    m_type_size(sizeof(T))
  {
    T::attribute_key(m_attribute_format_location);
  }

  
  /*!\fn WRATHAttributeStoreKey(type_tag<T>, const vecN<GLboolean, N>&, GLenum,
                                enum index_bit_count_type)
    Ctor to set index type, \ref m_buffer_object_hint 
    the type, attribute format, and default value. 
    \tparam T type to use for the attribute.
              The type T must implement the static method 
              attribute_key(WRATHDrawCallSpec::attribute_array_param &)
              which sets the passed reference in the same
              fashion as WRATHInterleavedAttributes::attribute_key()
              does.
    \param normalizeds fixed length array specifying the normalization
                       flag values for opengl_trait_value::m_normalized.
                       If N is larger than WRATHDrawCallSpec::attribute_array_params::array_size, then
                       indices beyond WRATHDrawCallSpec::attribute_array_params::array_size are ignored.
                       If N is smaller than WRATHDrawCallSpec::attribute_array_params::array_size, then
                       indices beyond the passed array are set as GL_FALSE.
    \param pbuffer_object_hint value to which to set \ref m_buffer_object_hint
    \param pindex_bit_count value to which to set \ref m_index_bit_count
   */
  template<typename T, unsigned int N>
  explicit
  WRATHAttributeStoreKey(type_tag<T>,
                         const vecN<GLboolean, N> &normalizeds,
                         GLenum pbuffer_object_hint=GL_STATIC_DRAW,
                         enum index_bit_count_type pindex_bit_count=index_16bits):
    m_buffer_object_hint(pbuffer_object_hint),
    m_index_bit_count(pindex_bit_count),      
    m_type_size(sizeof(T))
  {
    T::attribute_key(m_attribute_format_location);
    for(unsigned int i=0, 
          end_i=std::min(N, 
                         static_cast<unsigned int>(m_attribute_format_location.size()));
        i!=end_i; ++i)
      {
        m_attribute_format_location[i]=normalizeds[i];
      }    
  }
  
  /*!\fn WRATHAttributeStoreKey& type(type_tag<T>)
    Set the attribute type (but not the format).
    \tparam T type to use for the attribute
  */
  template<typename T>
  WRATHAttributeStoreKey&
  type(type_tag<T>)
  {
    m_type_size=sizeof(T);
    return *this;
  }
    
  /*!\fn WRATHAttributeStoreKey& buffer_object_hint
    Set the buffer object hint for attributes,
    default value is GL_STATIC_DRAW.
    \param v value to which to set \ref m_buffer_object_hint
  */
  WRATHAttributeStoreKey&
  buffer_object_hint(GLenum v)
  {
    m_buffer_object_hint=v;
    return *this;
  }
  
  /*!\fn WRATHAttributeStoreKey& index_bit_count
    Set the index bit count, default value is
    WRATHAttributeStore::index_16bits.
    \param v value to which to set \ref m_index_bit_count
  */
  WRATHAttributeStoreKey&
  index_bit_count(enum index_bit_count_type v)
  {
    m_index_bit_count=v;
    return *this;
  }

  /*!\fn WRATHAttributeStoreKey& type_and_format(type_tag<T>)
    Specify both the attribute type and format of data.
    The type T must implement the static method
    attribute_key(), such type includes WRATHInterleavedAttribute.
    \tparam T type of the attribute
   */
  template<typename T>
  WRATHAttributeStoreKey&
  type_and_format(type_tag<T>)
  {
    type(type_tag<T>());
    T::attribute_key(m_attribute_format_location);
    return *this;
  }

  /*!\fn WRATHAttributeStoreKey& type_and_format(type_tag<T>, const vecN<GLboolean, N> &)
    Specify both the attribute type and format of data.
    \tparam T type of the attribute.  The type T must implement the static method
              attribute_key(), such type includes WRATHInterleavedAttribute.
    \param normalizeds fixed length array specifying the normalization
                       flag values for opengl_trait_value::m_normalized.
                       If N is larger than WRATHDrawCallSpec::attribute_array_params::array_size, then
                       indices beyond WRATHDrawCallSpec::attribute_array_params::array_size are ignored.
                       If N is smaller than WRATHDrawCallSpec::attribute_array_params::array_size, then
                       indices beyond the passed array are left as is.
   */
  template<typename T, unsigned int N>
  WRATHAttributeStoreKey&
  type_and_format(type_tag<T>, 
                  const vecN<GLboolean, N> &normalizeds)
  {
    type(type_tag<T>());
    T::attribute_key(m_attribute_format_location);

    for(unsigned int i=0, 
          end_i=std::min(N, 
                         static_cast<unsigned int>(m_attribute_format_location.size()));
        i!=end_i; ++i)
      {
        m_attribute_format_location[i]=normalizeds[i];
      }

    return *this;
  }
  
  /*!\fn WRATHAttributeStoreKey& attribute_format(int, const opengl_trait_value&)
    Set the named attribute format.
    \param i which index of \ref m_attribute_format_location to set
    \param v new value for \ref m_attribute_format_location[i]
  */
  WRATHAttributeStoreKey&
  attribute_format(int i, const opengl_trait_value &v)
  {
    m_attribute_format_location[i]=v;
    return *this;
  }

  /*!\fn WRATHAttributeStoreKey& attribute_format(const WRATHDrawCallSpec::attribute_array_params&)
    Set all attribute formats.
    \param v new value for \ref m_attribute_format_location
  */
  WRATHAttributeStoreKey&
  attribute_format(const WRATHDrawCallSpec::attribute_array_params &v)
  {
    m_attribute_format_location=v;
    return *this;
  }
  
 
  /*!\fn bool operator<(const WRATHAttributeStoreKey &) const
    comparison operator for sorting, which sorts 
    in the following order:
     -# \ref m_buffer_object_hint
     -# \ref m_index_bit_count
     -# type()
     -# \ref m_attribute_format_location
     \param rhs object to which to compare
   */
  bool
  operator<(const WRATHAttributeStoreKey &rhs) const;

  /*!\fn bool operator==(const WRATHAttributeStoreKey &) const
    comparison operator for equlity, returns
    true if any only if all of the following
    are equal:
     - \ref m_buffer_object_hint
     - \ref m_index_bit_count
     - type()
     - \ref m_attribute_format_location
     \param rhs object to which to compare
   */
  bool
  operator==(const WRATHAttributeStoreKey &rhs) const;

  /*!\fn bool operator!=(const WRATHAttributeStoreKey &) const
    comparison operator for inequality,
    equivalent to !operator==(const WRATHAttributeStoreKey&).  
    \param rhs WRATHAttributeStoreKey to which to compare
   */
  bool
  operator!=(const WRATHAttributeStoreKey &rhs) const
  {
    return !operator==(rhs);
  }
  
  /*!\var m_attribute_format_location
    Specifies the bit packing of the attribute data
    as fed into glVertexAttribPointer, see
    also \ref WRATHRawDrawDataElement
    and \ref opengl_trait_value.
   */
  WRATHDrawCallSpec::attribute_array_params m_attribute_format_location;
  
  /*!\var m_buffer_object_hint
    Specifies the buffer object hint (and if) for
    the store of the attribute data. If 
    m_buffer_object_hint is GL_INVALID_VALUE
    then a GL buffer object is NOT used
    to store the attribute data.
   */
  GLenum m_buffer_object_hint;
  
  /*!\var m_index_bit_count
    Specifies the number of bits that the index
    type will have, which in turn determines
    the maximum number of elements one 
    WRATHAttributeStore of this WRATHAttributeStoreKey
    may store. 
   */
  enum index_bit_count_type m_index_bit_count;
  
  /*!\fn int type_size
    Returns the size, in bytes, of the attribute type.
   */
  int
  type_size(void) const
  {
    return m_type_size;
  }

  /*!\fn bool valid
    A WRATHAttributeStoreKey is said to be valid
    if all of the following conditions are true:
    - specified atleast one attribute
    - attributes used are continuously allocated starting at index 0
   */
  bool
  valid(void) const;

  /*!\fn enum index_bit_count_type index_bit_count_from_type
    Returns the index bit count enumeration based on the attribute type.
   */
  template<typename I>
  static
  enum index_bit_count_type
  index_bit_count_from_type(void);

private:
  int m_type_size;
};


/*!\class WRATHAttributeStore
  A WRATHAttributeStore is a store for attribute data,
  such stores are keyed by an attribute type and the
  maximum number of attributes allowed (one of
  256, 65536 or 2^32, i.e. index type GL_UNSIGNED_BYTE,
  GL_UNSIGNED_SHORT or GL_UNSIGNED_INT). Note that
  at construction a WRATHAttributeStore will allocate
  location 0 with the default value as specified by
  the WRATHAttributeStoreKey used to specify it.
  A WRATHAttributeStore can only be created by a
  WRATHAttributeStoreAllocator.

  A WRATHAttributeStore has two forms of data:
  explicit attribute data that is specified
  by a WRATHAttributeStoreKey and implicit
  attribute data that is specified by the
  WRATHAttributeStoreAllocator that created
  the WRATHAttributeStore. The implicit data
  is used to assign those attribute values
  associated to an implementation of
  WRATHCanvas, for example
  the implicit attribute data may be a
  texture coordinate into a texture
  that holds transformation data, etc.

  Allocation of attribute data allocates
  memory for both the explicit attribute
  data and the implicit attribute data.
  The explicit attribute data is manipulated
  via methods of WRATHAttributeStore. Indeed,
  for manipulations of explicit attribute data,
  a WRATHAttributeStore is mostly just a wrapper 
  over a WRATHBufferAllocator. Allocation and setting
  of attributes is handled by an underlying WRATHBufferAllocator.
  The following functions map to the underlying WRATHBufferAllocator
  and exhibit the same locking patterns:
  - attributes_allocated() 
  - max_cts_allocate_possible() 
  - max_fragmented_allocate_possible() 
  - read_pointer<T>(range_type<int>) const
  - read_pointer<T>(int, int) const
  - pointer<T>(range_type<int>)
  - pointer<T>(int, int)
  - deallocate_attribute_datas<iterator>(iterator, iterator)
  - deallocate_attribute_data(range_type<int>)
  - deallocate_attribute_data(int, int)
  - proxy_fragmented_allocate_attribute() 
  - fragmented_allocate_attribute_data(int, std::vector< range_type<int> >&)
  - proxy_attribute_allocate() 
  - allocate_attribute_data(int, range_type<int>&)
  - allocate_attribute_data(int)

  Manipulations of implicit attribute data values
  is accomplished in a more raw format by dealing
  with the WRATHBufferObject, implicit_attribute_data().
 */
class WRATHAttributeStore:
  public WRATHReferenceCountedObjectT<WRATHAttributeStore>
{
public:

  /*!\typedef DataSink
    DataSink type for WRATHAttributeStore is
    WRATHBufferAllocator::DataSink.
   */
  typedef WRATHBufferAllocator::DataSink DataSink;

  virtual
  ~WRATHAttributeStore();
    
  
  /*!\fn const WRATHDrawCallSpec::attribute_array_params& attribute_format_location
    Returns the formatting of the attribute type
    to feed to glVertexAttribPointer, see
    \ref WRATHDrawCallSpec::m_attribute_format_location.
    One important note: the return value includes the
    "implicit" attribute format values that are specified
    by the WRATHAttributeStoreAllocator object that
    created this WRATHAttributeStore. Those attributes
    are added at the first index I where the key().m_attribute_format_location[I].valid()
    is false.
   */
  const WRATHDrawCallSpec::attribute_array_params&
  attribute_format_location(void) const
  {
    return m_attribute_format_location;
  }

  /*!\fn unsigned int attribute_size
    Returns the size in bytes of the attribute type.
   */
  unsigned int
  attribute_size(void) const
  {
    return m_key.type_size();
  }

  /*!\fn GLenum index_type
    Returns the GL enumeration of the
    index type to be used with this 
    WRATHAttributeStore, which is one
    of GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT
    or GL_UNSIGNED_INT.
   */
  GLenum
  index_type(void) const
  {
    return m_index_type;
  }

  /*!\fn int index_type_size
    Returns the size in bytes of the
    index type to be used with this
    WRATHAttributeStore.
   */
  int
  index_type_size(void) const
  {
    return m_index_type_size;
  }

  /*!\fn enum WRATHAttributeStoreKey::index_bit_count_type index_bit_count
    Returns the enumeration of the index
    type to be used with this WRATHAttributeStore.
   */
  enum WRATHAttributeStoreKey::index_bit_count_type
  index_bit_count(void) const
  {
    return m_index_bits;
  }

  /*!\fn int allocate_attribute_data(int)
    Allocates memory in the attribute buffer
    object. Returns the location as an 
    _index_  and -1 on failure.
    Is essentially a wrapper over 
    WRATHBufferAllocator::allocate(int)
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence 
    allocate_attribute_data(int) has the same exact
    locking behavior as WRATHBufferAllocator::allocate(int).
    \param number_elements number of _elements_ to allocate
   */
  int
  allocate_attribute_data(int number_elements);

  /*!\fn enum return_code allocate_attribute_data(int, range_type<int>&)
    Allocates memory in the attribute buffer
    object. Returns the location as a range
    of indices. On failure, R is set as
    the range [-1,-1) and the return value 
    is \ref routine_fail. On success, R is set
    as the range of indices and \ref routine_success
    is returned.
    Is essentially a wrapper over 
    WRATHBufferAllocator::allocate(int)
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence 
    allocate_attribute_data(int, range_type<int>&) has the same exact
    locking behavior as WRATHBufferAllocator::allocate(int).
    \param number_elements number of _elements_ to allocate
    \param R (output) upon success, location of attribute allocation
                      is placed into R.

   */
  enum return_code
  allocate_attribute_data(int number_elements, range_type<int> &R)
  {
    R.m_begin=allocate_attribute_data(number_elements);
    R.m_end=(R.m_begin!=-1)?
      R.m_begin+number_elements:-1;
    return (R.m_begin!=-1)?
      routine_success:routine_fail;
  }

  /*!\fn enum return_code proxy_attribute_allocate
    Returns \ref routine_success if allocate_attribute_data()
    would succeed. 
    Is essentially a wrapper over 
    WRATHBufferAllocator::proxy_allocate(int)
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence 
    allocate_attribute_data(int) has the same exact
    locking behavior as WRATHBufferAllocator::proxy_allocate(int).
    \param number_elements number of elements to check to
                           see if could be allocated in one block.
   */
  enum return_code
  proxy_attribute_allocate(int number_elements) const;


  /*!\fn enum return_code fragmented_allocate_attribute_data
    Allocate attribute data in fragments, i.e. do not 
    insist that attributes allocated are continuously
    stored. Returns \ref routine_success on success and
    \ref routine_fail on failure.
    Is essentially a wrapper over 
    WRATHBufferAllocator::fragmented_allocate(int, std::vector< range_type<int> > &)
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence 
    fragmented_allocate_attribute_data(int, std::vector< range_type<int> > &) 
    has the same exact locking behavior as 
    WRATHBufferAllocator::fragmented_allocate(int, std::vector< range_type<int> > &).
    \param number_elements number of _elements_ to allocate
    \param out_allocations on allocation sucess, _appends_,
                           the locations of the fragments
                           of the allocating as a range_type
                           (i.e. marking the beginning and
                           ending of the fragmented allocation).
                           Note that the locations are offsets
                           in units of the elements size (not in bytes)
   */
  enum return_code
  fragmented_allocate_attribute_data(int number_elements,
                                     std::vector< range_type<int> > &out_allocations);

  /*!\fn enum return_code proxy_fragmented_allocate_attribute
    Returns \ref routine_success if fragmented_allocate_attribute_data()
    would succeed. 

    \param number_elements number of elements to check to
                           see if could be allocated.
   */
  enum return_code
  proxy_fragmented_allocate_attribute(int number_elements) const;

  /*!\fn void deallocate_attribute_data(int, int)
    Dellocates memory in the attribute buffer
    object.
    Is essentially a wrapper over 
    WRATHBufferAllocator::deallocate(int, int)
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence 
    deallocate_attribute_data(int, int) 
    has the same exact locking behavior as 
    WRATHBufferAllocator::deallocate(int, int).
    \param begin_element 1st element to free
    \param end_element one past the last element to free.
   */
  void
  deallocate_attribute_data(int begin_element, int end_element);

  /*!\fn void deallocate_attribute_data(range_type<int> R)
    Dellocates memory in the attribute buffer
    object. Provided as a conveniance, equivalent
    to 
    \code
    deallocate_attribute_data(R.m_begin, R.m_end);
    \endcode
    \param R range of element to free
   */
  void
  deallocate_attribute_data(range_type<int> R)
  {
    deallocate_attribute_data(R.m_begin, R.m_end);
  }

  /*!\fn void deallocate_attribute_datas(iterator, iterator)
    Deallocate a set of ranges specified by an STL
    iterator pair [begin, end). The iterator type
    must dereference into a range_type<int>.
    Provided as a conveniance, simple calls
    deallocate_attribute_data(range_type<int>) on
    each element of the STL range.
    \tparam iterator iterator type to range_type<int> 
    \param begin iterator to first range to deallocate
    \param end iterator to one past the last range to deallocate
   */
  template<typename iterator>
  void
  deallocate_attribute_datas(iterator begin, iterator end)
  {
    for(;begin!=end;++begin)
      {
        deallocate_attribute_data(*begin);
      }
  }

  /*!\fn c_array<T> pointer(int, int)
    Returns a write/read pointer to a range of
    allocated attribute data, the pointer is
    guaranteed to be valid until attribute data
    is allocated or deallocated.
    Is essentially a wrapper over 
    WRATHBufferAllocator::pointer<T>(int, int)
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence pointer<T>(int, int) 
    has the same exact locking behavior as 
    WRATHBufferAllocator::pointer<T>(int, int).
    \param first_element first element to pointer to
    \param number_elements number of elements in the range to point to.
   */
  template<typename T>
  c_array<T>
  pointer(int first_element, int number_elements)
  {
    return m_vertex_buffer->pointer<T>(first_element*sizeof(T), number_elements);
  }

  /*!\fn c_array<T> pointer(range_type<int> R)
    Returns a write/read pointer to a range of
    allocated attribute data, the pointer is
    guaranteed to be valid until attribute data
    is allocated or deallocated.
    Is essentially a wrapper over 
    WRATHBufferAllocator::pointer<T>(int, int)()
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence pointer<T>(range_type<int>) 
    has the same exact locking behavior as 
    WRATHBufferAllocator::pointer<T>(int, int)().
    \param R range_type<int> specifing beginning and end in elements.
   */
  template<typename T>
  c_array<T>
  pointer(range_type<int> R)
  {
    WRATHassert(R.m_end>=R.m_begin);
    return pointer<T>(R.m_begin, R.m_end-R.m_begin);
  }

  /*!\fn const_c_array<T> read_pointer(int, int) const 
    Returns a read only pointer to a range of
    allocated attribute data, the pointer is
    guaranteed to be valid until attribute data
    is allocated or deallocated.
    Is essentially a wrapper over 
    WRATHBufferAllocator::read_pointer<T>(int, int) const
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence read_pointer<T>(int, int) const 
    has the same exact locking behavior as 
    WRATHBufferAllocator::read_pointer<T>(int, int) const.
    \param first_element first element to pointer to
    \param number_elements number of elements in the range to point to.
   */
  template<typename T>
  const_c_array<T>
  read_pointer(int first_element, int number_elements) const
  {
    return m_vertex_buffer->read_pointer<T>(first_element*sizeof(T), number_elements);
  }

  /*!\fn const_c_array<T> read_pointer(range_type<int>) const
    Returns a read pointer to a range of
    allocated attribute data, the pointer is
    guaranteed to be valid until attribute data
    is allocated or deallocated.
    Is essentially a wrapper over 
    WRATHBufferAllocator::read_pointer<T>(int, int) const
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence read_pointer<T>(range_type<int>) const
    has the same exact locking behavior as 
    WRATHBufferAllocator::read_pointer<T>(int, int) const.
    \param R range_type<int> specifing beginning and end in elements.
   */
  template<typename T>
  const_c_array<T>
  read_pointer(range_type<int> R) const
  {
    WRATHassert(R.m_end>=R.m_begin);
    return read_pointer<T>(R.m_begin, R.m_end-R.m_begin);
  }

  /*!\fn int max_fragmented_allocate_possible
    Returns the maximum number of elements that 
    may be allocated by fragmented_allocate_attribute_data().
    Is essentially a wrapper over 
    WRATHBufferAllocator::max_fragmented_allocate_possible()
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence 
    max_fragmented_allocate_possible() 
    has the same exact locking behavior as 
    WRATHBufferAllocator::max_fragmented_allocate_possible().
   */
  int
  max_fragmented_allocate_possible(void) const;

  /*!\fn int max_cts_allocate_possible
    Returns the maximum number of continuous
    bytes that can be alloated *now*, i.e
    allocate() is gauranteed to succeeed
    (and poxy_allocate to return \ref routine_success)
    with the any value less than or equal
    to the return value of max_cts_allocate_possible().
    Is essentially a wrapper over 
    WRATHBufferAllocator::max_cts_allocate_possible()
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence 
    max_cts_allocate_possible() 
    has the same exact locking behavior as 
    WRATHBufferAllocator::max_cts_allocate_possible().
   */
  int
  max_cts_allocate_possible(void) const;

  /*!\fn int attributes_allocated
    Returns the number of elements
    allocated on this WRATHAttributeStore.
    Is essentially a wrapper over 
    WRATHBufferAllocator::bytes_allocated()
    with arguments and return value
    adjusted to be in size of elements
    rather than bytes. Hence 
    attributes_allocated(void) 
    has the same exact locking behavior as 
    WRATHBufferAllocator::bytes_allocated().
   */
  int
  attributes_allocated(void) const;

  /*!\fn WRATHBufferAllocator* buffer_allocator
    Returns the underlying WRATHBufferAllocator
    where the attribute data resides.
   */
  WRATHBufferAllocator*
  buffer_allocator(void)
  {
    return m_vertex_buffer;
  }

  /*!\fn DataSink& data_sink
    Returns a \ref DataSink object that manipulates
    the explicit attribute of this WRATHAttributeStore,
    equivalent to
    \code
    buffer_allocator()->data_sink();
    \endcode
   */
  DataSink&
  data_sink(void)
  {
    return m_vertex_buffer->data_sink();
  }

  /*!\fn const WRATHAttributeStoreKey& key
    Returns the key of this WRATHAttributeStore.
   */
  const WRATHAttributeStoreKey&
  key(void) const
  {
    return m_key;
  }

  /*!\fn WRATHMutex& mutex
    Returns the WRATHMutex used for the 
    attribute data held within this
    WRATHAttributeStore, equivalent to
    \code   
    buffer_allocator()->mutex();
    \endcode
   */
  WRATHMutex&
  mutex(void) 
  {
    return buffer_allocator()->mutex();
  }

  /*!\fn void add_implicit_store
    Adds(as necessary) an implicit attribute store.
    The buffer object of the implicit data can
    be fetched by implicit_attribute_data()
    \param idx index of implicit attribute store
   */ 
  void
  add_implicit_store(unsigned int idx);
  
  /*!\fn WRATHBufferObject* implicit_attribute_data
    Implicit attribute data is directly manipulated
    essentially by a WRATHCanvas
    derived object. It's purpose is to hold
    those attributes that are used to indicate
    "which" transformation/visibiliy/whatever
    nodes indicated by an object derived from
    WRATHCanvas::CustomDataBase.
    The mutex used by the buffer object is guaranteed
    to be _different_ than the mutex used by
    non-implicit attributes, i.e. it is not
    buffer_allocator()->mutex(). For those
    WRATHAttributeStore objects that do not
    have implicit attribute data, returns NULL.
    \param idx "index" of impliciat attribute data. A single
               WRATHAttributeStore may have multiple implicit
               data streams. The use case is to use common
               explicit attribute data across different draw
               calls with even different nodes.
   */
  WRATHBufferObject*
  implicit_attribute_data(unsigned int idx) const;

  /*!\fn const vecN<WRATHBufferObject*, WRATHDrawCallSpec::attribute_count>& buffer_object_vector
    Returns the buffer object vector, i.e. a listing for each
    attribute slot what buffer object is to be used,
    see WRATHDrawCallSpec::m_data_store for use
    together with the named implicit attribute data. 
    \param idx selector of which implicit attribute stream to use,
               same meaning as in implicit_attribute_data().
   */
  const vecN<WRATHBufferObject*, WRATHDrawCallSpec::attribute_count>&
  buffer_object_vector(unsigned int idx) const;

  /*!\fn unsigned int total_size
    Simple conveniance class to return how many attributes
    total are indicated by an array of range_type<int>'s.
    \param attr_locations reference to attribute locations
   */
  static
  unsigned int
  total_size(const std::vector<range_type<int> > &attr_locations);

private:

  /*
    implicit attribute data is directly manipulated
    essentially by a WRATHCanvas
    derived object. It's purpose is to hold
    those attributes that are used to indicate
    "which" transformation/visibiliy/whatever
    nodes indicated by an object derived from
    WRATHCanvas::CustomDataBase.
   */
  class per_implicit_store:
    public WRATHMutex, //make the WRATHMutex first, thus it is in scope first
    public WRATHBufferObject //to be used by the ctor of WRATHBufferObject
  {
  public:
    vecN<WRATHBufferObject*, WRATHDrawCallSpec::attribute_count> m_buffer_object_vector;

    per_implicit_store(const WRATHTripleBufferEnabler::handle &tr,
                       GLenum buffer_object_hint):
      WRATHMutex(),
      WRATHBufferObject(tr, buffer_object_hint, this),
      m_buffer_object_vector(NULL)
    {}
  };


  friend class WRATHAttributeStoreAllocator;

  WRATHAttributeStore(const WRATHAttributeStoreKey &pkey,
                      WRATHAttributeStoreAllocator *allocator,
                      bool allocate_implicit_attribute_data);

  void
  resize_implicit_stores(int req_size);

  per_implicit_store*
  fetch_implicit_store(unsigned int) const;

  WRATHAttributeStoreKey m_key;
  std::vector<uint8_t> m_value_at_index0;
  std::vector<opengl_trait_value> m_implicit_attribute_format;
  int m_number_non_implicit_attributes;

  WRATHDrawCallSpec::attribute_array_params m_attribute_format_location;
  enum WRATHAttributeStoreKey::index_bit_count_type m_index_bits;
  GLenum m_index_type;
  int m_index_type_size;

  GLenum m_buffer_object_hint;

  WRATHBufferAllocator *m_vertex_buffer;
  int m_implicit_attribute_size;

  WRATHMutex m_allocator_ptr_mutex;
  WRATHAttributeStoreAllocator *m_allocator;

  mutable WRATHMutex m_implicit_store_mutex;
  int m_req_implicit_attribute_size;
  std::map<unsigned int, per_implicit_store*> m_implicit_attribute_data;
};

/*!\class WRATHAttributeStoreAllocator 
  The purpose of a WRATHAttributeStoreAllocator is to fetch 
  (and as necessary) allocate an WRATHAttributeStore
  from a WRATHAttributeStoreKey together with an allocation
  requirement.

  In addition, ALL WRATHAttributeStore objects created
  by a fixed WRATHAttributeStoreAllocator have the exact
  same implicit attribute data. Of critical importance
  is that the default value of the implicit attribute
  data of a WRATHAttributeStoreAllocator guarantees
  that the vertex will be clipped. This value is
  written to as the implicit attribute value
  at index 0. 
 */
class WRATHAttributeStoreAllocator:
  public WRATHTripleBufferEnabler::PhasedDeletedObject
{
public:

  /*!\enum implicit_attribute_req_t
    Enumeration specifying weather or
    not to request an WRATHAttributeStore
    object with implicit attribute
    data.
   */
  enum implicit_attribute_req_t
    {
      /*!
       */
      include_implicit_attribute,

      /*!
       */
      skip_implicit_attribute
    };

  /*!\fn WRATHAttributeStoreAllocator(const WRATHTripleBufferEnabler::handle &r,
                                      const std::vector<opengl_trait_value> & pimplicit_attribute_format,
                                      const std::vector<uint8_t> &pvalue_at_index0);
    Ctor.
     \param r handle to a WRATHTripleBufferEnabler to
              which the users of the created object will
              sync. It is an error if the handle is not valid.
     \param pimplicit_attribute_format each WRATHAttributeStore allocated by this
                                       WRATHAttributeStoreAllocator will also hold
                                       "implicit" attribute data that is used by
                                       a WRATHCanvas to specify
                                       those attribute values that are determined
                                       by what node (i.e transformation, visbility, etc)
                                       a drawn element is on. Note that having the
                                       pimplicit_attribute_format parameter empty means
                                       that WRATHAttributeStore objects created by
                                       the WRATHAttributeStoreAllocator will not 
                                       have implicit attribute data. In this case
                                       such WRATHAttributeStore objects are NOT for
                                       use in WRATHItemGroup, rather for direct use
                                       for a WRATHRawDrawElement (for example holding mesh data)
     \param pvalue_at_index0 raw bytes to use for the implicit attribute value that
                             guarantees that the vertex will be clipped.
   */
  WRATHAttributeStoreAllocator(const WRATHTripleBufferEnabler::handle &r,
                               const std::vector<opengl_trait_value> &pimplicit_attribute_format,
                               const std::vector<uint8_t> &pvalue_at_index0):
    WRATHTripleBufferEnabler::PhasedDeletedObject(r),
    m_implicit_attribute_format(pimplicit_attribute_format),
    m_value_at_index0(pvalue_at_index0),
    m_phase_deleted(false)
  {}

  
  /*!\fn WRATHAttributeStoreAllocator(const WRATHTripleBufferEnabler::handle&,
                                      type_tag<T>, const T&)
    Ctor. Template friendly version for ctor. The type T is used as the 
    type for the implicit attributes type. The type T must
    provide:
    - an enumeration T::number_attributes indicating how many attributes the type T uses
    - a function T::attribute_key(vecN<opengl_trait_value, T::number_attributes>&) which for each attribute of T, "computes" the opengl_train_value correctly.

    Note that \ref WRATHInterleavedAttributes provides these features.

    \param r handle to a WRATHTripleBufferEnabler to
             which the users of the created object will
             sync. It is an error if the handle is not valid.
    \param pvalue_at_index0 the value to use for implicit attribute at index 0  that
                            guarantees that the vertex will be clipped.
   */
  template<typename T>
  WRATHAttributeStoreAllocator(const WRATHTripleBufferEnabler::handle &r,
                               type_tag<T>, const T &pvalue_at_index0=T()):
    WRATHTripleBufferEnabler::PhasedDeletedObject(r),
    m_implicit_attribute_format(T::number_attributes),
    m_value_at_index0(sizeof(T)),
    m_phase_deleted(false)
  {
    vecN<opengl_trait_value, T::number_attributes> attr;
    T::attribute_key(attr);
    std::copy(attr.begin(), attr.end(), 
              m_implicit_attribute_format.begin());

    std::memcpy(&m_value_at_index0[0], &pvalue_at_index0, sizeof(T));
  }
  
  
  virtual
  ~WRATHAttributeStoreAllocator();

  /*!\fn WRATHAttributeStore::handle attribute_store(const WRATHAttributeStoreKey &,
                                                     int ,
                                                     range_type<int> &,
                                                     enum implicit_attribute_req_t)
    Fetch an attribute store selected by an 
    WRATHAttributeStoreKey. Additionally,
    as required, allocates attributes in a
    continuous block as required.
    This methods is thread safe, i.e. can be called 
    on the same object from multiple threads.
    \param k WRATHAttributeStoreKey that specifies the attribute
             type and index type.

    \param req_number_elements_continuous specifies the number of elements to 
                                          allocate from the WRATHAttributeStore
                                          in one continuous block. 
    \param R range_type<int> where the location of the allocation
             request is placed.
    \param req enumeration that determines whether the fetched attribute store
               should include implicit attributes or not.
   */
  WRATHAttributeStore::handle
  attribute_store(const WRATHAttributeStoreKey &k,
                  int req_number_elements_continuous,
                  range_type<int> &R,
                  enum implicit_attribute_req_t req=include_implicit_attribute);

  /*!\fn WRATHAttributeStore::handle attribute_store(const WRATHAttributeStoreKey &,
                                                     int ,
                                                     std::vector<range_type<int> > &,
                                                     enum implicit_attribute_req_t req)
    Fetch an attribute store selected by an 
    WRATHAttributeStoreKey. Additionally,
    as required, allocates attributes in a
    multiple blocks as required.
    This methods is thread safe, i.e. can be called 
    on the same object from multiple threads.
    \param k WRATHAttributeStoreKey that specifies the attribute
             type and index type.
    \param req_number_elements specifies the number of elements to
                               allocate from the WRATHAttributeStore
                               in multiple blocks. 
    \param R writes the locations of the fragments
             of the allocating as a range_type
             (i.e. marking the beginning and
             ending of the fragmented allocation).
             Note that the locations are offsets
             in units of the elements size (not in bytes).
             Does NOT append, i.e. it clears R before
             appending the location of the allocated
             attributes.
    \param req enumeration that determines whether the fetched attribute store
               should include implicit attributes or not.
   */
  WRATHAttributeStore::handle
  attribute_store(const WRATHAttributeStoreKey &k,
                  int req_number_elements,
                  std::vector<range_type<int> > &R,
                  enum implicit_attribute_req_t req=include_implicit_attribute);

  /*!\fn WRATHAttributeStore::handle attribute_store(const WRATHAttributeStoreKey &,
                                                     int, int,
                                                     enum implicit_attribute_req_t)
    Fetch an attribute store selected by an WRATHAttributeStoreKey. 
    This methods is thread safe, i.e. can be called 
    on the same object from multiple threads.
    However, in a multi-threaded environment it is possible
    that another thread might use the returned attribute
    store and allocate. That allocation might result
    in that there is not sufficient room to allocate.
    Roughly speaking, in a single threaded environment,
    the returned handle is gauranteed to able to allocate
    as requested upon return, but in a multithreaded environment
    if another thread allocates from the returned store
    then the allocation guarantee may fail.
    
    \param k WRATHAttributeStoreKey that specifies the attribute type and index type.
    \param req_number_elements specifies the minimum that the returned 
                               WRATHAttributeStore has room to allocate
                               attributes, but not necessarily in one chunk.
    \param req_number_elements_continuous specifies the minimum that the returned 
                                          WRATHAttributeStore has room to allocate
                                          attributes in one continuous block. 
    \param req enumeration that determines whether the fetched attribute store
               should include implicit attributes or not.
    */
  WRATHAttributeStore::handle
  attribute_store(const WRATHAttributeStoreKey &k,
                  int req_number_elements=0,
                  int req_number_elements_continuous=0,
                  enum implicit_attribute_req_t req=include_implicit_attribute);
                  
  /*!\fn bool same_implicit_attribute_type
    Checks whether the target WRATHAttributeStoreAllocator has the
    same implicit attribute type as this by checking the size of the values and
    the implicit attribute format.
    \param ptr The target WRATHAttributeStoreAllocator to compare with
    \returns true if the implicit attribute type is the same, false otherwise
   */
  bool
  same_implicit_attribute_type(const WRATHAttributeStoreAllocator *ptr) const;

private:
  friend class WRATHAttributeStore;

  typedef WRATHAttributeStoreKey map_key;
  
  /*
    Notice that the value type for the std::map is _pointers_.
    This is because a WRATHAttributeStoreAllocator does not really "use"
    the WRATHAttributeStore's. Rather it just needs to know if they
    are alive or not. The dtor for WRATHAttributeStore will
    remove itself from the map, m_attribute_stores.
   */
  typedef std::map<map_key, std::set<WRATHAttributeStore*> > map_type;

  void
  unregister(WRATHAttributeStore*);


  
  WRATHMutex m_mutex;
  map_type m_attribute_stores;
  std::vector<opengl_trait_value> m_implicit_attribute_format;
  std::vector<uint8_t> m_value_at_index0;
  bool m_phase_deleted;
};

#include "WRATHAttributeStoreImplement.tcc"

/*! @} */



#endif
