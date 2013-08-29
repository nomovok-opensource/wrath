/*! 
 * \file item.cpp
 * \brief file item.cpp
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


#include "WRATHInterleavedAttributes.hpp"
#include "item.hpp"


// our attribute type is just a position on the place
typedef WRATHInterleavedAttributes<vec2> attribute_type;

// we need an attribute packer type to name the attribute names
class Packer:public WRATHAttributePacker
{
public:
  static
  Packer*
  fetch(void)
  {
    return WRATHAttributePacker::fetch_make<Packer>(PackerFactory());
  }

private:
  class PackerFactory:public WRATHAttributePacker::AttributePackerFactory
  {
  public:
    WRATHAttributePacker*
    create(void) const
    {
      return WRATHNew Packer();
    }

    static
    const std::vector<std::string>&
    attribute_names(void)
    {
      static std::vector<std::string> R(1, "pos");
      return R;
    }
  };

  Packer(void):
    WRATHAttributePacker(typeid(Packer).name(), 
                         PackerFactory::attribute_names().begin(),
                         PackerFactory::attribute_names().end())
  {}
};

item::
item(const WRATHItemDrawerFactory &factory, int subdrawer_id,
     WRATHCanvas *pcanvas,
     const WRATHCanvas::SubKeyBase &subkey,
     const parameters &params)
{
  //get our attribute store key:
  WRATHAttributeStoreKey store_key;
  store_key.type_and_format(type_tag<attribute_type>());

  //a triangle fan, so we need params.m_number_sides + 1 vertices
  int number_attributes_needed=params.m_number_sides + 1;

  //create/get the WRATHCanvas::DataHandle
  WRATHItemDrawState draw_state;

  /*
    conveniance function, equivalent to   
    draw_state
      .drawer(params.m_drawer.m_shader->fetch_drawer(factory, Packer::fetch(), subdrawer_id))
      .primitive_type(GL_TRIANGLES)
      .absorb(params.m_drawer.m_draw_state)
      .force_draw_order(params.m_drawer.m_force_draw_order)
      .draw_type(params.m_drawer.m_draw_type);  
  */
  params.m_drawer.set_item_draw_state_value(draw_state,
                                            factory, subdrawer_id,
                                            GL_TRIANGLES, Packer::fetch());
                                            


  //request a handle and at the same time allocate the needed attributes
  //we are going to request that the attributes are allocated in one
  //continuos block. A more profesional application will allow
  //the attributes to be allocated fragmented, but that makes the
  //filling of attribute data harder and we are just doing an example
  //here.
  m_data_handle=pcanvas->create_and_allocate(store_key, 
                                             number_attributes_needed,
                                             m_attribute_data_location,
                                             draw_state, 
                                             subkey);
  WRATHassert(m_data_handle.valid());

  //allocate the indices, we are to params.m_number_sides
  //triangles, so we need 3*params.m_number_sides indices 
  m_indices=m_data_handle.allocate_index_group<GLushort>(3*params.m_number_sides);
  WRATHassert(m_indices.valid());

  //now set the vertices.
  {
    WRATHAutoLockMutex(m_data_handle.attribute_mutex());
    c_array<attribute_type> ptr;
    ptr=m_data_handle.pointer<attribute_type>(m_attribute_data_location);

    ptr[params.m_number_sides].get<0>()=params.m_center;
    for(int i=0; i<params.m_number_sides; ++i)
      {
        float t, c, s;

        t=static_cast<float>(i)/static_cast<float>(params.m_number_sides);
        sincosf( 2.0f*M_PI*t, &s, &c);
        ptr[i].get<0>() = params.m_center + params.m_radius*vec2(c,s);
      }
  }

  //now set the indices
  {
    WRATHAutoLockMutex(m_indices.mutex());
    c_array<GLushort> ptr;

    ptr=m_indices.pointer();
    for(int i=0; i<params.m_number_sides; ++i)
      {
        ptr[3*i]=params.m_number_sides; //center vertex
        ptr[3*i+1]=i;
        if(i!=0)
          {
            ptr[3*i+2]=i-1;
          }
        else
          {
            ptr[3*i+2]=params.m_number_sides-1;
          }
      }

    //the values written to in ptr need to be offset
    //by m_attribute_data_location.m_begin
    //because that is where the attributes are located.
    for(c_array<GLushort>::iterator iter=ptr.begin(),
          end=ptr.end(); iter!=end; ++iter)
      {
        *iter += m_attribute_data_location.m_begin;
      }
  }

  
}


item::
~item(void)
{
  //free the indices we allocated
  m_indices.delete_group();

  //free the attribute we allocated
  m_data_handle.deallocate_attribute_data(m_attribute_data_location);

  //release the drawhandle
  m_data_handle.release_group();
}

WRATHCanvas*
item::
canvas_base(void) const
{
  return m_data_handle.parent();
}

void
item::
canvas_base(WRATHCanvas *c)
{
  enum return_code R;
  R=c->transfer(m_data_handle, 
                m_attribute_data_location,
                m_indices);

  WRATHassert(R==routine_success);
  WRATHunused(R);
}
