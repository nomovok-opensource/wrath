/*! 
 * \file WRATHTextItem.cpp
 * \brief file WRATHTextItem.cpp
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
#include "WRATHTextItem.hpp"

namespace
{
  
  typedef const_c_array< std::pair<int, WRATHText::additional_texture> >::iterator texture_iterator;

  typedef vecN<WRATHText::additional_texture, 
               WRATHText::number_additional_textures_supported> vec_array;

  class sub_range_key:
    public boost::tuple<WRATHTextureFont*, 
                        WRATHTextureFontDrawer*,
                        const WRATHTextAttributePacker*,
                        vec_array,
                        const WRATHFontShaderSpecifier*>
  {
  public:
    sub_range_key(const WRATHTextItem::Drawer &m,
                  WRATHTextureFont *fnt,
                  const WRATHItemDrawerFactory *fact,
                  int id,
                  const vec_array &v)
    {
      get<0>()=fnt;
      get<1>()=m.m_shader_specifier->fetch_texture_font_drawer(fnt, *fact, m.m_attribute_packer, id);
      get<2>()=m.m_attribute_packer;
      get<4>()=m.m_shader_specifier;

      for(std::map<unsigned int, std::string>::const_iterator 
            iter=m.m_shader_specifier->additional_samplers().begin(),
            end=m.m_shader_specifier->additional_samplers().end(); 
          iter!=end and iter->first<v.size(); ++iter)
        {
          get<3>()[iter->first]=v[iter->first];
        }
    }
  };
}




////////////////////////////////////////////////////////
// WRATHTextItem methods
WRATHTextItem::
WRATHTextItem(const WRATHItemDrawerFactory &fact,
              int psubdrawer_id,
              WRATHCanvas *pcontainer,
              const WRATHCanvas::SubKeyBase &subkey,
              enum WRATHTextItemTypes::text_opacity_t item_opacity,
              const Drawer &pdrawer,
              const draw_order &pdraw_order,
              const ExtraDrawState &extra_state):
  m_subkey(subkey.create_copy()),
  m_extra_state(extra_state),
  m_group(pcontainer),
  m_default_drawer(pdrawer),
  m_draw_order(pdraw_order),
  m_text_opacity(item_opacity),
  m_factory(fact.copy()),
  m_sub_drawer_id(psubdrawer_id)
{
  /*
    dump the texture jazz from m_extra_state
   */
  m_extra_state.opaque_pass_state().m_textures.clear();
  m_extra_state.translucent_pass_state().m_textures.clear();
  m_extra_state.m_common_pass_state.m_textures.clear();

}


WRATHTextItem::
~WRATHTextItem()
{
  for(std::list<WRATHBasicTextItem*>::iterator iter=m_all_items.begin(),
        end=m_all_items.end(); iter!=end; ++iter)
    {
      WRATHBasicTextItem *ptr(*iter);
      WRATHDelete(ptr);
    }
    
  WRATHDelete(m_factory);
  WRATHDelete(m_subkey);
}


void
WRATHTextItem::
clear(void)
{
  m_box.clear();

  /*
    We simply clear all text items:
   */
  for(std::map<text_item_key, std::list<WRATHBasicTextItem*> >::iterator
        map_iter=m_uncleared_items.begin(), map_end=m_uncleared_items.end(); 
      map_iter!=map_end; ++map_iter)
    {
      for(std::list<WRATHBasicTextItem*>::iterator iter=map_iter->second.begin(),
            end=map_iter->second.end(); iter!=end; ++iter)
        {
          WRATHBasicTextItem *ptr(*iter);
          ptr->clear();
        }
      std::list<WRATHBasicTextItem*> &cleared_list(m_cleared_items[map_iter->first]);
      cleared_list.splice(cleared_list.begin(), map_iter->second);
    }
}

WRATHBasicTextItem*
WRATHTextItem::
get_empty_text_item(WRATHTextItem::text_item_key k)
{
  std::map<text_item_key, std::list<WRATHBasicTextItem*> >::iterator iter;
  WRATHBasicTextItem *return_value;

  iter=m_cleared_items.find(k);
  if(iter!=m_cleared_items.end() and !iter->second.empty())
    {
      return_value=iter->second.back();
      iter->second.pop_back();
      m_uncleared_items[k].push_back(return_value);
    }
  else
    {
      ExtraDrawState item_extra_state(m_extra_state);
      int num_font_textures(k.get<1>()->glyph_glsl()->m_sampler_names.size());
      const std::map<unsigned int, std::string> &sampler_labels(k.get<3>()->additional_samplers());

      for(std::map<unsigned int, std::string>::const_iterator 
            iter=sampler_labels.begin(), end=sampler_labels.end();
          iter!=end; ++iter)
        {
          int i(iter->first), S(i+num_font_textures);
          const WRATHText::additional_texture &tex(k.get<2>()[i]);

          item_extra_state.m_common_pass_state.add_texture(S+GL_TEXTURE0, tex);
        }

      return_value=WRATHNew WRATHBasicTextItem(k.get<0>(), *m_subkey, m_group, k.get<1>(),
                                               m_text_opacity, 
                                               m_draw_order, item_extra_state);

      m_uncleared_items[k].push_back(return_value);
      m_all_items.push_back(return_value);
    }

  return return_value;
  
  
}

void
WRATHTextItem::
add_text(range_type<int> R, 
         const WRATHFormattedTextStream &ptext,
         const WRATHStateStream &state_stream)
{
  Drawer last_drawer, current_drawer(m_default_drawer);
  WRATHTextureFont *last_font, *current_font(NULL);
  WRATHText::font_shader::stream_iterator shader_stream;
  WRATHText::font_packer::stream_iterator packer_stream;
  vecN<range_type<texture_iterator>, WRATHText::number_additional_textures_supported> texture_stream;
  texture_array current_texture, last_texture;
  std::map<sub_range_key, std::vector< range_type<int> > > sub_ranges;
  int last_change_at;

  current_drawer.m_shader_specifier=
    WRATHText::font_shader::init_stream_iterator(state_stream,
                                                 R.m_begin,
                                                 current_drawer.m_shader_specifier,
                                                 shader_stream);

  current_drawer.m_attribute_packer=
    WRATHText::font_packer::init_stream_iterator(state_stream,
                                                 R.m_begin,
                                                 current_drawer.m_attribute_packer,
                                                 packer_stream);
  
  current_font=NULL;

  
  for(int T=0; T<WRATHText::number_additional_textures_supported; ++T)
    {
      current_texture[T]
        =state_stream.get_iterator_range(R.m_begin,
                                         current_texture[T],
                                         texture_stream[T],
                                         WRATHText::stream_id_additional_texture(T));
    }

  
  last_change_at=R.m_begin;

  last_drawer=current_drawer;
  last_font=current_font;
  last_texture=current_texture;


  for(int i=R.m_begin; i!=R.m_end; ++i)
    {
      bool bd, ba, bf, bt;

      bd=WRATHText::font_shader::update_value_from_change(i, current_drawer.m_shader_specifier,
                                                          shader_stream);

      ba=WRATHText::font_packer::update_value_from_change(i, current_drawer.m_attribute_packer,
                                                    packer_stream);

     
      const WRATHFormatter::glyph_instance &current_ch(ptext.data(i));
      if(current_ch.m_glyph!=NULL and current_ch.m_glyph->font()!=NULL)
        {
          current_font=current_ch.m_glyph->font();
        }
      bf=(current_font!=last_font);


      bt=false;
      for(int T=0; T<WRATHText::number_additional_textures_supported; ++T)
        {
          bool vv;

          vv=WRATHStateStream::update_value_from_change(i, 
                                                        current_texture[T], 
                                                        texture_stream[T]);
          bt=bt or (vv
                    and current_drawer.m_shader_specifier!=NULL
                    and current_drawer.m_shader_specifier->has_additional_sampler(T));
        }

      if(bd or ba or bf or bt)
        {
          if(i>last_change_at 
             and last_drawer.m_attribute_packer!=NULL
             and last_drawer.m_shader_specifier!=NULL
             and last_font!=NULL)
            {
              sub_range_key K(last_drawer, last_font, m_factory, m_sub_drawer_id, last_texture);
              sub_ranges[K].push_back(range_type<int>(last_change_at, i));
            }

          last_drawer=current_drawer;
          last_font=current_font;
          last_texture=current_texture;
          
          last_change_at=i;
        }
    }


  if(R.m_end>last_change_at 
     and last_drawer.m_attribute_packer!=NULL
     and last_drawer.m_shader_specifier!=NULL
     and last_font!=NULL)
    {
      sub_range_key K(last_drawer, last_font, m_factory, m_sub_drawer_id, last_texture);
      sub_ranges[K].push_back(range_type<int>(last_change_at, R.m_end));
    }


  for(std::map<sub_range_key, std::vector< range_type<int> > >::iterator
        iter=sub_ranges.begin(), end=sub_ranges.end(); iter!=end; ++iter)
    {
      WRATHBasicTextItem::draw_method D(iter->first.get<1>(), 
                                        iter->first.get<2>());

      add_text_implement(iter->second, 
                         ptext, state_stream, D, 
                         iter->first.get<0>(),
                         iter->first.get<3>(),
                         iter->first.get<4>());
    }
    

}


void
WRATHTextItem::
add_text_implement(c_array<range_type<int> > Rarray,
                   const WRATHFormattedTextStream &ptext,
                   const WRATHStateStream &state_stream,
                   WRATHBasicTextItem::draw_method pdrawer,
                   WRATHTextureFont *fnt,
                   const texture_array &texes,
                   const WRATHFontShaderSpecifier *spec)
{
  WRATHassert(pdrawer.m_program_drawer!=NULL);
  WRATHassert(pdrawer.m_attribute_packer!=NULL);
  WRATHassert(fnt!=NULL);

 

  /*
    we need to break Rarray into chunks small enough
    to fit within a single WRATHBasicTextItem.
    Comment: this is not exactly ideal, since
    a WRATHBasicTextItem might use a seperate
    attribute store for each texture page,
    but we are breaking the array of ranges
    into chunks based solely upon the font.
   */
  int current_attribute_count( (1<<16) - 1);
  int last_index(0), Rarray_size(Rarray.size());
  WRATHBasicTextItem *ptr(NULL);

  while(last_index<Rarray_size)
    {
      WRATHTextAttributePacker::allocation_allotment_type A;


      /*
        we pass the sub range of Rarray that is left to be handled,
        note that then the return value of A is relative to
        that subrange
       */
      A=pdrawer.m_attribute_packer->allocation_allotment(current_attribute_count,
                                                         Rarray.sub_array(last_index) , 
                                                         ptext, state_stream);      
      /*
        make the return value A relative to Rarray
       */
      A.m_handled_end+=last_index;


      current_attribute_count-=A.m_number_attributes;
      WRATHassert(current_attribute_count>=0);

      if(current_attribute_count<=0 
         or A.m_handled_end==Rarray_size 
         or !A.m_room_for_all)
        {
          /*
            we have that one baic text item can consume completely:
             Rarray[last_index], R[last_index+1], ... , R[A.m_handled_end-1]
            AND
            [ Rarray[A.m_handled_end].m_begin, A.m_sub_end)
           */
          c_array<range_type<int> > sub_range;
          range_type<int> *tweak_entry(NULL);
          range_type<int> entry_value_after;

          if(A.m_handled_end<Rarray_size)
            {
              /*
                modify Rarray[A.m_handled_end] to reflect the
                last one handled...
               */
              if(A.m_sub_end==Rarray[A.m_handled_end].m_end)
                {
                  ++A.m_handled_end;
                  sub_range=Rarray.sub_array( range_type<int>(last_index, A.m_handled_end));
                }
              else
                {
                  WRATHassert(A.m_sub_end<Rarray[A.m_handled_end].m_end);
                  WRATHassert(A.m_sub_end>=Rarray[A.m_handled_end].m_begin);

                  /*
                    change the entry at Rarray[A.m_handled_end]
                    and include that element in the sub_range
                   */
                  entry_value_after=range_type<int>(A.m_sub_end, 
                                                    Rarray[A.m_handled_end].m_end);
                  tweak_entry=&Rarray[A.m_handled_end];

                  Rarray[A.m_handled_end].m_end=A.m_sub_end;
                  sub_range=Rarray.sub_array( range_type<int>(last_index, A.m_handled_end+1));
                  
                }
            }
          else
            {
              sub_range=Rarray.sub_array( range_type<int>(last_index, Rarray_size));
            }

          ptr=get_empty_text_item(text_item_key(pdrawer, fnt, texes, spec));
          ptr->set_text(sub_range, ptext, state_stream);
          m_box.set_or(ptr->bounding_box());
          
          if(tweak_entry!=NULL)
            {
              *tweak_entry=entry_value_after;
            }

          last_index=A.m_handled_end;
          current_attribute_count= (1<<16) - 1;
        }
    }

  

 
}

void
WRATHTextItem::
canvas_base(WRATHCanvas *c)
{
  if(c==m_group)
    {
      return;
    }
  

  for(std::list<WRATHBasicTextItem*>::iterator iter=m_all_items.begin(),
        end=m_all_items.end(); iter!=end; ++iter)
    {
      WRATHBasicTextItem *ptr(*iter);
      ptr->canvas(c);
    }
  m_group=c;
}
