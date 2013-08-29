/*! 
 * \file WRATHTextDataStreamManipulator.cpp
 * \brief file WRATHTextDataStreamManipulator.cpp
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
#include "WRATHTextDataStreamManipulator.hpp"

WRATHText::effective_scale::stream_iterator::
stream_iterator():
  m_font(NULL),
  m_pixel_size(32.0f),
  m_scale(1.0f)
{}

float
WRATHText::effective_scale::stream_iterator::
sub_range(int start_index)
{
  
  m_scale=scale::sub_range(start_index, m_scale, m_scale_stream);
  m_pixel_size=pixel_size::sub_range(start_index, m_pixel_size, m_pixel_size_stream);
  m_font=font::sub_range(start_index, m_font, m_font_stream);

  return compute_effective_scale();

}


float
WRATHText::effective_scale::stream_iterator::
compute_effective_scale() const
{
  if(m_font!=NULL)
    {
      return m_scale*m_pixel_size/static_cast<float>(m_font->pixel_size());
    }
  else
    {
      return m_scale*m_pixel_size/32.0f;
    }
}

bool
WRATHText::effective_scale::stream_iterator::
update_value_from_change(int current_index, float &out_value)
{
  bool return_value(false);

  return_value=update_value_from_change(current_index);
  if(return_value)
    {
      out_value=compute_effective_scale();
    }

  return return_value;

}

bool
WRATHText::effective_scale::stream_iterator::
update_value_from_change(int current_index)
{
  bool return_value(false);

  if(font::update_value_from_change(current_index, m_font, m_font_stream))
    {
      return_value=true;
    }

  if(scale::update_value_from_change(current_index, m_scale, m_scale_stream))
    {
      return_value=true;
    }

  if(pixel_size::update_value_from_change(current_index, m_pixel_size, m_pixel_size_stream))
    {
      return_value=true;
    }

  return return_value;
}


float
WRATHText::effective_scale::stream_iterator::
init_stream_iterator(const WRATHStateStream &state_stream, int start_index)
{
  m_font=font::init_stream_iterator(state_stream, start_index, m_font, m_font_stream);
  m_pixel_size=pixel_size::init_stream_iterator(state_stream, start_index, m_pixel_size, m_pixel_size_stream);
  m_scale=scale::init_stream_iterator(state_stream, start_index, m_scale, m_scale_stream);

  return compute_effective_scale();
}
