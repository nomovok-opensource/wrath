/*! 
 * \file WRATHRectItem.cpp
 * \brief file WRATHRectItem.cpp
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
#include "WRATHRectItem.hpp"
#include "WRATHInterleavedAttributes.hpp"
#include "WRATHDefaultRectShader.hpp"

///////////////////////////////////////////////////
//WRATHRectItemTypes::Drawer methods
WRATHRectItemTypes::Drawer::
Drawer(const WRATHBrush &brush, WRATHDrawType ppass,
       enum WRATHBaseSource::precision_t v):
  base_class(&WRATHDefaultRectShader::shader_hoard().fetch(brush, v),
             WRATHDefaultRectAttributePacker::fetch(),
             ppass)
{
  WRATHDefaultRectShader::shader_hoard().add_state(brush, m_draw_passes[0].m_draw_state);
}

////////////////////////////////////////
//WRATHRectItem methods
WRATHRectItem::
WRATHRectItem(const WRATHItemDrawerFactory &factory, int subdrawer_id,
              WRATHCanvas *canvas,
              const WRATHCanvas::SubKeyBase &subkey,
              const Drawer &drawer)
{
  WRATHAttributeStoreKey attr_key;
  std::set<WRATHItemDrawState> skey;
  WRATHAttributeStore::handle attr_handle;

  m_packer=drawer.m_packer;
  m_immutable_packing_data=drawer.m_immutable_packing_data;

  WRATHassert(m_packer!=NULL);
  WRATHassert(canvas!=NULL);


  attr_key.buffer_object_hint(drawer.m_buffer_object_hint);
  m_packer->attribute_key(attr_key);
  attr_handle=canvas->attribute_store(attr_key, 4, m_attribute_data_location);
   
  for(unsigned int d=0, endd=drawer.m_draw_passes.size(); d<endd; ++d)
    {
      WRATHItemDrawState pkey;
      const WRATHRectItemTypes::DrawerPass &state(drawer.m_draw_passes[d]);
      
      pkey
        .primitive_type(GL_TRIANGLES)
        .drawer(state.m_shader->fetch_drawer(factory, drawer.m_packer, subdrawer_id))
        .absorb(state.m_draw_state)
        .force_draw_order(state.m_force_draw_order)
        .buffer_object_hint(drawer.m_buffer_object_hint)
        .draw_type(state.m_draw_type);

      skey.insert(pkey);
    }

  //get/create the draw group from which we allocate
  //index and texture data:
  m_item_group=canvas->create(attr_handle, skey, subkey);

  //set implicit data:
  m_item_group.set_implicit_attribute_data(m_attribute_data_location);

  //allocate and set indices:
  m_index_data_location=m_item_group.allocate_index_group<GLushort>(6);
  c_array<GLushort> ptr;

  
  WRATHLockMutex(m_index_data_location.mutex());

  ptr=m_index_data_location.pointer();
  ptr[0]=0+m_attribute_data_location.m_begin;
  ptr[1]=1+m_attribute_data_location.m_begin;
  ptr[2]=2+m_attribute_data_location.m_begin;
  ptr[3]=0+m_attribute_data_location.m_begin;
  ptr[4]=2+m_attribute_data_location.m_begin;
  ptr[5]=3+m_attribute_data_location.m_begin;

  WRATHUnlockMutex(m_index_data_location.mutex());

  
}


WRATHRectItem::
~WRATHRectItem()
{
  m_index_data_location.delete_group();
  m_item_group.deallocate_attribute_data(m_attribute_data_location);
  m_item_group.release_group();
}

void
WRATHRectItem::
set_parameters(const WRATHReferenceCountedObject::handle &rect)
{
  m_packer->set_attribute_data(m_item_group, 
                               m_attribute_data_location.m_begin,
                               rect, m_immutable_packing_data);
}

void 
WRATHRectItem::
canvas_base(WRATHCanvas *c)
{
  enum return_code R;

  R=c->transfer(m_item_group,
                m_attribute_data_location,
                m_index_data_location);

  WRATHassert(R==routine_success);
}
