/*! 
 * \file WRATHTextAttributePacker.cpp
 * \brief file WRATHTextAttributePacker.cpp
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
#include "WRATHTextAttributePacker.hpp"



//////////////////////////////////////////
// WRATHTextAttributePacker methods
WRATH_RESOURCE_MANAGER_IMPLEMENT(WRATHTextAttributePacker, 
                                 WRATHTextAttributePacker::ResourceKey);

WRATHTextAttributePacker::
WRATHTextAttributePacker(const ResourceKey &pname):
  m_resource_name(pname)
{
  resource_manager().add_resource(m_resource_name, this);
}

WRATHTextAttributePacker::
~WRATHTextAttributePacker()
{
  resource_manager().remove_resource(this);
}

const WRATHAttributePacker*
WRATHTextAttributePacker::
fetch_attribute_packer(int n) const
{
  WRATHAutoLockMutex(m_mutex);
  std::map<int, const WRATHAttributePacker*>::iterator iter;


  iter=m_packers.find(n);
  if(iter!=m_packers.end())
    {
      return iter->second;
    }

  const WRATHAttributePacker *q;
  std::vector<std::string> attrs;
  std::ostringstream ostr;

  attribute_names(attrs, n);
  ostr << resource_name() << "_" << n;

  q=WRATHNew WRATHAttributePacker(ostr.str(), attrs.begin(), attrs.end());
  m_packers[n]=q;

  return q;
}

int
WRATHTextAttributePacker::
highest_texture_page(range_type<int> R, 
                     const WRATHFormattedTextStream &pdata,
                     WRATHTextureFont *font)
{
  int return_value(-1);
  for(int c=R.m_begin; c<R.m_end; ++c)
    {        
      const WRATHTextureFont::glyph_data_type *G(pdata.data(c).m_glyph);
      if(G!=NULL and font==G->font())
        {
          return_value=std::max(return_value, G->texture_page());
        }
    }
  return return_value;
}

unsigned int
WRATHTextAttributePacker::
number_of_characters(range_type<int> R, 
                     const WRATHFormattedTextStream &pdata,
                     WRATHTextureFont *font, 
                     int texture_page)
{
  unsigned int return_value(0);
  
  for(int c=R.m_begin; c<R.m_end; ++c)
    {
      const WRATHTextureFont::glyph_data_type *G(pdata.data(c).m_glyph);
      if(G!=NULL and font==G->font() and texture_page==G->texture_page())
        {
          ++return_value;
        }
    }
  
  return return_value;
}


void
WRATHTextAttributePacker::
compute_bounding_box(range_type<int> R,
                     const WRATHFormattedTextStream &pdata,
                     const WRATHStateStream &state_stream,
                     WRATHTextAttributePacker::BBox &v) const
{
  R.m_end=std::min(R.m_end, 
                   static_cast<int>(pdata.data_stream().size()));

  WRATHText::scale::stream_iterator scale_stream;
  WRATHText::scale::type current_scale(1.0f);
  WRATHText::horizontal_stretching::stream_iterator horizontal_stretch_stream;
  WRATHText::vertical_stretching::stream_iterator vertical_stretch_stream;
  vec2 current_stretch(1.0f, 1.0f);

  current_scale
    =WRATHText::scale::init_stream_iterator(state_stream,
                                            R.m_begin, current_scale, 
                                            scale_stream);

  current_stretch.x()
    =WRATHText::horizontal_stretching::init_stream_iterator(state_stream,
                                                            R.m_begin, current_stretch.x(), 
                                                            horizontal_stretch_stream);

  current_stretch.y()
    =WRATHText::vertical_stretching::init_stream_iterator(state_stream,
                                                          R.m_begin, current_stretch.y(), 
                                                          vertical_stretch_stream);

  for(int c=R.m_begin; c<R.m_end; ++c)
    {
      WRATHText::scale::update_value_from_change(c, current_scale, scale_stream);   
      WRATHText::horizontal_stretching::update_value_from_change(c, current_stretch.x(), 
                                                                 horizontal_stretch_stream);   
      WRATHText::vertical_stretching::update_value_from_change(c, current_stretch.y(), 
                                                               vertical_stretch_stream);   
      if(pdata.data(c).m_glyph!=NULL)
        {
          vecN<vec2,2> pos;
          

          pos=pdata.position(c, current_scale*current_stretch);

          v.set_or(pos[0]);
          v.set_or(pos[1]);
        }
    }
}
