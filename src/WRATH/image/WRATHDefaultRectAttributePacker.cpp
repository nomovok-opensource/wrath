/*! 
 * \file WRATHDefaultRectAttributePacker.cpp
 * \brief file WRATHDefaultRectAttributePacker.cpp
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
#include <cstring>
#include "WRATHCanvas.hpp"
#include "WRATHDefaultRectAttributePacker.hpp"
#include "WRATHInterleavedAttributes.hpp"
#include "WRATHStaticInit.hpp"


namespace
{
  typedef vecN<GLubyte,2> normalized_coord_type;
  

  class attribute_type:
    public WRATHInterleavedAttributes<vec3, vec4, normalized_coord_type>
  {
  public:
    vec3&
    size_and_z(void)
    {
      return get<WRATHDefaultRectAttributePacker::size_and_z_location>();
    }

    vec4&
    brush_values(void)
    {
      return get<WRATHDefaultRectAttributePacker::brush_position_stretch_location>();
    }

    normalized_coord_type&
    normalized_coord(void)
    {
      return get<WRATHDefaultRectAttributePacker::normalized_location>();
    }
  };
  
  typedef const char *attribute_label_type;
  
  const_c_array<const char*>
  attribute_name_list(void)
  {
    static const attribute_label_type attribute_labels[]=
      {
        "size_and_z",
        "brush",
        "normalized_coordinate",
      };
    return const_c_array<attribute_label_type>(attribute_labels, 3);
  }
  
}


//////////////////////////////////////////
// WRATHDefaultRectAttributePacker methods
WRATHDefaultRectAttributePacker::
WRATHDefaultRectAttributePacker(void):
  WRATHRectAttributePacker(typeid(WRATHDefaultRectAttributePacker).name(),
                           attribute_name_list().begin(),
                           attribute_name_list().end())
{}

void
WRATHDefaultRectAttributePacker::
attribute_key(WRATHAttributeStoreKey &attrib_key) const 
{
  attrib_key.type_and_format(type_tag<attribute_type>());
  attrib_key.m_attribute_format_location[normalized_location].m_normalized=GL_TRUE;
}

void
WRATHDefaultRectAttributePacker::
set_attribute_data_implement(WRATHAbstractDataSink &sink, int attr_location,
                             const WRATHReferenceCountedObject::handle &prect,
                             const WRATHStateBasedPackingData::handle &) const
{
  const normalized_coord_type vs[]=
    {
      normalized_coord_type(0,0),
      normalized_coord_type(0,255),
      normalized_coord_type(255,255),
      normalized_coord_type(255,0)
    };   

  c_array<attribute_type> attrs;
  Rect::handle rect;
  vec3 value(0.0f, 0.0f, -1.0f);
  vec4 brush_value(0.0f, 0.0f, 1.0f, 1.0f);

  rect=prect.dynamic_cast_handle<Rect>();

  if(rect.valid())
    {
      value=vec3(rect->m_width_height.x(),
                 rect->m_width_height.y(),
                 rect->m_z);
      brush_value=vec4(rect->m_brush_offset.x(),
                       rect->m_brush_offset.y(),
                       rect->m_brush_stretch.x(),
                       rect->m_brush_stretch.y());
    }
  else
    {
      WRATHwarning("Invalid rect type passed to WRATHDefaultRectAttributePacker");
    }

  WRATHassert(&sink!=NULL);
  WRATHAutoLockMutex(sink.mutex());    
  attrs=sink.pointer<attribute_type>(range_type<int>(attr_location, attr_location+4));
  for(int i=0;i<4;++i)
    {
      attrs[i].size_and_z()=value;
      attrs[i].normalized_coord()=vs[i];
      attrs[i].brush_values()=brush_value; 
    }
}
