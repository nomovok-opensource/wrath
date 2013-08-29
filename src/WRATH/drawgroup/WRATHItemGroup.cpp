/*! 
 * \file WRATHItemGroup.cpp
 * \brief file WRATHItemGroup.cpp
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
#include <limits>
#include "WRATHItemGroup.hpp"




/////////////////////////////
// WRATHItemGroup methods
WRATHItemGroup::
WRATHItemGroup(const WRATHIndexGroupAllocator::handle &pindex_allocator,
               const std::vector<DrawCall> &spec,
               const WRATHCompiledItemDrawStateCollection &pitem_draw_state,
               int pimplicit_store):
  WRATHTripleBufferEnabler::PhasedDeletedObject(pindex_allocator->triple_buffer_enabler()),
  m_index_store(pindex_allocator),
  m_elements(spec.size()),
  m_key(pitem_draw_state),
  m_implicit_store(pimplicit_store)
{
  WRATHassert(spec.size()==pitem_draw_state.size());

  for(unsigned int i=0, endi=spec.size(); i<endi; ++i)
    {
      WRATHassert(spec[i].second.m_force_draw_order==pitem_draw_state.force_draw_order(i));


      m_elements[i]=WRATHNew WRATHRawDrawDataElement(spec[i].second);
      schedule_simulation_action(boost::bind(&WRATHRawDrawData::add_element, 
                                             spec[i].first, 
                                             m_elements[i]));
    }
}



WRATHItemGroup::
~WRATHItemGroup()
{
  for(unsigned int i=0, endi=m_elements.size(); i<endi; ++i)
    {
      WRATHassert(NULL==m_elements[i]->raw_draw_data());
      WRATHDelete(m_elements[i]);
    }
}

void
WRATHItemGroup::
phase_simulation_deletion(void)
{
  for(unsigned int i=0, endi=m_elements.size(); i<endi; ++i)
    {
      WRATHRawDrawData::remove_element(m_elements[i]);
    }
}



