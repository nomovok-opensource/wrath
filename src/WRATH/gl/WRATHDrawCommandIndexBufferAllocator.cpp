/*! 
 * \file WRATHDrawCommandIndexBufferAllocator.cpp
 * \brief file WRATHDrawCommandIndexBufferAllocator.cpp
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
#include "WRATHDrawCommandIndexBufferAllocator.hpp"


void
WRATHDrawCommandIndexBufferAllocator::
append_draw_elements(std::vector<index_range> &output)
{
  WRATHassert(m_params.m_index_buffer!=NULL);

  range_type<int> R(m_params.m_index_buffer->allocated_range());
  index_range V;

  V.m_location=R.m_begin;
  V.m_count= (R.m_end-R.m_begin)/m_params.index_type_size();

  output.push_back(V);
}
