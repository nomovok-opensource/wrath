/*! 
 * \file WRATHDefaultTextAttributePacker.cpp
 * \brief file WRATHDefaultTextAttributePacker.cpp
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
#include "WRATHDefaultTextAttributePacker.hpp"
#include "WRATHInterleavedAttributes.hpp"
#include "WRATHCanvas.hpp"

namespace
{
  typedef vec4 position_type;
  typedef vec2 glyph_stretch_type;
  typedef vecN<GLushort,2> glyph_size_type;
  typedef vecN<GLushort,2> glyph_bottom_left_type;
  typedef vecN<GLshort,2> glyph_normalized_coordinate_type;
  typedef vecN<GLubyte,4> color_type;
  typedef GLfloat custom_glyph_data_type;
  
          

  class character_attribute:
    public WRATHInterleavedAttributes<position_type, //position -- 0
                                      glyph_stretch_type, //stretch -- 1
                                      glyph_size_type, //glyph_size -- 2
                                      glyph_bottom_left_type, //glyph bottom left -- 3
                                      glyph_normalized_coordinate_type, //glyph_normalized -- 4
                                      color_type, //color --5
                                      custom_glyph_data_type //custom_data --6
                                      >  
  {
  public:
    
    custom_glyph_data_type&
    custom_glyph_data(void)
    {
      return get<WRATHDefaultTextAttributePacker::custom_data_location>();
    }

    glyph_normalized_coordinate_type&
    glyph_normalized_coordinate(void)
    {
      return get<WRATHDefaultTextAttributePacker::glyph_normalized_coordinate_location>();
    }

    color_type&
    color(void)
    {
      return get<WRATHDefaultTextAttributePacker::color_location>();
    }

    glyph_bottom_left_type&
    glyph_bottom_left(void)
    {
      return get<WRATHDefaultTextAttributePacker::glyph_bottom_left_texel_location>();
    }

    glyph_size_type&
    glyph_size(void)
    {
      return get<WRATHDefaultTextAttributePacker::glyph_size_location>();
    }
    
    position_type&
    position(void)
    {
      return get<WRATHDefaultTextAttributePacker::position_location>();
    }

    glyph_stretch_type&
    glyph_stretch(void)
    {
      return get<WRATHDefaultTextAttributePacker::glyph_stretch_location>();
    }

    void
    position_xy(const vec2 &v)
    {
      position().x()=v.x();
      position().y()=v.y();
    }
    
  };
  
  typedef WRATHDefaultTextAttributePacker* WRATHDefaultTextAttributePackerPtr;
  WRATHDefaultTextAttributePackerPtr&
  the_ptr(int e)
  {
    static vecN<WRATHDefaultTextAttributePackerPtr,2> R(NULL, NULL);
    return R[e];
  }

  typedef const char *attribute_label_type;
  
  const_c_array<attribute_label_type>
  packer_attribute_names(void)
  {
    static const attribute_label_type attribute_labels[]=
      {
        "pos",
        "glyph_stretch",
        "glyph_size",
        "glyph_bottom_left_texel",
        "glyph_normalized_coordinate", 
        "color",
	/*
	  Danger: Must make this match with the generated
	  GLSL found in WRATHFontShaderSpecifier::fetch_texture_font_drawer()
	 */
        "custom_data0",
      };
    static const_c_array<attribute_label_type> R(attribute_labels, 7);
    return R;
  }

  const char*
  packer_label(enum WRATHGenericTextAttributePacker::PackerType subpacker)
  {
    return subpacker==WRATHGenericTextAttributePacker::SubPrimitivePacker?
      "WRATHDefaultTextAttributePacker-SubPrimitives":
      "WRATHDefaultTextAttributePacker-FullQuad";
  }

                               
  color_type
  interpolate_color(const vecN<WRATHText::color_type, 4> &input_color,
                    vec2 glyph_coord)
  {
    color_type R;

    glyph_coord.y()=std::abs(glyph_coord.y());

    // why is .x not abs'd as well?
    //glyph_coord.x()=std::abs(glyph_coord.x());

    /*
      Do bilinear interpolation..
    */
    for(int I=0; I<4; ++I)
      {
        vec4 values_at_corner(input_color[0][I], //bottom_left_corner
                              input_color[1][I], //bottom_right_corner
                              input_color[2][I], //top_right_corner
                              input_color[3][I]);//top_left_corner 

        float bottom, top, v;

        bottom= values_at_corner[0] + glyph_coord.x()*( values_at_corner[1]-values_at_corner[0]);
        top=values_at_corner[3] + glyph_coord.x()*( values_at_corner[2]-values_at_corner[3]);

        v=bottom + glyph_coord.y()*(top-bottom);
        R[I]=static_cast<WRATHText::color_type::value_type>(v);
      }

    return R;
  }
}

WRATHDefaultTextAttributePacker::
WRATHDefaultTextAttributePacker(enum PackerType subpacker):
  WRATHGenericTextAttributePacker(sizeof(character_attribute),
                                  packer_label(subpacker),
                                  subpacker,
                                  packer_attribute_names().begin(), 
                                  packer_attribute_names().end())
{
  WRATHassert(the_ptr(subpacker)==NULL);
  the_ptr(subpacker)=this;
}

WRATHDefaultTextAttributePacker::
~WRATHDefaultTextAttributePacker()
{
  WRATHassert(the_ptr(type())==this);
  the_ptr(type())=NULL;
}

WRATHDefaultTextAttributePacker*
WRATHDefaultTextAttributePacker::
fetch(enum PackerType e)
{
  if(the_ptr(e)==NULL)
    {
      WRATHNew WRATHDefaultTextAttributePacker(e);
    }
  WRATHassert(the_ptr(e)!=NULL);
  return the_ptr(e);
}

void
WRATHDefaultTextAttributePacker::
attribute_key(WRATHAttributeStoreKey &pkey) const
{
  pkey
    .type_and_format(type_tag<character_attribute>());

  pkey.m_attribute_format_location[color_location].m_normalized=GL_TRUE;
  pkey.m_attribute_format_location[glyph_normalized_coordinate_location].m_normalized=GL_TRUE;
}





void
WRATHDefaultTextAttributePacker::
pack_attribute(enum WRATHFormattedTextStream::corner_type ct,
               const glyph_data &in_glyph,
               const vec2 &normalized_glyph_coordinate_float,
               vecN<GLshort,2> normalized_glyph_coordinate_short,
               c_array<uint8_t> packing_destination,
               const PackerState&) const
{
  c_array<character_attribute> attr;

  attr=packing_destination.reinterpret_pointer<character_attribute>();
  
  ivec2 native_bl(in_glyph.m_glyph->texel_lower_left(WRATHTextureFont::native_value));
  ivec2 native_sz(in_glyph.m_glyph->texel_size(WRATHTextureFont::native_value));

  attr[0].position()=position_type(in_glyph.m_native_position[0].x(), 
                                   in_glyph.m_native_position[0].y(), 
                                   in_glyph.m_z_position, 
                                   in_glyph.m_scale);
  attr[0].glyph_stretch()=glyph_stretch_type(in_glyph.m_horizontal_stretching,
                                             in_glyph.m_vertical_stretching);
  attr[0].glyph_size()=glyph_size_type(native_sz.x(), native_sz.y());
  attr[0].glyph_bottom_left()=glyph_bottom_left_type(native_bl.x(), native_bl.y());  
  attr[0].glyph_normalized_coordinate()=normalized_glyph_coordinate_short;
  attr[0].custom_glyph_data()=in_glyph.m_glyph->fetch_custom_float(0);
  
  
  if(ct==WRATHFormattedTextStream::not_corner)
    {
      attr[0].color()=interpolate_color(in_glyph.m_color,
                                        normalized_glyph_coordinate_float);
    }
  else
    {
      attr[0].color()=in_glyph.m_color[ct];
    }
}









  
