/*! 
 * \file WRATHGradientValueBase.hpp
 * \brief file WRATHGradientValueBase.hpp
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


#ifndef __WRATH_GRADIENT_VALUE_BASE_HPP__
#define __WRATH_GRADIENT_VALUE_BASE_HPP__


#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHGradient.hpp"


/*! \addtogroup Imaging
 * @{
 */

/*!\class WRATHGradientValueBase
  A WRATHGradientValueBase holds the
  texture y-coordinate for a WRATHGradient
  object. That y-coordinate is exposed
  as a per-item value with the name
  WRATH_GRADIENT_y_coordinate, i.e. in shader
  code use
  \code
  fetch_node_value(WRATH_GRADIENT_y_coordinate)
  \endcode
  to get the value. The value is available
  in only the fragment shader if the node fetching
  supports fragment value fetching otherwise
  the value is only available in the vertex
  shader.
 */
class WRATHGradientValueBase
{
public:
  
  enum
    {
      /*!
        Enumeration specifyig how many
        per node values needed to store
        the packed data of a 
        WRATHGradientValueBase object
       */
      number_per_node_values=1
    };

  /*!\fn WRATHGradientValueBase(float)
    Ctor to initialize the WRATHGradientValueBase
    \param py_texture_coordinate y-texture-coordinate of a \ref WRATHGradient
   */
  explicit
  WRATHGradientValueBase(float py_texture_coordinate=0.0f):
    m_y_texture_coordinate(py_texture_coordinate)
  {}

  /*!\fn WRATHGradientValueBase(const WRATHGradient *)
    Ctor to initialize a WRATHGradientValueBase from a \ref WRATHGradient
    \param gradient \ref WRATHGradient from which to extract the y-texture-coordinate
   */
  explicit
  WRATHGradientValueBase(const WRATHGradient *gradient):
    m_y_texture_coordinate(gradient->texture_coordinate_y())
  {}

  /*!\fn float y_texture_coordinate(void) const
    Returns the y-texture-coordinate that this
    WRATHGradientValueBase stores
   */
  float
  y_texture_coordinate(void) const
  {
    return m_y_texture_coordinate;
  }

  /*!\fn void y_texture_coordinate(float) 
    Set the y-texture-coordinate that this
    WRATHGradientValueBase stores
    \param v value to which to set the y-texture-coordinate
   */
  void
  y_texture_coordinate(float v)
  {
    m_y_texture_coordinate=v;
  }

  /*!\fn void y_texture_coordinate(const WRATHGradient*) 
    Set the y-texture-coordinate that this
    WRATHGradientValueBase stores from
    a \ref WRATHGradient object.
    Equivalent to
    \code
    y_texture_coordinate(v->texture_coordinate_y());
    \endcode
    \param v WRATHGradient object to use to get the y-texture-coordinate
   */
  void
  y_texture_coordinate(const WRATHGradient *v)
  {
    WRATHassert(v!=NULL);
    m_y_texture_coordinate=v->texture_coordinate_y();
  }

  /*!\fn void extract_values_at
    Extracts the values of this WRATHGradientValueBase
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
    Adds the per-item values that a WRATHGradientValueBase
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

private:
  float m_y_texture_coordinate;
};

/*! @} */

#endif
