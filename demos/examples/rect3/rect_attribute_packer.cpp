/*! 
 * \file rect_attribute_packer.cpp
 * \brief file rect_attribute_packer.cpp
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

#include "rect_attribute_packer.hpp"

/*
  very simple attribute type, just the normalized
  coordinates of the "rectangle"
 */
typedef WRATHInterleavedAttributes< vecN<GLubyte, 2> > attribute_type;


ExampleRectAttributePacker::
ExampleRectAttributePacker(void):
  WRATHRectAttributePacker(typeid(ExampleRectAttributePacker).name(), //for singleton mahinery to work must pass the typeid()
                                                                      //as the name of the attribute packer
                           attribute_names()
                           .name(0, "normalized_coordinate")) //just one attribute, with name "normalized_coordinate"
{}

void
ExampleRectAttributePacker::
attribute_key(WRATHAttributeStoreKey &attrib_key) const
{
  //get the argument values from the type
  attrib_key.type_and_format(type_tag<attribute_type>());
}


void
ExampleRectAttributePacker::
set_attribute_data_implement(WRATHAbstractDataSink &sink,
                             int attr_location,
                             const WRATHReferenceCountedObject::handle&,
                             const WRATHStateBasedPackingData::handle&) const
{
  /*
    our simple example does not need any parameters
    to produce the attribute data.
   */

  WRATHassert(&sink!=NULL);

  //auto-lock the sink
  WRATHAutoLockMutex(sink.mutex());    

  //rect attribute packers always pack 4 attributes
  //and are to pack the values for the corners in the order
  // [0] --> minx_miny
  // [1] --> minx_maxy
  // [2] --> maxx_maxy
  // [3] --> maxx_miny
  //
  c_array<attribute_type> attrs;
  attrs=sink.pointer<attribute_type>(range_type<int>(attr_location, attr_location+4));
  attrs[0].get<0>()=vecN<GLubyte, 2>(0, 0);
  attrs[1].get<0>()=vecN<GLubyte, 2>(0, 1);
  attrs[2].get<0>()=vecN<GLubyte, 2>(1, 1);
  attrs[3].get<0>()=vecN<GLubyte, 2>(1, 0);
}
