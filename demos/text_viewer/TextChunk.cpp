/*! 
 * \file TextChunk.cpp
 * \brief file TextChunk.cpp
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
#include "TextChunk.hpp"
#include "FilePacket.hpp"
#include "WRATHInterleavedAttributes.hpp"
#include "WRATHLayerItemDrawerFactory.hpp"
#include "NodePacker.hpp"


namespace
{
  class line_attr:public WRATHInterleavedAttributes<vec2, vecN<GLubyte,4> >
  {
  public:

    vec2&
    position(void)
    {
      return get<underline_pos_location>();
    }

    vecN<GLubyte,4>&
    color(void)
    {
      return get<underline_color_location>();
    }
  };

  typedef const char *cstring;
  
  const_c_array<cstring>
  line_attributes(void)
  {
    static const cstring R[]=
      {
        "pos",
        "color"
      };
    static const_c_array<cstring> v(R, 2);

    return v;
  }


  class LineAttributePacker:public WRATHAttributePacker
  {
  public:

    static
    LineAttributePacker*
    fetch(void)
    {
      if(the_ptr()==NULL)
        {
          WRATHNew LineAttributePacker();
        }
      return the_ptr();
    }

  private:
    LineAttributePacker(void):
      WRATHAttributePacker("line_attribute_packer",
                           line_attributes().begin(), line_attributes().end())
    {
      the_ptr()=this;
    }

    ~LineAttributePacker()
    {
      the_ptr()=NULL;
    }

    static
    LineAttributePacker*&
    the_ptr(void)
    {
      static LineAttributePacker *R(NULL);
      return R;
    }

  };

  


}

////////////////////////
// TextChunk::LinePacketData methods
TextChunk::LinePacketData::
LinePacketData(void):
  m_has_underlines(false),
  m_attribute_data_location(0, 0),
  m_number_attributes(0)
{}

TextChunk::LinePacketData::
~LinePacketData()
{
  if(m_has_underlines)
    {
      m_index_data_location.delete_group();
      m_item_group.deallocate_attribute_data(m_attribute_data_location);
      
      m_item_group.release_group();
    }
}


////////////////////////////
// TextChunk methods
TextChunk::
TextChunk(range_type<int> R,
          const WRATHFormattedTextStream &ptext,
          const WRATHStateStream &state_stream,
          WRATHLayer *pparent, 
          FilePacket *fpacket,
          WRATHLayerItemNodeRotateTranslate *text_transformation_node)
{
  m_sub=WRATHNew WRATHLayer(pparent);
  m_sub->simulation_matrix(WRATHLayer::modelview_matrix, float4x4());
  m_sub->simulation_composition_mode(WRATHLayer::modelview_matrix, WRATHLayer::compose_matrix);
  m_sub->simulation_matrix(WRATHLayer::projection_matrix, float4x4());
  m_sub->simulation_composition_mode(WRATHLayer::projection_matrix, WRATHLayer::compose_matrix);
    
    
  WRATHLayer::SubKey sk(text_transformation_node);

  m_text_item=WRATHNew 
    WRATHTextItem(NodePacker::Factory(), 0, //factory jazz
                  m_sub, sk, //subkey and itemgroup
                  fpacket->text_item_opacity_type(),
                  fpacket->texture_font_drawer(), //how to draw
                  WRATHTextItem::draw_order(), //draworder jazz
                  fpacket->extra_state().m_text_extra_state); //extra state

  m_text_item->add_text(R, ptext, state_stream);


  m_bbox=m_text_item->bounding_box();

  WRATHItemDrawer *drawer;

  drawer=
    fpacket->misc_drawers().m_line_drawer->fetch_drawer(NodePacker::Factory(),
                                                        LineAttributePacker::fetch(),
                                                        0);

  
  add_underlines(R, ptext, state_stream, 
                 pparent, 
                 fpacket->extra_state().m_line_extra_state,
                 drawer,
                 text_transformation_node);
  
}

TextChunk::
~TextChunk()
{
  WRATHPhasedDelete(m_text_item);      
}



void
TextChunk::
add_underlines(range_type<int> R,
               const WRATHFormattedTextStream &ptext,
               const WRATHStateStream &state_stream,
               WRATHLayer *pparent,
               const WRATHSubItemDrawState &pextra_state,
               WRATHItemDrawer *pline_drawer,
               WRATHLayerItemNodeRotateTranslate *text_transformation_node)
{
  
  std::list< range_type<int> > underline_ranges;
  std::list< range_type<int> > strike_though_ranges;
  std::list<per_line_data> underlines, strikethoughs;
  std::list<per_line_data> lines;
 

 

  find_line_ranges(strikethrough_stream_id,
                   R, state_stream, strike_though_ranges);

  compute_lines(ptext, state_stream, strike_though_ranges, strikethoughs);
  


  find_line_ranges(underline_stream_id,
                   R, state_stream, underline_ranges);
  compute_lines(ptext, state_stream, underline_ranges, underlines);

  //strike throughs y-coordinate is the middle of the line:
  for(std::list<per_line_data>::iterator iter=strikethoughs.begin(),
        end=strikethoughs.end(); iter!=end; ++iter)
    {
      //std::cout << (*iter);
      iter->m_y-= iter->m_max_ascend/2.0f;
    }


  lines.splice(lines.begin(), underlines);
  lines.splice(lines.begin(), strikethoughs);
  
  //now finally create the underline "objects"

  create_underlines(lines, pparent, pline_drawer, pextra_state,
                    text_transformation_node);
}


void
TextChunk::
find_line_ranges(int stream_id,
                 range_type<int> R,
                 const WRATHStateStream &state_stream,
                 std::list< range_type<int> > &out_line_ranges)
{
  bool current_line_value(false);
  int last_index(R.m_begin);
  std::list< range_type<int> > line_ranges;
  std::list<per_line_data> lines;

  typedef std::pair<int, line_stream_type> line_element;
  range_type<const_c_array<line_element>::iterator> line_iters;

  current_line_value
    =state_stream.get_iterator_range(R.m_begin, false, line_iters, stream_id);

  
  //first collect the underline_ranges:
  for(;line_iters.m_begin!=line_iters.m_end and last_index<R.m_end;
      ++line_iters.m_begin)
    {
      if(current_line_value and last_index<line_iters.m_begin->first)
        {
          range_type<int> rangeL;

          rangeL.m_begin=std::max(R.m_begin, last_index);
          rangeL.m_end=std::min(R.m_end, line_iters.m_begin->first);

          if(rangeL.m_begin<rangeL.m_end)
            {
              out_line_ranges.push_back(rangeL);
            }
        }
      current_line_value=line_iters.m_begin->second;
      last_index=line_iters.m_begin->first;
    }

}


void
TextChunk::
compute_lines(const WRATHFormattedTextStream &ptext,
              const WRATHStateStream &state_stream,
              const std::list< range_type<int> > &line_ranges,
              std::list<per_line_data> &lines)
{
  

  //any of the following force a break of
  //an underline:
  // EOL
  // font change
  // color change
  typedef const_c_array<std::pair<int, WRATHFormatter::LineData> > eol_array_type;
  typedef eol_array_type::iterator eol_iter_type;
    

  for(std::list< range_type<int> >::const_iterator
        iter=line_ranges.begin(), end=line_ranges.end();
      iter!=end; ++iter)
    {
      eol_array_type eol_array(ptext.eols());
      range_type<eol_iter_type> eol_iter_pair(eol_array.begin(), eol_array.end());
      WRATHFormatter::LineData L;
      WRATHText::scale::type sc(1.0f);
      vecN<GLubyte,4> color(FilePacket::link_color());
      bool found_non_white_char;
      WRATHText::effective_scale::stream_iterator sc_r;
      WRATHText::color_bottom_left::stream_iterator color_r;
      WRATHText::font::stream_iterator fnt_r;

      
      L=WRATHStateStream::sub_range(iter->m_begin, L, eol_iter_pair);

      sc=WRATHText::effective_scale::init_stream_iterator(state_stream, iter->m_begin, sc_r);
      color=WRATHText::color_bottom_left::init_stream_iterator(state_stream, iter->m_begin, color, color_r);

      

      per_line_data current;

      current.m_x.m_begin=ptext.data(iter->m_begin).m_position.x();
      current.m_y=ptext.data(iter->m_begin).m_position.y();
      current.m_color=color;
      current.m_character_range.m_begin=iter->m_begin;
      current.m_character_range.m_end=iter->m_begin;
      found_non_white_char=(ptext.data(iter->m_begin).m_glyph!=NULL
                            and ptext.data(iter->m_begin).m_glyph->texel_size()!=ivec2(0,0));

      for(int i=iter->m_begin; i<iter->m_end; ++i)
        {
          bool end_line(false);
          float prev_sc(sc);

          current.m_character_range.m_end=i;

          if(WRATHStateStream::update_value_from_change(i, L, eol_iter_pair))
            {
              if(found_non_white_char 
                 and current.m_character_range.m_begin<current.m_character_range.m_end)
                {
                  current.m_x.m_end=ptext.data(i-1).m_position.x();
                  if(ptext.data(i-1).m_glyph!=NULL)
                    {
                      float width(ptext.data(i-1).m_glyph->advance().x());
                      current.m_x.m_end+=prev_sc*width;
                    }
                  lines.push_back(current);
                  

                }
              current.m_x.m_begin=ptext.data(i).m_position.x();
              current.m_y=ptext.data(i).m_position.y();
              current.m_color=color;  
              current.m_max_ascend=0.0f;
              current.m_character_range.m_begin=i;
              found_non_white_char=false;
            }

          if(WRATHText::color_bottom_left::update_value_from_change(i, color, color_r))
            {
              end_line=true;
            }
          
          if(WRATHText::effective_scale::update_value_from_change(i, sc, sc_r))
            {
              end_line=true;
            }

          if(ptext.data(i).m_glyph==NULL
             or ptext.data(i).m_glyph->texel_size()==ivec2(0,0))
            {
              end_line=true;
            }

          if(end_line)
            {
              current.m_x.m_end=ptext.data(i).m_position.x();

              if(found_non_white_char 
                 and current.m_character_range.m_begin<current.m_character_range.m_end)
                {
                  lines.push_back(current);
                }

              current.m_character_range.m_begin=i;
              current.m_x.m_begin=ptext.data(i).m_position.x();
              current.m_color=color;
              current.m_max_ascend=0.0f;
              found_non_white_char=false;
            }

          if(!found_non_white_char
             and ptext.data(i).m_glyph!=NULL
             and ptext.data(i).m_glyph->texel_size()!=ivec2(0,0))
            {
              found_non_white_char=true;
              current.m_character_range.m_begin=i;
              current.m_x.m_begin=ptext.data(i).m_position.x();
            }

          if(ptext.data(i).m_glyph!=NULL)
            {
              const WRATHTextureFont::glyph_data_type *gl(ptext.data(i).m_glyph);
              float asc(gl->origin().y()+gl->bounding_box_size().y());
              current.m_max_ascend=std::max(current.m_max_ascend,
                                            sc*asc);
            }
        }

      if(current.m_character_range.m_begin<iter->m_end)
        {
          current.m_x.m_end=ptext.data(iter->m_end-1).m_position.x();
          current.m_character_range.m_end=iter->m_end;
          if(ptext.data(iter->m_end-1).m_glyph!=NULL)
            {
              float width(ptext.data(iter->m_end-1).m_glyph->advance().x());
              current.m_x.m_end+=sc*width;
            }
          lines.push_back(current);
        }
    }
 
}

void
TextChunk::
create_underlines(const std::list<per_line_data> &lines,
                  WRATHLayer *pparent,
                  WRATHItemDrawer *pline_drawer,
                  const WRATHSubItemDrawState &pextra_state,
                  WRATHLayerItemNodeRotateTranslate *text_transformation_node)
{

 

  if(pline_drawer==NULL or lines.empty())
    {
      return;
    }

  int number_lines(lines.size());

  WRATHAttributeStoreKey attr_key;

  attr_key.type(type_tag<line_attr>());

  line_attr::attribute_key(attr_key.m_attribute_format_location);
  attr_key.m_attribute_format_location[underline_color_location].m_normalized=true;
  
  
  WRATHAttributeStore::handle attr_handle;

  m_lines.m_number_attributes=4*number_lines;
  attr_handle=pparent->attribute_store(attr_key, 
                                       m_lines.m_number_attributes,
                                       m_lines.m_attribute_data_location);
  
  WRATHItemDrawState pkey(pline_drawer, GL_TRIANGLES);
  pkey
    .draw_type(WRATHDrawType::transparent_pass())
    .absorb(pextra_state);

  WRATHLayerItemNodeRotateTranslate *pnode;

  pnode=WRATHNew WRATHLayerItemNodeRotateTranslate(text_transformation_node);

  m_lines.m_has_underlines=true;
  m_lines.m_item_group=pparent->create(attr_handle, pkey, 
                                       WRATHLayer::SubKey(pnode));

  m_lines.m_item_group.set_implicit_attribute_data(m_lines.m_attribute_data_location);

  
  m_lines.m_index_data_location
    =m_lines.m_item_group.allocate_index_group<GLushort>(6*number_lines);


  c_array<GLushort> idx_ptr(m_lines.m_index_data_location.pointer());
  c_array<line_attr> attr_ptr;

  attr_ptr=m_lines.m_item_group.pointer<line_attr>(m_lines.m_attribute_data_location);

  std::list<per_line_data>::const_iterator iter, end;
  int current_line, current_idx, current_attr;


  
  WRATHBBox<2> pbox;
  for(iter=lines.begin(), end=lines.end(); iter!=end; ++iter)
    {
      vec2 a(iter->m_x.m_end, iter->m_y);
      vec2 b(iter->m_x.m_begin, iter->m_y+1.0f);

      pbox.set_or(a);
      pbox.set_or(b);
    }

  vec2 center;

  center=0.5f*(pbox.min_corner()+pbox.max_corner());
  pnode->translation(center);

  for(current_line=0, current_idx=0,
        current_attr=0,        
        iter=lines.begin(), 
        end=lines.end();
      iter!=end; ++iter, 
        ++current_line,
        current_attr+=4,
        current_idx+=6)
    {
      vec2 bl, tr;

      bl.x()=iter->m_x.m_begin;
      tr.x()=iter->m_x.m_end;

      bl.y()=iter->m_y+0.0f;
      tr.y()=iter->m_y+1.0f;

      bl-=center;
      tr-=center;

      for(int i=0;i<4;++i)
        {
          attr_ptr[i+current_attr].color()=iter->m_color;
        }

      attr_ptr[0+current_attr].position()=bl;
      attr_ptr[1+current_attr].position()=vec2(bl.x(), tr.y());
      attr_ptr[2+current_attr].position()=tr;
      attr_ptr[3+current_attr].position()=vec2(tr.x(), bl.y());

      idx_ptr[current_idx+0]=m_lines.m_attribute_data_location.m_begin+current_attr;
      idx_ptr[current_idx+1]=m_lines.m_attribute_data_location.m_begin+current_attr+1;
      idx_ptr[current_idx+2]=m_lines.m_attribute_data_location.m_begin+current_attr+2;

      idx_ptr[current_idx+3]=m_lines.m_attribute_data_location.m_begin+current_attr;
      idx_ptr[current_idx+4]=m_lines.m_attribute_data_location.m_begin+current_attr+2;
      idx_ptr[current_idx+5]=m_lines.m_attribute_data_location.m_begin+current_attr+3;
      
    }
}
