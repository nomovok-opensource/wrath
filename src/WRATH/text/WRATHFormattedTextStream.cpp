/*! 
 * \file WRATHFormattedTextStream.cpp
 * \brief file WRATHFormattedTextStream.cpp
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
#include <locale>
#include "WRATHFormattedTextStream.hpp"
#include "WRATHTextDataStreamManipulator.hpp"


WRATHFormattedTextStream::
WRATHFormattedTextStream(void):
  m_orientation(WRATHFormatter::y_increases_downward),
  m_yfactor(-1.0f),
  m_y_factor_positive(false)
{}

WRATHFormatter::pen_position_return_type 
WRATHFormattedTextStream::
set_text(WRATHFormatter::handle fmt, 
         const WRATHTextData &raw_data,
         const WRATHStateStream &state_stream)
{
  WRATHassert(fmt.valid());

  m_orientation=fmt->screen_orientation();
  m_y_factor_positive=(m_orientation==WRATHFormatter::y_increases_upward);
  m_yfactor= (m_y_factor_positive)?1.0f:-1.0f;

  m_data.clear();
  return fmt->format_text(raw_data, state_stream, m_data, m_eols);
}

ivec2
WRATHFormattedTextStream::
texture_coordinate(int i, enum WRATHFormattedTextStream::corner_type ct,
                   enum WRATHTextureFont::texture_coordinate_size L) const
{
  const int getx_fast_table[4]=
    {
      0, //bottom_left_corner
      1, //bottom_right_corner
      1, //top_right_corner
      0, //top_left_corner      
    };

  const int gety_fast_table[4]=
    {
      0, //bottom_left_corner
      0, //bottom_right_corner
      1, //top_right_corner
      1, //top_left_corner      
    };

  WRATHassert(ct<4);
  WRATHassert(ct>=0);

  vecN<ivec2,2> ts(texture_coordinate(i,L));

  return ivec2( ts[ getx_fast_table[ct] ].x(),
                ts[ gety_fast_table[ct] ].y());
}

vecN<ivec2, 2>
WRATHFormattedTextStream::
texture_coordinate(int i,
                   enum WRATHTextureFont::texture_coordinate_size L) const
{
  const WRATHFormattedTextStream::glyph_instance &G(data(i));
  
  WRATHassert(data(i).m_glyph!=NULL);
  const WRATHTextureFont::glyph_data_type &ch(*G.m_glyph);
  

  return vecN<ivec2, 2>(ch.texel_lower_left(L),
                        ch.texel_upper_right(L));
}

vec2
WRATHFormattedTextStream::
position(int i, enum corner_type ct, vec2 scale_factor,
         enum WRATHTextureFont::texture_coordinate_size L) const
{
  const int getx_fast_table[4]=
    {
      0, //bottom_left_corner
      1, //bottom_right_corner
      1, //top_right_corner
      0, //top_left_corner      
    };

  const int gety_fast_table[4]=
    {
      0, //bottom_left_corner
      0, //bottom_right_corner
      1, //top_right_corner
      1, //top_left_corner      
    };

  WRATHassert(ct<4);
  WRATHassert(ct>=0);

  vecN<vec2,2> pp(position(i,scale_factor,L));

  return vec2( pp[ getx_fast_table[ct] ].x(),
               pp[ gety_fast_table[ct] ].y());
  
  
}

vecN<vec2,2>
WRATHFormattedTextStream::
position(int i, vec2 scale_factor,
         enum WRATHTextureFont::texture_coordinate_size L) const
{
  const WRATHFormattedTextStream::glyph_instance &G(data(i));
  
  WRATHassert(data(i).m_glyph!=NULL);
  const WRATHTextureFont::glyph_data_type &ch(*G.m_glyph);

  vec2 bl(G.m_position.x()+scale_factor.x()*ch.origin(L).x(),
          G.m_position.y()+m_yfactor*scale_factor.y()*ch.origin(L).y());
  vec2 tr;

  tr=bl + vec2(scale_factor.x()*ch.display_size(L).x(), 
               scale_factor.y()*m_yfactor*ch.display_size(L).y());

  return vecN<vec2,2>(bl,tr);
}
