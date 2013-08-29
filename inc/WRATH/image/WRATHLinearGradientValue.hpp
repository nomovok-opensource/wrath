/*! 
 * \file WRATHLinearGradientValue.hpp
 * \brief file WRATHLinearGradientValue.hpp
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


#ifndef __WRATH_LINEAR_GRADIENT_VALUE_HPP__
#define __WRATH_LINEAR_GRADIENT_VALUE_HPP__


#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHGradientValueBase.hpp"
#include "WRATHGradient.hpp"
#include "WRATHGradientSource.hpp"


/*! \addtogroup Imaging
 * @{
 */

/*!\class WRATHLinearGradientValue
  A WRATHLinearGradientValue holds the values
  for specifying a linear gradient positional
  values. 
 */
class WRATHLinearGradientValue:public WRATHGradientValueBase
{
public:
  
  enum
    {
      /*!
	Enumeration specifyig how many
	per node values needed to store
	the packed data of a 
	WRATHLinearGradientValue object
       */
      number_per_node_values=WRATHGradientValueBase::number_per_node_values + 4
    };

  /*!\fn WRATHLinearGradientValue
    Ctor to initialize the WRATHLinearGradientValue
    \param pstart initial value to assign to location of the start position
                  of the linear gradient 
    \param pend initial value to assign to location of the end radius
                of the linear gradient 
   */
  WRATHLinearGradientValue(const vec2 &pstart=vec2(0.0f, 0.0f),
			   const vec2 &pend=vec2(1.0f, 1.0f))
  {
    set_gradient(pstart, pend);
  }

  /*!\fn start_gradient(void) const
    Returns the starting position of the gradient
   */
  const vec2&
  start_gradient(void) const
  {
    return m_p0;
  }

  /*!\fn void start_gradient(const vec2 &)
    Sets the starting position of the gradient
    \param p value to which to set the starting position
   */
  void
  start_gradient(const vec2 &p)
  {
    set_gradient(p, m_p1);
  }

  /*!\fn const vec2& end_gradient(void) const
    Returns the ending position of the gradient
   */
  const vec2&
  end_gradient(void) const
  {
    return m_p1;
  }

  /*!\fn void end_gradient(const vec2&)
    Sets the ending position of the gradient
    \param p value to which to set the ending position
   */
  void
  end_gradient(const vec2 &p)
  {
    set_gradient(m_p0, p);
  }

  /*!\fn void set_gradient(const vec2&, const vec2&)
    Set the starting and ending position of the 
    Gradient.
    \param pstart value to which to set the start of the gradient
    \param pend value to which to set the end of the gradient
   */
  void
  set_gradient(const vec2 &pstart, const vec2 &pend)
  {
    m_p0=pstart;
    m_p1=pend;

    vec2 v(m_p1-m_p0);      
    v/=std::max(0.0000001f, dot(v,v));
    m_delta_p=vec2(v.x(), v.y());
  }

  /*!\fn const vec2& normalized_delta_gradient
    Returns the delta vector used to compute the
    interpolate from a linear gradient at a position.
    The value is given by:
    \code
       (end_gradient()-start_gradient())/dot(end_gradient()-start_gradient(), end_gradient()-start_gradient())
    \endcode
    thus the interpolate at a point p is given by:
    \code
      dot(p - start_gradient(), normalized_delta_gradient())
    \endcode
   */
  const vec2&
  normalized_delta_gradient(void) const
  {
    return m_delta_p;
  }

  /*!\fn void extract_values_at
    Extracts the values of this WRATHLinearGradientValue
    object into a reorder_c_array<> starting at a given position
    suitable for the shader returned by gradient_source().
    \param start_index index value as passed to add_per_node_values_at()
                             to specify the per node values
    \param out_value target to which to pack the data, ala WRATHLayerItemNode::extract_values()
   */
  virtual
  void
  extract_values_at(int start_index, reorder_c_array<float> out_value);

  /*!\fn add_per_node_values_at
    Adds the per-item values that a WRATHLinearGradientValue
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
    to use with linear gradient values.
   */
  static
  const WRATHGradientSourceBase*
  gradient_source(void);

private:
  vec2 m_p0, m_p1, m_delta_p;
};

/*! @} */

#endif
