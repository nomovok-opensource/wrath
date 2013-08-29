/*! 
 * \file WRATHColumnFormatter.cpp
 * \brief file WRATHColumnFormatter.cpp
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
#include "WRATHColumnFormatter.hpp"
#include "WRATHTextDataStreamManipulator.hpp"

namespace
{
  class contraint_sorter_forward
  {
  public:
    bool
    operator()(const WRATHColumnFormatter::Constraint &lhs,
               const WRATHColumnFormatter::Constraint &rhs) const
    {
      return lhs.m_begin<rhs.m_begin;
    }

  };

  class contraint_sorter_reverse
  {
  public:
    bool
    operator()(const WRATHColumnFormatter::Constraint &lhs,
               const WRATHColumnFormatter::Constraint &rhs) const
    {
      return lhs.m_begin>rhs.m_begin;
    }

  };

  vec2 
  factor(const WRATHColumnFormatter::LayoutSpecification &L)
  {
    vec2 R(1.0f, 1.0f);

    if(L.m_screen_orientation==WRATHFormatter::y_increases_upward)
      {
        R.y()*=-1.0f;
      }

    if(L.m_pen_advance.y()==WRATHFormatter::decrease_coordinate)
      {
        R.y()*=-1.0f;
      }

    if(L.m_pen_advance.x()==WRATHFormatter::decrease_coordinate)
      {
        R.x()*=-1.0f;
      }
    

    return R;
  }

  class letter
  {
  public:
    letter(void):
      m_position(0),
      m_end(0),
      m_descend(0),
      m_ascend(0),
      m_offset(0),
      m_gl(NULL)
    {}

    float m_position;
    float m_end;
    float m_descend, m_ascend;
    float m_offset;
    const WRATHTextureFont::glyph_data_type *m_gl;

  };

}


std::ostream&
operator<<(std::ostream &ostr, const WRATHColumnFormatter::Constraint &obj)
{
  ostr << "{ begin=" << obj.m_begin;
  if(obj.m_constraint.first)
    {
      ostr << ", constraint=" << obj.m_constraint.second;
    }
  else
    {
      ostr << ", unconstrained";
    }
  ostr << "} ";
  
  return ostr;
}

////////////////////////////////////////////
// WRATHColumnFormatter methods
WRATHColumnFormatter::
WRATHColumnFormatter(const WRATHColumnFormatter::LayoutSpecification &L):
  m_layout(L),
  m_advance_character_index(m_layout.m_text_orientation),
  m_advance_line_index(1-m_advance_character_index),
  m_factor(factor(m_layout))
{
  //sort the constaints based of the pen advance mode:
  if(m_layout.m_pen_advance[m_advance_line_index]==increase_coordinate)
    {
      std::sort(m_layout.m_begin_line_constraints.begin(),
                m_layout.m_begin_line_constraints.end(),
                contraint_sorter_forward());

      std::sort(m_layout.m_end_line_constraints.begin(),
                m_layout.m_end_line_constraints.end(),
                contraint_sorter_forward());
    }
  else
    {
      std::sort(m_layout.m_begin_line_constraints.begin(),
                m_layout.m_begin_line_constraints.end(),
                contraint_sorter_reverse());

      std::sort(m_layout.m_end_line_constraints.begin(),
                m_layout.m_end_line_constraints.end(),
                contraint_sorter_reverse());
    }
}

WRATHColumnFormatter::
~WRATHColumnFormatter()
{}
  
void
WRATHColumnFormatter::
reset(void)
{
  m_pen_position=m_layout.m_start_position;
  m_current_max_descend=0.0f;
  m_current_max_ascend=0.0f;
  m_newline_space=0.0f;
  m_tab_width=0.0f;
  m_space_width=0.0f;
  m_font_scale=1.0f;
  m_scaled_factor=m_factor;
  m_last_character_advance=0.0f;
  m_font=NULL;
  m_previous_glyph=std::make_pair(m_font, WRATHTextureFont::glyph_index_type());
  m_line_empty=true;
  m_last_eol_idx=0;
  m_base_line_offset=vec2(0.0f, 0.0f);
  m_added_line=m_layout.m_add_leading_eol;

  m_begin_line_constraint_iter=m_layout.m_begin_line_constraints.begin();
  m_end_line_constraint_iter=m_layout.m_end_line_constraints.begin();

  m_begin_line_current_value=std::make_pair(true, m_pen_position[m_advance_character_index]);
  m_end_line_current_value=std::make_pair(false, 0.0f);

  increment_contraints();

  m_pen_position[m_advance_character_index]=m_begin_line_current_value.second;
}

void
WRATHColumnFormatter::
increment_contraints(void)
{
  increment_contraint(m_begin_line_constraint_iter, 
                      m_begin_line_current_value,
                      m_layout.m_begin_line_constraints.end());

  increment_contraint(m_end_line_constraint_iter,
                      m_end_line_current_value,
                      m_layout.m_end_line_constraints.end());

 
}

void
WRATHColumnFormatter::
increment_contraint(std::vector<Constraint>::iterator &iter,
                    std::pair<bool, float> &update_value,
                    std::vector<Constraint>::iterator end)
{
  //increment iterator to the last iterator
  //that affect this position.
  std::vector<Constraint>::iterator prev(iter);

  for(;iter!=end and constraint_in_affect(iter->m_begin); ++iter)
    {
      prev=iter;
    }
  
  iter=prev;
  if(iter!=end)
    {
      update_value=iter->m_constraint;
    }
}



bool
WRATHColumnFormatter::
constraint_in_affect(float begin)
{
  return 
    (m_factor[m_advance_line_index]<0.0f and begin>m_pen_position[m_advance_line_index])
    or
    (m_factor[m_advance_line_index]>0.0f and begin<m_pen_position[m_advance_line_index]);
}

bool
WRATHColumnFormatter::
require_new_line(void)
{
  return 
    m_end_line_current_value.first 

    and

    (
     
     (m_factor[m_advance_character_index]>0.0f 
     and m_pen_position[m_advance_character_index]>m_end_line_current_value.second)
     
     or
     
     (m_factor[m_advance_character_index]<0.0f 
      and m_pen_position[m_advance_character_index]<m_end_line_current_value.second)
     );
    
}

void
WRATHColumnFormatter::
add_new_line(std::vector<WRATHFormatter::glyph_instance> &out_data,
             std::vector<std::pair<int, LineData> > &out_eols,
             int flags)
{
  LineData L(m_last_eol_idx, out_data.size());
  float moveby_line(0), moveby_char(0);

  L.m_max_ascend=m_current_max_ascend;
  L.m_max_descend=m_current_max_descend;

  if(m_added_line)
    {
      if(m_line_empty)
        {
          moveby_line=m_scaled_factor[m_advance_line_index]*m_newline_space;
        }
      else if(m_factor[m_advance_line_index]>0.0)
        {
          moveby_line=m_current_max_ascend;
        }
      else
        {
          moveby_line=-m_current_max_descend;
        }
    }

  float slack(0.0f);
  vecN<float, 3> choice_maker(0.0f, 0.0f, 0.0f);

  if(m_end_line_current_value.first and L.m_range.m_end>L.m_range.m_begin)
    {
      slack=m_end_line_current_value.second 
        - out_data[L.m_range.m_end-1].m_position[m_advance_character_index];

            
      slack=m_factor[m_advance_character_index]
        * std::max(0.0f, m_factor[m_advance_character_index]*slack);
      choice_maker[align_text_begin]=0.0f;
      choice_maker[align_text_end]=slack;
      choice_maker[align_center]=slack/2.0f;
    }

  moveby_char=choice_maker[m_layout.m_alignment];
  
  for(int i=L.m_range.m_begin; i<L.m_range.m_end; ++i)
    {
      out_data[i].m_position[m_advance_line_index]+=moveby_line;
      out_data[i].m_position[m_advance_character_index]+=moveby_char;
    }
  
  if(!m_line_empty and L.m_range.m_begin!=L.m_range.m_end)
    {
      L.m_pen_position_start=out_data[L.m_range.m_begin].m_position;
    }
  else
    {
      L.m_pen_position_start=m_pen_position;
    }

  L.m_pen_position_end=L.m_pen_position_start;
  L.m_pen_position_end[m_advance_character_index]
    = m_pen_position[m_advance_character_index] + moveby_char;
  
  if(m_added_line)
    {
      if(m_line_empty)
        {
          m_pen_position[m_advance_line_index]+=moveby_line;
        }
      else
        {
          m_pen_position[m_advance_line_index]
            +=m_factor[m_advance_line_index]*m_current_max_ascend;
        }
    }

  if(flags&record_eol)
    {
      out_eols.push_back(std::make_pair(m_last_eol_idx, L));
    }

  if(flags&advance_pen_to_next_line)
    {
      m_pen_position[m_advance_line_index]
        +=m_factor[m_advance_line_index]*(m_layout.m_line_spacing+m_current_max_descend);


      increment_contraints();
      m_pen_position[m_advance_character_index]=m_begin_line_current_value.second;
      m_current_max_descend=0.0f;
      m_current_max_ascend=0.0f;
      m_line_empty=true;
      m_last_eol_idx=out_data.size();
    }

  m_added_line=true;
}

enum WRATHFormatter::screen_orientation_type
WRATHColumnFormatter::
screen_orientation(void)
{
  return m_layout.m_screen_orientation;
}


WRATHFormatter::pen_position_return_type 
WRATHColumnFormatter::
format_text(const WRATHTextData &raw_data,
            const WRATHStateStream &state_stream,
            std::vector<WRATHFormatter::glyph_instance> &out_data,
            std::vector<std::pair<int, LineData> > &out_eols)
{
  
  
  WRATHText::effective_scale::stream_iterator effective_scale_pair;
  WRATHText::baseline_shift_x::stream_iterator baseline_pair_x;
  WRATHText::baseline_shift_y::stream_iterator baseline_pair_y;
  WRATHText::kerning::stream_iterator kerning_pair;
  WRATHText::horizontal_stretching::stream_iterator horizontal_stretch_pair;
  WRATHText::vertical_stretching::stream_iterator vertical_stretch_pair;
  WRATHText::word_spacing::stream_iterator word_spacing_pair;
  WRATHText::letter_spacing::stream_iterator letter_spacing_pair;
  WRATHText::letter_spacing_type::stream_iterator letter_spacing_type_pair;
  bool kerning_enabled(true);
  pen_position_return_type return_value;
  std::vector<letter> current_word;
  ivec2 kern_ivec2;
  float kern;
  float word_spacing(0.0f);
  vec2 horiz_vert_stretch(1.0f, 1.0f);
  float &horiz_stretch(horiz_vert_stretch[0]);
  float &vert_stretch(horiz_vert_stretch[1]);
  bool last_character_is_white_space(!m_layout.m_word_space_on_line_begin);
  bool word_present_on_line(false);
  float letter_spacing(0.0f);
  enum WRATHText::letter_spacing_e letter_spacing_type(WRATHText::letter_spacing_absolute);

  reset();
  m_last_eol_idx=out_data.size();
  
  m_font_scale=WRATHText::effective_scale::init_stream_iterator(state_stream, 0, effective_scale_pair);
  m_font=effective_scale_pair.font();

  kerning_enabled=WRATHText::kerning::init_stream_iterator(state_stream, 0, kerning_enabled, kerning_pair);
  word_spacing=WRATHText::word_spacing::init_stream_iterator(state_stream, 0, 
                                                             word_spacing, 
                                                             word_spacing_pair);

  
  letter_spacing
    =WRATHText::letter_spacing::init_stream_iterator(state_stream, 0, 
                                                     letter_spacing, 
                                                     letter_spacing_pair);

  
  letter_spacing_type
    =WRATHText::letter_spacing_type::init_stream_iterator(state_stream, 0, 
                                                          letter_spacing_type, 
                                                          letter_spacing_type_pair);
 

  horiz_stretch
    =WRATHText::horizontal_stretching::init_stream_iterator(state_stream, 0, 
                                                            horiz_stretch,
                                                            horizontal_stretch_pair);
  vert_stretch
    =WRATHText::vertical_stretching::init_stream_iterator(state_stream, 0, 
                                                          vert_stretch, 
                                                          vertical_stretch_pair);
  

  m_base_line_offset.x()=WRATHText::baseline_shift_x::init_stream_iterator(state_stream,
                                                                           0, m_base_line_offset.x(), 
                                                                           baseline_pair_x);

  m_base_line_offset.y()=WRATHText::baseline_shift_y::init_stream_iterator(state_stream,
                                                                           0, m_base_line_offset.y(), 
                                                                           baseline_pair_y);

  m_newline_space=(m_font!=NULL)?m_font->new_line_height():0.0f;
  m_tab_width=(m_font!=NULL)?m_font->tab_width():0.0f;
  m_space_width=(m_font!=NULL)?m_font->space_width():0.0f;
  m_scaled_factor=m_font_scale*m_factor;

  WRATHText::baseline_shift_x::update_value_from_change(0, m_base_line_offset.x(), baseline_pair_x);
  WRATHText::baseline_shift_y::update_value_from_change(0, m_base_line_offset.y(), baseline_pair_y);
 
  for(int loc=0, 
        end=raw_data.character_data().size();   
      loc<end; ++loc)
    {
      bool word_ends(false), add_eol(false), word_break_ok(false);
      WRATHTextData::character ch(raw_data.character_data(loc));
      const WRATHTextureFont::glyph_data_type *gl(NULL);
      letter new_letter;
      bool is_control_space_char(false);

      //font changes 
      if(WRATHText::effective_scale::update_value_from_change(loc, m_font_scale, effective_scale_pair))
        {
          m_font=effective_scale_pair.font();
          m_newline_space=(m_font!=NULL)?m_font->new_line_height():0.0f;
          m_tab_width=(m_font!=NULL)?m_font->tab_width():0.0f;
          m_scaled_factor=m_font_scale*m_factor;
        }
            
      std::pair<WRATHTextureFont*, WRATHTextureFont::glyph_index_type> G(m_font, ch.glyph_index());

      WRATHText::kerning::update_value_from_change(loc, kerning_enabled, kerning_pair);
      WRATHText::word_spacing::update_value_from_change(loc, word_spacing, word_spacing_pair);
      WRATHText::letter_spacing::update_value_from_change(loc, letter_spacing, letter_spacing_pair);
      WRATHText::letter_spacing_type::update_value_from_change(loc, letter_spacing_type, 
                                                               letter_spacing_type_pair);
      
      WRATHText::horizontal_stretching::update_value_from_change(loc, horiz_stretch, horizontal_stretch_pair);
      WRATHText::vertical_stretching::update_value_from_change(loc, vert_stretch, vertical_stretch_pair);
      /*
        If there are state changes that trigger an
        end of word, then when noted they need to
        "print the word" to out_data before the code
        below as the code below advances the pen, etc.
       */
       
      if(!G.second.valid())
        {
          if(m_layout.m_ignore_control_characters)
            {
              G=m_font->glyph_index_meta(ch.character_code());
            }
          else
            {
              switch(ch.character_code().m_value)
                {
                case '\t':
                  m_pen_position[m_advance_character_index]+=
                    m_scaled_factor[m_advance_character_index]*m_tab_width;
                  word_ends=true;
                  is_control_space_char=true;
                  break;
                  
                case '\n':
                  add_eol=true;
                  word_ends=true;
                  is_control_space_char=true;
                  break;
                  
                default:
                  G=m_font->glyph_index_meta(ch.character_code());
                }
            }
        }
      
      if(G.second.valid() and G.first!=NULL)
        {
          gl=&G.first->glyph_data(G.second);
        }

      if(gl==NULL or gl->texel_size()==ivec2(0,0))
        {
          word_break_ok=m_layout.m_empty_glyph_word_break
	    or m_layout.m_break_words
            or (m_layout.m_word_breakers.find(ch)!=m_layout.m_word_breakers.end());
        }
      else
        {
          word_break_ok=m_layout.m_break_words
	    or (m_layout.m_word_breakers.find(ch)!=m_layout.m_word_breakers.end());
        }
      
      /*
	word_ends means that a word really ended, so add word_spacing
        should we add \t to the check? other white spaces? maybe
        just a function to say if something is a white space?
       */
      word_ends=word_ends or ch==WRATHTextData::character(' ');
      if(!is_control_space_char and !last_character_is_white_space and word_ends)
	{
	  m_pen_position[m_advance_character_index]+=word_spacing;
	}


      if(kerning_enabled and G.first==m_previous_glyph.first and G.second.valid() and m_previous_glyph.second.valid())
        {
          if(m_layout.m_pen_advance[m_advance_character_index]==WRATHFormatter::decrease_coordinate)
            {
              kern_ivec2=m_font->kerning_offset(G.second, m_previous_glyph.second);
            }
          else
            {
              kern_ivec2=m_font->kerning_offset(m_previous_glyph.second, G.second);
            }
          kern=static_cast<float>(kern_ivec2[m_advance_character_index])/64.0f;
        }
      else
        {
          kern_ivec2=ivec2(0,0);
          kern=0.0f;
        }

      m_pen_position[m_advance_character_index]+=m_scaled_factor[m_advance_character_index]*kern;
      

      new_letter.m_position=m_pen_position[m_advance_character_index];
      new_letter.m_ascend=0.0f;
      new_letter.m_descend=0.0f;
      new_letter.m_offset=m_base_line_offset[m_advance_line_index];
      new_letter.m_gl=gl;

      
      //baseline offset changes, notice that we
      //update it here and to the next location.
      //this is because we need to modify the
      //advance of the pen if the offset changes.
      //Specifically, if the offset changes,
      //we will then use the bounding box
      //of the glyph to move the pen.
      vecN<bool,2> offset_change;
      offset_change.x()
        =WRATHText::baseline_shift_x::update_value_from_change(loc+1, 
                                                               m_base_line_offset.x(),
                                                               baseline_pair_x);
      
      offset_change.y()
        =WRATHText::baseline_shift_y::update_value_from_change(loc+1,
                                                               m_base_line_offset.y(),
                                                               baseline_pair_y);

      if(gl!=NULL)
        {
          vec2 orig(gl->origin()*horiz_vert_stretch);
          vec2 bb_size(gl->bounding_box_size()*horiz_vert_stretch);
          vec2 bb(orig+bb_size);
          float adv(gl->advance()[m_advance_character_index]*horiz_vert_stretch[m_advance_character_index]);
          float advance(0.0f);

	  /*
	    empty glyph means white space as far as we are concerned.
	   */
	  if(!last_character_is_white_space 
	     or !m_layout.m_eat_white_spaces 
	     or gl->texel_size()!=ivec2(0,0))
	    {
	      if(offset_change[m_advance_line_index])
		{
		  advance=m_scaled_factor[m_advance_character_index]*bb[m_advance_character_index];
		}
	      else
		{
		  advance=m_scaled_factor[m_advance_character_index]*adv;
		}
	      
	      if(letter_spacing_type==WRATHText::letter_spacing_absolute)
		{
		  advance+=letter_spacing;
		}
	      else
		{
		  advance+=letter_spacing*bb_size[m_advance_line_index];
		}
	    }
            
          m_pen_position[m_advance_character_index]+=advance;

          if(horiz_vert_stretch[m_advance_line_index]>0.0f)
            {
              new_letter.m_descend=-m_font_scale*orig[m_advance_line_index];          
              new_letter.m_ascend=m_font_scale*bb[m_advance_line_index];
            }
          else
            {
              new_letter.m_ascend=m_font_scale*orig[m_advance_line_index]; 
              new_letter.m_descend=-m_font_scale*bb[m_advance_line_index];
            }

          m_current_max_descend
            =std::max(m_current_max_descend, new_letter.m_descend + new_letter.m_offset);
          
          m_current_max_ascend
            =std::max(m_current_max_ascend, new_letter.m_ascend - new_letter.m_offset);

          m_line_empty=false;
        }

      new_letter.m_end=m_pen_position[m_advance_character_index];        
      current_word.push_back(new_letter);

      last_character_is_white_space=(gl==NULL or gl->texel_size()==ivec2(0,0));

      if(require_new_line())
        {
          if(!word_present_on_line)
            {
              for(std::vector<letter>::const_iterator
                    letter_iter=current_word.begin(), 
                    letter_end=current_word.end();
                  letter_iter!=letter_end; ++letter_iter)
                {
                  WRATHFormatter::glyph_instance c;
                  
                  c.m_position[m_advance_character_index]=letter_iter->m_position;              
                  c.m_position[m_advance_line_index]
                    =m_pen_position[m_advance_line_index] + letter_iter->m_offset;
                  c.m_glyph=letter_iter->m_gl;
                  
                  out_data.push_back(c);
                }
              current_word.clear();
            }


          add_new_line(out_data, out_eols, advance_pen_to_next_line|record_eol);
          word_present_on_line=false;

          if(!current_word.empty())
            {
              float offset_by;

              m_line_empty=false;
              offset_by=m_pen_position[m_advance_character_index]
                - current_word.front().m_position;

              for(std::vector<letter>::iterator
                    letter_iter=current_word.begin(), 
                    letter_end=current_word.end();
                  letter_iter!=letter_end; ++letter_iter)
                {
                  letter_iter->m_position+=offset_by;
                  letter_iter->m_end+=offset_by;

                   m_current_max_descend
                     =std::max(m_current_max_descend, 
                               letter_iter->m_descend + letter_iter->m_offset);
          
                   m_current_max_ascend
                     =std::max(m_current_max_ascend, 
                               letter_iter->m_ascend - letter_iter->m_offset);
                }

              m_pen_position[m_advance_character_index]
                =current_word.back().m_end;
              last_character_is_white_space=false;
            }
          else
            {
              last_character_is_white_space=!m_layout.m_word_space_on_line_begin;
            }
        }

      
          
      if(offset_change[m_advance_line_index])
        {
          m_previous_glyph=std::make_pair(m_font, WRATHTextureFont::glyph_index_type());
        }
      else
        {
          m_previous_glyph=G;
        }

      if(word_break_ok)
        {
          for(std::vector<letter>::const_iterator
                letter_iter=current_word.begin(), 
                letter_end=current_word.end();
              letter_iter!=letter_end; ++letter_iter)
            {
              WRATHFormatter::glyph_instance c;

              c.m_position[m_advance_character_index]=letter_iter->m_position;              
              c.m_position[m_advance_line_index]
                =m_pen_position[m_advance_line_index] + letter_iter->m_offset;
              c.m_glyph=letter_iter->m_gl;

              out_data.push_back(c);
            }
          word_present_on_line=true;
          current_word.clear();
        }

      if(add_eol)
        {
          add_new_line(out_data, out_eols, advance_pen_to_next_line|record_eol);
          if(!current_word.empty())
            {
              float offset_by;

              m_line_empty=false;
              offset_by=m_pen_position[m_advance_character_index]
                - current_word.front().m_position;

              for(std::vector<letter>::iterator
                    letter_iter=current_word.begin(), 
                    letter_end=current_word.end();
                  letter_iter!=letter_end; ++letter_iter)
                {
                  letter_iter->m_position+=offset_by;
                  letter_iter->m_end+=offset_by;
                }

              m_pen_position[m_advance_character_index]
                =current_word.back().m_end;
            }
          else
            {
              last_character_is_white_space=true;
            }
        }

    }

  for(std::vector<letter>::const_iterator
        letter_iter=current_word.begin(), 
        letter_end=current_word.end();
      letter_iter!=letter_end; ++letter_iter)
    {
      WRATHFormatter::glyph_instance c;
      
      c.m_position[m_advance_character_index]=letter_iter->m_position;
      c.m_position[m_advance_line_index]
        = m_pen_position[m_advance_line_index] + letter_iter->m_offset;
      c.m_glyph=letter_iter->m_gl;

      out_data.push_back(c);
    }

  //push down the last line of text by the anount needed
  //to fit the line, also record the EOL.
  add_new_line(out_data, out_eols, record_eol);
  return_value.m_exact_pen_position=m_pen_position;
  
  //now move the pen to the start of the next line:
  m_pen_position[m_advance_line_index]
    +=m_factor[m_advance_line_index]*(m_layout.m_line_spacing+m_current_max_descend);
  increment_contraints();
  m_pen_position[m_advance_character_index]=m_begin_line_current_value.second;
  return_value.m_descend_start_pen_position=m_pen_position;


  return return_value;
}
