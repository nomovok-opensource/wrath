// -*- C++ -*-

/*! 
 * \file WRATHInterleavedAttributesPrivate.tcc
 * \brief file WRATHInterleavedAttributesPrivate.tcc
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


#if  !defined(WRATH_HEADER_INTERLEAVED_ATTRIBUTES_HPP_) || defined(WRATH_HEADER_INTERLEAVED_ATTRIBUTES_PRIVATE_TCC_)
#error "Direction inclusion of private header file WRATHInterleavedAttributesPrivate.tcc"
#endif


#define WRATH_HEADER_INTERLEAVED_ATTRIBUTES_PRIVATE_HPP_

#include "WRATHConfig.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/utility.hpp>

namespace InterleavedAttributes_Helper_WRATH
{
  template<size_t N, typename TupleType>
  static
  void
  extract(opengl_trait_value &output, const TupleType &ps)
  {
    //get the offset of the named attribute:
    const char *ps_ptr, *n_ptr;

    ps_ptr=reinterpret_cast<const char*>(boost::addressof(ps));
    n_ptr=reinterpret_cast<const char*>(boost::addressof(boost::get<N>(ps)));

    ptrdiff_t offset( n_ptr-ps_ptr);

    output.m_offset=offset;
    output.traits( get_type_tag(boost::get<N>(ps)), 
                   get_type_tag(ps));
  }

  template<size_t sz, typename TupleType, unsigned int OutputSize>
  struct attribute_key_helper
  {
    static
    void
    extract_values(vecN<opengl_trait_value, OutputSize> &output,
                   const TupleType &ps)
    {
      //first handle before element
      attribute_key_helper<sz-1, TupleType, OutputSize>::extract_values(output, ps);

      //extract the type information of sz 'th element:
      if(sz<=OutputSize)
        {
          extract<sz-1,TupleType>(output[sz-1], ps);
        }
    }
  };

  //make the 0-case now:
  template<typename TupleType, unsigned int OutputSize>
  struct attribute_key_helper<0, TupleType, OutputSize>
  {
    static
    void
    extract_values(vecN<opengl_trait_value, OutputSize>&, const TupleType&)
    {}
  };

  template<typename TupleType, unsigned int  OutputSize>
  struct attribute_key
  {
    static
    void
    extract_values(vecN<opengl_trait_value, OutputSize> &output, const TupleType &ps)
    {
      attribute_key_helper<
        boost::tuples::length<TupleType>::value, 
          TupleType,
          OutputSize
          >::extract_values(output, ps);
    }
  };
}

