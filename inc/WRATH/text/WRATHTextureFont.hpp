/*! 
 * \file WRATHTextureFont.hpp
 * \brief file WRATHTextureFont.hpp
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





#ifndef __WRATH_TEXTURE_FONT_HPP__
#define __WRATH_TEXTURE_FONT_HPP__

#include "WRATHConfig.hpp"
#include <stdint.h>
#include <boost/utility.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include "WRATHgl.hpp"
#include "vecN.hpp"
#include "vectorGL.hpp"
#include "c_array.hpp"
#include "WRATHNew.hpp"
#include "WRATHResourceManager.hpp"
#include "WRATHTextureChoice.hpp"
#include "WRATHGLProgram.hpp"
#include "WRATHFontSupport.hpp"
#include "WRATHFontDatabase.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\typedef WRATHTextureFontKey
  Key used for the resource name of a WRATHTextureFont,
  which is a tuple comprising of 
  - a const_handle to a WRATHFontDatabase::Font
  - a pixel size.
  - a string providing type information, advised to be typeid().name()
 */
typedef boost::tuple<WRATHFontDatabase::Font::const_handle, int, std::string> WRATHTextureFontKey;

/*!\class WRATHTextureFont
  A WRATHTextureFont is an interface class for fonts realized as texture(s), for
  example a coverage value stored in the texture is called a glyph cache.

  In this system the shapes of the characters associated to a given font are
  encoded in one or more GL textures. This information is accesed by the
  WRATHTextureFontDrawer's associated fragment shaders and used to render each
  character.

  The way each font encodes this information into textures and how the fragment
  shaders use the information to render the actual text are implementation
  dependent, and therefore defined by dervied classes of WRATHTextureFont.
 */  
class WRATHTextureFont:boost::noncopyable
{
public:
  
  class glyph_data_type;

  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHTextureFont, WRATHTextureFontKey);
  /// @endcond

  /*!\enum font_scalability_tag
    Enumeration type tag to indicate if a texture font
    is scalable or not.
   */
  enum font_scalability_tag
    {
      /*!
        indicates the derived class of WRATHTextureFont
        is scalable, i.e. can draw the glyphs are different
        sizes freely.
       */
      font_is_scalable,

      /*!
        indicates the derived class of WRATHTextureFont
        is NOT scalable, i.e. drawing the glyph at a 
        different size can give poor render results.
       */
      font_is_not_scalable
    };

  /*!\typedef glyph_index_type
    Conveniance typedef to WRATHFontSupport::glyph_index_type
   */
  typedef WRATHFontSupport::glyph_index_type glyph_index_type;

  /*!\typedef character_code_type
    Conveniance typedef to WRATHFontSupport::character_code_type
   */
  typedef WRATHFontSupport::character_code_type character_code_type;

  /*!\typedef font_glyph_index
    Conveniance typedef for when using glyphs from a sibling font
   */
  typedef std::pair<WRATHTextureFont*, glyph_index_type> font_glyph_index;

  /*!\typedef font_fetcher_t
    Typedef for function type fetching (possibly creating) a font
    from a pixel size and a handle to a WRATHFontDatabase::Font
   */
  typedef WRATHTextureFont* (*font_fetcher_t)(int psize,
                                              const WRATHFontDatabase::Font::const_handle &hndl);

  /*!\class sub_primitive_attribute
    Typically, the quad of a glyph will have
    a large portion of which is clear. For GPU's
    for which discard impacts performance 
    (for example SGX), it is usually wiser
    to render primitives which cover the glyph
    but do not cover the majority of the empty
    space of the glyph. A sub_primitive_attribute holds
    the attribute data for such a primitive: a texel
    coordinates and the position within the texture
    for a vertex.
   */
  class sub_primitive_attribute
  {
  public:

    /*!\fn sub_primitive_attribute(void)
      Default ctor, leaves all values uninitialized.
     */
    sub_primitive_attribute(void)
    {}

    /*!\fn sub_primitive_attribute(const glyph_data_type&,
                                   int, int)
      Ctor to set attribute from a _texel_ coordinate,
      calls set(const glyph_data_type&, int ,int) to set the values.

      \param in_glyph glyph_data to which to be an attribute
      \param relative_native_texel_coordinate_x x-texel coordinate within the glyph
      \param relative_native_texel_coordinate_y y-texel coordinate within the glyph
     */
    sub_primitive_attribute(const glyph_data_type &in_glyph,
                            int relative_native_texel_coordinate_x,
                            int relative_native_texel_coordinate_y)
    {
      set(in_glyph, relative_native_texel_coordinate_x, relative_native_texel_coordinate_y);
    }

    /*!\fn sub_primitive_attribute(const glyph_data_type&, const ivec2&)
      Ctor to set attribute from a _texel_ coordinate,
      calls set(const glyph_data_type&, const ivec2&) to set the values.

      \param in_glyph glyph_data to which to be an attribute
      \param relative_native_texel_coordinate texel coordinate within the glyph
     */
    sub_primitive_attribute(const glyph_data_type &in_glyph,
                            const ivec2 &relative_native_texel_coordinate)
    {
      set(in_glyph, relative_native_texel_coordinate);
    }

    /*!\fn void set(const glyph_data_type&, int, int)
      Sets the values of the attribute from a _texel_ coordinate
      of the native resolution of a \ref glyph_data_type object.
      (i.e. in pixels not a texture coordinate).
      The assumption is that \ref glyph_data_type::texel_values() 
      of the \ref glyph_data_type are already set. This ctor
      correctly sets, the minified the magnified
      texel coordinate and the position within the
      glyph from a relative texel coordinate within
      the glyph. For example, sub_primitive_attribute(gl, ivec2(0,0))
      will give an attribute that is the bottom left
      and sub_primitive_attribute(gl, gl.texel_size()) that
      is the top right.
      \param in_glyph glyph_data to which to be an attribute
      \param relative_native_texel_coordinate_x x-texel coordinate within the glyph
      \param relative_native_texel_coordinate_y y-texel coordinate within the glyph 
     */
    void
    set(const glyph_data_type &in_glyph,
        int relative_native_texel_coordinate_x,
        int relative_native_texel_coordinate_y);

    /*!\fn void set(const glyph_data_type&, const ivec2&)
      Conveniance, equivalent to
      \code   
      set(in_glyph, relative_native_texel_coordinate.x(), relative_native_texel_coordinate.y());
      \endcode
     */
    void
    set(const glyph_data_type &in_glyph,
        const ivec2 &relative_native_texel_coordinate)
    {
      set(in_glyph, relative_native_texel_coordinate.x(), relative_native_texel_coordinate.y());
    }

    /*!\var m_texel_coordinates
      Texel coordinates of attribute The texel coordinates
      are in pixels within the texture, not in the
      range [0,1], just as \ref glyph_data_type::texel_upper_right()
      and \ref glyph_data_type::texel_lower_left().
     */
    ivec2 m_texel_coordinates;
       
    /*!\var m_position_within_glyph_coordinate
      The position within the texel of the attribute,
      a value of (0,0) indicates the bottom left corner
      of the glyph and (1,1) indicate the top right
      corner.
     */
    vec2 m_position_within_glyph_coordinate;
  };

  /*!\class glyph_data_type
    A glyph_data_type holds the "position" of 
    and dimensions of a character within the 
    texture, both values are given in _pixels_,
    this way if the texture is resized the
    data is still valid.
   */
  class glyph_data_type:boost::noncopyable
  {
  public:
    /*!\fn glyph_data_type
      Default ctor, initializes as all
      texture coordinates at corners (0,0),
      all sizes as (0,0), origin as (0,0) advance as 0
      and texture page as 0 and the owning font as
      NULL.
     */
    glyph_data_type(void):
      m_font(NULL),
      m_texels(std::make_pair(ivec2(0,0), ivec2(0,0))),
      m_sizes(vec2(0,0)),
      m_origin(vec2(0,0)),
      m_advance(0,0),
      m_iadvance(0,0),
      m_texture_page(-1),
      m_bbox_size(0,0)
    {}
   
    virtual
    ~glyph_data_type()
    {}

    /*!\fn const ivec2& texel_lower_left
      Gives the exact pixel of the texel
      used for the lower left hand corner
      of the glyph.
     */
    const ivec2&
    texel_lower_left(void) const
    {
      return m_texels.first;
    }

    /*!\fn const ivec2& texel_upper_right
      Gives the exact pixel of the texel
      used for the upper right hand corner
      of the glyph.
     */
    const ivec2&
    texel_upper_right(void) const
    {
      return m_texels.second;
    }

    /*!\fn ivec2 texel_size
      Returns the size if the glyph on the texture,
      i.e. texel_upper_right()-texel_lower_left()
     */
    ivec2
    texel_size(void) const
    {
      return texel_upper_right()-texel_lower_left();
    }
   
    /*!\fn const vec2& origin(void) const
      Gives the offset to display the glyph,
      for example the letter 'y' hangs below
      the origin.
     */
    const vec2&
    origin(void) const
    {
      return m_origin;
    }

    /*!\fn const vec2& display_size
      Returns \ref texel_size() as a vec2.
     */
    const vec2&
    display_size(void) const
    {
      return m_sizes;
    }

    /*!\fn float display_width
      Equivalent to \code display_size().x() \endcode
     */
    float
    display_width(void) const
    {
      return m_sizes.x();
    }

    /*!\fn float display_height
      Equivalent to \code display_size().y() \endcode
     */
    float
    display_height(void) const
    {
      return m_sizes.y();
    }

    /*!\fn const vec2& advance(void) const
      Returns the amount to advance the "pen"
      after drawing the glyph. Units are in 
      pixels.
     */
    const vec2&
    advance(void) const
    {
      return m_advance;
    }

    /*!\fn const ivec2& iadvance(void) const
      Returns the amount to advance the "pen"
      after drawing the glyph. Units are in 
      26.6 pixels, i.e. in units of 64th's of
      a pixel.
     */
    const ivec2&
    iadvance(void) const
    {
      return m_iadvance;
    }

    /*!\fn int texture_page(void) const
      Returns of which texture page the glyph is a part.
      This value is the argument to pass to
      WRATHTextureFont::texture_binder(int) to get the
      the texture binder set that stores the glyph.
     */
    int
    texture_page(void) const
    {
      return m_texture_page;
    }

    /*!\fn const_c_array<WRATHTextureChoice::texture_base::handle> texture_binder
      Provided as a conveniance, returns the texture binder
      of the page of the glyph, i.e. is equivalent to:
      \code
      font()->texture_binder(texture_page())
      \endcode
      However, if font() is NULL, returns an empty array.
     */
    const_c_array<WRATHTextureChoice::texture_base::handle>
    texture_binder(void) const
    {
      return (font()!=NULL)?
        font()->texture_binder(texture_page()):
        const_c_array<WRATHTextureChoice::texture_base::handle>();
    }

    /*!\fn ivec2 texture_size
      Provided as a conveniance, returns the texture size
      of the page of the glyph, i.e. is equivalent to:
      \code
      font()->texture_size(L, texture_page())
      \endcode
      However, if font() is NULL, returns ivec2(0,0).
     */
    ivec2
    texture_size(void) const
    {
      return (font()!=NULL)?
        font()->texture_size(texture_page()):
        ivec2(0,0);
    }

    /*!\fn bool support_sub_primitives
      Returns true if this glyph support 
      sub-primitive drawing, see also
      \ref sub_primitive_attributes(), 
      \ref sub_primitive_indices().
     */
    bool
    support_sub_primitives(void) const
    {
      return !m_sub_primitive_attributes.empty()
        and !m_sub_primitive_indices.empty();
    }
    
    /*!\fn const std::vector<sub_primitive_attribute>& sub_primitive_attributes
      Typically, the quad of a glyph will have
      a large portion of which is clear. For GPU's
      for which discard impacts performance 
      (for example SGX), it is usually wiser
      to render primitives which cover the glyph
      but do not cover the majority of the empty
      space of the glyph. The routine sub_primitive_attributes()
      returns an array of attribute data of
      type \ref sub_primitive_attribute givin the
      attributes of the primitives. The coordinates are
      ONLY for the native resolution of the font.
      Note that a glyph may or may not support
      sub-primitives. In the case it does, both
      sub_primitive_attributes() and sub_primitive_indices()
      will be non-empty.
     */
    const std::vector<sub_primitive_attribute>&
    sub_primitive_attributes(void) const
    {
      return m_sub_primitive_attributes;
    }

    /*!\fn const sub_primitive_attribute& sub_primitive_attribute_value(int) const
      Equivalent to:
      \code
      sub_primitive_attributes()[idx]
      \endcode
      \param idx index of sub-primitive attribute to fetch
     */
    const sub_primitive_attribute&
    sub_primitive_attribute_value(int idx) const
    {
      return m_sub_primitive_attributes[idx];
    }

    /*!\fn const std::vector<uint16_t>& sub_primitive_indices(void) const
      See also \ref sub_primitive_attributes(). Returns
      the indices that specifies triangles for
      the sub primitives. These are indices into
      the array returned by sub_primitive_attributes().
     */
    const std::vector<uint16_t>&
    sub_primitive_indices(void) const
    {
      return m_sub_primitive_indices;
    }

    /*!\fn uint16_t sub_primitive_index(int) const
      Equivalent to:
      \code
      sub_primitive_indices()[idx]
      \endcode
      \param idx index of sub-primitive index to fetch
     */
    uint16_t
    sub_primitive_index(int idx) const
    {
      return m_sub_primitive_indices[idx];
    }

    /*!\fn WRATHTextureFont* font(void) const
      Returns the font that generated
      this glyph_data_type.
     */
    WRATHTextureFont*
    font(void) const
    {
      return m_font;
    }

    /*!\fn character_code_type character_code(void) const
      Returns the character_code of the glyph.
     */
    character_code_type
    character_code(void) const
    {
      return m_character_code;
    }

    /*!\fn glyph_index_type glyph_index(void) const
      Returns the glyph_index of the glyph.
    */    
    glyph_index_type
    glyph_index(void) const
    {
      return m_glyph_index;
    }

    /*!\fn const vec2& bounding_box_size(void) const
      Returns the size of the 
      bounding box to be used for
      line advancing of the glyph.
     */
    const vec2&
    bounding_box_size(void) const
    {
      return m_bbox_size;
    }

    /*!\fn glyph_data_type& texel_values
      Set the texel_lower_left() and texel_lower_right().
      \param bl value to assign to texel_lower_left()
      \param sz _SIZE_ in pixels of glyph on the texture,
                i.e. will assign to texel_upper_right()
                the value bl+sz.
     */
    glyph_data_type&
    texel_values(const ivec2 &bl, const ivec2 &sz)
    {
      m_texels.first=bl;
      m_texels.second=bl+sz;
      m_sizes=vec2( static_cast<float>(sz.x()),
                       static_cast<float>(sz.y()) );
      return *this;
    }

    /*!\fn glyph_data_type& origin(const vec2&)
      Set the value returned by origin(void).
      Initial value is (0,0).
      \param v value to assign to origin(void).
     */
    glyph_data_type&
    origin(const vec2 &v)
    {
      m_origin=v;
      return *this;
    }

    /*!\fn glyph_data_type& origin(const ivec2&)
      Set the value returned by origin(void).
      Initial value is (0,0).
      \param v value to assign to origin(void).
     */
    glyph_data_type&
    origin(const ivec2 &v)
    {
      m_origin.x()=static_cast<float>(v.x());
      m_origin.y()=static_cast<float>(v.y());
      return *this;
    }
    
    /*!\fn glyph_data_type& advance(const vec2 &)
      Set the value returned by advance(void) and
      iadvance(void). Initial value is (0,0). 
      \param v value to assign to advance(void).  
               Units are in pixels.
     */
    glyph_data_type&
    advance(const vec2 &v)
    {
      m_advance=v;
      m_iadvance=ivec2( 64.0f*v.x(), 64.0f*v.y());
      return *this;
    }

    /*!\fn glyph_data_type& iadvance(const ivec2 &)
      Set the value returned by advance(void) and
      iadvance(void). Initial value is (0,0).
      \param v value to assign to advance(void). 
               Units are in 26.6 pixels, i.e. in 
               units of 64th's of a pixel.
     */
    glyph_data_type&
    iadvance(const ivec2 &v)
    {
      m_iadvance=v;
      m_advance=vec2( static_cast<float>(v.x())/64.0f, 
                      static_cast<float>(v.y())/64.0f);
      return *this;
    }

    /*!\fn glyph_data_type& bounding_box_size(const vec2&)
      Set the value returned by bounding_box_size(void).
      Initial value is (0,0).
      \param v value to assign to bounding_box_size(void).
     */
    glyph_data_type&
    bounding_box_size(const vec2 &v)
    {
      m_bbox_size=v;
      return *this;
    }

    /*!\fn glyph_data_type& bounding_box_size(const ivec2&)
      Set the value returned by bounding_box_size(void).
      Initial value is (0,0).
      \param v value to assign to bounding_box_size(void).
     */
    glyph_data_type&
    bounding_box_size(const ivec2 &v)
    {
      m_bbox_size.x()=static_cast<float>(v.x());
      m_bbox_size.y()=static_cast<float>(v.y());
      return *this;
    }

    /*!\fn glyph_data_type& font(WRATHTextureFont*)
      Set the value returned by font(void).
      Initial value is NULL.
      \param v value to assign to font(void).
     */
    glyph_data_type&
    font(WRATHTextureFont *v)
    {
      m_font=v;
      return *this;
    }

    /*!\fn std::vector<sub_primitive_attribute>& sub_primitive_attributes(void)
      Returns a reference to the the array
      holding the sub-primitive attributes.
     */
    std::vector<sub_primitive_attribute>&
    sub_primitive_attributes(void) 
    {
      return m_sub_primitive_attributes;
    }

    /*!\fn glyph_data_type& number_sub_primitive_attributes(int)
      Set the number of sub-primitive attributes, equivalent
      to
      \code
      sub_primitive_attributes().resize(cnt)
      \endcode
      \param cnt number of sub-primitive attribute to have
     */
    glyph_data_type&
    number_sub_primitive_attributes(int cnt)
    {
      m_sub_primitive_attributes.resize(cnt);
      return *this;
    }

    /*!\fn sub_primitive_attribute& sub_primitive_attribute_value(int)
      Returns a refernce to the named sub-primitive
      attribute, equivalent to
      \code
      sub_primitive_attributes()[idx]
      \endcode 
      \param idx index of sub-primitive attribute to fetch
     */
    sub_primitive_attribute&
    sub_primitive_attribute_value(int idx)
    {
      return m_sub_primitive_attributes[idx];
    }

    /*!\fn glyph_data_type& sub_primitive_attribute_value(int, const sub_primitive_attribute&)
      Set the value of the named sub-primitive attribute,
      equivalent to
      \code
      sub_primitive_attributes()[idx]=v;
      \endcode 
      \param idx index of sub-primitive attribute to set
      \param v value to which to assign to the named sub-primitive attribute
     */
    glyph_data_type&
    sub_primitive_attribute_value(int idx, 
                             const sub_primitive_attribute &v)
    {
      m_sub_primitive_attributes[idx]=v;
      return *this;
    }

    /*!\fn std::vector<uint16_t>& sub_primitive_indices(void) 
      Returns a reference to the the array
      holding the indices for the triangle
      "commands" of the sub-primitives.
     */
    std::vector<uint16_t>&
    sub_primitive_indices(void) 
    {
      return m_sub_primitive_indices;
    }

    /*!\fn glyph_data_type& number_sub_primitive_indices
      Set the number of sub-primitive attributes, equivalent
      to
      \code
      sub_primitive_indices().resize(cnt)
      \endcode
      \param cnt number of sub-primitive attribute to have
     */
    glyph_data_type&
    number_sub_primitive_indices(int cnt)
    {
      m_sub_primitive_indices.resize(cnt);
      return *this;
    }

    /*!\fn uint16_t& sub_primitive_index(int)
      Returns a refernce to the named sub-primitive
      index, equivalent to
      \code
      sub_primitive_indices()[idx]
      \endcode 
      \param idx index of sub-primitive index to fetch
     */
    uint16_t&
    sub_primitive_index(int idx)
    {
      return m_sub_primitive_indices[idx];
    }

    /*!\fn glyph_data_type& sub_primitive_index(int, uint16_t)
      Set the value of the named sub-primitive index,
      equivalent to
      \code
      sub_primitive_indices()[idx]=v;
      \endcode 
      \param idx index of sub-primitive attribute to set
      \param v value to which to assign to the named sub-primitive attribute
     */
    glyph_data_type&
    sub_primitive_index(int idx, uint16_t v)
    {
      m_sub_primitive_indices[idx]=v;
      return *this;
    }

    /*!\fn glyph_data_type& texture_page(int)
      Set the value returned by texture_page(void).
      Initial value is -1.
      \param v value to assign to texture_page(void).
     */
    glyph_data_type&
    texture_page(int v)
    {
      m_texture_page=v;
      return *this;
    }

    /*!\fn glyph_data_type& character_code(character_code_type)
      Set the value returned by character_code(void).
      Initial value is 0.
      \param v value to assign to character_code(void).
     */
    glyph_data_type&
    character_code(character_code_type v)
    {
      m_character_code=v;
      return *this;
    }

    /*!\fn glyph_data_type& glyph_index(glyph_index_type)
      Set the value returned by glyph_index(void).
      Initial value is 0.
      \param v value to assign to glyph_index(void).
     */
    glyph_data_type&
    glyph_index(glyph_index_type v)
    {
      m_glyph_index=v;
      return *this;
    }

    /*!\var m_custom_float_data
      The custom floating point data
      of the glyph. This custom floating point
      data can be used by a WRATHTextureFont
      derived object type to hold floating point
      data that is specific to that type.
     */    
    std::vector<float> m_custom_float_data;

    /*!\fn float fetch_custom_float
      Provided as a conveniance, equivalent to
      \code
      if(v<m_custom_float_data.size())
        return m_custom_float_data[v];
      else
        return 0.0f;
      \endcode
     */
    float
    fetch_custom_float(unsigned int v) const
    {
      return (v<m_custom_float_data.size())?
        m_custom_float_data[v]:
        0.0f;
    }
    
  private:

    WRATHTextureFont *m_font;
    std::pair<ivec2, ivec2> m_texels;
    vec2  m_sizes;
    vec2  m_origin;
    vec2 m_advance;
    ivec2 m_iadvance;
    int m_texture_page;
    vec2 m_bbox_size;
    character_code_type m_character_code;
    glyph_index_type m_glyph_index;

    std::vector<sub_primitive_attribute> m_sub_primitive_attributes;
    std::vector<uint16_t> m_sub_primitive_indices;
  };


  /*!\class GlyphGLSL
    A GlyphGLSL object represents _how_
    a WRATHTextureFont derived object computes
    if(and/or how much) a fragment is covered.
    
    A GlyphGLSL specifies if the positional
    data within the glyph is must be computed
    in the vertex shader or can be computed 
    (non-linearly) in the fragment shader.

    For the case where the position of the glyph
    is linear, a GlyphGLSL implements:
    - In the vertex shader, implement
      \code
      void pre_compute_glyph(in vec2 glyph_position, 
                             in vec2 glyph_bottom_left,
                             in vec2 glyph_size,
                             in vec2 glyph_texture_reciprocal_size,
                             in float glyph_custom_data[])
      \endcode
      where glyph_position is the position in texels,
      glyph_bottom_left is from \ref glyph_data_type::texel_lower_left(),
      glyph_size are is from \ref glyph_data_type::texel_size(),
      glyph_texture_reciprocal_size is the reciprocal of the
      size of the texture and glyph_custom_data is an array
      of same size as \ref m_custom_data_use and the values 
      are those values coming from the glyph_data_type::m_custom_float_data
      taken from \ref m_custom_data_use. If \ref m_custom_data_use
      is empty, the last argument is to be omitted.

   - In the Fragment shader, implement the functions
     \code
     float is_covered(void)
     float compute_coverage(void)
     \endcode
     The function <B>is_covered()</B> returns
     either 1.0 or 0.0 corresponding to
     if the fragment is covered or not. The 
     function <B>compute_coverage()</B> 
     computes a value in the range [0.0, 1.0] 
     indicating how much of the fragment is
     covered. The return value is used to
     render the glyph with anti-aliasing.
   
    For the case where the position of the glyph
    is non-linear, a GlyphGLSL implements:
    - In the vertex shader, implement
      \code
      void pre_compute_glyph(in vec2 glyph_bottom_left,
                             in vec2 glyph_size,
                             in vec2 glyph_texture_reciprocal_size,
                             in float glyph_custom_data[])
      \endcode
      where glyph_bottom_left is from \ref glyph_data_type::texel_lower_left(),
      glyph_size are is from \ref glyph_data_type::texel_size(),
      glyph_texture_reciprocal_size is the reciprocal of the
      size of the texture and glyph_custom_data is an array
      of same size as \ref m_custom_data_use and the values 
      are those values coming from the glyph_data_type::m_custom_float_data
      taken from \ref m_custom_data_use. If \ref m_custom_data_use
      is empty, the last argument is to be omitted.

   - In the Fragment shader, implement the functions
     \code
     float is_covered(in vec2 glyph_position, in vec2 glyph_reciprocal_size)
     float compute_coverage(in vec2 glyph_position, in vec2 glyph_reciprocal_size)
     \endcode
     The function <B>is_covered()</B> returns
     either 1.0 or 0.0 corresponding to
     if the fragment is covered or not. The 
     function <B>compute_coverage()</B> 
     computes a value in the range [0.0, 1.0] 
     indicating how much of the fragment is
     covered. The return value is used to
     render the glyph with anti-aliasing.
   */
  class GlyphGLSL
  {
  public:

    /*!\enum glyph_position_linearity
      Enumeration type to specify shader source
      code for linearity of glyph position type.
    */
    enum glyph_position_linearity
      {
        /*!
          Indicates position within glyph is
          linear and thus computed entirely
          from vertex shader
         */
        linear_glyph_position,

        /*!
          Indicates position within glyph is
          non-linear and computed from fragment
          shader
         */
        nonlinear_glyph_position,

        /*!
          Number linearity types
         */
        num_linearity_types
      };

    /*!\typedef source_set
      Array of shader source code indexed by
      \ref glyph_position_linearity
     */
    typedef vecN<WRATHGLShader::shader_source, num_linearity_types> source_set;
    
    virtual
    ~GlyphGLSL() 
    {}

    /*!\var m_custom_data_use
      An array of indices into \ref glyph_data_type::m_custom_float_data
      specifying what floats from there to use and in what
      order, initial value is an empty array
     */
    std::vector<int> m_custom_data_use;

    /*!\var m_pre_vertex_processor
      GLSL source coded added _before_ vertex source 
      code indexed by \ref glyph_position_linearity
     */
    source_set m_pre_vertex_processor;

    /*!\var m_pre_fragment_processor
      GLSL source coded added _before_ fragment source 
      code indexed by \ref glyph_position_linearity
     */
    source_set m_pre_fragment_processor;

    /*!\var m_vertex_processor
      GLSL source code that implements the function 
      <B>pre_compute_glyph</B> indexed by \ref 
      glyph_position_linearity
     */
    source_set m_vertex_processor;

    /*!\var m_fragment_processor
      GLSL source code that implements the functions 
      <B>is_covered</B> and <B>compute_coverage</B>.
      indexed by \ref glyph_position_linearity
     */
    source_set m_fragment_processor;

    /*!\var m_sampler_names
      An array of sampler names used by the GLSL 
      code. The value at index I is the sampler 
      name for texture binder texture_binder(page)[I], 
      i.e. the order of the texture binders returned
      by \ref texture_binder(int) is the same
      order of the sampler names.
     */
    std::vector<std::string> m_sampler_names;

    /*!\var m_global_names
      List of global variables (uniforms, functions and varyings)
      that the GLSL source code induces. Does NOT include
      the values from \ref m_sampler_names.
     */
    std::vector<std::string> m_global_names;
  };
  
  /*!\fn WRATHTextureFont
    Ctor, registers the font with the resouce manager.
    \param pname resource name for the font
    \param pfetcher function pointer with which to construct
                    sibling fonts, i.e texture fonts of the exact 
                    same type sourced from different 
                    WRATHFontDatabase::Font::const_handle
                    objects
   */
  WRATHTextureFont(const WRATHTextureFontKey &pname,
                   font_fetcher_t pfetcher);

  virtual
  ~WRATHTextureFont();

  /*!\fn boost::signals2::connection connect_dtor
    The dtor of a WRATHTextureFont emit's a signal, use this function
    to connect to that signal. The signal is emitted just before
    the WRATHTextureFont is removed from the resource manager which
    in turn is before the underlying GL resources are marked
    as free.
   */
  boost::signals2::connection 
  connect_dtor(const boost::signals2::signal<void () >::slot_type &slot)
  {
    return m_dtor_signal.connect(slot);
  }

  /*!\fn const glyph_data_type& glyph_data(glyph_index_type)
    To be implemented by a derived class to
    give the glyph_data for the named glyph_index.
    A derived class should implement glyph_data()
    so that the look up is roughly just indexing
    into an array. If the glyph index is invalid
    or not supported by the font, will return
    \ref empty_glyph(). 
    \param glyph glyph index of the characer to
                 fetch, NOT a character_code.
   */
  virtual
  const glyph_data_type&
  glyph_data(glyph_index_type glyph)=0;

  /*!\fn int number_glyphs
    To be implemented by a derived class to
    return the number of glyphs that the
    font holds. For 0<=I<number_glyphs(),
    the font is expected to have a backing
    glyph for a glyph_index_type with a
    glyph_index_type::value() of I. 
   */
  virtual
  int
  number_glyphs(void)=0;

  /*!\fn glyph_index_type glyph_index
    To be implemented by a derived class to
    to give the glyph index of the named
    character from a character code. The
    expectation of a derived classes 
    implemetnation is that this method
    performs a lookup into an std::map
    or similar structure. If the font
    does not have that character code, then
    it returns a glyph_index_type object
    whose glyph_index_type::valid() method 
    returns false.
    \param C character code
   */
  virtual
  glyph_index_type
  glyph_index(character_code_type C)=0;

  /*!\fn character_code_type character_code
    To be implemented by a derived class
    to return the character code of a
    glyph index, if the glyph index is
    not valid then returns the character
    code associated to 0.
    \param G of which glyph index to query the character code
  */
  virtual
  character_code_type
  character_code(glyph_index_type G)=0;

  /*!\fn ivec2 kerning_offset(glyph_index_type, glyph_index_type)
    To be implemented by a derived class to
    return the kerning between two glyphs,
    following FreeType, in units 26.6 pixel
    units, i.e the number of pixels is
    the return value bit shifted to right
    6 bits (i.e divide by 64). If either
    of the passed glyph indice's is invalid
    or out of range of the font, then (0,0)
    is returned.
    \param left_glyph glyph index of left side of kerning query
    \param right_glyph glyph index of right side of kerning query
   */
  virtual
  ivec2
  kerning_offset(glyph_index_type left_glyph,
                 glyph_index_type right_glyph)=0;

  /*!\fn font_glyph_index glyph_index_meta(character_code_type)
    Returns a std::pair giving WRATHTextureFont 
    pointer and a glyph_index into the WRATHTextureFont
    for looking up a glyph for a given character
    code. If the font does not support the passed
    characer code, then checks for font's
    within the WRATHTextureFont's meta family
    for a font that supports the character code.    
    If no font supports that character code,
    then the WRATHTextureFont pointer is NULL and
    the glyph_index has that the method value() returns
    false.
   */
  font_glyph_index
  glyph_index_meta(character_code_type ch); 

  /*!\fn float new_line_height
    To be implemented a derived class to
    return the height of a line, i.e. the 
    number of pixels to advance on a new-line.
   */
  virtual
  float
  new_line_height(void)=0;

  /*!\fn float space_width
    Returns the width, in pixel, of the 
    space character, equivalent to
    \code 
      glyph_data(' ').advance().x() 
    \endcode
   */  
  float
  space_width(void)
  {
    character_code_type C(' ');
    glyph_index_type G(glyph_index(C));

    return glyph_data(G).advance().x();
  }

  /*!\fn float tab_width
    Returns the width, in pixel, of the 
    "tab" character, equivalent to
     \code 4.0f*space_width() \endcode
   */  
  float
  tab_width(void)
  {
    return space_width()*4.0f;
  }

  /*!\fn ivec2 texture_size
    To be implemented a derived class to
    return the size of the textures, in pixels,
    used by the font.
    \param texture_page which texture page.
   */
  virtual
  ivec2
  texture_size(int texture_page)=0;

  /*!\fn const_c_array<WRATHTextureChoice::texture_base::handle> texture_binder
    To be implemented a derived class to return 
    an array of handles to WRATHTextureChoice::texture_base
    to bind the named texture page of the font 
    to texture units and possibly flush any
    changes to the texture needed by the
    texture font. The reason why it is an array
    is that some font rendering algorithms
    require more than one texture.
    \param texture_page which texture page.
   */
  virtual
  const_c_array<WRATHTextureChoice::texture_base::handle>
  texture_binder(int texture_page)=0;

  /*!\fn int number_texture_pages
    To be implemented by a derived class to
    report the current number of texture 
    pages the WRATHTextureFont has.
   */
  virtual
  int
  number_texture_pages(void)=0;

  /*!\fn const GlyphGLSL* glyph_glsl
    To be implemented by a derived class to
    return the GLSL code (and sampler detail
    data) for drawing glyphs of the texture
    font. The return value is -type- dependent
    and not object dependent, i.e the return
    value may only depend on the underlying
    type of the object but not the object
    itself.
   */
  virtual
  const GlyphGLSL*
  glyph_glsl(void)=0;

  /*!\fn vec2 texture_size_reciprocal
    Returns the texture size multiplier
    to be used by a shader to draw the 
    glyph.
    \param texture_page which texture page
   */  
  vec2
  texture_size_reciprocal(int texture_page)
  {
    ivec2 r(texture_size(texture_page));

    r.x()=std::max(r.x(), 1);
    r.y()=std::max(r.y(), 1);

    return vec2( 1.0f/static_cast<float>(r.x()),
                 1.0f/static_cast<float>(r.y()) );
  }

  /*!\fn const WRATHTextureFontKey& resource_name
   Returns the resource key of the font,
   see WRATHTextureFontKey.
   */  
  const WRATHTextureFontKey& 
  resource_name(void) const
  {
    return m_name;
  }

  /*!\fn int pixel_size
    Provided as a conveniance,
    equivalent to resource_name().second
   */
  int
  pixel_size(void) const
  {
    return resource_name().get<1>();
  }

  /*!\fn const WRATHFontDatabase::Font::const_handle& source_font
    Provided as a conveniance, equivalent to
    \code
    resource_name().get<0>();
    \endcode
   */
  const WRATHFontDatabase::Font::const_handle&
  source_font(void) const
  {
    return resource_name().get<0>();
  }

  /*!\fn const std::string& simple_name
    Provided as a conveniance, equivalent to
    \code
    resource_name().get<0>()->label();
    \endcode
   */
  const std::string&
  simple_name(void) const
  {
    return resource_name().get<0>()->label();
  }

  /*!\fn const glyph_data_type& empty_glyph
    Returns a glyph_data_type object indicating
    an emptying glyph with an invalid glyph 
    index code but whose font is this.
   */
  const glyph_data_type&
  empty_glyph(void) const
  {
    return m_empty_glyph;
  }

  /*!\fn void increment_use_count
    Increment the use count for the WRATHTextureFont.
    Unless creating one's own custom UI item
    that uses a WRATHTextureFont directly,
    there is no need for one to call this.
   */
  void
  increment_use_count(void);

  /*!\fn void decrement_use_count
    Decrement the use count for the WRATHTextureFont.
    Unless creating one's own custom UI item
    that uses a WRATHTextureFont directly,
    there is no need for one to call this.
    If the source_font() is a non-registered
    font then the WRATHTextureFont will be 
    deleted when the use count is decremented to zero.
   */
  void
  decrement_use_count(void);

  /*!\fn ivec2 kerning_offset(std::pair<WRATHTextureFont*, glyph_index_type>,
                              std::pair<WRATHTextureFont*, glyph_index_type>)
    If the font's are the same and non-NULL returns
    the kerning between the two glyphs of that font,
    otherwise returns 0.
    \param left_glyph font and glyph index of left side of kerning query
    \param right_glyph font and glyph index of right side of kerning query
   */
  static
  ivec2
  kerning_offset(std::pair<WRATHTextureFont*, glyph_index_type> left_glyph,
                 std::pair<WRATHTextureFont*, glyph_index_type> right_glyph);


protected:
  /*!\fn on_increment_use_count()
    Called on exit to increment_use_count().
    Default implementation does nothing.
   */
  virtual
  void
  on_increment_use_count(void)
  {}

  /*!\fn on_decrement_use_count()
    Called on entry to decrement_use_count().
    Default implementation does nothing.
   */
  virtual
  void
  on_decrement_use_count(void)
  {}

private:

  void
  on_font_delete(void);

  WRATHTextureFontKey m_name;
  boost::signals2::signal<void () > m_dtor_signal;
  glyph_data_type m_empty_glyph;

  font_fetcher_t m_fetcher;
  vecN<void*, WRATHFontDatabase::last_resort+1> m_meta_texture_font;
  
  int m_use_count;
  int m_source_font_deleted;
  WRATHFontDatabase::Font::connect_t m_connect;

#ifdef WRATHDEBUG
  WRATHTextureFont *m_self;
#endif

};



/*! @} */


#endif
