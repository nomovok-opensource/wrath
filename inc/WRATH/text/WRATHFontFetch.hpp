/*! 
 * \file WRATHFontFetch.hpp
 * \brief file WRATHFontFetch.hpp
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


#ifndef __WRATH_FONT_FETCH_HPP__
#define __WRATH_FONT_FETCH_HPP__

#include "WRATHConfig.hpp"
#include "WRATHFontDatabase.hpp"
#include "WRATHFontSupport.hpp"
#include "WRATHTextureFont.hpp"
#include "WRATHTextDataStreamManipulator.hpp"

class WRATHTextureFont;


/*! \addtogroup Text
 * @{
 */

/*!\namespace WRATHFontFetch
  Namespace to encapsulate font fetching routines
  and types
 */
namespace WRATHFontFetch
{
  /*
    Bring into scope WRATHFontDatabase
   */
  using namespace WRATHFontDatabase;

  /*!\typedef font_fetcher_t
    Typedef for fetching (possibly creating) a font
    from a pixel size and a handle to a WRATHFontDatabase::Font
   */
  typedef WRATHTextureFont::font_fetcher_t font_fetcher_t;

  /*!\class font_handle
    Provided as a conveniance, constructable
    from a Font::const_handle (which it really
    is in fact) or a FontProperties
   */
  class font_handle:public Font::const_handle
  {
  public:
    /*!\fn font_handle
      Ctor. Inits as invalid handle.
     */ 
    font_handle(void)
    {}

    /*!\fn font_handle(const Font::const_handle&)
      Ctor. Inits as from a \ref Font::const_handle
      \param hnd value to take
     */ 
    font_handle(const Font::const_handle &hnd):
      Font::const_handle(hnd)
    {}

    /*!\fn font_handle(const FontProperties&)
      Ctor. Inits as from a FontProperties
      \param prop description of font from which to fetch a handle
     */ 
    font_handle(const FontProperties &prop):
      Font::const_handle(fetch_font_entry(prop))
    {}

    /*!\fn font_handle(const std::string&, int)
      Ctor. Inits as from a filename and face index
      \param pfilename name of file holding font
      \param face_index index of face within the file if file holds multiple fonts
     */
    font_handle(const std::string &pfilename, int face_index):
      Font::const_handle(fetch_font_entry(pfilename, face_index))
    {}
  };

  /*!\fn void font_fetcher(font_fetcher_t)
    Specifies the default font fetcher for use
    when fetching fonts without specifying
    the return type. Default value is
    \ref WRATHTextureFontFreeType_TMix::fetch_font.
    \param v function pointer value to which to use
             fetch fonts.
   */
  void
  font_fetcher(font_fetcher_t v);

  /*!\fn font_fetcher_t font_fetcher(void)
    Returns the default font fetcher for use
    when fetching fonts without specifying
    the return type. Default value is
    \ref WRATHTextureFontFreeType_TMix::fetch_font.
   */
  font_fetcher_t
  font_fetcher(void);

  /*!\fn void font_fetcher(type_tag<T>)
    Conveniance function to set the default 
    font fetcher, equivalent to
    \code
    font_fetcher(&T::fetch_font);
    \endcode
    The class T must implement the static function 
    \code
    WRATHTextureFont*
    fetch_font(int psize, const Font::const_handle &hndl);
    \endcode
    \tparam T class type implementing the static function fetch_font
   */
  template<typename T>
  void
  font_fetcher(type_tag<T>)
  {
    font_fetcher_t v(&T::fetch_font);
    font_fetcher(v);
  }
  
  /*!\fn void default_font_pixel_size(int)
    For those fetch_font functions of 
    WRATHFontFetch that do not take
    a pixel size a default value is
    used, this specifies that
    pixel size. Default value is 64.
   */
  void
  default_font_pixel_size(int v);

  /*!\fn int default_font_pixel_size(void)
    For those fetch_font functions of 
    WRATHFontFetch that do not take
    a pixel size a default value is
    used, this returns that
    pixel size. Default value is 64.
   */
  int
  default_font_pixel_size(void);

  /*!\fn fetch_font(int, const font_handle&, font_fetcher_t)
    Conveninace function to fetch a \ref WRATHTextureFont
    using a \ref font_handle.    
    \param psize pixel size to use.                                     
    \param fnt handle to font source 
    \param v function pointer to use to fetch the font, if NULL then
             use the default font fetcher as set by \ref font_fetcher().
   */
  WRATHTextureFont*
  fetch_font(int psize, const font_handle &fnt, font_fetcher_t v=NULL);

  /*!\fn fetch_font(const font_handle&, font_fetcher_t)
    Conveninace function to fetch a \ref WRATHTextureFont
    using a \ref font_handle to
    specify the font. Equivalent to
    \code
    fetch_font(default_font_pixel_size(), spec, v);
    \endcode              
    \param spec handle to font source 
    \param v font fetcher function pointer used to load/fetch the font
             If NULL, then uses the fetch_font_t set in font_fetcher(font_fetcher_t).
   */
  inline
  WRATHTextureFont*
  fetch_font(const font_handle &spec, font_fetcher_t v=NULL)
  {
    return fetch_font(default_font_pixel_size(), spec, v);
  }

  /*!\fn fetch_font(int, const font_handle&, type_tag<T>)
    Conveninace function to fetch a \ref WRATHTextureFont
    using a \ref font_handle to specify the font. Equivalent to
    \code
    fetch_font(psize, spec, &T::fetch_font)
    \endcode
    The class T must implement the static function 
    \code
    WRATHTextureFont*
    fetch_font(int psize, const font_handle &);
    \endcode
    \tparam class T that implement static function fetch_font
    \param psize pixel size to use.                                     
    \param spec handle to font source 
   */
  template<typename T>
  WRATHTextureFont*
  fetch_font(int psize, const font_handle &spec, type_tag<T>)
  {
    font_fetcher_t v(&T::fetch_font);
    return fetch_font(psize, spec, v);
  }

  /*!\fn fetch_font(const font_handle&, type_tag<T>)
    Conveninace function to fetch a \ref WRATHTextureFont
    using a \ref font_handle to specify the font. Equivalent to
    \code
    fetch_font(spec, &T::fetch_font)
    \endcode
    The class T must implement the static function 
    \code
    WRATHTextureFont*
    fetch_font(int psize, const std::string &pfilename, int face_index);
    \endcode                                
    \param spec font source
   */
  template<typename T>
  WRATHTextureFont*
  fetch_font(const font_handle &spec, type_tag<T>)
  {
    font_fetcher_t v(&T::fetch_font);
    return fetch_font(spec, v);
  }

  /*!\fn void default_font(const font_handle&)
    Specify the default font source used in WRATHTextDataStream
    objects when they are initialized.
    \param v default font value to use 
   */
  void
  default_font(const font_handle &v);

  /*!\fn font_handle default_font(void)
    Returns the default font used in WRATHTextDataStream
    objects when they are initialized as a font_handle
   */
  font_handle
  default_font(void);

  /*!\fn fetch_default_font(font_fetcher_t)
    Provided as a conveniance, equivalent to:
    \code
    fetch_font(default_font(), v);
    \endcode
    \param v function pointer to use to fetch the font, if NULL then
             use the default font fetcher as set by \ref font_fetcher().
   */
  inline
  WRATHTextureFont*
  fetch_default_font(font_fetcher_t v=NULL)
  {
    return fetch_font(default_font(), v);
  }

};

namespace WRATHText
{
  /*!\fn font::set_type set_font(const WRATHFontFetch::font_handle&)
    Overload for a function manipulator of \ref WRATHText::font
    taking a \ref WRATHFontFetch::font_handle object.
    Equivalent to
    \code
    WRATHText::set_font(WRATHFontFetch::fetch_font(spec)) 
    \endcode
    \param spec font source
   */
  inline
  font::set_type
  set_font(const WRATHFontFetch::font_handle &spec)
  {
    return set_font(WRATHFontFetch::fetch_font(spec));
  }

  /*!\fn font::set_type set_font(const WRATHFontFetch::font_handle&, type_tag<T>)
    Overload for a function manipulator of \ref WRATHText::font
    taking a \ref WRATHFontFetch::font_handle object.
    Equivalent to
    \code
    WRATHText::set_font(WRATHFontFetch::fetch_font(spec, type_tag<T>()) ) 
    \endcode
    \tparam T WRATHTextureFont derived type 
    \param spec font source
   */
  template<typename T>
  inline
  font::set_type
  set_font(const WRATHFontFetch::font_handle &spec, type_tag<T>)
  {
    return set_font(WRATHFontFetch::fetch_font(spec, type_tag<T>()) );
  }

  /*!\fn font::push_type push_font(const WRATHFontFetch::font_handle&)
    Overload for a function manipulator of \ref WRATHText::font
    taking a \ref WRATHFontFetch::font_handle object.
    Equivalent to
    \code
    WRATHText::push_font(WRATHFontFetch::fetch_font(spec)) 
    \endcode
    \param spec font source
   */
  inline
  font::push_type
  push_font(const WRATHFontFetch::font_handle &spec)
  {
    return push_font(WRATHFontFetch::fetch_font(spec));
  }

  /*!\fn font::push_type push_font(const WRATHFontFetch::font_handle&, type_tag<T>)
    Overload for a function manipulator of \ref WRATHText::font
    taking a \ref WRATHFontFetch::font_handle object.
    Equivalent to
    \code
    WRATHText::push_font(WRATHFontFetch::fetch_font(spec, type_tag<T>()) ) 
    \endcode
    \tparam T WRATHTextureFont derived type 
    \param spec font source
   */
  template<typename T>
  inline
  font::push_type
  push_font(const WRATHFontFetch::font_handle &spec, type_tag<T>)
  {
    return push_font(WRATHFontFetch::fetch_font(spec, type_tag<T>()) );
  }
}


/*! @} */

#endif
