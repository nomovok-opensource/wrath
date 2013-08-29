/*! 
 * \file WRATHTextDataStream.hpp
 * \brief file WRATHTextDataStream.hpp
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




#ifndef __WRATH_TEXT_DATA_STREAM_HPP__
#define __WRATH_TEXT_DATA_STREAM_HPP__

#include "WRATHConfig.hpp"

#ifdef WRATH_USE_BOOST_LOCALE
#include <boost/locale.hpp>
#endif

#include <sstream>
#include <locale> 
#include "WRATHUtil.hpp"
#include "WRATHTextData.hpp"
#include "WRATHFormatter.hpp"
#include "WRATHFormattedTextStream.hpp"
#include "WRATHStateStream.hpp"
#include "WRATHStateStreamManipulators.hpp"
#include "WRATHColumnFormatter.hpp"
#include "WRATHTextDataStreamManipulator.hpp"

/*! \addtogroup Text
 * @{
 */


/*!\class WRATHTextDataStream
  A WRATHTextDataStream represents an easy way to
  stream output into a WRATHTextData.
 */
class WRATHTextDataStream:boost::noncopyable
{
private:

  template<typename T>
  class stream_holder;

public:
  /*!\class stream_type
    Proxy class for character streams
    used so that one can use operator<<
    with WRATHTextDataStream objects easily.
   */
  template<typename T>
  class stream_type
  {
  public:
    /*!\fn stream_type
      Ctor. Create a stream_type from a 
      WRATHTextDataStream. Internally really
      fetches the correct stream from the
      WRATHTextDataStream.
      \param p WRATHTextDataStream to which to print via 
               the constructed stream_type.
     */
    stream_type(WRATHTextDataStream *p):
      m_target(p->get_stream(type_tag<T>()))
    {}

    /*!\fn bool valid
      Returns true if the stream_type refers
      to a WRATHTextDataStream.
     */
    bool
    valid(void)
    {
      return m_target!=NULL;
    }

    /*!\fn WRATHTextDataStream* target
      Returns the WRATHTextDataStream to which
      this stream_type prints.
     */
    WRATHTextDataStream*
    target(void) const
    {
      return m_target!=NULL?
        m_target->m_parent:NULL;
    }
   
  private:
    friend class WRATHTextDataStream;
    stream_holder<T> *m_target;
  };

  /*!\fn WRATHTextDataStream(WRATHFormatter::handle)
    Ctor. 
    \param fmt handle to formatter to use for formatting, if invalid
               then the formatting will be initialized to
               a WRATHColumnFormatter with default arguments.
   */
  explicit
  WRATHTextDataStream(WRATHFormatter::handle fmt=WRATHFormatter::handle());

  /*!\fn WRATHTextDataStream(const WRATHColumnFormatter::LayoutSpecification&)
    Ctor. 
    \param L WRATHColumnFormatter::LayoutSpecification for a 
             WRATHColumnFormatter that is used to set the formatter.
   */  
  explicit
  WRATHTextDataStream(const WRATHColumnFormatter::LayoutSpecification &L);

  ~WRATHTextDataStream();

  /*!\fn stream_type<T> typed_stream
    Returns a stream proxy object which one can put data easily
    into a WRATHTextDataStream using the same format and features
    as found in C++ iostreams, including iostream manipulators.
    The underlying stream objects are essentially
    std::basic_ostringstream<T>, one for each type T. Hence
    IOstream state set by manipulators is independent of each
    type T. In contrast, the change state stream, state_stream()
    is tied to the WRATHTextDataStream(). A WRATHTextDataStream has
    the "intelligence" so that text added through different
    streams and through the append() method are correctly
    serialized.
   */
  template<typename T>
  stream_type<T>
  typed_stream(void) 
  {
    return stream_type<T>(this);
  }

  /*!\fn stream_type<char> stream
    Provided as a conveniance, equivalent
    to typed_stream<char>().
   */
  stream_type<char>
  stream(void)
  {
    return typed_stream<char>();
  }

  /*!\fn stream_type<wchar_t> wstream
    Provided as a conveniance, equivalent
    to typed_stream<wchar_t>().
   */
  stream_type<wchar_t>
  wstream(void)
  {
    return typed_stream<wchar_t>();
  }

  /*!\fn void clear
    Clears the text, all buffered data
    and resets the change state stream.
   */
  void
  clear(void);

  /*!\fn void set_stream_defaults
    Sets state stream to their default
    values for those state stream
    values that have a default (for
    example WRATHText::font). Values
    that were set that do not have a
    default are not affected.
   */
  void
  set_stream_defaults(void);

#ifdef WRATH_USE_BOOST_LOCALE

  /*!\fn boost::locale::generator& locale_generator()
    Returns a boost::locale::generator suitable
    for locale generation via boost.
    Function only supported if WRATH_USE_BOOST_LOCALE
    is defined
   */
  static
  boost::locale::generator&
  locale_generator(void);

#endif

  /*!\fn std::locale create_locale(const char*)
    Create a locale from a locale name.    
    \param e name of locale
  */
  static
  std::locale
  create_locale(const char *e);

  /*!\fn void locale(const std::locale&)
    Set the locale for the stream used for
    capital case conversion, etc.
    \param e locale to use
  */
  void
  locale(const std::locale &e);

  /*!\fn void locale(const char*)
    Set the locale for the stream used for
    capital case conversion, etc. Equivalent to
    \code
    locale(create_locale(e));
    \endcode 
    \param e name of locale to use
  */
  void
  locale(const char *e)
  {
    locale(create_locale(e));
  }

  /*!\fn const std::locale& locale(void)
    Return the locale for the stream used for
    capital case conversion, etc.
  */
  const std::locale&
  locale(void)
  {
    return m_locale.back();
  }
  
  /*!\fn void push_locale(const std::locale&)
    Push the locale for the stream used for
    capital case conversion, etc.
  */
  void
  push_locale(const std::locale &e);

  /*!\fn void push_locale(const char*)
    Push the locale for the stream used for
    capital case conversion, etc. Equivalent to
    \code
    push_locale(create_locale(e));
    \endcode 
    \param e name of locale to use
  */
  void
  push_locale(const char *e)
  {
    push_locale(create_locale(e));
  }
  
  /*!\fn enum return_code pop_locale(void)
    Pops the locale for the stream used for
    capital case conversion, etc.
  */
  enum return_code
  pop_locale(void);
  
  /*!\fn enum WRATHText::capitalization_e capitalization(void)
    Returns the current capitalization mode.
   */
  enum WRATHText::capitalization_e
  capitalization(void)
  {
    return m_cap.back();
  }
  
  /*!\fn void capitalization(enum WRATHText::capitalization_e)
    Sets the current capitalization mode.
    \param e mdoe to use
   */
  void
  capitalization(enum WRATHText::capitalization_e e);
  
  /*!\fn void push_capitalization(enum WRATHText::capitalization_e)
    Pushes the current capitalization mode.
    \param e mdoe to use
   */
  void
  push_capitalization(enum WRATHText::capitalization_e e);
  
  /*!\fn enum return_code pop_capitalization(void)
    Pops the capitalization mode.
  */
  enum return_code
  pop_capitalization(void);

  /*!\fn const WRATHTextData& raw_text
    Returns the raw text of this 
    WRATHTextDataStream, i.e. the 
    raw "string" unformatted.
   */
  const WRATHTextData&
  raw_text(void) const
  {
    flush_streams();
    return m_raw_text;
  }

  /*!\fn void append(iterator, iterator)
    Append raw character codes.
    \tparam iterator iterator type to character code
    \param begin iterator to 1st character code to add
    \param end iterator to one past the last character code to add    
   */
  template<typename iterator>
  void
  append(iterator begin, iterator end)
  {
    if(begin==end)
      {
        return;
      }

    if(m_current_stream!=&m_append_stream)
      {
        flush_streams();
        m_current_stream=&m_append_stream;
      }

    for(iterator iter=begin; iter!=end; ++iter)
      {
        WRATHTextData::character C(*begin);

        if(C.is_glyph_index())
          {
            m_append_stream.append(begin, iter);

            flush_streams();
            m_raw_text.push_back(C);
            begin=iter;
            ++begin;

            m_current_stream=&m_append_stream;
          }
      }
    m_append_stream.append(begin, end);
    m_format_dirty=true;
  }

  /*!\fn void append(WRATHTextData::character)
    Append a raw character.
    \param C character to append
   */
  void
  append(WRATHTextData::character C);

  /*!\fn void set_state
    Absorb current state values of a WRATHStateStream
    \param st WRATHStateStream from which to take the state values 
    \param copy_stacks if true the stacks are also copied
                       from st.
   */
  void
  set_state(const WRATHStateStream &st, bool copy_stacks=false)
  {
    flush_streams();
    m_state_stream.set_state(st, copy_stacks);
  }
        
  /*!\fn void format(WRATHFormatter::handle) const
    Set the text formatting as according to fmt.
    Formatting is doing lazily, i.e. the formatting
    is only done when the formatted text is requested,
    as such setting the formatter is a low cost operation,
    but when it is changed, the next time the formatted
    text is requested (via \ref formatted_text()), the
    text is reformatted.
    \param fmt handle to a WRATHFormatter to
               specify formatting.
   */
  void
  format(WRATHFormatter::handle fmt) const
  {
    m_format_dirty=m_format_dirty or fmt!=m_formatter;
    m_formatter=fmt;
  }

  /*!\fn void format(const WRATHColumnFormatter::LayoutSpecification&) const
    Equivalent to \code format(WRATHNew WRATHColumnFormatter(L)) \endcode
    \param L WRATHColumnFormatter::LayoutSpecification for a WRATHColumnFormatter.
   */
  void
  format(const WRATHColumnFormatter::LayoutSpecification &L) const
  {
    format(WRATHNew WRATHColumnFormatter(L));
  }

  /*!\fn const WRATHFormattedTextStream& formatted_text
    Returns the text data formatted and layed-out, 
    it will flush and update the underlying
    data as necessary.
   */
  const WRATHFormattedTextStream&
  formatted_text(void) const
  {
    flush_streams();
    execute_formatting();
    return m_formatted_data;
  }

  /*!\fn const WRATHFormatter::pen_position_return_type& end_text_pen_position
    Returns the "pen position" following
    the formatted text. Note: this routine 
    will flush any streams and perform
    formatting of the text if the underlying
    data has changed.
   */
  const WRATHFormatter::pen_position_return_type&
  end_text_pen_position(void) const
  {
    flush_streams();
    execute_formatting();
    return m_end_text_pen_position;
  }

  /*!\fn const WRATHStateStream& state_stream
    Returns a const reference to the WRATHStateStream
    that holds all the state changes.
   */
  const WRATHStateStream&
  state_stream(void) const 
  {
    return m_state_stream;
  }

  /*!\fn const_c_array< std::pair<int,S> > state_change_stream
    Returns the array assoicated to a type and ID.
    \param pID ID needed if there are multiple independent
               arrays of the same type.
   */
  template<typename S>
  const_c_array< std::pair<int,S> >
  state_change_stream(int pID=0) const
  {
    return m_state_stream.state_stream<S>(pID);
  }

  /*!\fn stream_type<T> operator<<(stream_type<T>, const S&)
    Template overloaded operator<< which will accept
    any type S for which operator<<(std::basic_ostringstream<T>&, const S&)
    has been overloaded.
    \param stream WRATHTextDataStream to "print to"
    \param obj value to stream to the WRATHTextDataStream stream
   */
  template<typename T, typename S>
  friend
  stream_type<T>
  operator<<(stream_type<T> stream, const S &obj)
  {
    WRATHassert(stream.valid());
    
    stream.target()->stream_generic_object(stream, obj);
    return stream;
  }

  /*!\fn stream_type<T> operator<<(stream_type<T>, const WRATHStateStreamManipulators::set_state_type<S>&)
    Template overloaded operator<< which will accept
    a WRATHStateStreamManipulators::set_state_type<S> to change a state.
    \param stream WRATHTextDataStream to "print to"
    \param obj value to stream to the WRATHTextDataStream stream
   */
  template<typename T, typename S>
  friend
  stream_type<T>
  operator<<(stream_type<T> stream, const WRATHStateStreamManipulators::set_state_type<S> &obj)
  {
    int loc;

    //WRATHTextDataStream::raw_text() flushes the text as needed.
    WRATHassert(stream.valid());
    loc=stream.target()->raw_text().character_data().size();

    stream.target()->m_state_stream.increment_time_to_value(loc);
    stream.target()->m_state_stream.set_state(type_tag<S>(), obj.data(), obj.ID());    
    stream.target()->m_format_dirty=true;

    return stream;
  }

  /*!\fn stream_type<T> operator<<(stream_type<T>, const WRATHStateStreamManipulators::push_state_type<S>&)
    Template overloaded operator<< which will accept
    a WRATHStateStreamManipulators::push_state_type<S> to push the state
    stack.
    \param stream WRATHTextDataStream to "print to"
    \param obj value to stream to the WRATHTextDataStream stream
   */
  template<typename T, typename S>
  friend
  stream_type<T>
  operator<<(stream_type<T> stream, const WRATHStateStreamManipulators::push_state_type<S> &obj)
  {
    int loc;

    //WRATHTextDataStream::raw_text() flushes the text as needed.
    WRATHassert(stream.valid());
    loc=stream.target()->raw_text().character_data().size();

    stream.target()->m_state_stream.increment_time_to_value(loc);
    stream.target()->m_state_stream.push_state(type_tag<S>(), obj.data(), obj.ID());    
    stream.target()->m_format_dirty=true;

    return stream;
  }
  
  /*!\fn stream_type<T> operator<<(stream_type<T>, const WRATHStateStreamManipulators::pop_state_type<S>&)
    Template overloaded operator<< which will accept
    a WRATHStateStreamManipulators::pop_state_type<S> to pop the state
    stack.
    \param stream WRATHTextDataStream to "print to"
    \param obj value to stream to the WRATHTextDataStream stream
   */
  template<typename T, typename S>
  friend
  stream_type<T>
  operator<<(stream_type<T> stream, const WRATHStateStreamManipulators::pop_state_type<S> &obj)
  {
    int loc;

    //WRATHTextDataStream::raw_text() flushes the text as needed.
    WRATHassert(stream.valid());
    loc=stream.target()->raw_text().character_data().size();

    stream.target()->m_state_stream.increment_time_to_value(loc);
    stream.target()->m_state_stream.pop_state(type_tag<S>(), obj.ID());    
    stream.target()->m_format_dirty=true;

    return stream;
  }
  
  /*!\fn stream_type<T> operator<<(stream_type<T>, const WRATHStateStreamManipulators::get_state_type<S>&)
    Template overloaded operator<< which will accept
    a WRATHStateStreamManipulators::get_state_type to query a state.
    \param stream WRATHTextDataStream to "print to"
    \param obj value to stream to the WRATHTextDataStream stream
   */
  template<typename T, typename S>
  friend
  stream_type<T>
  operator<<(stream_type<T> stream, const WRATHStateStreamManipulators::get_state_type<S> &obj)
  {
    enum return_code R;
    
    stream.target()->flush_streams();
    R=stream.target()->m_state_stream.get_state(type_tag<S>(), obj.target(), obj.ID());
    if(obj.return_value()!=NULL)
      {
        *obj.return_value()=R;
      }

    return stream;
  }

  /*!\fn stream_type<T> operator<<(stream_type<T>, const WRATHStateStreamManipulators::get_state_cast_type<S0, S1>&)
    Template overloaded operator<< which will accept
    a WRATHStateStreamManipulators::get_state_cast_type to query a state.
    \param stream WRATHTextDataStream to "print to"
    \param obj value to stream to the WRATHTextDataStream stream
   */
  template<typename T, typename S0, typename S1>
  friend
  stream_type<T>
  operator<<(stream_type<T> stream, const WRATHStateStreamManipulators::get_state_cast_type<S0, S1> &obj)
  {
    enum return_code R;
    
    stream.target()->flush_streams();
    R=stream.target()->m_state_stream.get_state_cast(type_tag<S0>(), 
                                                     type_tag<S1>(),
                                                     obj.target(), obj.ID());
    if(obj.return_value()!=NULL)
      {
        *obj.return_value()=R;
      }

    return stream;
  }
  
  /*!\fn stream_type<T> operator<<(stream_type<T>, WRATHTextureFont::glyph_index_type)
    Special case of operator<< to push back an explicit
    glyph index code.
    \param stream WRATHTextDataStream to "print to"
    \param G value to stream to the WRATHTextDataStream stream
   */
  template<typename T>
  friend
  stream_type<T>
  operator<<(stream_type<T> stream, WRATHTextureFont::glyph_index_type G)
  {
    WRATHTextData::character C(G);

    stream.target()->flush_streams();
    stream.target()->m_raw_text.push_back(C);   
    stream.target()->m_format_dirty=true; 
    return stream;
  }

  /*!\fn stream_type<T> operator<<(stream_type<T>, WRATHText::get_stream_size_type)
    Special case of operator<< to query the size of the stream.
    \param stream WRATHTextDataStream to query
    \param G query object
   */
  template<typename T>
  friend
  stream_type<T>
  operator<<(stream_type<T> stream, WRATHText::get_stream_size_type G)
  {
    WRATHassert(G.m_target!=NULL);
    stream.target()->flush_streams();
    *G.m_target=stream.target()->raw_text().character_data().size();
    return stream;
  }

private:
  /*
    Basic idea of stream handling:
    we have a pure virtual base class stream_holder_base
    (having methods flush and clear), a template class
    stream_holder which contains a std::basic_ostringstream.
    Finally we have an std::map with keys const
    reference to std::type_info and values as pointers
    to stream_holder_base. Fetching the correct stream
    then corresponds to an std::map look up and then
    a dynamic cast.

    "Buffering" is accomplished as follows:
    only one stream_holder (at most) is active, 
    and that is specified by m_current_stream.
    Flushing then is just checking if it is not NULL,
    and if not, calling flush() and changing it to NULL.
    Flushing occurs whenever any of the following occur:\n
    \n 1) request for the raw text data by calling raw_text()
    \n 2) whenever a data is placed directly to m_raw_text
    \n 3) whenever data is streamed into a stream and that
            stream is not m_current_stream\n\n
    note that 1) occurs whenever state changes are added
    and whenever the formatted data is requested.
   */
  typedef WRATHUtil::TypeInfoSortable key_type;

  class stream_holder_base:boost::noncopyable
  {
  public:
    WRATHTextDataStream *m_parent;

    stream_holder_base(WRATHTextDataStream *p):
      m_parent(p)
    {}

    virtual
    ~stream_holder_base()
    {}

    virtual
    void
    flush(void)=0;

    virtual
    void
    clear(void)=0;
  };

  template<typename T>
  class stream_holder:public stream_holder_base
  {
  public:
    std::basic_ostringstream<T> m_stream;

    stream_holder(WRATHTextDataStream *p):
      stream_holder_base(p)
    {}

    ~stream_holder()
    {}

    virtual
    void
    flush(void);

    virtual
    void
    clear(void)
    {
      m_stream.str(std::basic_string<T>());
    }
  };

  class append_stream_holder:public stream_holder_base
  {
  public:
    std::vector<uint32_t> m_data;

    append_stream_holder(WRATHTextDataStream *p):
      stream_holder_base(p)
    {}

    virtual
    void
    flush(void);

    virtual
    void
    clear(void)
    {
      m_data.clear();
    }

    template<typename iterator>
    void
    append(iterator begin, iterator end)
    {
      m_data.reserve( m_data.size() + std::distance(begin,end));
      for(;begin!=end; ++begin)
        {
          WRATHTextData::character C(*begin);
          m_data.push_back(C.character_code().m_value);
        }
    }
  };

  template<typename T>
  stream_holder<T>*
  get_stream(type_tag<T>)
  {
    key_type K(typeid(T));
    std::map<key_type, stream_holder_base*>::iterator iter;

    iter=m_streams.find(K);
    if(iter==m_streams.end())
      {
        m_streams[K]=WRATHNew stream_holder<T>(this);
        iter=m_streams.find(K);
      }
    return dynamic_cast<stream_holder<T>*>(iter->second);
  }

  template<typename T, typename S>
  void
  stream_generic_object(stream_type<T> stream, const S &obj)
  {
    if(m_current_stream!=NULL and m_current_stream!=stream.m_target)
      {
        flush_streams();
        stream.m_target->m_stream.imbue(stream.m_target->m_parent->locale());
      }
    m_current_stream=stream.m_target;

    stream.m_target->m_stream << obj;
  }

  void
  execute_formatting(void) const;

  void
  flush_streams(void) const;

  void
  init(void);

  std::map<key_type, stream_holder_base*> m_streams;
  mutable stream_holder_base *m_current_stream;

  append_stream_holder m_append_stream;

  WRATHStateStream m_state_stream;
  WRATHTextData m_raw_text;

  mutable WRATHFormatter::pen_position_return_type m_end_text_pen_position;
  mutable bool m_format_dirty;
  mutable WRATHFormatter::handle m_formatter;
  mutable WRATHFormattedTextStream m_formatted_data;

  std::vector<std::locale> m_locale;
  std::vector<enum WRATHText::capitalization_e> m_cap;
};

#include "WRATHTextDataStreamManipulatorsImplement.tcc"


/*!\fn WRATHTextDataStream::stream_type<T> operator<<(WRATHTextDataStream::stream_type<T>, 
                                                      const WRATHText::set_colors_type&)
  Manipulator implementation to set the color of letters
  with a \ref WRATHText::set_colors_type
  \tparam character type
  \param stream stream to affect
  \param c color value setter
 */ 
template<typename T>
WRATHTextDataStream::stream_type<T>
operator<<(WRATHTextDataStream::stream_type<T> stream,
           const WRATHText::set_colors_type &c);

/*!\fn WRATHTextDataStream::stream_type<T> operator<<(WRATHTextDataStream::stream_type<T>, 
                                                      const WRATHText::push_colors_type&)
  Manipulator implementation to set the color of letters
  with a \ref WRATHText::push_colors_type
  \tparam character type
  \param stream stream to affect
  \param c color value pusher
 */ 
template<typename T>
WRATHTextDataStream::stream_type<T>
operator<<(WRATHTextDataStream::stream_type<T> stream,
           const WRATHText::push_colors_type &c);


/*!\fn WRATHTextDataStream::stream_type<T> operator<<(WRATHTextDataStream::stream_type<T>, 
                                                      const WRATHText::pop_colors_type&)
  Manipulator implementation to pop the color of letters
  with a \ref WRATHText::pop_colors_type
  \tparam character type
  \param stream stream to affect
  \param c color value popper
 */ 
template<typename T>
WRATHTextDataStream::stream_type<T>
operator<<(WRATHTextDataStream::stream_type<T> stream,
           const WRATHText::pop_colors_type &c);


/*!\fn WRATHTextDataStream::stream_type<T> operator<<(WRATHTextDataStream::stream_type<T>, 
                                                      const WRATHText::get_color_type&)
  Manipulator implementation to get the color of letters
  with a \ref WRATHText::get_color_type
  \tparam character type
  \param stream stream to affect
  \param c color value getter
 */ 
template<typename T>
WRATHTextDataStream::stream_type<T>
operator<<(WRATHTextDataStream::stream_type<T> stream,
           const WRATHText::get_color_type &c);

/*!\fn WRATHTextDataStream::stream_type<T> operator<<(WRATHTextDataStream::stream_type<T>, 
                                                      WRATHText::stream_defaults)
  Manipulator implementation for \ref WRATHText::stream_defaults
  to essentially call WRATHTextDataStream::set_stream_defaults()
  \param stream stream to affect
 */ 
template<typename T>
WRATHTextDataStream::stream_type<T>
operator<<(WRATHTextDataStream::stream_type<T> stream,
           WRATHText::stream_defaults)
{
  stream.target()->set_stream_defaults();
  return stream;
}
           

/*! @} */



#endif
