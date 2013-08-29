/*! 
 * \file WRATHTextureCoordinateDynamic.hpp
 * \brief file WRATHTextureCoordinateDynamic.hpp
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


#ifndef __WRATH_TEXTURE_COORDINATE_NORMALIZED_DYNAMIC_HPP__
#define __WRATH_TEXTURE_COORDINATE_NORMALIZED_DYNAMIC_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHImage.hpp"
#include "WRATHTextureCoordinate.hpp"



/*! \addtogroup Imaging
 * @{
 */

/*!\class WRATHTextureCoordinateDynamic
  A WRATHTextureCoordinateDynamic inherits
  from WRATHTextureCoordinate to have
  texture coordinates to map to. However,
  it also stores the x- and y-repeat modes
  and the source returned by source()
  allows for changing the repeat mode dynamically
  without changing shaders. The repeat specification
  is packed into one floating point number:
  - WRATH_IMAGE_repeat_mode
  which has a value A+B/10 (i.e. A.B) 
  where A encodes the x-repeat mode and
  B encodes teh y-repeat mode. The repeat modes 
  are encoded as:
  - 2 --> simple
  - 4 --> clamp
  - 6 --> repeat
  - 8 --> mirror_repeat
  
  Thus the x-repeat mode is given by
  floor(V) and the y-repeat mdoe is given
  by fract(V) where V=fetch_node_value(WRATH_IMAGE_repeat_mode)
 */
class WRATHTextureCoordinateDynamic:public WRATHTextureCoordinate
{
public:

  enum
    {
      /*!
	Enumeration specifyig how many
	per node values needed to store
	the packed data of a 
	WRATHTextureCoordinateDynamic 
        object
       */
      number_per_node_values=WRATHTextureCoordinate::number_per_node_values + 1
    };

  /*!\fn WRATHTextureCoordinateDynamic
    Ctor. Initializes to use entire texture with repeat mode
    along both axis as \ref WRATHTextureCoordinate::repeat.
   */
  WRATHTextureCoordinateDynamic(void)
  {
    set(repeat, repeat);
  }

  using WRATHTextureCoordinate::set;
    
  /*!\fn set(enum repeat_mode_type, enum repeat_mode_type)
    Set the repeat mode for each direction
    \param pxmode mode to which to set the x-repeat mode
    \param pymode mode to which to set the y-repeat mode
   */
  void
  set(enum repeat_mode_type pxmode,
      enum repeat_mode_type pymode);

  /*!\fn enum repeat_mode_type x_mode(void) const
    Returns the x-repeat mode
   */
  enum repeat_mode_type
  x_mode(void) const
  {
    return m_mode_x;
  }

  /*!\fn enum repeat_mode_type y_mode(void) const
    Returns the y-repeat mode
   */
  enum repeat_mode_type
  y_mode(void) const
  {
    return m_mode_y;
  }
  
  /*!\fn void extract_values_at
    Extracts the values of this WRATHTextureCoordinate
    object into a reorder_c_array<> starting at a given position
    suitable for the shader returned by gradient_source().
    \param start_index index value as passed to add_per_node_values_at()
                             to specify the per node values
    \param out_value target to which to pack the data, ala WRATHLayerItemNode::extract_values()
   */
  virtual
  void
  extract_values_at(int start_index, reorder_c_array<float> out_value);

  /*!\fn void add_per_node_values_at
    Adds the per-item values that a WRATHTextureCoordinate
    object requires for shader returned by gradient_source().
    The method places the per-node value locations at a given
    starting index.
    \param start_index starting index to specify the node value locations
    \param spec object to which to specify the node values to add, see
                WRATHLayerItemNode::node_function_packet::add_per_node_values()
    \param available specifies what stages per-node value fetching is available
   */
  static
  void
  add_per_node_values_at(int start_index,
			   WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
			   const WRATHLayerNodeValuePackerBase::function_packet &available);

  /*!\fn const WRATHTextureCoordinateSourceBase* source
    Returns the WRATHTextureCoordinateSourceBase to be
    used for dynamic repeat modes.
   */
  static
  const WRATHTextureCoordinateSourceBase*
  source(void);

private:
  enum repeat_mode_type m_mode_x, m_mode_y;  
  float m_shader_value;
};


/*! @} */

#endif
