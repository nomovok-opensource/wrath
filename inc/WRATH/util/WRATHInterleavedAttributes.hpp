/*! 
 * \file WRATHInterleavedAttributes.hpp
 * \brief file WRATHInterleavedAttributes.hpp
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




#ifndef WRATH_HEADER_INTERLEAVED_ATTRIBUTES_HPP_
#define WRATH_HEADER_INTERLEAVED_ATTRIBUTES_HPP_

#include "WRATHConfig.hpp"
#include "opengl_trait.hpp"
#include "WRATHInterleavedAttributesPrivate.tcc"






/*! \addtogroup GLUtility
 * @{
 */

/*!\class WRATHInterleavedAttributes
  A WRATHInterleavedAttributes is a boost::tuple
  with the added member function attribute_key()
  which returns the arguments needed for 
  glVertexAttribPointer. The uses case is for having
  interleaved arrays of data to fed to GL.
  For example the type:\n\n

  WRATHInterleavedAttributes<vec2, vec3> \n\n

  will create a type where the first sizeof(vec2) bytes
  are for a vec2 fetched via get<0> and the next
  set of bytes are for the vec3 fetched via get<1>.
  The size of a WRATHInterleavedAttributes is
  the same a class consisting of the same types.
  The main point for using a WRATHInterleavedAttributes is
  that it provides the static method attribute_key()
  which returns the packing and location of each
  "member" of the type.
 */
template<
  typename T1 = boost::tuples::null_type,
  typename T2 = boost::tuples::null_type,
  typename T3 = boost::tuples::null_type,
  typename T4 = boost::tuples::null_type,
  typename T5 = boost::tuples::null_type,
  typename T6 = boost::tuples::null_type,
  typename T7 = boost::tuples::null_type,
  typename T8 = boost::tuples::null_type,
  typename T9 = boost::tuples::null_type,
  typename T10 = boost::tuples::null_type>
class WRATHInterleavedAttributes:public boost::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>
{
public:
  /*!\typedef base_class
    conveniance typedef to underlying base class
   */ 
  typedef boost::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> base_class;

  /*!\fn WRATHInterleavedAttributes(void)
    Empty ctor.
   */ 
  WRATHInterleavedAttributes(void)
  {}
 
  /*!\fn WRATHInterleavedAttributes(const base_class &)
    copy ctor
    \param v value from which to copy
   */
  WRATHInterleavedAttributes(const base_class &v):
    base_class(v)
  {}

  enum
    {
      /*!
        Indicates the number of fields (i.e. attributes)
        the type has.
       */
      number_attributes=boost::tuples::length<base_class>::value
    };

  /*!\fn vecN<opengl_trait_value, number_attributes> attribute_key(void)
    Returns a fixed length array of length the 
    number of elements of this WRATHInterleavedAttributes.
    The normalization flags of each element are set as false.
    For each i, the i'th element of the array give the 
    glVertexAttribPointer arguments (see \ref opengl_trait_value) 
    for the value assoicated assoicated to get<i>(). Not only
    are opengl_trait_value::m_stride, opengl_trait_value::m_count
    and opengl_trait_value::m_type set correctly, also 
    opengl_trait_value::m_offset is set to the location within
    the WRATHInterleavedAttributes.
   */
  static
  vecN<opengl_trait_value, number_attributes>
  attribute_key(void)
  {
    vecN<opengl_trait_value, number_attributes> return_value;

    attribute_key(return_value);
    return return_value;
  }

  /*!\fn vecN<opengl_trait_value, number_attributes> attribute_key(const vecN<GLboolean, number_attributes>&)
    Returns a fixed length array of length the 
    number of elements of this WRATHInterleavedAttributes.
    The normalization flags to use are specified by the
    function call. For each i, the i'th element of the array give the 
    glVertexAttribPointer arguments (see opengl_trait_value) 
    for the value assoicated assoicated to get<i>(). Not only
    are opengl_trait_value::m_stride, opengl_trait_value::m_count
    and opengl_trait_value::m_type set correctly, also 
    opengl_trait_value::m_offset is set to the location within
    the WRATHInterleavedAttributes.
    \param normalizeds fixed length array specifying the normalization
                       flag values for \ref opengl_trait_value::m_normalized.                       
   */
  static
  vecN<opengl_trait_value, number_attributes>
  attribute_key(const vecN<GLboolean, number_attributes> &normalizeds)
  {
    vecN<opengl_trait_value, number_attributes> return_value;

    attribute_key(return_value, normalizeds);
    return return_value;
  }  

  /*!\fn int attribute_key(vecN<opengl_trait_value, N>&)
    Sets a fixed length array of opengl_trait_value 's.
    The normalization flags of each element are set as false.
    For each i, the i'th element of the array give the 
    glVertexAttribPointer arguments (see opengl_trait_value) 
    for the value assoicated assoicated to get<i>(). Not only
    are opengl_trait_value::m_stride, opengl_trait_value::m_count
    and opengl_trait_value::m_type set correctly, also 
    opengl_trait_value::m_offset is set to the location within
    the structure. It is NOT an error for N to not equal
    the length of this, however entries beyond are not set.
    Return value is the length of this.
    \param return_value fixed length array to modify
   */
  template<unsigned int N>
  static
  int
  attribute_key(vecN<opengl_trait_value, N> &return_value) 
  {
    WRATHInterleavedAttributes R;
    R.attribute_key_implement(return_value);

    return number_attributes;
  }  

  /*!\fn int attribute_key(vecN<opengl_trait_value, N>&, const vecN<GLboolean, N>&)
    Sets a fixed length array of opengl_trait_value 's.
    The normalization flags to use are specified by the
    function call. For each i, the i'th element of the array give the 
    glVertexAttribPointer arguments (see opengl_trait_value) 
    for the value assoicated assoicated to get<i>(). Not only
    are opengl_trait_value::m_stride, opengl_trait_value::m_count
    and opengl_trait_value::m_type set correctly, also 
    opengl_trait_value::m_offset is set to the location within
    the structure. It is NOT an error for N to not equal
    the length of this, however entries beyond are not set.
    Return value is the length of this.
    \param return_value fixed length array to modify
    \param normalizeds fixed length array specifying the normalization
                       flag values for opengl_trait_value::m_normalized.      
   */
  template<unsigned int N>
  static
  int
  attribute_key(vecN<opengl_trait_value, N> &return_value,
                const vecN<GLboolean, N> &normalizeds) 
  {
    WRATHInterleavedAttributes R;
    R.attribute_key_implement(return_value);

    for(unsigned int I=0; I!=N;++I)
      {
        return_value[I].m_normalized=normalizeds[I];
      }

    return number_attributes;
  }

private:

  //template meta programming: the art of obsuficating
  //iteration into tail/begin recursion to make the bloody
  //C++ compiler happy.

  //this is gross and POD vs class purists will
  //have a freak out on this, but attribute data
  //is passed directly bit for bit to GL anyways.

  //TODO: should be something to get the "type"
  //and feed that to a type tag.. ick.
  
  template<unsigned int N>
  void
  attribute_key_implement(vecN<opengl_trait_value, N> &return_value) const
  {
    
    InterleavedAttributes_Helper_WRATH::
      attribute_key<base_class, N>::extract_values(return_value,
                                                   *this);

  }

};


/*! @} */

#endif
