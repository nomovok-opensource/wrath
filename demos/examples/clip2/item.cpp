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


#include "WRATHConfig.hpp"
#include "item.hpp"





///////////////////////////////////////////
// item methods
item::
item(const WRATHItemDrawerFactory &factory, int subdrawer_id,
     WRATHCanvas *pcanvas,
     const WRATHCanvas::SubKeyBase &subkey,
     const parameters &params)
{
  //the packer determines the store key and primitive type.
  GLenum primitive_type;
  WRATHAttributeStoreKey store_key;
  ItemAttributePacker::allocation_needs_t needs;

  primitive_type=params.m_drawer.m_packer->attribute_key(store_key);
  needs=params.m_drawer.m_packer->allocation_needs(params.m_polygon_spec);
  
  //create the GL state vector for multipass drawing.
  std::set<WRATHItemDrawState> multi_pass_draw_state;

  /***************************************************
    The function Drawer::set_item_draw_state_value sets the
    multi-pass draw state value from it's entires, it is 
    equivalent to the loop:
  
  
    for(unsigned int d=0, endd=params.m_drawer.m_draw_passes.size(); d<endd; ++d)
      {
        WRATHItemDrawState draw_state;
        const WRATHItemTypes::DrawerPass &pass(params.m_drawer.m_draw_passes[d]);
        
        draw_state
          .drawer(pass.m_shader->fetch_drawer(factory, params.m_drawer.m_packer, subdrawer_id))
          .primitive_type(primitive_type)
          .absorb(pass.m_draw_state)
          .force_draw_order(pass.m_force_draw_order)
          .draw_type(pass.m_draw_type)
          .buffer_object_hint(params.m_drawer.m_buffer_object_hint);
        
        multi_pass_draw_state.insert(draw_state);
      }
  */


  params.m_drawer.set_item_draw_state_value(multi_pass_draw_state,
                                            factory, subdrawer_id,
                                            primitive_type);


  

  //request a handle and at the same time allocate the needed attributes
  m_data_handle=pcanvas->create_and_allocate(store_key, //attribute format
                                             needs.m_number_attributes, //number attributes needed
                                             m_attribute_data_location, //location to write locations 
                                             multi_pass_draw_state, //GL state vector
                                             subkey); //subkey encoding node for item
  WRATHassert(m_data_handle.valid());

  //allocate the indices
  m_indices=m_data_handle.allocate_index_group<GLushort>(needs.m_number_indices);
  WRATHassert(m_indices.valid());

  //let the packer pack the data:
  params.m_drawer.m_packer->pack_attributes(params.m_polygon_spec,
                                            m_attribute_data_location,
                                            m_data_handle.attribute_store()->data_sink(),
                                            m_indices);

  

  
}


item::
~item(void)
{
  //free the indices we allocated
  m_indices.delete_group();

  //free the attribute we allocated
  m_data_handle.deallocate_attribute_datas(m_attribute_data_location.begin(),
                                           m_attribute_data_location.end());

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
