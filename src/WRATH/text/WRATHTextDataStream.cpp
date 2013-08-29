/*! 
 * \file WRATHTextDataStream.cpp
 * \brief file WRATHTextDataStream.cpp
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
#include "WRATHTextDataStream.hpp"
#include "WRATHFontFetch.hpp"
#include "WRATHStaticInit.hpp"
#include <cstddef>

void
WRATHTextDataStream::append_stream_holder::
flush(void)
{
  /*
    TODO: 
      wchar_t may or may not be a 32-bit character 
      (it is on g++ under Linux, but relying on
      that in general is bad practice).
      In general wchar_t may or may not have anything
      to do with UTF32 (or UTF-anyting). I do not want 
      to break out iconv to handle this and the utf32 types
      for C++ are C++0x (std::char32_t I am looking at
      you!)
   */
  WRATHTextDataStreamImplement::append_converted(m_parent->m_raw_text,
                                                 type_tag<wchar_t>(),
                                                 m_data,
                                                 m_parent->m_locale.back(),
                                                 m_parent->m_cap.back());
  clear();
}

/////////////////////////////////
// WRATHTextDataStream methods
WRATHTextDataStream::
WRATHTextDataStream(WRATHFormatter::handle fmt):
  m_current_stream(NULL),
  m_append_stream(this),
  m_format_dirty(false),
  m_formatter(fmt)
{
  init();
}


WRATHTextDataStream::
WRATHTextDataStream(const WRATHColumnFormatter::LayoutSpecification &L):
  m_current_stream(NULL),
  m_append_stream(this),
  m_format_dirty(false),
  m_formatter(WRATHNew WRATHColumnFormatter(L))
{
  init();    
}


WRATHTextDataStream::
~WRATHTextDataStream()
{
  for(std::map<key_type, stream_holder_base*>::iterator
        iter=m_streams.begin(), end=m_streams.end();
      iter!=end; ++iter)
    {
      WRATHDelete(iter->second);
    }
}

#ifdef WRATH_USE_BOOST_LOCALE
boost::locale::generator&
WRATHTextDataStream::
locale_generator(void)
{
  WRATHStaticInit();
  static boost::locale::generator R;
  return R;
}
#endif

std::locale
WRATHTextDataStream::
create_locale(const char *e)
{
  #ifdef WRATH_USE_BOOST_LOCALE
    {
      return locale_generator()(e);
    }
  #else
    {
      return std::locale(e);
    }
  #endif  
}


void
WRATHTextDataStream::
locale(const std::locale &e)
{
  flush_streams();
  m_locale.back()=e;
}

void
WRATHTextDataStream::
push_locale(const std::locale &e)
{
  flush_streams();
  m_locale.push_back(e);
}

enum return_code
WRATHTextDataStream::
pop_locale(void)
{
  if(m_locale.size()>1)
    {
      flush_streams();
      m_locale.pop_back();
      return routine_success;
    }
  return routine_fail;
}

void
WRATHTextDataStream::
capitalization(enum WRATHText::capitalization_e e)
{
  flush_streams();
  m_cap.back()=e;
}

void
WRATHTextDataStream::
push_capitalization(enum WRATHText::capitalization_e e)
{
  flush_streams();
  m_cap.push_back(e);
}

enum return_code
WRATHTextDataStream::
pop_capitalization(void)
{
  if(m_cap.size()>1)
    {
      flush_streams();
      m_cap.pop_back();
      return routine_success;
    }
  return routine_fail;
}




void
WRATHTextDataStream::
append(WRATHTextData::character C)
{
  if(C.is_glyph_index())
    {
      flush_streams();
      m_format_dirty=true;
      m_raw_text.push_back(C);
    }
  else
    {
      if(m_current_stream!=&m_append_stream)
        {
          flush_streams();
          m_current_stream=&m_append_stream;
        }
      m_append_stream.m_data.push_back(C.character_code().m_value);
    }
}

void
WRATHTextDataStream::
clear(void)
{
  m_format_dirty=true;
  m_raw_text.clear();
  m_state_stream.reset();
  
  m_append_stream.clear();
  for(std::map<key_type, stream_holder_base*>::iterator
        iter=m_streams.begin(), end=m_streams.end();
      iter!=end; ++iter)
    {
      iter->second->clear();
    }
  m_current_stream=NULL;

  set_stream_defaults();
  
}

void
WRATHTextDataStream::
execute_formatting(void) const
{
  WRATHassert(m_current_stream==NULL);

  if(m_format_dirty)
    {
      if(!m_formatter.valid())
        {
          WRATHColumnFormatter::LayoutSpecification L;
          m_formatter=WRATHNew WRATHColumnFormatter(L);
        }
      
      m_end_text_pen_position=m_formatted_data.set_text(m_formatter, raw_text(),  m_state_stream);
      m_format_dirty=false;
    }
}


void
WRATHTextDataStream::
flush_streams(void) const
{
  if(m_current_stream!=NULL)
    {
      m_current_stream->flush();
      m_current_stream=NULL;
      m_format_dirty=true;
    }
}


void
WRATHTextDataStream::
init(void)
{
  push_locale("");
  m_cap.push_back(WRATHText::capitalization_as_in_stream);
  set_stream_defaults();
}

void
WRATHTextDataStream::
set_stream_defaults(void)
{

  stream() << WRATHText::set_z_position(-1.0f) 
           << WRATHText::set_kerning(true)
           << WRATHText::set_word_spacing(0.0f)
           << WRATHText::set_letter_spacing_type(WRATHText::letter_spacing_absolute)
           << WRATHText::set_letter_spacing(0.0f)
           << WRATHText::set_horizontal_stretching(1.0f)
           << WRATHText::set_vertical_stretching(1.0f)
           << WRATHText::set_color(0xFF, 0xFF, 0xFF, 0xFF)
           << WRATHText::set_font(WRATHFontFetch::fetch_default_font())
           << WRATHText::set_scale(1.0f)
           << WRATHText::set_pixel_size(32.0f)
           << WRATHText::set_baseline_shift_y(0.0f)
           << WRATHText::set_baseline_shift_x(0.0f);
}



////////////////////////////////////////////
// misc methods...



WRATHStateStream&
operator<<(WRATHStateStream& stream,
           const WRATHText::set_colors_type &c)
{
  if(c.m_bits&WRATHText::bottom_left_corner_bit)
    {
      stream << WRATHText::set_color_bottom_left(c.m_value);
    }
  if(c.m_bits&WRATHText::bottom_right_corner_bit)
    {
      stream << WRATHText::set_color_bottom_right(c.m_value);
    }
  if(c.m_bits&WRATHText::top_right_corner_bit)
    {
      stream << WRATHText::set_color_top_right(c.m_value);
    }
  if(c.m_bits&WRATHText::top_left_corner_bit)
    {
      stream << WRATHText::set_color_top_left(c.m_value);
    }

  return stream;
}




WRATHStateStream&
operator<<(WRATHStateStream& stream,
           const WRATHText::push_colors_type &c)
{
  if(c.m_bits&WRATHText::bottom_left_corner_bit)
    {
      stream << WRATHText::push_color_bottom_left(c.m_value);
    }
  if(c.m_bits&WRATHText::bottom_right_corner_bit)
    {
      stream << WRATHText::push_color_bottom_right(c.m_value);
    }
  if(c.m_bits&WRATHText::top_right_corner_bit)
    {
      stream << WRATHText::push_color_top_right(c.m_value);
    }
  if(c.m_bits&WRATHText::top_left_corner_bit)
    {
      stream << WRATHText::push_color_top_left(c.m_value);
    }
  
  return stream;
}





WRATHStateStream&
operator<<(WRATHStateStream& stream,
           const WRATHText::pop_colors_type &c)
{
  if(c.m_bits&WRATHText::bottom_left_corner_bit)
    {
      stream << WRATHText::pop_color_bottom_left();
    }
  if(c.m_bits&WRATHText::bottom_right_corner_bit)
    {
      stream << WRATHText::pop_color_bottom_right();
    }
  if(c.m_bits&WRATHText::top_right_corner_bit)
    {
      stream << WRATHText::pop_color_top_right();
    }
  if(c.m_bits&WRATHText::top_left_corner_bit)
    {
      stream << WRATHText::pop_color_top_left();
    }
  
  return stream;
}




WRATHStateStream&
operator<<(WRATHStateStream& stream,
           const WRATHText::get_color_type &c)
{
  
  if(c.m_bit&WRATHText::bottom_right_corner_bit)
    {
      stream << WRATHText::get_color_bottom_right(c.m_target);
    }
  else if(c.m_bit&WRATHText::top_right_corner_bit)
    {
      stream << WRATHText::get_color_top_right(c.m_target);
    }
  else if(c.m_bit&WRATHText::top_left_corner_bit)
    {
      stream << WRATHText::get_color_top_left(c.m_target);
    }
  else
    {
      stream << WRATHText::get_color_bottom_left(c.m_target);
    }

  return stream;
} 

