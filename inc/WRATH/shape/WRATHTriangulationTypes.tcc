//  -*- C++ -*-

/*! 
 * \file WRATHTriangulationTypes.tcc
 * \brief file WRATHTriangulationTypes.tcc
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


/*
  Should only be included from WRATHTriangulation.hpp
 */

#if !defined(WRATH_HEADER_TRIANGULATION_HPP_) || defined(WRATH_HEADER_TRIANGULATION_TYPE_TCC_)
#error "Direction inclusion of private header file WRATHTriangulationTypes.tcc"
#endif
#define WRATH_HEADER_TRIANGULATION_TYPE_TCC_


#include "WRATHConfig.hpp"
#include "uint128.h"

/////////////////////////////////////////
// template type madness for handling products
// essentially we need to handle that the product
// of two N-bit integers is a 2N-bit integer.
namespace WRATHTriangulationPrivate
{

//base type for non-integer types, like floats:
template<typename T>
class Product
{
public:
  typedef T product_type;
};

template<>
class Product<int8_t>
{
public:
  typedef int16_t product_type;
};

template<>
class Product<int16_t>
{
public:
  typedef int32_t product_type;
};

template<>
class Product<int32_t>
{
public:
  typedef int64_t product_type;
};

/*
 template hackery:

 The InCircle method needs to compute a 4-way product,
 i.e. a product of products. An N-bit integer then needs
 to have a 4N-bit integer. Icks. We get around this for
 32-bit integers as follows: we use WRATHUtil::uint128
 and store the positive and negative terms. All other
 data types don't need this hack.

 T= terms fed to add_product. Thus internally
 needs to store Product<T>::product_type
 value.
*/
template<typename T>
class SumOfProducts
{
public:
  
  SumOfProducts(void):
    m_value(0)
  {}
  
  void
  add_product(T v1, T v2)
  {
    m_value+= product_type(v1)*product_type(v2);
  }
  
  bool
  is_positive(void) const
  {
    return m_value>product_type(0);
  }

private:  
  typedef typename Product<T>::product_type product_type;
  product_type m_value;
};

template<>
class SumOfProducts<int64_t>
{
public:

  SumOfProducts(void):
    m_negative(0), m_positive(0)
  {}

  void
  add_product(int64_t a, int64_t b)
  {
    if((a<0) xor (b<0))
      {
        a=int64abs(a);
        b=int64abs(b);
        m_negative+= 
          WRATHUtil::uint128(static_cast<uint64_t>(a))
          * WRATHUtil::uint128(static_cast<uint64_t>(b));
      }
    else
      {
        a=int64abs(a);
        b=int64abs(b);
        m_positive+= 
          WRATHUtil::uint128(static_cast<uint64_t>(a))
          * WRATHUtil::uint128(static_cast<uint64_t>(b));
      }
  }
  
  bool
  is_positive(void) const
  {
    return m_positive>m_negative;
  }
  
private:
  
  WRATHUtil::uint128 m_negative, m_positive;

  int64_t
  int64abs(int64_t a)
  {
    return (a<0)?-a:a;
  }
};


template<typename T>
class DataType
{
public:
  typedef T type;
  typedef typename Product<T>::product_type product_type;
  typedef SumOfProducts<product_type> product_product_type;
};

class TriangulationException
{};

class ConnectedComponentException
{};

}
