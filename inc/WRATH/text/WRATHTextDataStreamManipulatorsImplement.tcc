/*  -*- C++ -*- */

/*! 
 * \file WRATHTextDataStreamManipulatorsImplement.tcc
 * \brief file WRATHTextDataStreamManipulatorsImplement.tcc
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



#if !defined(WRATH_HEADER_TEXT_DATA_STREAM_HPP_) 
#error "Direction inclusion of private header file WRATHTextDataStreamManipulatorsImplement.tcc"
#endif


#ifdef WRATH_USE_BOOST_LOCALE
#include <boost/locale.hpp>
#else
#include <boost/algorithm/string.hpp>
#endif

namespace WRATHTextDataStreamImplement
{
#ifdef WRATH_USE_BOOST_LOCALE

  template<typename T, typename C>
  void
  convert_to_lower(type_tag<C>, T &in, const std::locale &loc)
  {
    std::basic_string<C> temp(in.begin(), in.end());

    temp=boost::locale::to_lower(temp, loc);
    in=T(temp.begin(), temp.end());
  }

  template<typename T, typename C>
  void
  convert_to_upper(type_tag<C>, T &in, const std::locale &loc)
  {
    std::basic_string<C> temp(in.begin(), in.end());

    temp=boost::locale::to_upper(temp, loc);
    in=T(temp.begin(), temp.end());
  }

  template<typename T, typename C>
  void
  convert_to_title(type_tag<C>, T &in, const std::locale &loc)
  {
    std::basic_string<C> temp(in.begin(), in.end());

    temp=boost::locale::to_title(temp, loc);
    in=T(temp.begin(), temp.end());
  }

#else

  template<typename T, typename C>
  void
  convert_to_lower(type_tag<C>, T &in, const std::locale &loc)
  {
    std::basic_string<C> temp(in.begin(), in.end());
    boost::to_lower(temp, loc);
    in=T(temp.begin(), temp.end());
  }

  template<typename T, typename C>
  void
  convert_to_upper(type_tag<C>, T &in, const std::locale &loc)
  {
    std::basic_string<C> temp(in.begin(), in.end());
    boost::to_upper(temp, loc);
    in=T(temp.begin(), temp.end());
  }
  
  template<typename T, typename C>
  void
  convert_to_title(type_tag<C>, T&, const std::locale &)
  {
  }

#endif

  template<typename T, typename C>
  void
  append_converted(WRATHTextData &raw_data,
                   type_tag<C>, 
                   T &stuff,
                   const std::locale &loc,
                   enum WRATHText::capitalization_e cap)
  {
    switch(cap)
    {
    case WRATHText::capitalization_all_lower_case:
      {
        WRATHTextDataStreamImplement::convert_to_lower(type_tag<C>(), stuff, loc);
        raw_data.append(stuff.begin(), stuff.end());
      }
      break;
      
    case WRATHText::capitalization_all_upper_case:
      {
        WRATHTextDataStreamImplement::convert_to_upper(type_tag<C>(), stuff, loc);
        raw_data.append(stuff.begin(), stuff.end());
      }
      break;
      
    case WRATHText::capitalization_title_case:
      {
        WRATHTextDataStreamImplement::convert_to_title(type_tag<C>(), stuff, loc);
        raw_data.append(stuff.begin(), stuff.end());
      }
      break;
      
    default:
      {
        raw_data.append(stuff.begin(), stuff.end());
      }
    
    } //of switch()
  
  }

                   


}

template<typename T>
void
WRATHTextDataStream::stream_holder<T>::
flush(void)
{
  std::basic_string<T> stuff(m_stream.str());
  WRATHTextDataStreamImplement::append_converted(m_parent->m_raw_text,
                                                 type_tag<T>(), stuff,
                                                 this->m_parent->m_locale.back(),
                                                 this->m_parent->m_cap.back());
  
  clear();
}


//////////////////////////////////////////////
// for color 
template<typename T>
inline
WRATHTextDataStream::stream_type<T>
operator<<(WRATHTextDataStream::stream_type<T> stream,
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


template<typename T>
inline
WRATHTextDataStream::stream_type<T>
operator<<(WRATHTextDataStream::stream_type<T> stream,
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



template<typename T>
inline
WRATHTextDataStream::stream_type<T>
operator<<(WRATHTextDataStream::stream_type<T> stream,
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


template<typename T>
inline
WRATHTextDataStream::stream_type<T>
operator<<(WRATHTextDataStream::stream_type<T> stream,
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

///////////////////////////////////////////////////
// macro for those properties that are stored in the
// text_stream not the state_stream!
#define WRATH_TEXT_DATA_STREAM_HELPER_IMPLEMENT(fcn, stack, type, manip)  \
template<typename T> \
inline \
WRATHTextDataStream::stream_type<T> \
operator<<(WRATHTextDataStream::stream_type<T> stream, \
           const WRATHText::manip::set_type &c) \
{ \
  stream.target()->fcn(c.data()); \
  return stream; \
} \
template<typename T> \
inline \
WRATHTextDataStream::stream_type<T> \
operator<<(WRATHTextDataStream::stream_type<T> stream, \
           const WRATHText::manip::push_type &c) \
{ \
  stream.target()->push_##fcn(c.data()); \
  return stream; \
} \
template<typename T> \
inline \
WRATHTextDataStream::stream_type<T> \
operator<<(WRATHTextDataStream::stream_type<T> stream, \
           const WRATHText::manip::pop_type) \
{ \
  stream.target()->pop_##fcn(); \
  return stream; \
} \
template<typename T> \
inline \
WRATHTextDataStream::stream_type<T> \
operator<<(WRATHTextDataStream::stream_type<T> stream, \
           const WRATHText::manip::get_type &v) \
{ \
  v.target()=stream.target()->fcn(); \
  return stream; \
} \
template<typename T> \
inline \
WRATHTextDataStream::stream_type<T> \
operator<<(WRATHTextDataStream::stream_type<T> stream, \
           const WRATHText::manip::get_uncasted_type &v) \
{ \
  v.target().m_value=stream.target()->fcn(); \
  return stream; \
}


WRATH_TEXT_DATA_STREAM_HELPER_IMPLEMENT(locale, m_locale, std::locale, localization)
WRATH_TEXT_DATA_STREAM_HELPER_IMPLEMENT(capitalization, m_cap, enum WRATHText::capitalization_e, capitalization)

namespace WRATHText
{
  inline
  localization::set_type
  set_localization(const char *localization_name)
  {
    return set_localization( WRATHTextDataStream::create_locale(localization_name) );
  }
}
