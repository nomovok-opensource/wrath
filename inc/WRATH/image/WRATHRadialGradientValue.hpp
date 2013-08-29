/*! 
 * \file WRATHRadialGradientValue.hpp
 * \brief file WRATHRadialGradientValue.hpp
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


#ifndef __WRATH_RADIAL_GRADIENT_VALUE_HPP__
#define __WRATH_RADIAL_GRADIENT_VALUE_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHGradientValueBase.hpp"
#include "WRATHGradient.hpp"
#include "WRATHGradientSource.hpp"


/*! \addtogroup Imaging
 * @{
 */

/*!\class WRATHRadialGradientValue
  A WRATHRadialGradientValues tracks values
  for radial gradients. 
 */ 
class WRATHRadialGradientValue:public WRATHGradientValueBase
{
public:
  
  enum
    {
      /*!
	Enumeration specifyig how many
	per node values needed to store
	the packed data of a 
	WRATHRadialGradientValue object
       */
      number_per_node_values=WRATHGradientValueBase::number_per_node_values + 7
    };

  /*!\fn WRATHRadialGradientValue(const vec2&, float, const vec2&, float)
  
    Ctor to initialize the WRATHRadialGradientValue.

    \param pstart initial value to assign to location of the start position
                  of the radial gradient 
    \param pstart_radius initial value to assign to location of the start radius
                         of the radial gradient 
    \param pend initial value to assign to location of the end position
                 of the radial gradient 
    \param pend_radius initial value to assign to location of the end radius
                       of the radial gradient 
   */
  explicit
  WRATHRadialGradientValue(const vec2 &pstart=vec2(0.0f, 0.0f),
                           float pstart_radius=0.0f,
                           const vec2 &pend=vec2(1.0f, 1.0f),
                           float pend_radius=1.0f)
  {
    set_gradient(pstart, pstart_radius, pend, pend_radius);
  }

  /*!\fn const vec2& start_position(void) const
    Returns the starting position of the gradient
   */
  const vec2&
  start_position(void) const
  {
    return m_p0;
  }

  /*!\fn void start_position(const vec2 &)
    Sets the starting position of the gradient
    \param p value to which to set the starting position
   */
  void
  start_position(const vec2 &p)
  {
    m_p0=p;
    update_pack_values();    
  }

  /*!\fn const vec2& end_position(void) const
    Returns the ending position of the Gradient
   */
  const vec2&
  end_position(void) const
  {
    return m_p1;
  }

  /*!\fn void end_position(const vec2&)
    Sets the ending position of the gradient
    \param p value to which to set the ending position
   */
  void
  end_position(const vec2 &p)
  {
    m_p1=p;
    update_pack_values();
  }

  /*!\fn void set_position(const vec2&, const vec2&)
    Set the starting and ending position of the 
    gradient.
    \param pstart value to which to set the start of the Gradient
    \param pend value to which to set the end of the Gradient
   */
  void
  set_position(const vec2 &pstart, const vec2 &pend)
  {
    m_p0=pstart;
    m_p1=pend;
    update_pack_values();
  }

  /*!\fn float start_radius(void) const
    Returns the starting position of the gradient
   */
  float
  start_radius(void) const
  {
    return m_r0;
  }

  /*!\fn void start_radius(float)
    Sets the starting radius of the gradient
    \param p value to which to set the starting radius
   */
  void
  start_radius(float p)
  {
    m_r0=p;
    update_pack_values();
  }

  /*!\fn float end_radius(void) const
    Returns the ending radius of the gradient
   */
  float
  end_radius(void) const
  {
    return m_r1;
  }

  /*!\fn void end_radius(float)
    Sets the ending radius of the gradient
    \param p value to which to set the ending radius
   */
  void
  end_radius(float p)
  {
    m_r1=p;
    update_pack_values();
  }

  /*!\fn void set_radius(float, float)
    Set the starting and ending radius of the 
    gradient.
    \param pstart value to which to set the start of the Gradient
    \param pend value to which to set the end of the Gradient
   */
  void
  set_radius(float pstart, float pend)
  {
    m_r0=pstart;
    m_r1=pend;
    update_pack_values();
  }

  /*!\fn void set_gradient(const vec2&, float, const vec2&, float)
    Set the starting and ending positions and radii of the gradient
    \param pstart value to which to set the start position of the gradient
    \param pend value to which to set the end position of the gradient
    \param rstart value to which to set the start radius of the gradient
    \param rend value to which to set the end radius of the gradient
   */
  void
  set_gradient(const vec2 &pstart, float rstart, const vec2 &pend, float rend)
  {
    set_position(pstart, pend);
    set_radius(rstart, rend);
    update_pack_values();
  }

  /*!\fn void extract_values_at
    Extracts the values of this WRATHRadialGradientValue
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
    Adds the per-item values that a WRATHRadialGradientValue
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


  /*!\fn const WRATHGradientSourceBase* gradient_source
    Returns the WRATHGradientSource object
    to use with radial gradient whose positional
    data is determined by a WRATHRadialGradientValue
    packed into a node.
   */
  static
  const WRATHGradientSourceBase*
  gradient_source(void);

private:

  void
  update_pack_values(void);

  vec2 m_p0, m_p1;
  float m_r0, m_r1;

  float m_A, m_A_r0_delta_r, m_r0_r0;
  vec2 m_A_delta_p;

};



/*! @} */

#endif
