//  -*- C++ -*-

/*! 
 * \file WRATHShapeItemImplement.tcc
 * \brief file WRATHShapeItemImplement.tcc
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


/*
  Should only be included from WRATHShapeItem.hpp
 */

#if !defined(WRATH_HEADER_SHAPE_ITEM_HPP_) || defined(WRATH_HEADER_SHAPE_ITEM_IMPLEMENT_TCC_)
#error "Direction inclusion of private header file WRATHShapeItemImplement.tcc"
#endif
#define WRATH_HEADER_SHAPE_ITEM_IMPLEMENT_TCC_

namespace WRATHShapeDrawerImplementHelper
{
  using namespace WRATHShapeItemTypes;


  void
  init(std::vector<ShapeDrawerPass>&,
       const WRATHShaderSpecifier*,
       enum shape_opacity_t,
       const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle&,
       int);

  void
  init(std::vector<ShapeDrawerPass> &draw_passes,
       enum fill_shape_t,
       const WRATHBrush &brush, enum shape_opacity_t aa,
       const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h,
       int pitem_pass, enum WRATHBaseSource::precision_t v);

  
  void
  init(std::vector<ShapeDrawerPass> &draw_passes,
       enum stroke_shape_t,
       const WRATHBrush &brush, enum shape_opacity_t aa,
       const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h,
       int pitem_pass, enum WRATHBaseSource::precision_t v);

}


/////////////////////////////////////////
// WRATHShapeItemTypes::ShapeDrawer<T> methods
template<typename T>
WRATHShapeItemTypes::ShapeDrawer<T>::
ShapeDrawer(const WRATHShaderSpecifier *sh,
            const WRATHShapeAttributePacker<T> *p,
            enum shape_opacity_t aa,
            const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h,
            int pitem_pass)
{
  this->m_packer=p;
  WRATHShapeDrawerImplementHelper::init(this->m_draw_passes, sh, aa, h, pitem_pass);
}


template<typename T>
WRATHShapeItemTypes::ShapeDrawer<T>::
ShapeDrawer(enum WRATHShapeItemTypes::fill_shape_t,
            const WRATHBrush &brush, enum shape_opacity_t aa,
            const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h,
            int pitem_pass, enum WRATHBaseSource::precision_t v)
{
  this->m_packer=WRATHDefaultFillShapeAttributePackerT<T>::fetch();
  WRATHShapeDrawerImplementHelper::init(this->m_draw_passes, fill_shape, brush, aa, h, pitem_pass, v);
}

template<typename T>
WRATHShapeItemTypes::ShapeDrawer<T>::
ShapeDrawer(enum WRATHShapeItemTypes::stroke_shape_t,
            const WRATHBrush &brush, enum shape_opacity_t aa,
            const WRATHTwoPassDrawer::DrawTypeSpecifier::const_handle &h,
            int pitem_pass, enum WRATHBaseSource::precision_t v)
{
  this->m_packer=WRATHDefaultStrokeAttributePackerT<T>::fetch();
  WRATHShapeDrawerImplementHelper::init(this->m_draw_passes, stroke_shape, brush, aa, h, pitem_pass, v);
}

//////////////////////////////////////
// WRATHShapeItem methods
template<typename T>
void
WRATHShapeItem::
construct(const WRATHItemDrawerFactory &factory, int subdrawer_id,
          WRATHCanvas *canvas,
          const WRATHCanvas::SubKeyBase &subkey,
          const WRATHShape<T> &shape,
          const WRATHShapeItemTypes::ShapeDrawer<T> &drawer,
          WRATHShapeProcessorPayload payload,
          const WRATHShapeAttributePackerBase::PackingParametersBase &additional_packing_params)
{
  WRATHassert(!drawer.m_draw_passes.empty());
  WRATHassert(&shape!=NULL);
  WRATHassert(drawer.m_packer!=NULL);
  WRATHassert(canvas!=NULL);

  m_immutable_packing_data=drawer.m_immutable_packing_data;

  WRATHShapeAttributePackerBase::allocation_requirement_type reqs;    
  reqs=drawer.m_packer->allocation_requirement(&shape, payload, additional_packing_params, m_immutable_packing_data);

    
  init_key_and_allocate(reqs,
                        factory, subdrawer_id, 
                        canvas, subkey, drawer.m_packer,
                        drawer.m_buffer_object_hint,
                        drawer.m_draw_passes);

  drawer.m_packer->set_attribute_data(&shape, payload, 
                                      m_primary_item_group.attribute_store(),
                                      m_attribute_data_location, 
                                      m_primary_index_data_location,
                                      m_secondary_index_data_location,
                                      additional_packing_params,
                                      m_immutable_packing_data);
    
  
  
}

template<typename T>
void
WRATHShapeItem::
change_shape(const WRATHShapeItemTypes::shape_valueT<T> &pshape,
             const WRATHShapeAttributePackerBase::PackingParametersBase &additional_packing_params)
{

  const WRATHShapeAttributePacker<T> *packer;
  packer=dynamic_cast<const WRATHShapeAttributePacker<T>*>(m_packer);
  WRATHassert(packer!=NULL);

  if(packer==NULL)
    {
      return;
    }

  WRATHShapeProcessorPayload payload(pshape.m_payload);
  if(!payload.valid())
    {
      packer->default_payload(&pshape.m_shape);
    }

  WRATHShapeAttributePackerBase::allocation_requirement_type reqs;

  reqs=packer->allocation_requirement(&pshape.m_shape, payload, 
                                      additional_packing_params, m_immutable_packing_data);
  allocate_indices_and_attributes(reqs);

  packer->set_attribute_data(&pshape.m_shape, payload, 
                             m_primary_item_group.attribute_store(),
                             m_attribute_data_location, 
                             m_primary_index_data_location,
                             m_secondary_index_data_location,
                             additional_packing_params,
                             m_immutable_packing_data);
  
  
}
