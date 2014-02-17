/*! 
 * \file WRATHFontDatabase.hpp
 * \brief file WRATHFontDatabase.hpp
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


#ifndef WRATH_HEADER_FONT_DATABASE_HPP_
#define WRATH_HEADER_FONT_DATABASE_HPP_

#include "WRATHConfig.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <set>
#include <map>
#include <string>
#include <vector>
#include <boost/utility.hpp>
#include <boost/signals2.hpp>
#include "vecN.hpp"
#include "c_array.hpp"
#include "WRATHFontSupport.hpp"
#include "WRATHReferenceCountedObject.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\namespace WRATHFontDatabase
  WRATHFontDatabase represents an interface to
  query and augment the database of font 
  descriptions and sources.

  Font sources are either registered or
  unregistered. Unregistered sources
  are not added to the database, but do
  have a meta font familiy associated 
  to them. Since unregistered fonts
  are not added to the database, a user
  must save the handles themselves
  to later delete the font. Registered
  fonts are NOT removeable and once
  added stay in the database.
 */
namespace WRATHFontDatabase
{
  class FontProperties;
  class Font;
  class MetaFont;
  class FontDatabaseImplement;

  using namespace WRATHFontSupport;

  /*!\class FontProperties
    Represents defining properties of a font
    for the database.
   */
  class FontProperties
  {
  public:

    /*!\fn FontProperties
      Ctor. Initializes \ref m_bold as false
      and \ref m_italic as false.
     */
    FontProperties(void):
      m_bold(false),
      m_italic(false)
    {}

    /*!\fn FontProperties& bold(bool)
      Sets \ref m_bold.
      Default value is false.
      \param v value to use
     */
    FontProperties&
    bold(bool v)
    {
      m_bold=v;
      return *this;
    }

    /*!\fn FontProperties& italic(bool)
      Sets \ref m_italic.
      Default value is false.
      \param v value to use
     */
    FontProperties&
    italic(bool v)
    {
      m_italic=v;
      return *this;
    }

    /*!\fn FontProperties& style_name(const std::string&)
      Sets \ref m_style_name.
      Default value is an empty string.
      \param v value to use
     */
    FontProperties&
    style_name(const std::string &v)
    {
      m_style_name=v;
      return *this;
    }

    /*!\fn FontProperties& family_name(const std::string&)
      Sets \ref m_family_name.
      Default value is an empty string.
      \param v value to use
     */
    FontProperties&
    family_name(const std::string &v)
    {
      m_family_name=v;
      return *this;
    }

    /*!\fn FontProperties& foundry_name(const std::string&)
      Sets \ref m_foundry_name.
      Default value is an empty string.
      \param v value to use
     */
    FontProperties&
    foundry_name(const std::string &v)
    {
      m_foundry_name=v;
      return *this;
    }

    /*!\fn bool operator<(const FontProperties &) const
      Comparison operator for sorting.
      \param rhs value to which to compare
     */
    bool
    operator<(const FontProperties &rhs) const;

    /*!\var m_bold
      Specifies if the font is to be bold
      or not.
     */
    bool m_bold;

    /*!\var m_italic
      Specifies if the font is to be italic
      or not.
     */
    bool m_italic;

    /*!\var m_style_name
      Specifies the style name of the font.
      Examples are "Bold", "Bold Italic",
      "Book", "Condensed", "Condensed Bold Obliquie".
      The value for style is NOT orthogonal to
      the value of \ref m_italic and \ref m_bold.
      For example, under a standard GNU/Linux system
      the style names "Condensed Bold Oblique",
      "Condensed Oblique", "Condensed Bold" 
      and "Condensed" give different fonts for
      the family name "DejaVu Serif". 
     */
    std::string m_style_name;

    /*!\var m_family_name
      Specifies the fmaily name of
      the font, for example "Sans"
     */
    std::string m_family_name;

    /*!\var m_foundry_name
      Specifies the foundry name of the
      font, i.e. the maker of the font.
      Some systems (for example those using
      fontconfig) this value is ignored.
     */
    std::string m_foundry_name;
  };

  /*!\enum meta_font_matching
    Enumeration to describe how to perform
    font matching.
   */ 
  enum meta_font_matching
    {
      /*!
        Foundry, family and style names match
        in addition the flags bold and italic
        also match.
       */
      exact_match,

      /*!
        Family and style names match
        in addition the flags bold and italic
        also match
       */
      family_style_bold_italic_match,

      /*!
        Family names match
        in addition the flags bold and italic
        also match
       */
      family_bold_italic_match,

      /*!
        Flags bold and italic match
       */
      bold_italic_match,

      /*!
        No requirements for matching.
       */
      last_resort,
    };

  /*!\class FontMemorySource
    A FontMemorySource represents memory from
    which a font is read.
   */
  class FontMemorySource:
    public WRATHReferenceCountedObjectT<FontMemorySource>
  {
  public:
    /*!\fn FontMemorySource(std::vector<uint8_t> &)
      Constructs a FontMemorySource. After ctor,
      the contents of bytes is transferred to
      the FontMemorySource and bytes is made empty.
      \param bytes font held in memory
     */ 
    explicit
    FontMemorySource(std::vector<uint8_t> &bytes)
    {
      std::swap(bytes, m_memory);
    }

    /*!\fn const_c_array<uint8_t> data(void) const
      Returns the raw bytes of the FontMemorySource
     */ 
    const_c_array<uint8_t>
    data(void) const
    {
      return m_memory;
    }

  private:
    std::vector<uint8_t> m_memory;
  };

  /*!\class Font
    A Font represents an entry within
    the WRATHFontDatabase which is a single
    font in a file (or memory source)
   */
  class Font:public WRATHReferenceCountedObjectT<Font>
  {
  public:

    /*!\typedef signal_t
      Conveniance typedef for the signal type
      fired whenever a non-registered Font is
      to be deleted.
     */
    typedef boost::signals2::signal<void (void)> signal_t;

    /*!\typedef connect_t
      Conveniance typedef for the connection type.
    */
    typedef boost::signals2::connection connect_t;

    /*!\fn const std::string& name(void)
      Returns the name of the font,
      for fonts sourced from a file
      it is the filename.
     */
    const std::string&
    name(void) const
    {
      return m_filename;
    }

    /*!\fn int face_index(void) const
      Returns the face index of the font. Some
      font files define multiple faces within
      them, the selection of the face is from
      a face index.
     */ 
    int
    face_index(void) const
    {
      return m_face_index;
    }

    /*!\fn const FontMemorySource::const_handle& memory_source(void) const
      If a font is not from a file, returns
      a handle to the FontMemorySource
      of the font. If the font is from
      a file, returns an invalid handle.
     */
    const FontMemorySource::const_handle&
    memory_source(void) const
    {
      return m_memory_source;
    }

    /*!\fn const std::string& label(void) const
      Returns the label of the font, 
      which is given by
      \code
      name() ":" face_index() 
      \endcode
     */
    const std::string&
    label(void) const
    {
      return m_label;
    }

    /*!\fn const FontProperties& properties(void) const
      Returns the properties of the font.
     */
    const FontProperties&
    properties(void) const
    {
      return m_properties;
    }

    /*!\fn const MetaFont* meta_font
      Returns the MetaFont for use when a given
      character code is not within a font.
      \param v font matching specification
     */
    const MetaFont*
    meta_font(enum meta_font_matching v) const
    {
      return m_meta_font[v];
    }

    /*!\fn bool is_registered_font
      Returns true if the font is considered
      registerd by WRATHFontdatabase.
     */
    bool
    is_registered_font(void) const
    {
      return m_is_registered_font;
    }

    /*!\fn connect_t connect_unregistered_delete()
      If the font is not registered to the font database,
      then it can be deleted as well. This signal is fired
      whenever the font is marked for deletion with
      WRATHFontDatabase::release_unregistered_font(). 
      Note that the signal is never fired if the font 
      is registered (i.e. is_registered_font() returns true).
      It is considered an error to connect to the signal
      if the font is registered. Under debug builds, will
      assert.
      \param subscriber slot called on signal fire
      \param gp_order order of slot call. Lower values of gp_order
                      are guarnteed to be call those of higher values
                      of gp_order. Slots connected with the same
                      value of gp_order are called in a non-deterministic
                      order (i.e. order of calling connect_dtor does
                      not imply any order about the order of being called).
     */
    connect_t
    connect_unregistered_delete(const signal_t::slot_type &subscriber, 
                                int gp_order=0) const
    {
      WRATHassert(!m_is_registered_font);
      return m_signal.connect(gp_order, subscriber);
    }

  private:

    friend class FontDatabaseImplement;
    Font(const FontMemorySource::const_handle &h, 
         const std::string &pfilename, int pindex, FT_Face pface);

    FontMemorySource::const_handle m_memory_source;
    std::string m_filename;
    int m_face_index;
    std::string m_label;
    FontProperties m_properties;
    vecN<MetaFont*, 1+last_resort> m_meta_font;
    bool m_is_registered_font;
    mutable signal_t m_signal;
  };

  /*!\class MetaFont
    A MetaFont represents a family of fonts
    for the purpose of font merging (i.e.
    if a font does not have a glyph for a 
    specified character code, then to use 
    the glyph from another font).
   */
  class MetaFont:boost::noncopyable
  {
  public:
    /*!\typedef signal_t
      Conveniance typedef for the signal type
      fired whenever a font is added to a MetaFont
     */
    typedef boost::signals2::signal<void (Font::const_handle)> signal_t;

    /*!\typedef connect_t
      Conveniance typedef for the connection type.
    */
    typedef boost::signals2::connection connect_t;

    /*!\fn Font::const_handle first_font(void) const
      Returns a handle to the first font
      of the MetaFont
     */
    Font::const_handle
    first_font(void) const
    {
      return (!m_font_list.empty())?
        m_font_list.front():
        Font::const_handle();
    }

    /*!\fn connect_t connect(const signal_t::slot_type&, int gp_order) const
      Connect to the signal fired when a font is added
      to this MetaFont. 
      \param subscriber slot called on signal fire
      \param gp_order order of slot call. Lower values of gp_order
                      are guarnteed to be call those of higher values
                      of gp_order. Slots connected with the same
                      value of gp_order are called in a non-deterministic
                      order (i.e. order of calling connect_dtor does
                      not imply any order about the order of being called).
    */
    connect_t
    connect(const signal_t::slot_type &subscriber, int gp_order=0) const
    {
      return m_signal.connect(gp_order, subscriber);
    }

    /*!\fn connect_t connect_and_append(const signal_t::slot_type &, int, 
                                        std::list<Font::const_handle> &) const
      First append all font entries of this MetaFont
      into an std::list, then connect to the signal fired 
      when a font is added to this MetaFont. Calls are
      under a common mutex lock so that if a concurrent
      thread attempts to add a font to this MetaFont,
      that font adding will fire a signal AFTER this
      function returns and thus the signal will get caught
      \param subscriber slot called on signal fire. The slot may be
                        called from different threads than the thread in
                        which the connection was made. You have been warned.
      \param gp_order order of slot call. Lower values of gp_order
                      are guarnteed to be call those of higher values
                      of gp_order. Slots connected with the same
                      value of gp_order are called in a non-deterministic
                      order (i.e. order of calling connect_dtor does
                      not imply any order about the order of being called)
      \param out_list std::list to which to append the font list
    */
    connect_t
    connect_and_append(const signal_t::slot_type &subscriber, int gp_order, 
                       std::list<Font::const_handle> &out_list) const;

    /*!\fn connect_t connect_and_append(const signal_t::slot_type &,std::list<Font::const_handle> &) const
      Provided as a conveniance, equivalent to
      \code
      connect_and_append(subscriber, 0, out_list);
      \endcode
      \param subscriber slot called on signal fire
      \param out_list std::list to which to append the font list
     */
    connect_t
    connect_and_append(const signal_t::slot_type &subscriber, 
                       std::list<Font::const_handle> &out_list) const
    {
      return connect_and_append(subscriber, 0, out_list);
    }

  private:
    friend class FontDatabaseImplement;

    MetaFont(void);

    void
    add_font(const Font::const_handle &hnd);

    mutable WRATHMutex m_mutex;
    std::set<Font::const_handle> m_font_set;
    std::list<Font::const_handle> m_font_list;
    mutable signal_t m_signal;


  };

  /*!\fn const MetaFont* master_meta_font(void)
    Returns the master MetaFont, which is the
    MetaFont consisting of a list of ALL fonts
   */
  const MetaFont*
  master_meta_font(void);

  /*!\fn Font::const_handle fetch_font_entry(const std::string&, int,
                                             const FontMemorySource::const_handle&) 
    Returns a handle to a Font sourcing from
    the specified filename and face index
    or from a FontMemorySource. 
    If the filename (or FontMemorySource) is an 
    invalid font file or if the specified index 
    does not exist within the filename (respectively 
    FontMemorySource), returns an invalid handle.
    Repeated calls with the same (filename, face_index)
    pair will return the exact same handle value.
    In particular, if a (filename, face_index)
    pair already exists in the font database, the
    FontMemory object is ignored. Thus, in the case
    that the entry (filename, face_index) will
    source from the exact source it was specified
    as in the first call to fetch_font_entry() with
    the entry (filename, face_index).
    
    Maybe called from multiple threads concurrently.

    Note that fetching a Font from a (filename, face_index)
    pair adds it to the font database if that pair
    has yet been encountered.

    It is acceptable to add fonts from multiple
    threads concurrently. However, any previously
    formatted text \ref WRATHFormattedTextStream)
    and thus text attribute from such formatted text
    (see \ref WRATHTextAttributePacker) will not
    use that new font as a fallback until the text
    is formatted and repacked.

    \param pfilename filename from which to source the font
    \param pface_index face index within the file from which
                       to source the font
    \param h if valid indicates to source the file from a
             FontMemorySource and thus pfilename gives the
             name of the font rather than a filename
   */
  Font::const_handle
  fetch_font_entry(const std::string &pfilename, int pface_index,
                   const FontMemorySource::const_handle &h=FontMemorySource::const_handle());

  
  /*!\fn std::vector<Font::const_handle> fetch_font_entries(const std::string&, 
                                                            const FontMemorySource::const_handle&)
    Returns an array of all fonts within a file (or in a memory source).
    If the file is an invalid file (repsectively the memory source
    is in invalid file data) then returns an empty array.
    \param pfilename filename from which to source the fonts
    \param h if valid indicates to source the file from a
             FontMemorySource and thus pfilename gives the
             name of the fonts rather than a filename
   */
  std::vector<Font::const_handle>
  fetch_font_entries(const std::string &pfilename,
                     const FontMemorySource::const_handle &h=FontMemorySource::const_handle());

 

  /*!\fn Font::const_handle create_unregistered_font(const std::string&, int, 
                                                     const FontMemorySource::const_handle &)
     Create a font that is NOT registered to the
     font database. The font maybe sourced from
     a file or memory. Since the font is not
     a part of the font database, it is not
     in the master meta font (\ref master_meta_font()).
     However, it will still support font merging
     from other fonts that are registered.

     \param pname specifies the name of the font, this value is the return
                  value for Font::name(), but has no impact on the font
                  database since the font is not added to the database.
     \param pface_index specifies the face index of the font
     \param h a handle to the FontMemorySource from which to source
              the font. If the handle is not valid, then sources
              from a file.
   */
  Font::const_handle
  create_unregistered_font(const std::string &pname, int pface_index,
                           const FontMemorySource::const_handle &h);


  /*!\fn std::vector<Font::const_handle> create_unregistered_fonts(const std::string&, 
                                                                   const FontMemorySource::const_handle&)
     An analogue of fetch_font_entries() for unregistered fonts.
     \param pname specifies the name of the fonts, this value is the return
                  value for Font::name(), but has no impact on the font
                  database since the font is not added to the database.
     \param h a handle to the FontMemorySource from which to source
              the font. If the handle is not valid, then sources
              from a file.
    
   */
  std::vector<Font::const_handle>
  create_unregistered_fonts(const std::string &pname, 
                            const FontMemorySource::const_handle &h);

  /*!\fn enum return_code release_unregistered_font(const Font::const_handle&)
    For a font created with create_unregistered_font()
    or create_unregistered_fonts(), release the unregistered
    font. If the font passed is registered returns
    routine_fail and does nothing otherwise deletes the
    font from possible use (including deleting resources,
    for example WRATHTextureFont derived objects, that 
    source from the font).
    \param hnd handle to font as returned by \ref create_unregistered_font() or
               \ref create_unregistered_fonts()
   */
  enum return_code
  release_unregistered_font(const Font::const_handle &hnd);
  
  /*!\fn release_unregistered_fonts(iterator, iterator)
    Provided as a conveniance to call 
    release_unregistered_font() on a set
    of fonts.
    \tparam iterator iterator type to Font::const_handle
    \param begin iterator to 1st font to release
    \param end iterator to one past the last font to release
   */
  template<typename iterator>
  void
  release_unregistered_fonts(iterator begin, iterator end)
  {
    for(;begin!=end; ++begin)
      {
        release_unregistered_font(*begin);
      }
  }

  /*!\fn Font::const_handle fetch_font_entry(const FontProperties&)
    Fetch a handle to a Font from a description of 
    a font. Will use a platform specific font
    matching library.
    \param properties Font properties
   */
  Font::const_handle
  fetch_font_entry(const FontProperties &properties);

  /*!\fn Font::const_handle fetch_font_entry_naive(const FontProperties&)
    Fetch a handle to a Font from a description of 
    a font. Will simply use the current fonts
    in the WRATHFontDatabase to attempt to perform
    a match.
    \param properties Font properties
   */
  Font::const_handle
  fetch_font_entry_naive(const FontProperties &properties);
  

}

/*! @} */

#endif
