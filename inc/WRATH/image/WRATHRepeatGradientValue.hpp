/*! 
 * \file WRATHRepeatGradientValue.hpp
 * \brief file WRATHRepeatGradientValue.hpp
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


#ifndef __WRATH_REPEAT_GRADIENT_VALUES_HPP__
#define __WRATH_REPEAT_GRADIENT_VALUES_HPP__


#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHGradient.hpp"
#include "WRATHGradientSource.hpp"



/*! \addtogroup Imaging
 * @{
 */

/*!\class WRATHRepeatGradientValue
  A WRATHRepeatGradientValues is to be used to build
  upon a pre-existing gradient to create a gradient
  that makes a repeat of the starting gradient on
  a specified window as follows:

  Let the window have the corners start_window()
  and end_window(), let:
  \code
    w=start_window()
    s=end_window() - start_window()
  \endcode
  then the gradient interpolate is computed 
  by feeding in the value q to it's
  wrath_compute_gradient() where q is computed as 
  follows:
  \code
  \n\n nf=  ( p - w )/s 
  \n\n  q= w + fract(f)*s
  \endcode
 */
class WRATHRepeatGradientValue
{
public:
  
  enum
    {
      /*!
        Enumeration specifyig how many
        per node values needed to store
        the packed data of a 
        WRATHRepeatGradientValue object
       */
      number_per_node_values=4
    };

  /*!\fn WRATHRepeatGradientValue
    Ctor to initialize the WRATHRepeatGradientValue
    \param pstart_window specifies the start of the repeat window
    \param pend_window specifies the end of the repeat window
   */
  explicit
  WRATHRepeatGradientValue(const vec2 &pstart_window=vec2(0.0f, 0.0f),
                           const vec2 &pend_window=vec2(0.0f, 0.0f)):
    m_start_window(pstart_window),
    m_end_window(pend_window)
  {
  }

  /*!\fn const vec2& start_window(void) const
    Returns the starting position of the position window
   */
  const vec2&
  start_window(void) const
  {
    return m_start_window;
  }

  /*!\fn void start_window(const vec2 &)
    Sets the starting position of the position window
    \param p value to which to set the starting position
   */
  void
  start_window(const vec2 &p)
  {
    m_start_window=p;
  }

  /*!\fn const vec2& end_window(void) const
    Returns the ending position of the position window
   */
  const vec2&
  end_window(void) const
  {
    return m_end_window;
  }

  /*!\fn void end_window(const vec2&)
    Sets the ending position of the position window
    \param p value to which to set the ending position
   */
  void
  end_window(const vec2 &p)
  {
    m_end_window=p;
  }

  /*!\fn void set_window(const vec2&, const vec2&)
    Set the starting and ending position of the 
    position window
    \param pstart value to which to set the start of the position window
    \param pend value to which to set the end of the position window
   */
  void
  set_window(const vec2 &pstart, const vec2 &pend)
  {
    m_start_window=pstart;
    m_end_window=pend;
  }

  /*!\fn void extract_values_at
    Extracts the values of this WRATHRepeatGradientValue
    object into a reorder_c_array<> starting at a given position
    suitable for the shader returned by gradient_source().
    \param start_index index value as passed to add_per_node_values_at()
                             to specify the per node values
    \param out_value target to which to pack the data, ala WRATHLayerItemNode::extract_values()
   */
  void
  extract_values_at(int start_index, reorder_c_array<float> out_value);

  /*!\fn void add_per_node_values_at
    Adds the per-item values that a WRATHRepeatGradientValue
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
    Returns a WRATHGradientSourceBase object for a
    repeat gradient whose underlying gradient data
    is given by the passed WRATHGradientSourceBase 
    object
    \param src source for underlying gradient onto 
               which WRATHRepeatGradientValue applies
   */
  static
  const WRATHGradientSourceBase*
  gradient_source(const WRATHGradientSourceBase *src);

private:
  vec2 m_start_window, m_end_window;
};


/*! @} */

#endif
