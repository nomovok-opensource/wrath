//  -*- C++ -*-

/*! 
 * \file WRATHAttributeStoreImplement.tcc
 * \brief file WRATHAttributeStoreImplement.tcc
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


#ifndef __WRATH_ATTRIBUTE_STORE_IMPLEMENT_TCC__
#define __WRATH_ATTRIBUTE_STORE_IMPLEMENT_TCC__

namespace WRATHAttributeStoreKeyImplement
{
  /**/
  template<typename I>
  class index_bit_count_from_typeT
  {};

  template<>
  class index_bit_count_from_typeT<GLubyte>
  {
  public:
    static
    enum WRATHAttributeStoreKey::index_bit_count_type
    f(void) { return WRATHAttributeStoreKey::index_8bits; }
  };

  template<>
  class index_bit_count_from_typeT<GLushort>
  {
  public:
    static
    enum WRATHAttributeStoreKey::index_bit_count_type
    f(void) { return WRATHAttributeStoreKey::index_16bits; }
  };

  template<>
  class index_bit_count_from_typeT<GLuint>
  {
  public:
    static
    enum WRATHAttributeStoreKey::index_bit_count_type
    f(void) { return WRATHAttributeStoreKey::index_32bits; }
  };
  /**/

  
}

template<typename I>
enum WRATHAttributeStoreKey::index_bit_count_type
WRATHAttributeStoreKey::
index_bit_count_from_type(void)
{
  return WRATHAttributeStoreKeyImplement::index_bit_count_from_typeT<I>::f();
}

#endif
