/*! 
 * \file WRATHGradientValueBase.cpp
 * \brief file WRATHGradientValueBase.cpp
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



#include "WRATHConfig.hpp"
#include "WRATHGradientValueBase.hpp"

void
WRATHGradientValueBase::
add_per_node_values_at(int start, WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
			 const WRATHLayerNodeValuePackerBase::function_packet &func)
{
  /*
    TODO:
      Is the assumption that gradient only used in the fragment shader
      a good assumption?
   */
  if(func.supports_per_node_value(GL_FRAGMENT_SHADER))
    {
      spec
        .add_source(start, "WRATH_GRADIENT_y_coordinate", GL_FRAGMENT_SHADER);
    }
  else
    {
      spec
        .add_source(start, "WRATH_GRADIENT_y_coordinate", GL_VERTEX_SHADER);
    }
}

void
WRATHGradientValueBase::
extract_values_at(int start, reorder_c_array<float> out_value)
{
  out_value[start]=m_y_texture_coordinate;
}
