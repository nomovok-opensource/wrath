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
  
          

  class character_attribute:
    public WRATHInterleavedAttributes<position_type, //position -- 0
                                      glyph_stretch_type, //stretch -- 1
                                      glyph_size_type, //glyph_size -- 2
                                      glyph_bottom_left_type, //glyph bottom left -- 3
                                      glyph_normalized_coordinate_type, //glyph_normalized -- 4
                                      color_type //color --5
				      >  
  {
  public:

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

  template<unsigned int N>
  class character_attribute_with_custom
  {
  public:
    character_attribute m_base;
    GLfloat m_custom[N];
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
      };
    static const_c_array<attribute_label_type> R(attribute_labels, 6);
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
  WRATHGenericTextAttributePacker(packer_label(subpacker),
                                  subpacker)
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

size_t
WRATHDefaultTextAttributePacker::
attribute_size(int n) const
{
  return (n==0)?
    sizeof(character_attribute):
    sizeof(character_attribute_with_custom<1>) + sizeof(float)*(n-1);
}

#define CHECK_SIZE(i) WRATHassert((i)==0 or sizeof(character_attribute_with_custom<(i)>)==attribute_size((i)))
#define CHECK_SIZE_GRP(n) \
  CHECK_SIZE(4*n); \
  CHECK_SIZE(4*n+1); \
  CHECK_SIZE(4*n+2); \
  CHECK_SIZE(4*n+3); 

void
WRATHDefaultTextAttributePacker::
attribute_names(std::vector<std::string> &out_names, int number_custom_data) const
{
  CHECK_SIZE(0);
  CHECK_SIZE(1);
  CHECK_SIZE(2);
  CHECK_SIZE(3);
  CHECK_SIZE(4);
  CHECK_SIZE(5);
  CHECK_SIZE(6);

  unsigned int N, R;

  N=number_custom_data/4;
  R=number_custom_data%4;
  if(R>0) 
    {
      ++N;
    }
  
  out_names.resize(packer_attribute_names().size()+N);
  std::copy(packer_attribute_names().begin(),
	    packer_attribute_names().end(),
	    out_names.begin());
  for(unsigned int i=0, k=packer_attribute_names().size(); i<N; ++i, ++k)
    {
      std::ostringstream ostr;
      ostr << "custom_data" << i;
      out_names[k]=ostr.str();
    }
}


void
WRATHDefaultTextAttributePacker::
generate_custom_data_glsl(WRATHGLShader::shader_source &out_src,
			  int number_custom_data_to_use) const
{
  int N, R, idx;
  const char *swizzle[]={".x", ".y", ".z", ".w" };
  std::ostringstream ostr;

  N=number_custom_data_to_use/4;
  R=number_custom_data_to_use%4;
  for(int i=0; i<N; ++i)
    {
      ostr << "\nshader_in highp vec4 custom_data" << i << ";";
    }
  
  if(R==1)
    {
      ostr << "\nshader_in highp float custom_data" << N << ";";
    }
  else if(R>1)
    {
      ostr << "\nshader_in highp vec" << R << " custom_data" << N << ";";
    }
  
  /*
    create the function that returns the data as an array
  */
  ostr << "\nvoid wrath_font_shader_custom_data_func(out wrath_font_custom_data_t v)"
       << "\n{";
  
  idx=0;
  for(int i=0; i<N; ++i)
    {
      for(int j=0; j<4; ++j, ++idx)
	{
	  ostr << "\n\tv.values[" << idx << "]=" 
	       << "custom_data" << i << swizzle[j] << ";";
	}
    }

  if(R==1)
    {
      ostr << "\n\tv.values[" << idx 
	   << "]=custom_data" << N << ";";
    }
  else
    {
      for(int j=0; j<R; ++j, ++idx)
	{
	  ostr << "\n\tv.values[" << idx << "]=" 
	       << "custom_data" << N << swizzle[j] << ";";
	}
    }
  ostr << "\n}\n";

  out_src.add_source(ostr.str(), WRATHGLShader::from_string);
}
  
void
WRATHDefaultTextAttributePacker::
attribute_key(WRATHAttributeStoreKey &pkey,
	      int number_custom_floats) const
{
  pkey
    .type_and_format(type_tag<character_attribute>());

  pkey.m_attribute_format_location[color_location].m_normalized=GL_TRUE;
  pkey.m_attribute_format_location[glyph_normalized_coordinate_location].m_normalized=GL_TRUE;

   if(number_custom_floats!=0)
    {
      ptrdiff_t offset;
      const char *p1, *p2;
      character_attribute_with_custom<1> conveniance;
      int attr_slot, num_remaining;

      /*
	we are going to potentially unsafely assume that
	sizeof(character_attribute_with_custom<N+1>) is
	same as sizeof(character_attribute_with_custom<N>) + 4
	for N>=1
       */
      pkey.m_type_size = sizeof(character_attribute_with_custom<1>)
	+ sizeof(float)*(number_custom_floats-1);

      p1=reinterpret_cast<const char*>(boost::addressof(conveniance));
      p2=reinterpret_cast<const char*>(boost::addressof(conveniance.m_custom[0]));
      offset=p2-p1;
      num_remaining=number_custom_floats;

      attr_slot=character_attribute::number_attributes;
      /*
	every 4 custom values adds a new vec4 attribute
       */      
      for(;num_remaining>=4 and attr_slot<WRATHDrawCallSpec::attribute_count;
	  num_remaining-=4, ++attr_slot, offset+=4*sizeof(float))
	{
	  pkey.m_attribute_format_location[attr_slot].m_offset=offset;
	  pkey.m_attribute_format_location[attr_slot].traits( type_tag<vec4>() );
	}
      /*
	remaining values use a float, vec2, or vec3
       */
      if(attr_slot<WRATHDrawCallSpec::attribute_count)
	{
	  WRATHassert(num_remaining<4);
	  switch(num_remaining)
	    {
	    case 0:
	      break;

	    case 1:
	      pkey.m_attribute_format_location[attr_slot].m_offset=offset;
	      pkey.m_attribute_format_location[attr_slot].traits( type_tag<float>() );
	      ++attr_slot;
	      break;

	    case 2:
	      pkey.m_attribute_format_location[attr_slot].m_offset=offset;
	      pkey.m_attribute_format_location[attr_slot].traits( type_tag<vec2>() );
	      ++attr_slot;
	      break;

	    case 3:
	      pkey.m_attribute_format_location[attr_slot].m_offset=offset;
	      pkey.m_attribute_format_location[attr_slot].traits( type_tag<vec3>() );
	      ++attr_slot;
	      break;
	    }
	}

      //now adjust the stride:
      for(int i=0; i<attr_slot; ++i)
	{
	  pkey.m_attribute_format_location[i].m_stride = pkey.m_type_size;	    
	}
    }
}





void
WRATHDefaultTextAttributePacker::
pack_attribute(enum WRATHFormattedTextStream::corner_type ct,
               const glyph_data &in_glyph,
               const vec2 &normalized_glyph_coordinate_float,
               vecN<GLshort,2> normalized_glyph_coordinate_short,
	       const std::vector<int> &custom_data_use,
               c_array<uint8_t> packing_destination,
               const PackerState&) const
{
  c_array<character_attribute> attr;

  attr=packing_destination
    .sub_array(0, sizeof(character_attribute))
    .reinterpret_pointer<character_attribute>();
  
  ivec2 native_bl(in_glyph.m_glyph->texel_lower_left());
  ivec2 native_sz(in_glyph.m_glyph->texel_size());

  attr[0].position()=position_type(in_glyph.m_native_position[0].x(), 
				   in_glyph.m_native_position[0].y(), 
				   in_glyph.m_z_position, 
				   in_glyph.m_scale);
  attr[0].glyph_stretch()=glyph_stretch_type(in_glyph.m_horizontal_stretching,
					     in_glyph.m_vertical_stretching);
  attr[0].glyph_size()=glyph_size_type(native_sz.x(), native_sz.y());
  attr[0].glyph_bottom_left()=glyph_bottom_left_type(native_bl.x(), native_bl.y());  
  attr[0].glyph_normalized_coordinate()=normalized_glyph_coordinate_short;
  
  if(ct==WRATHFormattedTextStream::not_corner)
    {
      attr[0].color()=interpolate_color(in_glyph.m_color,
					       normalized_glyph_coordinate_float);
    }
  else
    {
      attr[0].color()=in_glyph.m_color[ct];
    }

  if(!custom_data_use.empty())
    {
      c_array<character_attribute_with_custom<1> > attr_with;
      attr_with=packing_destination
	.sub_array(0, sizeof(character_attribute_with_custom<1>))
	.reinterpret_pointer<character_attribute_with_custom<1> >();

      WRATHassert(&attr_with[0].m_base==&attr[0]);

      for(int i=0, endi=custom_data_use.size(); i<endi; ++i)
	{
	  attr_with[0].m_custom[i]=in_glyph.m_glyph->fetch_custom_float(custom_data_use[i]);
	}
    }
}









  
