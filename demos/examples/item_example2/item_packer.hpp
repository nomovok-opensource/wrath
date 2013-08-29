/*! 
 * \file item_packer.hpp
 * \brief file item_packer.hpp
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


#ifndef __ITEM_PACKER_HPP__
#define __ITEM_PACKER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHCanvas.hpp"
#include "WRATHAttributePacker.hpp"

/*
  our generic attribute packing interface for
  our item class
 */
class ItemAttributePacker:public WRATHAttributePacker
{
public:
  /*
    class to specify how to pack data
   */
  class packer_data
  {
  public:
    //item is a polygon, gives how many sides to it.
    int m_number_sides;

    //center of polygon
    vec2 m_center;

    //radius of polygon
    float m_radius;
  };

  /*
    class to specify required number of indices 
    and attributes for the packing.
   */
  class allocation_needs_t
  {
  public:
    int m_number_indices;
    int m_number_attributes;
  };

  ItemAttributePacker(const ResourceKey &presource_name,
                      const std::vector<std::string> &attr_names):
    WRATHAttributePacker(presource_name, attr_names)
  {}

  /*
    Determine number of indices and attibutes needed
   */
  virtual
  allocation_needs_t
  allocation_needs(const packer_data &P) const=0;

  /*
    Set the attribute store key and return the
    primitive type 
   */
  virtual
  GLenum
  attribute_key(WRATHAttributeStoreKey &K) const=0;
  
  /*
    pack attributes:
    \param P how to pack
    \param attribute_destination where to pack attribute data
    \param attr_location location within attribute_destination to pack data
    \param index_destination location to pack indices
   */
  virtual
  void
  pack_attributes(const packer_data &P,
                  const std::vector<range_type<int> > &attr_location,
                  WRATHAbstractDataSink &attribute_store,
                  WRATHIndexGroupAllocator::index_group<GLushort> index_destination) const=0;

  static
  ItemAttributePacker*
  example_packer(void);
};


#endif
