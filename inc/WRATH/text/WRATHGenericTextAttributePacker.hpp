/*! 
 * \file WRATHGenericTextAttributePacker.hpp
 * \brief file WRATHGenericTextAttributePacker.hpp
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



/*! \addtogroup Text
 * @{
 */

#ifndef __WRATH_GENERIC_TEXT_ATTRIBUTE_PACKER_HPP__
#define __WRATH_GENERIC_TEXT_ATTRIBUTE_PACKER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHTextAttributePacker.hpp"
#include "WRATHCanvas.hpp"

/*!\class WRATHGenericTextAttributePacker
  A WRATHGenericTextAttributePacker provides 
  a simpler, per-attribute packing interface
  while handling the details of walking 
  a WRATHFormattedTextStream. The class
  WRATHGenericTextAttributePacker implements
  both allocation_requirement() and 
  set_attribute_data(). A derived class
  needs to implement:
  - \ref pack_attribute()
  - \ref attribute_key()

  and may optionally implement

  - \ref begin_range()
  - \ref end_range().
 */
class WRATHGenericTextAttributePacker:public WRATHTextAttributePacker
{
public:

  /*!\enum PackerType
    There are two versions for it's packing: using
    a single quad per glyph or using multiple primitives
    per glyph (see \ref WRATHTextureFont::glyph_data_type::support_sub_primitives() ).
   */
  enum PackerType
    {
      /*!
        use the packer that uses a single
        quad for the glyph.
       */
      SingleQuadPacker=0,
            
      /*!
        Use a packer that uses the sub-primitives
        of a glyph, thus a single glyph is
        multiple primitives that potentially
        cover a smaller area than the primitive
        of the glyph.
       */
      SubPrimitivePacker=1
    };

  /*!\class glyph_data
    A glyph_data holds the commonly used data
    for packing the attribute associated to
    a glyph.
   */
  class glyph_data
  {
  public:
    /*!\fn glyph_data
      Default ctor.
     */
    glyph_data(void):
      m_z_position(-1.0f),
      m_scale(1.0f),
      m_horizontal_stretching(1.0f),
      m_vertical_stretching(1.0f),
      m_color(WRATHText::color_type(0xff, 0xff, 0xff, 0xff)),
      m_character_data(NULL),
      m_glyph(NULL)
    {}

    /*!\var m_index
      The index into the WRATHFormattedTextDataStream
      from which this glyph_data originates
     */
    int m_index;

    /*!\var m_z_position
      \ref WRATHGenericTextAttributePacker::set_attribute_data
      tracks the z-position from the WRATHStateStream 
      (see \ref WRATHText::z_position), this is the z-value
      at \ref m_index within the WRATHStateStream.
     */
    WRATHText::z_position::type m_z_position;

    /*!\var m_scale
      \ref WRATHGenericTextAttributePacker::set_attribute_data
      tracks the scale factor from the WRATHStateStream 
      (see \ref WRATHText::scale), this is the scale factor
      at \ref m_index within the WRATHStateStream.
     */
    WRATHText::scale::type m_scale;

    /*!\var m_horizontal_stretching
      \ref WRATHGenericTextAttributePacker::set_attribute_data
      tracks the strecthing factor from the WRATHStateStream 
      (see \ref WRATHText::horizontal_stretching), 
      this is the stretch factor at \ref m_index within the 
      WRATHStateStream. Note that the stretch factor does NOT 
      have the scaling factor (\ref m_scale) applied to it, thus the glyph 
      should be scaled by \ref m_scale * \ref m_horizontal_stretching
      horizontally.
     */
    WRATHText::scale::type m_horizontal_stretching;

    /*!\var m_vertical_stretching
      \ref WRATHGenericTextAttributePacker::set_attribute_data
      tracks the strecthing factor from the WRATHStateStream 
      (see \ref WRATHText::vertical_stretching), 
      this is the stretch factor at \ref m_index within the 
      WRATHStateStream. Note that the stretch factor does NOT 
      have the scaling factor (\ref m_scale) applied to it, thus the glyph 
      should be scaled by \ref m_scale * \ref m_vertical_stretching
      vertically.
     */
    WRATHText::scale::type m_vertical_stretching;

    /*!\var m_color
      \ref WRATHGenericTextAttributePacker::set_attribute_data
      tracks the color values from the WRATHStateStream 
      (see \ref WRATHText::set_color), this is the color
      values (each corner of the glyph) at \ref m_index.
      within the WRATHStateStream. The indexing within
      color is via WRATHFormattedTextStream::corner_type 
     */
    vecN<WRATHText::color_type, 4> m_color;

    /*!\var m_character_data
      A pointer to the character data at \ref m_index 
      of the WRATHFormattedTextStream
     */
    const WRATHFormatter::glyph_instance *m_character_data;

    /*!\var m_glyph
      Provided as a conveniance, same value
      as m_character_data->m_glyph.
     */
    const WRATHTextureFont::glyph_data_type *m_glyph;

    /*!\var m_native_position
      Values as returned by 
      WRATHFormattedTextStream::position(\ref m_index, \ref m_scale)
      
     */
    vecN<vec2, 2> m_native_position;
  };

  /*!\typedef PackerState
    A WRATHTextAttributePacker object is stateless.
    In order to track state of a packing operation,
    a WRATHGenericTextAttributePacker derived class
    will pack such data into a WRATHReferenceCountedObject
    derived object.
   */
  typedef WRATHReferenceCountedObject::handle PackerState;

  /*!\fn WRATHGenericTextAttributePacker(const ResourceKey&, 
                                         enum PackerType)
    Ctor. Specifies the resource name of the attribute packer
    and the names of each attribute in an array of strings.
    \param pname resource name (see \ref resource_name) of the packer
    \param tp to specify if the packer prefers to pack sub-quads in packing
   */
  WRATHGenericTextAttributePacker(const ResourceKey &pname,
                                  enum PackerType tp):
    WRATHTextAttributePacker(pname),
    m_type(tp)
  {}
    
  virtual
  ~WRATHGenericTextAttributePacker()
  {}

  /*!\fn enum PackerType type(void) const
    Returns the packer type, i.e. if packs sub-quads
    when packing attributes for glyphs.
   */
  enum PackerType 
  type(void) const
  {
    return m_type;
  }

  /*!\fn size_t attribute_size
    To be implemented by a dervied class to 
    return the size of the attribute type that
    this WRATHGenericTextAttributePacker packs.
    \param number_custom_data_to_use indicates the size of \ref
                                     WRATHTextureFont::GlyphGLSL::m_custom_data_use
				     which are to also be packed into attributes
   */
  virtual
  size_t
  attribute_size(int number_custom_data_to_use) const=0;
  
  /*!\fn allocation_requirement_type allocation_requirement
    Implementation of allocation_requirement().
    WRATHGenericTextAttributePacker derived objects
    must NOT reimplement allocation_requirement().
    \param R array of ranges of glyphs of pdata to set attribute data
    \param font font to use, only those glyphs of that font
                and the specified texture page are to be packed.
    \param texture_page texture page to use, only those glyphs 
                        of that texture page and the specified
                        font are to be packed.
    \param pdata formatted text stream to get the characters from
    \param state_stream state change stream of pdata  
   */
  allocation_requirement_type
  allocation_requirement(const_c_array<range_type<int> > R,
                         WRATHTextureFont *font,
                         int texture_page,
                         const WRATHFormattedTextStream &pdata,
                         const WRATHStateStream &state_stream) const;

  /*!\fn allocation_allotment_type allocation_allotment
    Implementation of allocation_allotment().
    WRATHGenericTextAttributePacker derived objects
    must NOT reimplement allocation_allotment().
    \param attributes_allowed maximum number attributes allowed
    \param Rinput input of ranges to investigate
    \param pdata formatted text stream to get the characters from
    \param state_stream state change stream of pdata  
   */
  virtual
  allocation_allotment_type
  allocation_allotment(int attributes_allowed,
                       const_c_array<range_type<int> > Rinput,
                       const WRATHFormattedTextStream &pdata,
                       const WRATHStateStream &state_stream) const;

  /*!\fn PackerState begin_range
    To be optionally implemented by a derived class.
    Called when starting a new range of text to
    pack. Default implementation is to return
    an invalid handle and do nothing
    \param R range of indices into pdata to pack
    \param font pointer to font, as in set_attribute_data()
    \param texture_page which texture page of font, as in set_attribute_data()
    \param pdata stream of formatted text to pack, as in set_attribute_data()
    \param state_stream state stream of formatted text to pack, as in set_attribute_data()
   */
  virtual
  PackerState
  begin_range(const range_type<int> &R,
              WRATHTextureFont *font,
              int texture_page,
              const WRATHFormattedTextStream &pdata,
              const WRATHStateStream &state_stream) const;

  /*!\fn void end_range
    To be optionally implemented by a derived class.
    Called when ending a range of text to
    pack. Default implementation is to do nothing.
    \param packer_state return value of the matching \ref begin_range() call
    \param R range of indices into pdata to pack
    \param font pointer to font, as in \ref set_attribute_data()
    \param texture_page which texture page of font, as in \ref set_attribute_data()
    \param pdata stream of formatted text to pack, as in \ref set_attribute_data()
    \param state_stream state stream of formatted text to pack, as in \ref set_attribute_data()
   */
  virtual
  void
  end_range(const PackerState &packer_state,
            const range_type<int> &R,
            WRATHTextureFont *font,
            int texture_page,
            const WRATHFormattedTextStream &pdata,
            const WRATHStateStream &state_stream) const;

  /*!\fn void current_glyph
    To be optionally implemented by a derived class
    to note when the attribute packing has advanced
    to the next character within a range of characters 
    from a WRATHFormattedTextStream. The typical use
    case for implementing this function is to a note
    change in a state stream held in the WRATHStateStream.
    The default implementation is a no-op.
    \param in_glyph current glyph, especially note \ref glyph_data::m_index
    \param pdata stream of formatted text to pack, as in \ref set_attribute_data()
    \param state_stream state stream of formatted text to pack, as in \ref set_attribute_data()
    \param packer_state packing state object handle as returned by \ref begin_range()
   */
  virtual
  void
  current_glyph(const glyph_data &in_glyph,
                const WRATHFormattedTextStream &pdata,
                const WRATHStateStream &state_stream,
                const PackerState &packer_state) const;

  /*!\fn void pack_attribute
    To be implemented by a derived class to perform the actual packing
    of a _single_ _attribute_ 
    \param ct which corner of the glyph, if the glyph is NOT being packed via sub attribute packing.
              If the glyph is being packed with sub-attribute packing then ct is 
              WRATHFormattedTextStream::no_corner
    \param in_glyph data common to entire glyph
    \param normalized_glyph_coordinate_float normalized glyph coordinates, with the convention
                                             that if y-coordinate increases downwards, then
                                             the top normalized coordinate is -1 instead
                                             of +1.
    \param normalized_glyph_coordinate_short normalized_glyph_coordinate_float presented as a 
                                             short consumable by GL's glVertexAttributePointer
                                             with normalize set to GL_TRUE
    \param custom_data_use taken from WRATHTextureFont::GlyphGLSL::m_custom_data_use
    \param packing_destination packing destination location to pack attribute, an implementation
                               will reinterpret_cast the pointer to the attribute type it packs,
                               the value of packing_destination.size() is \ref attribute_size();
    \param packer_state packing state object handle as returned by \ref begin_range()
   */
  virtual
  void
  pack_attribute(enum WRATHFormattedTextStream::corner_type ct,
                 const glyph_data &in_glyph,
                 const vec2 &normalized_glyph_coordinate_float,
                 vecN<GLshort,2> normalized_glyph_coordinate_short,
		 const std::vector<int> &custom_data_use,
                 c_array<uint8_t> packing_destination,
                 const PackerState &packer_state) const=0;

protected:
  /*!\fn set_attribute_data_implement
    Implementation of \ref set_attribute_data().
    WRATHGenericTextAttributePacker derived objects
    must NOT reimplement \ref set_attribute_data().
   */
  void
  set_attribute_data_implement(const_c_array<range_type<int> > R,
                               WRATHTextureFont *font,
                               int texture_page,
                               WRATHAbstractDataSink &attribute_store, 
                               const std::vector<range_type<int> > &attr_location,
                               WRATHAbstractDataSink &index_group,
                               const WRATHFormattedTextStream &pdata,
                               const WRATHStateStream &state_stream,
                               BBox *out_bounds_box) const;

private:
  enum PackerType m_type;
};


/*! @} */
#endif
