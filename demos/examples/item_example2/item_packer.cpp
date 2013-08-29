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
#include "WRATHInterleavedAttributes.hpp"
#include "WRATHAttributePackerHelper.hpp"
#include "item_packer.hpp"

// our attribute type is just a position on the place
typedef WRATHInterleavedAttributes<vec2> attribute_type;

// we need an attribute packer type to name the attribute names
namespace
{
  class ExamplePacker:public ItemAttributePacker
  {
  public:

    ExamplePacker(void):
      ItemAttributePacker(typeid(ExamplePacker).name(),
                          WRATHAttributePacker::attribute_names()
                          .name(0, "pos"))
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
  
  // one point for the center and one for each size
  R.m_number_attributes=1+P.m_number_sides;
  
  //to draw a triangle fan
  //to draw the polygone manes we need 
  //P.m_number_sides triangles, which is
  //then 3*P.m_number_sides number indices
  R.m_number_indices=3*P.m_number_sides;
  
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
  std::vector<attribute_type> attributes_in_one_chunk(1+P.m_number_sides);
  std::vector<GLushort> indices_in_one_chunk(3*P.m_number_sides);

  attributes_in_one_chunk[P.m_number_sides].get<0>()=P.m_center;
  for(int i=0; i<P.m_number_sides; ++i)
    {
      float t, c, s;
      t=static_cast<float>(i)/static_cast<float>(P.m_number_sides);
      sincosf( 2.0f*M_PI*t, &s, &c);
      attributes_in_one_chunk[i].get<0>()=P.m_center + P.m_radius*vec2(c,s);
    }

  for(int i=0; i<P.m_number_sides; ++i)
    {
      indices_in_one_chunk[3*i]=P.m_number_sides; //center vertex
      indices_in_one_chunk[3*i+1]=i;
      if(i!=0)
        {
          indices_in_one_chunk[3*i+2]=i-1;
        }
      else
        {
          indices_in_one_chunk[3*i+2]=P.m_number_sides-1;
        }
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
