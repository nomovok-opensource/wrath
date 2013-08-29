/*! 
 * \file WRATHShapeItem.cpp
 * \brief file WRATHShapeItem.cpp
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
#include "WRATHShapeItem.hpp"

namespace
{
  enum
    {
      primary_implicit_store=0,
      secondary_implicit_store=1,
    };

  void
  filter_brush(WRATHBrush &brush, 
               enum WRATHShapeItemTypes::shape_opacity_t aa,
               bool shader_supports_aa)
  {
    brush.anti_alias(aa!=WRATHShapeItemTypes::shape_opaque_non_aa and shader_supports_aa);
    brush.make_consistent();
  }

  inline
  enum WRATHShapeItemTypes::shape_opacity_t 
  disallow_2pass_rendering(enum WRATHShapeItemTypes::shape_opacity_t aa)
  {
    return (aa==WRATHShapeItemTypes::shape_opaque)?
      WRATHShapeItemTypes::shape_opaque_non_aa:
      aa;
  }

  
  void
  index_reallocator_helper(WRATHCanvas::DataHandle &item_group,
                           WRATHIndexGroupAllocator::index_group<GLushort> &index_group, 
                           int needed_allocated)
  {
    if(!item_group.valid())
      {
        return;
      }

    if(needed_allocated>0 and (!index_group.valid() or index_group.size() < needed_allocated) )
      {
        if(index_group.valid())
          {
            index_group.delete_group();
          }
        index_group=item_group.allocate_index_group<GLushort>(needed_allocated);        
      }
    else if(index_group.valid())
      {
        WRATHAutoLockMutex(index_group.mutex());
        c_array<GLushort> ptr;
        ptr=index_group.pointer();
        std::fill(ptr.begin(), ptr.end(), 0);
      }
  }
}


////////////////////////////////////////
// WRATHShapeDrawerImplementHelper methods
void
WRATHShapeDrawerImplementHelper::
init(std::vector<WRATHShapeItemTypes::ShapeDrawerPass> &draw_passes,
     const WRATHShaderSpecifier *sp,
     enum shape_opacity_t aa,
     const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h,
     int pitem_pass)
{

  switch(aa)
    {
    case shape_transparent:
      {
        ShapeDrawerPass v(&sp->fetch_sub_shader(WRATHTwoPassDrawer::pure_transluscent), 
                          h->draw_type(WRATHTwoPassDrawer::pure_transluscent, pitem_pass));
        draw_passes.push_back(v);
        draw_passes[0].m_draw_state
          .add_gl_state_change(WRATHTextureFontDrawer::translucent_pass_state_change());
      }
      break;

    case shape_opaque:
      {
        draw_passes.resize(2);
        
        draw_passes[0]=ShapeDrawerPass(&sp->fetch_sub_shader(WRATHTwoPassDrawer::opaque_draw_pass),
                                       h->draw_type(WRATHTwoPassDrawer::opaque_draw_pass, pitem_pass));

        draw_passes[1]=ShapeDrawerPass(&sp->fetch_sub_shader(WRATHTwoPassDrawer::transluscent_draw_pass),
                                       h->draw_type(WRATHTwoPassDrawer::transluscent_draw_pass, pitem_pass));
        draw_passes[1].m_draw_state
          .add_gl_state_change(WRATHTextureFontDrawer::translucent_pass_state_change());

        draw_passes[1].m_use_secondary_indices=true;
      }
      break;

    case shape_opaque_non_aa:
      {
        /*
          note that we do NOT grab the sub-shader, rather we grab
          the original shader, this is because the sub-shader
          will still perform an AA computation and do a discard 
          in it's fragment shader of the opaque pass and that 
          is not necessary.
         */
        ShapeDrawerPass v(sp, h->draw_type(WRATHTwoPassDrawer::opaque_draw_pass, pitem_pass));
        draw_passes.push_back(v);
      }
      break;
    }
}


void
WRATHShapeDrawerImplementHelper::
init(std::vector<WRATHShapeItemTypes::ShapeDrawerPass> &draw_passes,
     enum WRATHShapeItemTypes::fill_shape_t,
     const WRATHBrush &pbrush, enum shape_opacity_t aa,
     const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h,
     int pitem_pass, enum WRATHBaseSource::precision_t v)
{
  /*
    TODO: support AA rendering for filling
   */
  WRATHBrush brush(pbrush);

  aa=disallow_2pass_rendering(aa);
  filter_brush(brush, aa, false);
  init(draw_passes, 
       &WRATHDefaultShapeShader::shader_hoard().fetch(brush, v),
       aa, h, pitem_pass);
  
  for(unsigned int I=0, endI=draw_passes.size(); I<endI; ++I)
    {
      WRATHassert(draw_passes[I].m_shader!=NULL);
      WRATHDefaultShapeShader::shader_hoard().add_state(brush, draw_passes[I].m_draw_state);
    }
}

void
WRATHShapeDrawerImplementHelper::
init(std::vector<WRATHShapeItemTypes::ShapeDrawerPass> &draw_passes,
     enum WRATHShapeItemTypes::stroke_shape_t,
     const WRATHBrush &pbrush, enum shape_opacity_t aa,
     const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h,
     int pitem_pass, enum WRATHBaseSource::precision_t v)
{
  WRATHBrush brush(pbrush);
  
  filter_brush(brush, aa, true);
  init(draw_passes, 
       &WRATHDefaultShapeShader::shader_brush(brush, v),
       aa, h, pitem_pass);
  
  for(unsigned int I=0, endI=draw_passes.size(); I<endI; ++I)
    {
      WRATHassert(draw_passes[I].m_shader!=NULL);
      WRATHDefaultShapeShader::shader_hoard().add_state(brush, draw_passes[I].m_draw_state);
    }
}







//////////////////////////////////////////////
// WRATHShapeItem methods
WRATHShapeItem::
~WRATHShapeItem()
{
  if(m_primary_item_group.valid())
    {
      if(m_primary_index_data_location.valid())
        {
          m_primary_index_data_location.delete_group();
        }

      m_primary_item_group.deallocate_attribute_datas(m_attribute_data_location.begin(),
                                                      m_attribute_data_location.end());
      m_primary_item_group.release_group();
    }

  if(m_secondary_item_group.valid())
    {
      if(m_secondary_index_data_location.valid())
        {
          m_secondary_index_data_location.delete_group();
        }
      m_secondary_item_group.release_group();
    }
}


WRATHCanvas*
WRATHShapeItem::
canvas_base(void) const
{
  WRATHassert(!m_secondary_item_group.valid()
              or m_secondary_item_group.parent()==m_primary_item_group.parent());
  return m_primary_item_group.parent();
}


void
WRATHShapeItem::
canvas_base(WRATHCanvas *c)
{
  {
    enum return_code R;
    
    R=c->transfer(m_primary_item_group,
                  m_attribute_data_location,
                  m_primary_index_data_location);
    
    WRATHassert(R==routine_success);
    WRATHunused(R);
  }

  if(m_secondary_item_group.valid())
    {
      enum return_code R;
      R=c->transfer(m_secondary_item_group,
                    m_attribute_data_location,
                    m_secondary_index_data_location);
  
      WRATHassert(R==routine_success);
      WRATHunused(R);
    }
}

void
WRATHShapeItem::
init_key_and_allocate(WRATHShapeAttributePackerBase::allocation_requirement_type reqs,
                      const WRATHItemDrawerFactory &factory, int subdrawer_id,
                      WRATHCanvas *pcanvas,
                      const WRATHCanvas::SubKeyBase &subkey,
                      const WRATHShapeAttributePackerBase *packer,
                      GLenum buffer_object_hint,
                      const std::vector<WRATHShapeItemTypes::ShapeDrawerPass> &draw_passes)
{
  WRATHAttributeStoreKey attr_key;
  GLenum primitive_type;
  bool has_secondary_pass;

  m_packer=packer;
  has_secondary_pass=packer->has_secondary_pass();
  m_allocated_number_attributes=reqs.m_number_attributes;

  /*
    Get attribute format and primitive type.
  */
  attr_key.buffer_object_hint(buffer_object_hint);
  primitive_type=packer->attribute_key(attr_key);
  
  /*
    Get the attribute store and allocate attribute data:
  */
  WRATHAttributeStore::handle attr_handle;
  attr_handle=pcanvas->attribute_store(attr_key,
                                       reqs.m_number_attributes,
                                       m_attribute_data_location);
  WRATHassert(attr_handle.valid());

  /*
    Create the draw group keys
  */
  std::set<WRATHItemDrawState> primary_skey, secondary_skey;
  for(std::vector<WRATHShapeItemTypes::ShapeDrawerPass>::const_iterator 
        iter=draw_passes.begin(), end=draw_passes.end(); iter!=end; ++iter)
    {
      WRATHItemDrawState key;
  
      key
        .primitive_type(primitive_type)
        .drawer(iter->m_shader->fetch_drawer(factory, packer, subdrawer_id))
        .absorb(iter->m_draw_state)
        .force_draw_order(iter->m_force_draw_order)
        .buffer_object_hint(buffer_object_hint)
        .draw_type(iter->m_draw_type);

      if(iter->m_use_secondary_indices and has_secondary_pass)
        {
          secondary_skey.insert(key);
        }
      else
        {
          primary_skey.insert(key);
        }
    }

  m_primary_item_group=pcanvas->create(attr_handle, primary_skey, subkey, primary_implicit_store);
  WRATHassert(m_primary_item_group.valid());
  m_primary_item_group.set_implicit_attribute_data(m_attribute_data_location);

  if(has_secondary_pass)
    {
      m_secondary_item_group=pcanvas->create(attr_handle, secondary_skey, subkey, secondary_implicit_store);
      WRATHassert(m_secondary_item_group.valid());
      m_secondary_item_group.set_implicit_attribute_data(m_attribute_data_location);
    }

  /*
    Allocate index data
  */
  if(reqs.m_primary_number_indices>0)
    {
      m_primary_index_data_location
        =m_primary_item_group.allocate_index_group<GLushort>(reqs.m_primary_number_indices);
      WRATHassert(m_primary_index_data_location.valid());
    }

  if(has_secondary_pass and reqs.m_secondary_number_indices>0)
    {
      m_secondary_index_data_location
        =m_secondary_item_group.allocate_index_group<GLushort>(reqs.m_secondary_number_indices);
      WRATHassert(m_secondary_index_data_location.valid());
    }
}

void
WRATHShapeItem::
allocate_indices_and_attributes(WRATHShapeAttributePackerBase::allocation_requirement_type reqs)
{
  if(reqs.m_number_attributes>m_allocated_number_attributes)
    {
      int delta(reqs.m_number_attributes-m_allocated_number_attributes);

      /*
        first try to increase the size of the number
        of attribute allocated:
       */
      if(routine_fail==m_primary_item_group.fragmented_allocate_attribute_data(delta, m_attribute_data_location))
        {
          /* 
             failed to resize, get a new WRATHAttributeStore for the job..
           */
          WRATHCanvas *the_canvas(canvas_base());
          WRATHAttributeStore::handle attr_handle;
          std::vector<range_type<int> > new_locs;
          WRATHCanvas::DataHandle new_group;

          attr_handle=the_canvas->attribute_store(m_primary_item_group.attribute_store()->key(), 
                                                  reqs.m_number_attributes, 
                                                  new_locs);
          WRATHassert(attr_handle.valid());

          if(!attr_handle.valid())
            {
              return;
            }
          
          new_group=the_canvas->create(attr_handle, 
                                       m_primary_item_group.item_draw_state(), 
                                       *m_primary_item_group.custom_data()->subkey(),
                                       primary_implicit_store);
          WRATHassert(new_group.valid());

          if(!new_group.valid())
            {
              return;
            }

          m_primary_item_group.release_group();
          m_primary_item_group=new_group;
          m_primary_item_group.set_implicit_attribute_data(m_attribute_data_location);

          if(m_secondary_item_group.valid())
            {
              WRATHCanvas::DataHandle G;

              G=the_canvas->create(attr_handle, m_secondary_item_group.item_draw_state(), 
                                   *m_secondary_item_group.custom_data()->subkey(),
                                   secondary_implicit_store);
              
              m_secondary_item_group.release_group();
              m_secondary_item_group=G;
              m_secondary_item_group.set_implicit_attribute_data(m_attribute_data_location);
            }

          /*
            force recreate the index groups
           */
          if(m_primary_index_data_location.valid())
            {
              m_primary_index_data_location.delete_group();
            }

          if(m_secondary_index_data_location.valid())
            {
              m_secondary_index_data_location.delete_group();
            }
        }      
      m_allocated_number_attributes=reqs.m_number_attributes;
    }
  index_reallocator_helper(m_primary_item_group, m_primary_index_data_location, 
                           reqs.m_primary_number_indices);

  index_reallocator_helper(m_secondary_item_group, m_secondary_index_data_location, 
                           reqs.m_secondary_number_indices);

}
