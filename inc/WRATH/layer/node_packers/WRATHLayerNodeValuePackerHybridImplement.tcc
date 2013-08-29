// -*- C++ -*-

/*! 
 * \file WRATHLayerNodeValuePackerHybridImplement.tcc
 * \brief file WRATHLayerNodeValuePackerHybridImplement.tcc
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


#if !defined(__WRATH_LAYER_ITEM_UNIFORM_PACKER_HYBRID_HPP__) || defined(__WRATH_LAYER_ITEM_UNIFORM_PACKER_HYBRID_IMPLEMENT__)
#error "Direction inclusion of private header file WRATHLayerNodeValuePackerHybridImplement.tcc" 
#endif

#define __WRATH_LAYER_ITEM_UNIFORM_PACKER_HYBRID_IMPLEMENT__

namespace WRATHLayerNodeValuePackerHybridImplement
{
  class Payload:public WRATHLayerNodeValuePackerBase::SpecDataProcessedPayload
  {
  public:
    typedef handle_t<Payload> handle;
    typedef const_handle_t<Payload> const_handle;

    SpecDataProcessedPayload::handle m_vertex;
    SpecDataProcessedPayload::handle m_fragment;
    WRATHLayerNodeValuePackerBase::ProcessedActiveNodeValuesCollection m_vertex_spec;
    WRATHLayerNodeValuePackerBase::ProcessedActiveNodeValuesCollection m_fragment_spec;
    
  };

  const WRATHLayerNodeValuePackerBase::function_packet*
  fetch_function_packet(const WRATHLayerNodeValuePackerBase::function_packet *VertexPacker,
                        const WRATHLayerNodeValuePackerBase::function_packet *FragmentPacker);
};

template<typename VertexPacker, typename FragmentPacker>
WRATHLayerNodeValuePackerHybrid<VertexPacker, FragmentPacker>::
WRATHLayerNodeValuePackerHybrid(WRATHLayerBase *layer,
                                const SpecDataProcessedPayload::const_handle &payload,
                                const ProcessedActiveNodeValuesCollection&):
  WRATHLayerNodeValuePackerBase(layer, payload, 
                                ProcessedActiveNodeValuesCollection()) //feed an empty spec to the base class
{
  WRATHLayerNodeValuePackerHybridImplement::Payload::const_handle h;

  WRATHassert(payload.dynamic_cast_handle<WRATHLayerNodeValuePackerHybridImplement::Payload>().valid());
  h=payload.static_cast_handle<WRATHLayerNodeValuePackerHybridImplement::Payload>();

  
  /*
    limit the spec's passed to the actual stage in question.
   */
  m_vertex_packer=WRATHNew VertexPacker(layer, h->m_vertex, h->m_vertex_spec);
  m_fragment_packer=WRATHNew FragmentPacker(layer, h->m_fragment, h->m_fragment_spec);
}
    
template<typename VertexPacker, typename FragmentPacker>
void
WRATHLayerNodeValuePackerHybrid<VertexPacker, FragmentPacker>::
on_place_on_deletion_list(void)
{
  WRATHPhasedDelete(m_vertex_packer);
  WRATHPhasedDelete(m_fragment_packer);
  WRATHLayerNodeValuePackerBase::on_place_on_deletion_list();
}
  
template<typename VertexPacker, typename FragmentPacker>                          
void
WRATHLayerNodeValuePackerHybrid<VertexPacker, FragmentPacker>::
append_state(WRATHSubItemDrawState &skey)
{
  m_vertex_packer->append_state(skey);
  m_fragment_packer->append_state(skey);
}

template<typename VertexPacker, typename FragmentPacker>      
const WRATHLayerNodeValuePackerBase::function_packet&
WRATHLayerNodeValuePackerHybrid<VertexPacker, FragmentPacker>::
functions(void)
{
  return *WRATHLayerNodeValuePackerHybridImplement::fetch_function_packet(&VertexPacker::functions(),
                                                                          &FragmentPacker::functions());
}


template<typename VertexPacker, typename FragmentPacker>    
void
WRATHLayerNodeValuePackerHybrid<VertexPacker, FragmentPacker>::
assign_slot(int slot, WRATHLayerItemNodeBase* h, int highest_slot)
{
  m_vertex_packer->assign_slot(slot, h, highest_slot);
  m_fragment_packer->assign_slot(slot, h, highest_slot);
  WRATHLayerNodeValuePackerBase::assign_slot(slot, h, highest_slot);
}
