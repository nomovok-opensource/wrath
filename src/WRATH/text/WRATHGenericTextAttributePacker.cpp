/*! 
 * \file WRATHGenericTextAttributePacker.cpp
 * \brief file WRATHGenericTextAttributePacker.cpp
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
#include <limits>
#include "WRATHGenericTextAttributePacker.hpp"

namespace
{
  
  
  

  vecN<GLshort,2>
  compute_normalized_coordinate_short(bool y_factor_positive,
                                      const vec2 &glyph_coord)
  {
    const float value_max(std::numeric_limits<GLshort>::max());
    const float value_min(std::numeric_limits<GLshort>::min());

    vec2 v(value_max, y_factor_positive?value_max:value_min);
    vec2 as_float;

    as_float=v*glyph_coord;
    return vecN<GLshort,2>(static_cast<GLshort>(as_float.x()),
                           static_cast<GLshort>(as_float.y()));
  }

  inline
  int
  length_of_range(const range_type<int> &R)
  {
    return R.m_end-R.m_begin;
  }

  inline
  c_array<uint8_t>
  get_pointer(const range_type<int> &R,
              WRATHAbstractDataSink &buffer,
              size_t attr_size)
  {
    return buffer.pointer<uint8_t>(R.m_begin*attr_size,
                                   length_of_range(R)*attr_size);
  }

  inline
  c_array<uint8_t>
  get_attribute_reference(int attr_index,
                          c_array<uint8_t> attr_pointer,
                          size_t attribute_size)
  {
    return attr_pointer.sub_array(attr_index*attribute_size, attribute_size);
  }
}

/////////////////////////////////////////
//WRATHGenericTextAttributePacker methods
WRATHGenericTextAttributePacker::PackerState
WRATHGenericTextAttributePacker::
begin_range(const range_type<int>&,
            WRATHTextureFont*,
            int,
            const WRATHFormattedTextStream&,
            const WRATHStateStream&) const
{
  return PackerState();
}

void
WRATHGenericTextAttributePacker::
current_glyph(const glyph_data&,
              const WRATHFormattedTextStream&,
              const WRATHStateStream&,
              const PackerState&) const
{
}

void
WRATHGenericTextAttributePacker::
end_range(const PackerState&,
          const range_type<int>&,
          WRATHTextureFont*,
          int,
          const WRATHFormattedTextStream&,
          const WRATHStateStream&) const
{
}


WRATHTextAttributePacker::allocation_allotment_type
WRATHGenericTextAttributePacker::
allocation_allotment(int attributes_allowed,
                     const_c_array<range_type<int> > Rarray,
                     const WRATHFormattedTextStream &pdata,
                     const WRATHStateStream &) const
{
  allocation_allotment_type return_value;
  if(m_type==SubPrimitivePacker)
    {
      for(const_c_array<range_type<int> >::iterator 
            iter=Rarray.begin(), end=Rarray.end(); 
          iter!=end and return_value.m_room_for_all;  ++iter)
        {
          for(int I=iter->m_begin; I!=iter->m_end and return_value.m_room_for_all; ++I)
            {
              const WRATHTextureFont::glyph_data_type *G(pdata.data(I).m_glyph);

              if(G!=NULL)
                {
                  int num_for_glyph;

                  num_for_glyph=G->sub_primitive_attributes().size();
                  if(num_for_glyph<=attributes_allowed)
                    {
                      attributes_allowed-=num_for_glyph;
                    }
                  else
                    {
                      return_value.m_room_for_all=false;
                      return_value.m_sub_end=I;
                    }
                }
            }
          
          if(return_value.m_room_for_all)
            {
              ++return_value.m_handled_end;
            }
        }
    }
  else
    {
      int glyphs_allowed(attributes_allowed>>2); //4 attributes per glyph
    
      for(const_c_array<range_type<int> >::iterator 
            iter=Rarray.begin(), end=Rarray.end(); 
          iter!=end and return_value.m_room_for_all;  ++iter)
        {
          int count;

          count=iter->m_end - iter->m_begin;
          if(count<=glyphs_allowed)
            {
              return_value.m_number_attributes+=(count<<2);
              ++return_value.m_handled_end;
              glyphs_allowed-=count;
            }
          else
            {              
              return_value.m_number_attributes+=(glyphs_allowed<<2);
              return_value.m_room_for_all=false;
              return_value.m_sub_end=iter->m_begin + glyphs_allowed;              
            }
        }
    }

  return return_value;
}

WRATHTextAttributePacker::allocation_requirement_type
WRATHGenericTextAttributePacker::
allocation_requirement(const_c_array<range_type<int> > Rarray,
                       WRATHTextureFont *font,
                       int texture_page,
                       const WRATHFormattedTextStream &pdata,
                       const WRATHStateStream&) const
{
  allocation_requirement_type return_value;

  if(m_type==SubPrimitivePacker)
    {
      for(const_c_array<range_type<int> >::iterator iter=Rarray.begin(),
            end=Rarray.end(); iter!=end; ++iter)
        {
          for(int c=iter->m_begin; c<iter->m_end; ++c)
            {
              const WRATHTextureFont::glyph_data_type *G(pdata.data(c).m_glyph);
              if(G!=NULL and font==G->font() and texture_page==G->texture_page())
                {
                  if(G->support_sub_primitives())
                    {
                      return_value.m_number_attributes+=G->sub_primitive_attributes().size();
                      return_value.m_number_indices+=G->sub_primitive_indices().size();
                    }
                  else
                    {
                      return_value.m_number_attributes+=4;
                      return_value.m_number_indices+=6;
                    }
                }
            }
        }
    }
  else
    {
      unsigned int number_chars;  
      number_chars=
        WRATHTextAttributePacker::number_of_characters(Rarray.begin(), Rarray.end(), 
                                                       pdata, font, texture_page); 
      
      return_value.m_number_attributes=4*number_chars;
      return_value.m_number_indices=6*number_chars;

    }

  return return_value;
}

void
WRATHGenericTextAttributePacker::
set_attribute_data_implement(const_c_array<range_type<int> > Rarray,
                             WRATHTextureFont *font,
                             int texture_page,
                             WRATHAbstractDataSink &attribute_store, 
                             const std::vector<range_type<int> > &attr_location,
                             WRATHAbstractDataSink &index_group,
                             const WRATHFormattedTextStream &pdata,
                             const WRATHStateStream &state_stream,
                             BBox *out_bounds_box) const
{
  const GLushort quad[6]=
    {
      0,1,2,
      0,2,3,
    };


  const GLshort value_max(std::numeric_limits<GLshort>::max());
  const GLshort value_min(std::numeric_limits<GLshort>::min());
  allocation_requirement_type AA(allocation_requirement(Rarray, font, texture_page, pdata, state_stream));
  WRATHassert(static_cast<unsigned int>(AA.m_number_attributes)<=WRATHAttributeStore::total_size(attr_location));

  if(AA.m_number_attributes==0 or AA.m_number_indices==0)
    {
      return;
    }

  const vecN<GLshort,2> normalized_value_positive[4]=
    {
      /*[bottom_left_corner]= */ vecN<GLshort,2>(0, 0), 
      /*[bottom_right_corner]=*/ vecN<GLshort,2>(value_max, 0),
      /*[top_right_corner]=   */ vecN<GLshort,2>(value_max, value_max), 
      /*[top_left_corner]=    */ vecN<GLshort,2>(0, value_max), 
    };

  const vecN<GLshort,2> normalized_value_negative[4]=
    {
      /*[bottom_left_corner]= */ vecN<GLshort,2>(0, 0), 
      /*[bottom_right_corner]=*/ vecN<GLshort,2>(value_max, 0),
      /*[top_right_corner]=   */ vecN<GLshort,2>(value_max, value_min), 
      /*[top_left_corner]=    */ vecN<GLshort,2>(0, value_min), 
    };

  const vec2 float_normalized_coordinate_positive[4]=
    {
      /*[bottom_left_corner]= */ vec2(0.0f, 0.0f),
      /*[bottom_right_corner]=*/ vec2(1.0f, 0.0f),
      /*[top_right_corner]=   */ vec2(1.0f, 1.0f),
      /*[top_left_corner]=    */ vec2(0.0f, 1.0f)
    };

  const vec2 float_normalized_coordinate_negative[4]=
    {
      /*[bottom_left_corner]= */ vec2(0.0f,  0.0f),
      /*[bottom_right_corner]=*/ vec2(1.0f,  0.0f),
      /*[top_right_corner]=   */ vec2(1.0f, -1.0f),
      /*[top_left_corner]=    */ vec2(0.0f, -1.0f)
    };

  const vec2 *use_float_normalized_coordinate;
  const vecN<GLshort,2> *use_normalzied_array;
  c_array<GLushort> indices;
  c_array<uint8_t> attrs;
  const_c_array< range_type<int> >::iterator char_range_iter, char_range_end;
  std::vector<range_type<int> >::const_iterator attr_range_iter, attr_range_end;
  unsigned int current_attr_index(0);
  int indx(0), total_attribute_count(0);
  std::vector<GLushort> index_remapper;
  bool y_factor_positive;
  PackerState packer_state;
  const std::vector<int> &custom_data_use(font->glyph_glsl()->m_custom_data_use);
  int num_customs(font->glyph_glsl()->m_custom_data_use.size());
  size_t sattr_size(this->attribute_size(num_customs));

  y_factor_positive=pdata.y_factor_positive();

  use_normalzied_array=(y_factor_positive)?
    normalized_value_positive:
    normalized_value_negative;

  use_float_normalized_coordinate=(y_factor_positive)?
    float_normalized_coordinate_positive:
    float_normalized_coordinate_negative;

  /*
    Lock before aquiring the pointers:
   */
  WRATHAutoLockMutex(attribute_store.mutex());
  WRATHAutoLockMutex(index_group.mutex());

  indices=index_group.pointer<GLushort>(0, AA.m_number_indices);
  if(!attr_location.empty())
    {
      attrs=get_pointer(attr_location[0], attribute_store, sattr_size);
    }

  
  for(char_range_iter=Rarray.begin(), 
        char_range_end=Rarray.end(),
        attr_range_iter=attr_location.begin(),
        attr_range_end=attr_location.end();
      char_range_iter!=char_range_end and attr_range_iter!=attr_range_end; 
      ++char_range_iter)
    {
      range_type<int> R(*char_range_iter);
      glyph_data the_glyph;
      WRATHText::color_bottom_left::stream_iterator color_bottom_left_stream;
      WRATHText::color_bottom_right::stream_iterator color_bottom_right_stream;
      WRATHText::color_top_left::stream_iterator color_top_left_stream;
      WRATHText::color_top_right::stream_iterator color_top_right_stream;
      WRATHText::z_position::stream_iterator zposition_stream;
      WRATHText::effective_scale::stream_iterator scale_stream;
      WRATHText::horizontal_stretching::stream_iterator horizontal_stretch_stream;
      WRATHText::vertical_stretching::stream_iterator vertical_stretch_stream;
      float scale;
     

      the_glyph.m_color[WRATHFormattedTextStream::bottom_left_corner]
        =WRATHText::color_bottom_left::init_stream_iterator(state_stream,
                                                            R.m_begin, 
                                                            the_glyph.m_color[WRATHFormattedTextStream::bottom_left_corner], 
                                                            color_bottom_left_stream);
     
      the_glyph.m_color[WRATHFormattedTextStream::bottom_right_corner]
        =WRATHText::color_bottom_right::init_stream_iterator(state_stream,
                                                             R.m_begin, 
                                                             the_glyph.m_color[WRATHFormattedTextStream::bottom_right_corner], 
                                                             color_bottom_right_stream);

      the_glyph.m_color[WRATHFormattedTextStream::top_right_corner]
        =WRATHText::color_top_right::init_stream_iterator(state_stream,
                                                          R.m_begin, 
                                                          the_glyph.m_color[WRATHFormattedTextStream::top_right_corner], 
                                                          color_top_right_stream);

      the_glyph.m_color[WRATHFormattedTextStream::top_left_corner]
        =WRATHText::color_top_left::init_stream_iterator(state_stream,
                                                         R.m_begin, 
                                                         the_glyph.m_color[WRATHFormattedTextStream::top_left_corner], 
                                                         color_top_left_stream);

      the_glyph.m_z_position
        =WRATHText::z_position::init_stream_iterator(state_stream,
                                                     R.m_begin, the_glyph.m_z_position, 
                                                     zposition_stream);
      scale
        =WRATHText::effective_scale::init_stream_iterator(state_stream,
                                                          R.m_begin, scale_stream);
      

      the_glyph.m_horizontal_stretching=
        WRATHText::horizontal_stretching::init_stream_iterator(state_stream,
                                                               R.m_begin, the_glyph.m_horizontal_stretching,
                                                               horizontal_stretch_stream);

      the_glyph.m_vertical_stretching=
        WRATHText::vertical_stretching::init_stream_iterator(state_stream,
                                                             R.m_begin, the_glyph.m_vertical_stretching,
                                                             vertical_stretch_stream);
      
      packer_state=begin_range(R, font, texture_page, pdata, state_stream);

      /*
        for each character, pack the attribute data
        that makes the primitives of the character
        and the index data.
       */
      for(the_glyph.m_index=R.m_begin; 
          the_glyph.m_index<R.m_end and attr_range_iter!=attr_range_end; 
          ++the_glyph.m_index)
        {
          

          the_glyph.m_character_data=&pdata.data(the_glyph.m_index);
          the_glyph.m_glyph=the_glyph.m_character_data->m_glyph;
          
          

          WRATHText::z_position::update_value_from_change(the_glyph.m_index, 
                                                          the_glyph.m_z_position, zposition_stream);

          WRATHText::effective_scale::update_value_from_change(the_glyph.m_index, 
                                                               scale, scale_stream);

          the_glyph.m_scale=scale;


          WRATHText::horizontal_stretching::update_value_from_change(the_glyph.m_index, 
                                                                     the_glyph.m_horizontal_stretching, 
                                                                     horizontal_stretch_stream);

          WRATHText::vertical_stretching::update_value_from_change(the_glyph.m_index, 
                                                                   the_glyph.m_vertical_stretching, 
                                                                   vertical_stretch_stream);

          WRATHText::color_bottom_left::update_value_from_change(the_glyph.m_index, 
                                                                 the_glyph.m_color[WRATHFormattedTextStream::bottom_left_corner], 
                                                                 color_bottom_left_stream);

          WRATHText::color_bottom_right::update_value_from_change(the_glyph.m_index, 
                                                                 the_glyph.m_color[WRATHFormattedTextStream::bottom_right_corner], 
                                                                 color_bottom_right_stream);

          WRATHText::color_top_right::update_value_from_change(the_glyph.m_index, 
                                                               the_glyph.m_color[WRATHFormattedTextStream::top_right_corner], 
                                                               color_top_right_stream);

          WRATHText::color_top_left::update_value_from_change(the_glyph.m_index, 
                                                              the_glyph.m_color[WRATHFormattedTextStream::top_left_corner], 
                                                              color_top_left_stream);

          current_glyph(the_glyph, pdata, state_stream, packer_state);

          if(the_glyph.m_glyph!=NULL 
             and the_glyph.m_glyph->texture_page()==texture_page 
             and font==the_glyph.m_glyph->font())
            {              
              /*
                TODO: should we set flags to check if 
                the_glyph.m_scale*vec2(the_glyph.m_horizontal_stretching,
                                       the_glyph.m_vertical_stretching)

                needs to recomputed? since these values change quite rarely.                                       
               */
              the_glyph.m_native_position=pdata.position(the_glyph.m_index, 
                                                         the_glyph.m_scale*vec2(the_glyph.m_horizontal_stretching,
                                                                                the_glyph.m_vertical_stretching), 
                                                         WRATHTextureFont::native_value);

              if(out_bounds_box!=NULL)
                {
                  out_bounds_box->set_or(the_glyph.m_native_position[0]);
                  out_bounds_box->set_or(the_glyph.m_native_position[1]);
                }

              if(m_type==SubPrimitivePacker and the_glyph.m_glyph->support_sub_primitives())
                {
                  index_remapper.resize(the_glyph.m_glyph->sub_primitive_attributes().size());

                  for(int k=0, end_k=the_glyph.m_glyph->sub_primitive_attributes().size();
                        attr_range_iter!=attr_range_end and k<end_k; ++k)
                    {
                      index_remapper[k]=current_attr_index + attr_range_iter->m_begin;
                      
                      const WRATHTextureFont::sub_primitive_attribute &tt(the_glyph.m_glyph->sub_primitive_attributes()[k]);
                      vec2 signed_normalized(tt.m_position_within_glyph_coordinate);

                      signed_normalized.x()=tt.m_position_within_glyph_coordinate.x();
                      signed_normalized.y()=(y_factor_positive)?
                        tt.m_position_within_glyph_coordinate.y():
                        -tt.m_position_within_glyph_coordinate.y();

                      pack_attribute(WRATHFormattedTextStream::not_corner,
                                     the_glyph,
                                     signed_normalized,
                                     compute_normalized_coordinate_short(y_factor_positive, 
                                                                         tt.m_position_within_glyph_coordinate),
				     custom_data_use,
                                     get_attribute_reference(current_attr_index, 
                                                             attrs, sattr_size),
                                     packer_state);

                      ++total_attribute_count;
                      ++current_attr_index;
                      if(current_attr_index*sattr_size==attrs.size())
                        {
                          ++attr_range_iter;
                          if(attr_range_iter!=attr_range_end)
                            {
                              current_attr_index=0;
                              attrs=get_pointer(*attr_range_iter, 
                                                attribute_store,
                                                sattr_size);
                            }
                        }
                    }

                  for(int k=0, end_k=the_glyph.m_glyph->sub_primitive_indices().size(); k<end_k; ++k, ++indx)
                    {
                      indices[indx]=index_remapper[the_glyph.m_glyph->sub_primitive_indices()[k] ];
                    }

                }
              else
                {
                  vecN<int, 4> quad_indices;

                  for(int k=0; k<4 and attr_range_iter!=attr_range_end; ++k)
                    {
                      quad_indices[k]=current_attr_index + attr_range_iter->m_begin;


                      pack_attribute(static_cast<WRATHFormattedTextStream::corner_type>(k),
                                     the_glyph,
                                     use_float_normalized_coordinate[k],
                                     use_normalzied_array[k],
				     custom_data_use,
                                     get_attribute_reference(current_attr_index, 
                                                             attrs, 
                                                             sattr_size),
                                     packer_state);

                      ++total_attribute_count;
                      ++current_attr_index;
                      if(current_attr_index*sattr_size==attrs.size())
                        {
                          ++attr_range_iter;
                          if(attr_range_iter!=attr_range_end)
                            {
                              current_attr_index=0;
                              attrs=get_pointer(*attr_range_iter, 
                                                attribute_store, 
                                                sattr_size);
                            }
                        }

                    } //of for(int k=0; ...)

                  /*
                    now the index data:
                  */
                  for(int ii=0;ii<6;++ii)
                    {
                      indices[ii+indx]=quad_indices[ quad[ii] ];
                    }
                  indx+=6;

                } //of else


            } //of if(the_glyph.m_glyph!=NULL and ..

        } //of for(the_glyph.m_index=R.m_begin...

      end_range(packer_state, R, font, texture_page, pdata, state_stream);

    } //of for(char_range_iter=Rarray.begin(), ...)
  
  
}
