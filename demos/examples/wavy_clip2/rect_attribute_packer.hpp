/*! 
 * \file rect_attribute_packer.hpp
 * \brief file rect_attribute_packer.hpp
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
#include "WRATHRectAttributePacker.hpp"

class ExampleRectAttributePacker:public WRATHRectAttributePacker
{
public:

  /*
    Our ExampleRectAttributePacker is to be a singleton,
    thus we can use the singleton machinery provided
    by \ref WRATHAttributePacker::fetch_make to create it.
   */
  static
  ExampleRectAttributePacker*
  fetch(void)
  {
    return WRATHAttributePacker::fetch_make<ExampleRectAttributePacker>(Factory());
  }

  /*
    Our attribute packer class has that all the attributes
    it packs are the SAME for any rectangle it packs,
    thus it's rect properties are "empty", so
    it returns an invalid handle.
   */
  static
  WRATHReferenceCountedObject::handle
  rect_properties(void)
  {
    return WRATHReferenceCountedObject::handle();
  }


  /*
    pure-virtual function from WRATHRectAttributePacker
    used to specify the attribute type and format
    used by the attribute packer
   */
  virtual
  void
  attribute_key(WRATHAttributeStoreKey &attrib_key) const;

protected:
  
  /*
    pure-virtual function from WRATHRectAttributePacker
    used to perform the actual attribute packing
   */
  virtual
  void
  set_attribute_data_implement(WRATHAbstractDataSink &sink,
                               int attr_location,
                               const WRATHReferenceCountedObject::handle &rect,
                               const WRATHStateBasedPackingData::handle &h) const;

private:

  class Factory:public WRATHAttributePacker::AttributePackerFactory
  {
  public:
    virtual
    WRATHAttributePacker*
    create(void) const 
    {
      return WRATHNew ExampleRectAttributePacker();
    }
  };

  ExampleRectAttributePacker(void);

};
