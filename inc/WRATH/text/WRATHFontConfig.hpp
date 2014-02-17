/*! 
 * \file WRATHFontConfig.hpp
 * \brief file WRATHFontConfig.hpp
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


#ifndef WRATH_HEADER_FONT_CONFIG_HPP_
#define WRATH_HEADER_FONT_CONFIG_HPP_

#include "WRATHConfig.hpp"
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>
#include "type_tag.hpp"
#include "WRATHTextureFont.hpp"
#include "WRATHTextDataStreamManipulator.hpp"
#include "WRATHFontFetch.hpp"

/*! \addtogroup Text
 * @{
 */
/*!\namespace WRATHFontConfig
  The namespace WRATHFontConfig provides
  a (simpler to use) C++ interface to
  Fontconfig as it applies to WRATH
 */
namespace WRATHFontConfig
{
  /*!\class InFontSpecification
    A FontSpecification is a way of specifying 
    a font for \ref fetch_font_entry_detailed(const InFontSpecification&)
    and \ref fetch_font_entry(const InFontSpecification&).
   */
  class InFontSpecification
  {
  public:
    /*!\fn InFontSpecification(void)
      Ctor, initializes the InFontSpecification as
      not having any requirements for a font.
     */
    InFontSpecification(void):
      m_family_name(false, ""),
      m_foundary_name(false, ""),
      m_style(false, ""),
      m_weight(true, FC_WEIGHT_NORMAL),
      m_slant(true, FC_SLANT_ROMAN)
    {}

    /*!\var m_family_name
      If .first is true, .second is fed as the font
      family name to use. Default value is .first is
      false and .second is an empty string.
     */
    std::pair<bool, std::string> m_family_name;

    /*!\var m_foundary_name
      If .first is true, .second is fed as the font
      foundary (i.e. what company/organization created
      the font) to use. Default value is .first is
      false and .second is an empty string.
     */
    std::pair<bool, std::string> m_foundary_name;
    
    /*!\var m_style
      If .first is true, .second is fed as the font
      style to use, style will override both \ref
      m_weight and \ref m_slant. Default value is 
      .first is false and .second is an empty string.
     */
    std::pair<bool, std::string> m_style;
    
    /*!\var m_weight
      If .first is true, .second is fed as the font
      weight to use, must be one of FC_WEIGHT_THIN,
      FC_WEIGHT_EXTRALIGHT, FC_WEIGHT_ULTRALIGHT,
      FC_WEIGHT_LIGHT, FC_WEIGHT_BOOK, FC_WEIGHT_REGULAR,
      FC_WEIGHT_NORMAL, FC_WEIGHT_MEDIUM, FC_WEIGHT_DEMIBOLD,
      FC_WEIGHT_SEMIBOLD, FC_WEIGHT_BOLD, FC_WEIGHT_EXTRABOLD,
      FC_WEIGHT_ULTRABOLD, FC_WEIGHT_BLACK, FC_WEIGHT_HEAVY,
      FC_WEIGHT_EXTRABLACK or FC_WEIGHT_ULTRABLACK.
      Default value is .first is true and .second is
      FC_WEIGHT_NORMAL.
     */
    std::pair<bool, int> m_weight;
    
    /*!\var m_slant
      If .first is true, .second is fed as the font
      slant to use, must be one of FC_SLANT_ROMAN,
      FC_SLANT_ITALIC or FC_SLANT_OBLIQUE. Default
      value is .first is true and .second is
      FC_SLANT_ROMAN.
     */
    std::pair<bool, int> m_slant;

    /*!\var m_languages
      m_languages is a list of string representing
      languages that a font should support. The strings
      are of the form: "Ll-Tt where Ll is a two or three 
      letter language from ISO 639 and Tt is a territory 
      from ISO 3166", [http://www.freedesktop.org/software/fontconfig/fontconfig-devel/fclangsetadd.html].
     */
    std::set<std::string> m_languages;

    /*!\fn InFontSpecification& add_language
      Adds to the set \ref m_languages.
      \param v value to which to add to m_languages
     */
    InFontSpecification&
    add_language(const std::string &v)
    {
      m_languages.insert(v);
      return *this;
    }

    /*!\fn InFontSpecification& family_name
      Sets \ref m_family_name, returns a reference to this.
      \param v value to which to set \ref m_family_name
     */
    InFontSpecification&
    family_name(const std::string &v)
    {
      m_family_name.second=v;
      m_family_name.first=true;
      return *this;
    }

    /*!\fn InFontSpecification& foundry_name
      Sets \ref m_foundary_name, returns a reference to this.
      \param v value to which to set \ref m_foundary_name
     */
    InFontSpecification&
    foundry_name(const std::string &v)
    {
      m_foundary_name.second=v;
      m_foundary_name.first=true;
      return *this;
    }

    /*!\fn InFontSpecification& style
      Sets \ref m_style, returns a reference to this.
      \param v value to which to set \ref m_style
     */
    InFontSpecification&
    style(const std::string &v)
    {
      m_style.second=v;
      m_style.first=true;
      return *this;
    }

    /*!\fn InFontSpecification& weight
      Sets \ref m_weight, returns a reference to this.
      \param v value to which to set \ref m_weight
     */
    InFontSpecification&
    weight(int v)
    {
      m_weight.second=v;
      m_weight.first=true;
      return *this;
    }

    /*!\fn InFontSpecification& slant
      Sets \ref m_slant, returns a reference to this.
      \param v value to which to set \ref m_slant
     */
    InFontSpecification&
    slant(int v)
    {
      m_slant.second=v;
      m_slant.first=true;
      return *this;
    }

    /*
      ToDo: should we add any of the other properties?
     */
  };

  /*!\class FontSpecification
    A FontSpecification is a conveniance class
    to list the properties of a font as WRATH
    and Fontconfig see a font.
   */
  class FontSpecification:
    public WRATHFontDatabase::Font::const_handle
  {
  public:
    /*!\var m_fontconfig_details
      Specifiers how Fontconfig sees the font.
     */
    InFontSpecification m_fontconfig_details;

    /*!\fn WRATHFontDatabase::Font::const_handle font
      \ref FontSpecification inherits publically from 
      WRATHFontDatabase::Font::const_handle, 
      provided as a readability conveniance to see
      the WRATHFontDatabase::Font::const_handle value.
     */
    WRATHFontDatabase::Font::const_handle&
    font(void) { return *this; }
  };

  /*!\typedef FontList
    Conveniance typedef. The fonts used by WRATHFontConfig
    can be queried directly. The list is stored as a map
    keyed by handles to fonts with values as 
    FontSpecification objects.
   */
  typedef std::map<WRATHFontDatabase::Font::const_handle, FontSpecification> FontList;

  /*!\fn const FontList& font_list(void)
    Returns a reference to a map
    containing all those fonts that
    used in Fontconfig to fetch a font.
   */
  const FontList&
  font_list(void);
  
  /*!\fn const FontSpecification& fetch_font_entry_detailed(const InFontSpecification&)
    Fetches a FontSpecification to a font from a font description
    of Fontconfig
    \param spec description of font to fetch
   */
  const FontSpecification&
  fetch_font_entry_detailed(const InFontSpecification &spec);
  
  /*!\fn WRATHFontDatabase::Font::const_handle fetch_font_entry(const InFontSpecification&)
    Fetches a handle to a font from a font description
    of Fontconfig
    \param spec description of font to fetch
   */
  WRATHFontDatabase::Font::const_handle
  fetch_font_entry(const InFontSpecification &spec);
}

/*!\fn std::ostream& operator<<(std::ostream&, const WRATHFontConfig::InFontSpecification&)
  Overloaded operator<< to print the values of an WRATHFontConfig::InFontSpecification.
  \param ostr std::ostream stream to which to print
  \param obj WRATHFontConfig::InFontSpecification to print
*/
std::ostream&
operator<<(std::ostream &ostr, const WRATHFontConfig::InFontSpecification &obj);


/*!\fn std::ostream& operator<<(std::ostream &, const WRATHFontConfig::FontSpecification&)
  Overloaded operator<< to print the values of an FontSpecification.
  \param ostr std::ostream stream to which to print
  \param obj FontSpecification to print
*/
std::ostream&
operator<<(std::ostream &ostr, const WRATHFontConfig::FontSpecification &obj);

namespace WRATHText
{
  /*!\fn font::set_type set_font(const WRATHFontConfig::InFontSpecification&)
    Overload for a function manipulator of \ref WRATHText::font
    taking a \ref WRATHFontConfig::InFontSpecification object.
    Equivalent to
    \code
    WRATHText::set_font(WRATHFontFetch::fetch_font(WRATHFontConfig::fetch_font_entry(spec))) 
    \endcode
   */
  inline
  font::set_type
  set_font(const WRATHFontConfig::InFontSpecification &spec)
  {
    WRATHFontFetch::font_handle v;
    v=WRATHFontConfig::fetch_font_entry(spec);
    return set_font(WRATHFontFetch::fetch_font(v));
  }

  
  /*!\fn font::push_type push_font(const WRATHFontConfig::InFontSpecification&)
    Overload for a function manipulator of \ref WRATHText::font
    taking a \ref WRATHFontConfig::InFontSpecification object.
    Equivalent to
    \code
    WRATHText::push_font(WRATHFontFetch::fetch_font(WRATHFontConfig::fetch_font_entry(spec))) 
    \endcode
   */
  inline
  font::push_type
  push_font(const WRATHFontConfig::InFontSpecification &spec)
  {
    WRATHFontFetch::font_handle v;
    v=WRATHFontConfig::fetch_font_entry(spec);
    return push_font(WRATHFontFetch::fetch_font(v));
  }
}


/*! @} */

#endif
