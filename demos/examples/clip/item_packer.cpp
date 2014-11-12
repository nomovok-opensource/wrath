/*! 
 * \file item_packer.cpp
 * \brief file item_packer.cpp
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


#include "WRATHConfig.hpp"
#include <complex>
#include "WRATHInterleavedAttributes.hpp"
#include "WRATHAttributePackerHelper.hpp"
#include "item_packer.hpp"

/*
  .get<0> --> position on the unit circle
  .get<1> --> 0==inside ring, 1==outside ring
 */
typedef WRATHInterleavedAttributes<vec2, float> attribute_type;

// we need an attribute packer type to name the attribute names
namespace
{
  class ExamplePacker:public ItemAttributePacker
  {
  public:

    ExamplePacker(void):
      ItemAttributePacker(typeid(ExamplePacker).name(),
                          WRATHAttributePacker::attribute_names()
                          .name(0, "circle")
                          .name(1, "ring") )
    {}

    virtual
    allocation_needs_t
    allocation_needs(const packer_data &P) const;

    virtual
    void
    pack_attributes(const packer_data &P,
                    const std::vector<range_type<int> > &attr_location,
                    WRATHAbstractDataSink &attribute_store,
                    WRATHIndexGroupAllocator::index_group<GLushort> index_destination) const;

    virtual
    GLenum
    attribute_key(WRATHAttributeStoreKey &K) const;
  };

  class ExamplePackerFactory:public WRATHAttributePacker::AttributePackerFactory
  {
  public:
    WRATHAttributePacker*
    create(void) const
    {
      return WRATHNew ExamplePacker();
    }
  };
  
}

//////////////////////////////////////////
// ExamplePacker methods
ItemAttributePacker::allocation_needs_t
ExamplePacker::
allocation_needs(const packer_data &P) const
{
  allocation_needs_t R;
  
  // two polygons each of P.m_number_sides
  R.m_number_attributes=2*P.m_number_sides;
  
  //for each side a quad which is 2 triangles
  //so 6 indices for each side
  R.m_number_indices=6*P.m_number_sides;
  
  return R;    
}

GLenum
ExamplePacker::
attribute_key(WRATHAttributeStoreKey &K) const
{
  K.type_and_format(type_tag<attribute_type>());
  return GL_TRIANGLES;
}

void
ExamplePacker::
pack_attributes(const packer_data &P,
                const std::vector<range_type<int> > &attr_location,
                WRATHAbstractDataSink &attribute_store,
                WRATHIndexGroupAllocator::index_group<GLushort> index_destination) const
{
   /*
    We will use WRATHAttributePackerHelper to pack the attribute 
    data when it is fragmented; the class uses an iterator interface
    to write the attributes; for this SIMPLE example we will first
    write the attributes and indices to an std::vector and then
    use those on the WRATHAttributePackerHelper. The
    WRATHAttributePackerHelper takes indices relative to the
    data fed to it and writes indices to where it is located
    in the fragmented attribute store.
   */
  std::vector<attribute_type> attributes_in_one_chunk(2*P.m_number_sides);
  std::vector<GLushort> indices_in_one_chunk(6*P.m_number_sides);

  /*
    we are to tessellate the unit circle into
    N points (N=P.m_number_sides). Rather
    than calling sincos for each point we
    take advantage of complex arithmatic
    where e^(it)= cos(t) + i*sin(t)
    Let F = e^(it) with t=2*PI/N
    and then let z[0]=1.0 and z[i+1]=z[i]*F
   */
  float t, s, c;
  t=2.0f*M_PI/static_cast<float>(P.m_number_sides);
  sincosf(t, &s, &c);
  std::complex<float> F(c,s), z(1.0f, 0.0f);

  for(int i=0; i<P.m_number_sides; ++i, z*=F)
    {
      vec2 C(z.real(), z.imag());

      //inner ring
      attributes_in_one_chunk[i + 0].get<0>()=C;
      attributes_in_one_chunk[i + 0].get<1>()=0.0f;

      //outer ring
      attributes_in_one_chunk[i + P.m_number_sides].get<0>()=C;
      attributes_in_one_chunk[i + P.m_number_sides].get<1>()=1.0f;
    }

  for(int i=0; i<P.m_number_sides; ++i)
    {
      GLushort prev;

      prev=(i!=0)?
        i-1:
        P.m_number_sides-1;

      indices_in_one_chunk[6*i + 0]=prev;
      indices_in_one_chunk[6*i + 1]=i;
      indices_in_one_chunk[6*i + 2]=i+P.m_number_sides;

      indices_in_one_chunk[6*i + 3]=prev;
      indices_in_one_chunk[6*i + 4]=i+P.m_number_sides;
      indices_in_one_chunk[6*i + 5]=prev+P.m_number_sides;
    }

  /*
    now finally write the data.
   */
  {
    //WRATHAttributePackerHelper does NOT lock,
    //so we need to lock
    WRATHAutoLockMutex(attribute_store.mutex());
    WRATHAutoLockMutex(index_destination.mutex());

    //set location to which to write attributes
    WRATHAttributePackerHelper<attribute_type, GLushort> writer(attribute_store, 
                                                                attr_location.begin(), 
                                                                attr_location.end());

    //write attributes to attribute_store
    writer.add_attribute_data(attributes_in_one_chunk.size(),
                              attributes_in_one_chunk.begin(),
                              attributes_in_one_chunk.end());

    //write indices, the class WRATHAttributePackerHelper
    //performs the remapping from indices relative to the
    //input data given to add_attribute_data to the indices
    //as specified where the attribute data is allocated in
    //attr_location
    c_array<GLushort> indices_write_ptr;
    indices_write_ptr=index_destination.pointer();

    WRATHDefaultIndexWriter<GLushort> index_writer(indices_write_ptr);
    writer.add_indices(indices_in_one_chunk.begin(), indices_in_one_chunk.end(),
                       index_writer);

  }

  /*
    exercise:
     create a iterator like-types that compute
     the index and attribute values when they
     are dereferenced rather than what the example has:
     first store values in a arrays and use
     those arrays.
   */
    
}


//////////////////////////////////////////////////////
// ItemAttributePacker methods
ItemAttributePacker*
ItemAttributePacker::
example_packer(void)
{
  return WRATHAttributePacker::fetch_make<ExamplePacker>(ExamplePackerFactory());
}
