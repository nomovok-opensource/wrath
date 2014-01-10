/*! 
 * \file WRATHDefaultTextAttributePacker.hpp
 * \brief file WRATHDefaultTextAttributePacker.hpp
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


#ifndef __WRATH_DEFAULT_TEXT_ATTRIBUTE_PACKER_HPP__
#define __WRATH_DEFAULT_TEXT_ATTRIBUTE_PACKER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHGenericTextAttributePacker.hpp"

/*!\class WRATHDefaultTextAttributePacker 
  A WRATHDefaultTextAttributePacker is an
  example of a WRATHGenericTextAttributePacker.

  TODO: 
    1) have it "work" so that number of
       custom data slots can be specified in 
       the fetch as well.
    2) Support mix fonts, somehow.
 */
class WRATHDefaultTextAttributePacker:public WRATHGenericTextAttributePacker
{
public:
  enum 
    {
      /*!
        location of draw position, a vec4 (in GLSL),
        attribute name is "pos"
        - .xy=position of bottom left relative to transformation node
        - .z=geometric z_position
        - .w=scaling factor applied to glyph (see \ref WRATHText::effective_scale)
       */
      position_location,  

      /*!
        location of glyph strech factor,
        a vec2 in GLSL, name in GLSL is "glyph_stretch"
       */
      glyph_stretch_location,

      /*!
        Location of the size of the glyph as according
        to \ref WRATHTextureFont::glyph_data_type::texel_size().
        Attribute name in GLSL is "glyph_size".
       */
      glyph_size_location,

      /*!
        Location of the texel (in pixel coordinates) of the glyphas 
        according to \ref WRATHTextureFont::glyph_data_type::texel_lower_left().
        Attribute name in GLSL is "glyph_bottom_left_texel".
      */
      glyph_bottom_left_texel_location,   

      /*!
        location of normalized coordinate within the glyph,
        i.e. the bottom left is (0,0) and the top right is (1,1).
        If the y-coordinate increases down the screen, then the
        top right normalized coordinate is (1, -1).
        Attribute name in GLSL is "glyph_normalized_coordinate".
       */
      glyph_normalized_coordinate_location,

      /*!
        location of color, a vec4 (in GLSL) .
        Attribute name in GLSL is "color".
      */
      color_location,

      /*!
        Custom data location; attribute packer
	supports up to -one- custom data value
	from the glyph (!)
       */
      custom_data_location
    };
  
  /*!\fn fetch
    A WRATHDefaultTextAttributePacker is stateless.
    There are two versions for it's packing: using
    a single quad per glyph or using multiple primitives
    per glyph (see \ref WRATHTextureFont::glyph_data_type::support_sub_primitives() ).
    \param tp Determines which packer type to fetch
   */
  static
  WRATHDefaultTextAttributePacker*
  fetch(enum PackerType tp=SingleQuadPacker);

  /*!\fn fetch_single_quad_packer
    A WRATHDefaultTextAttributePacker is stateless.
    There are two versions for it's packing: using
    a single quad per glyph or using multiple primitives
    per glyph (see \ref WRATHTextureFont::glyph_data_type::support_sub_primitives() ).
    This function returns the packer for using a single
    quad per glyph.
   */
  static
  WRATHDefaultTextAttributePacker*
  fetch_single_quad_packer(void)
  {
    return fetch(SingleQuadPacker);
  }

  /*!\fn fetch_sub_primitive_packer
    A WRATHDefaultTextAttributePacker is stateless.
    There are two versions for it's packing: using
    a single quad per glyph or using multiple primitives
    per glyph (see \ref WRATHTextureFont::glyph_data_type::support_sub_primitives() ).
    This function returns the packer for using a multiple
    primitives per glyph.    
   */
  static
  WRATHDefaultTextAttributePacker*
  fetch_sub_primitive_packer(void)
  {
    return fetch(SubPrimitivePacker);
  }

  virtual
  ~WRATHDefaultTextAttributePacker();

  virtual
  size_t
  attribute_size(int number_custom_data_to_use) const;
  
  virtual
  void
  attribute_names(std::vector<std::string> &out_names, int n) const;

  virtual
  void
  generate_custom_data_glsl(WRATHGLShader::shader_source &out_src,
			    int number_custom_data_to_use) const;

  virtual
  void
  pack_attribute(enum WRATHFormattedTextStream::corner_type ct,
                 const glyph_data &in_glyph,
                 const vec2 &normalized_glyph_coordinate_float,
                 vecN<GLshort,2> normalized_glyph_coordinate_short,
		 const std::vector<int> &custom_data_use,
                 c_array<uint8_t> packing_destination,
                 const PackerState &packer_state) const;
 
  virtual
  void
  attribute_key(WRATHAttributeStoreKey &pkey,
		int number_custom_data_to_use) const;
  
private:

  explicit
  WRATHDefaultTextAttributePacker(enum PackerType subpacker);
};
/*! @} */


#endif
