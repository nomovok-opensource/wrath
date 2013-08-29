/*! 
 * \file WRATHBasicTextItem.cpp
 * \brief file WRATHBasicTextItem.cpp
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
#include <limits>
#include "WRATHBasicTextItem.hpp"
#include "WRATHTextAttributePacker.hpp"



////////////////////////////////////////
// WRATHBasicTextItem::draw_method methods
bool
WRATHBasicTextItem::draw_method::
operator<(const draw_method &rhs) const
{
  if(m_program_drawer!=rhs.m_program_drawer)
    {
      return m_program_drawer<rhs.m_program_drawer;
    }
  
  if(m_attribute_packer!=rhs.m_attribute_packer) 
    {
      return m_attribute_packer<rhs.m_attribute_packer;
    }
  return false;
}

//////////////////////////////////
// WRATHBasicTextItem::type_page_type methods
WRATHBasicTextItem::per_page_type::
per_page_type(int page, WRATHBasicTextItem *parent):
  m_parent(parent),
  m_texture_page(page)
{
  m_parent->generate_key(m_key, m_attribute_key, page);
}

WRATHBasicTextItem::per_page_type::
~per_page_type()
{
  if(m_index_data_location.valid())
    {
      m_index_data_location.delete_group();
    }

  if(m_item_group.valid())
    {
      
      m_item_group.deallocate_attribute_datas(m_attribute_location.begin(), 
                                              m_attribute_location.end());
      m_item_group.release_group();
    }
}

void
WRATHBasicTextItem::per_page_type::
clear(void)
{
  if(m_index_data_location.valid())
    {
      //set all indices to 0:
      WRATHAutoLockMutex(m_index_data_location.mutex());
      c_array<GLushort> ptr(m_index_data_location.pointer());
      std::fill(ptr.begin(), ptr.end(), 0);
    }
  m_required=WRATHTextAttributePacker::allocation_requirement_type();
}

void
WRATHBasicTextItem::per_page_type::
change_attribute_store(void)
{

  WRATHassert(m_attribute_location.empty());


  m_attribute_store=
    m_parent->m_group_collection->attribute_store(m_attribute_key, 
                                                  m_required.m_number_attributes, 
                                                  m_attribute_location);
  
  m_allocated.m_number_attributes=m_required.m_number_attributes;
}


void
WRATHBasicTextItem::per_page_type::
allocate_room_if_needed(void)
{ 
  /*
    First add more attribute room if needed
   */
  if(m_required.m_number_attributes>m_allocated.m_number_attributes)
    {
      int more_needed;
      WRATHCanvas::DataHandle new_group;

      more_needed=m_required.m_number_attributes - m_allocated.m_number_attributes;

      if(!m_item_group.valid() or
         m_item_group.fragmented_allocate_attribute_data(more_needed, m_attribute_location)==routine_fail)
        {
          if(m_item_group.valid())
            {
              m_item_group.deallocate_attribute_datas(m_attribute_location.begin(),
                                                      m_attribute_location.end());
              m_attribute_location.clear();
            }

          change_attribute_store(); 

          new_group=
            m_parent->m_group_collection->create(m_attribute_store,
                                                       m_key, 
                                                       *m_parent->m_subkey);

          new_group.set_implicit_attribute_data(m_attribute_location);
        }
      else
        {
          new_group=m_item_group;
        }
      
      m_allocated.m_number_attributes=m_required.m_number_attributes;

      if(new_group.item_group()!=m_item_group.item_group())
        {
          /*
            setting to 0 will make index resize
            code below delete the old group.
           */
          m_allocated.m_number_indices=0;

          if(m_item_group.valid())
            {
              m_item_group.release_group();
            }
        }     
      m_item_group=new_group;

    }


  /*
    Then resize index buffer if needed
   */
  if(m_required.m_number_indices>m_allocated.m_number_indices)
    {
      if(m_index_data_location.valid())
        {
          m_index_data_location.delete_group();
          WRATHassert(!m_index_data_location.valid());
        }
      
      m_index_data_location=
        m_item_group.allocate_index_group<GLushort>(m_required.m_number_indices);
      m_allocated.m_number_indices=m_required.m_number_indices;

    }
}

void
WRATHBasicTextItem::per_page_type::
set_text(const_c_array<range_type<int> > R,
         const WRATHFormattedTextStream &pdata,
         const WRATHStateStream &state_stream,
         WRATHTextAttributePacker::BBox *out_bounds_box)
{

  m_required=m_parent->m_packer->allocation_requirement(R,
                                                        m_parent->m_font, 
                                                        m_texture_page,
                                                        pdata, state_stream);
  allocate_room_if_needed();

  if(m_item_group.valid())
    {
      {
        /*
          "clear" all the indices since any indices within
          m_index_data_location beyond m_required.m_number_indices
          are not set by the attribute packer.
         */
        WRATHAutoLockMutex(m_index_data_location.mutex());
        c_array<GLushort> indices_ptr(m_index_data_location.pointer());
        std::fill(indices_ptr.begin(), indices_ptr.end(), 0);
      }
      m_parent->m_packer->set_attribute_data(R, 
                                             m_parent->m_font, 
                                             m_texture_page, 
                                             m_item_group, 
                                             m_attribute_location,
                                             m_index_data_location, 
                                             pdata, state_stream, out_bounds_box);

      

    }
  
}


void
WRATHBasicTextItem::per_page_type::
canvas(WRATHCanvas *c)
{
  enum return_code R;

  R=c->transfer(m_item_group,
                m_attribute_location,
                m_index_data_location);
  
  WRATHassert(R==routine_success);
  WRATHunused(R);
}


///////////////////////////////////////
// WRATHBasicTextItem methods
WRATHBasicTextItem::
WRATHBasicTextItem(draw_method pdrawer,
                   const WRATHCanvas::SubKeyBase &subkey,
                   WRATHCanvas *pcontainer,
                   WRATHTextureFont *pfont,
                   enum WRATHTextItemTypes::text_opacity_t opacity_type,
                   const draw_order &pdraw_order,
                   const WRATHBasicTextItem::ExtraDrawState &extra_state):
  m_subkey(subkey.create_copy()),
  m_extra_state(extra_state),
  m_group_collection(pcontainer),
  m_font(pfont),
  m_drawer(pdrawer.m_program_drawer),
  m_packer(pdrawer.m_attribute_packer),
  m_draw_order(pdraw_order)
{
  init(opacity_type);
}

void
WRATHBasicTextItem::
init(enum WRATHTextItemTypes::text_opacity_t opacity_type)
{
  if(opacity_type==WRATHTextItemTypes::text_opaque 
     and !m_drawer->has_tranlucent_pass())
    {
      opacity_type=WRATHTextItemTypes::text_opaque_non_aa;
    }

  switch(opacity_type)
    {
    default:
    case WRATHTextItemTypes::text_transparent:
      m_passes.push_back(WRATHTextureFontDrawer::pure_transluscent);
      break;

    case WRATHTextItemTypes::text_opaque:
      m_passes.push_back(WRATHTextureFontDrawer::opaque_draw_pass);
      m_passes.push_back(WRATHTextureFontDrawer::transluscent_draw_pass);
      break;

    case WRATHTextItemTypes::text_opaque_non_aa:
      m_passes.push_back(WRATHTextureFontDrawer::opaque_draw_pass);
      break;
    }

  if(!m_draw_order.m_pass_specifier.valid())
    {
      m_draw_order.m_pass_specifier=WRATHTextureFontDrawer::default_pass_specifier();
    }
  m_font->increment_use_count();
}

WRATHBasicTextItem::
~WRATHBasicTextItem()
{

  for(std::vector<per_page_type*>::iterator 
        iter=m_items.begin(), end=m_items.end(); iter!=end; ++iter)
    {
      WRATHDelete(*iter);
    }
  WRATHDelete(m_subkey);
  m_font->decrement_use_count();
}

void
WRATHBasicTextItem::
clear(void)
{
  for(std::vector<per_page_type*>::iterator 
        iter=m_items.begin(), end=m_items.end(); iter!=end; ++iter)
    {
      (*iter)->clear();
    }
  m_box.clear();
}


void
WRATHBasicTextItem::
preallocate_subitems(unsigned int number_pages)
{
  unsigned int old_size;

  old_size=m_items.size();
  m_items.resize( std::max(old_size, number_pages));

  for(;old_size<number_pages; ++old_size)
    {
      m_items[old_size]=WRATHNew per_page_type(old_size, this);
    }
}

void
WRATHBasicTextItem::
set_text(const_c_array<range_type<int> > R,
         const WRATHFormattedTextStream &ptext,
         const WRATHStateStream &state_stream)
{
  int max_page;

  clear();

  max_page=WRATHTextAttributePacker::highest_texture_page(R, ptext, m_font);
  if(max_page<0)
    {
      return;
    }

  preallocate_subitems(1+std::max(0,max_page));
  for(std::vector<per_page_type*>::iterator 
        iter=m_items.begin(), end=m_items.end(); iter!=end; ++iter)
    {
      (*iter)->set_text(R, ptext, state_stream, &m_box);
    }

  
}

void
WRATHBasicTextItem::
canvas(WRATHCanvas *c)
{
  if(c==m_group_collection)
    {
      return;
    }

  for(std::vector<per_page_type*>::iterator 
        iter=m_items.begin(), end=m_items.end(); iter!=end; ++iter)
    {
      (*iter)->canvas(c);
    }
  m_group_collection=c;
}

void
WRATHBasicTextItem::
generate_key(std::set<WRATHItemDrawState> &skey,
             WRATHAttributeStoreKey &attribute_key,
             int page)
{

  m_packer->attribute_key(attribute_key);

  for(int i=0, end_i=m_passes.size(); i!=end_i; ++i)
    {
      WRATHItemDrawState pkey;
      WRATHDrawType nm;
      enum WRATHTextureFontDrawer::drawing_pass_type tp;
      const_c_array<WRATHTextureChoice::texture_base::handle> Ts;
      
      tp=m_passes[i];
      nm=m_draw_order.m_pass_specifier->draw_type(tp, m_draw_order.m_item_pass);
      
      pkey
        .primitive_type(GL_TRIANGLES)
        .drawer(m_drawer->drawer_named(tp))
        .add_uniform(m_drawer->texture_size_named_uniform(tp, m_font, page))
        .force_draw_order(m_draw_order.named_draw_order(tp))
        .draw_type(nm);
      
      if(tp==WRATHTextureFontDrawer::transluscent_draw_pass 
         or tp==WRATHTextureFontDrawer::pure_transluscent)
        {
          pkey.add_gl_state_change(WRATHTextureFontDrawer::translucent_pass_state_change());
        }
      

      Ts=m_font->texture_binder(page);
      for(unsigned int i=0, end_i=Ts.size(); i<end_i; ++i)
        {
          pkey.add_texture(GL_TEXTURE0+i, Ts[i]);
        }
      pkey.absorb(m_extra_state.named_state(tp));
      pkey.absorb(m_extra_state.m_common_pass_state);
      
      skey.insert(pkey);
    }
  

}

